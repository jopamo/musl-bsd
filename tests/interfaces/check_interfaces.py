#!/usr/bin/env python3
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


def output(env, *args):
    return subprocess.run(
        args,
        check=True,
        env=env,
        stdout=subprocess.PIPE,
        text=True,
    ).stdout.strip()


def require_equal(actual, expected, label):
    if actual != expected:
        raise SystemExit(f"{label}: expected {expected!r}, got {actual!r}")


def rewritten_build_flags(flags, source_root, library_root):
    result = []
    for flag in flags:
        if flag.startswith("-I") and flag.endswith("/musl-bsd/overlay/include"):
            result.append(f"-I{source_root / 'overlay/include'}")
        elif flag.startswith("-L") and (
            flag.endswith("/lib/musl-bsd")
            or flag.endswith("/lib64/musl-bsd")
            or flag.endswith("/lib")
            or flag.endswith("/lib64")
        ):
            result.append(f"-L{library_root}")
        else:
            result.append(flag)
    return result


def compile_source_contract(env, cc, readelf, source_root, build_root, cflags, libs):
    with tempfile.TemporaryDirectory(prefix="musl-bsd-source-interface-") as temp:
        temp = Path(temp)
        source = temp / "source.c"
        source.write_text(
            """
#include <gnu/libc-version.h>
const char *musl_bsd_consumer_version(void) {
    return gnu_get_libc_version();
}
int main(void) {
    return musl_bsd_consumer_version() == 0;
}
"""
        )
        library_root = temp / "lib"
        library_root.mkdir()
        (library_root / "libmusl-bsd-core.a").symlink_to(
            build_root / "libmusl-bsd-core.a"
        )
        build_cflags = rewritten_build_flags(cflags, source_root, library_root)
        build_libs = rewritten_build_flags(libs, source_root, library_root)

        executable = temp / "source-consumer"
        subprocess.run(
            [cc, *build_cflags, str(source), "-o", str(executable), *build_libs],
            check=True,
            env=env,
        )
        dynamic = output(env, readelf, "-dW", str(executable))
        if "libmusl-bsd-glibc-host" in dynamic:
            raise SystemExit("source interface added a host-runtime DT_NEEDED")

        shared = temp / "libsource-consumer.so"
        subprocess.run(
            [
                cc,
                "-shared",
                "-fPIC",
                *build_cflags,
                str(source),
                "-o",
                str(shared),
                *build_libs,
            ],
            check=True,
            env=env,
        )
        symbols = output(env, readelf, "--dyn-syms", "-W", str(shared))
        if "musl_bsd_consumer_version" not in symbols:
            raise SystemExit("source consumer did not export its own API")
        for forbidden in ("gnu_get_libc_version", "musl_bsd_core_abi"):
            if forbidden in symbols:
                raise SystemExit(
                    f"source interface leaked compatibility symbol {forbidden}"
                )


def compile_host_contract(env, cc, readelf, build_root, libs):
    with tempfile.TemporaryDirectory(prefix="musl-bsd-host-interface-") as temp:
        temp = Path(temp)
        source = temp / "host.c"
        source.write_text("int main(void) { return 0; }\n")
        executable = temp / "host-consumer"
        library_root = temp / "lib"
        library_root.mkdir()
        (library_root / "libmusl-bsd-glibc-host.so.2").symlink_to(
            build_root / "libmusl-bsd-glibc-host.so.2"
        )
        build_libs = rewritten_build_flags(
            libs, Path("/nonexistent"), library_root
        )
        subprocess.run(
            [cc, str(source), "-o", str(executable), *build_libs],
            check=True,
            env=env,
        )
        dynamic = output(env, readelf, "-dW", str(executable))
        if "Shared library: [libmusl-bsd-glibc-host.so.2]" not in dynamic:
            raise SystemExit("host interface did not force its qualified DT_NEEDED")


def main():
    if len(sys.argv) != 9:
        raise SystemExit(
            "usage: check_interfaces.py PKG_CONFIG CC READELF "
            "SOURCE_ROOT BUILD_ROOT PREFIX LIBDIR enabled|disabled"
        )

    pkg_config, cc, readelf = sys.argv[1:4]
    source_root = Path(sys.argv[4])
    build_root = Path(sys.argv[5])
    prefix = Path(sys.argv[6])
    libdir_name = sys.argv[7]
    runtime_enabled = sys.argv[8] == "enabled"
    libdir = prefix / libdir_name
    sysroot = Path("/musl-bsd-test-sysroot")

    env = os.environ.copy()
    env["PKG_CONFIG_LIBDIR"] = str(build_root)
    env.pop("PKG_CONFIG_PATH", None)
    env.pop("PKG_CONFIG_SYSROOT_DIR", None)

    headers_cflags = shlex.split(
        output(env, pkg_config, "--cflags", "musl-bsd-headers")
    )
    headers_libs = shlex.split(
        output(env, pkg_config, "--libs", "musl-bsd-headers")
    )
    require_equal(
        headers_cflags,
        [f"-I{libdir}/musl-bsd/overlay/include"],
        "headers Cflags",
    )
    require_equal(headers_libs, [], "headers Libs")

    source_cflags = shlex.split(
        output(env, pkg_config, "--cflags", "musl-bsd-source")
    )
    source_libs = shlex.split(
        output(env, pkg_config, "--libs", "musl-bsd-source")
    )
    require_equal(source_cflags, headers_cflags, "source Cflags")
    require_equal(
        source_libs,
        [
            f"-L{libdir}/musl-bsd",
            "-l:libmusl-bsd-core.a",
        ],
        "source Libs",
    )
    if any(".so" in flag for flag in source_libs):
        raise SystemExit("source interface can select a shared object")

    sysroot_env = env.copy()
    sysroot_env["PKG_CONFIG_SYSROOT_DIR"] = str(sysroot)
    sysroot_flags = shlex.split(
        output(
            sysroot_env,
            pkg_config,
            "--cflags",
            "--libs",
            "musl-bsd-source",
        )
    )
    expected_sysroot_include = f"-I{sysroot}{libdir}/musl-bsd/overlay/include"
    expected_sysroot_library = f"-L{sysroot}{libdir}/musl-bsd"
    if expected_sysroot_include not in sysroot_flags:
        raise SystemExit("source interface include path escaped the target sysroot")
    if expected_sysroot_library not in sysroot_flags:
        raise SystemExit("source interface library path escaped the target sysroot")

    compile_source_contract(
        env,
        cc,
        readelf,
        source_root,
        build_root,
        source_cflags,
        source_libs,
    )

    host_exists = (
        subprocess.run(
            [pkg_config, "--exists", "musl-bsd-glibc-host"],
            env=env,
            check=False,
        ).returncode
        == 0
    )
    if host_exists != runtime_enabled:
        raise SystemExit(
            "host interface availability does not match runtime qualification"
        )

    if runtime_enabled:
        host_libs = shlex.split(
            output(env, pkg_config, "--libs", "musl-bsd-glibc-host")
        )
        require_equal(
            host_libs,
            [
                f"-L{libdir}",
                "-Wl,--push-state,--no-as-needed",
                "-l:libmusl-bsd-glibc-host.so.2",
                "-Wl,--pop-state",
            ],
            "host Libs",
        )
        if any("libmusl-bsd-core" in flag for flag in host_libs):
            raise SystemExit("host interface can fall back to the source archive")
        compile_host_contract(env, cc, readelf, build_root, host_libs)


if __name__ == "__main__":
    main()
