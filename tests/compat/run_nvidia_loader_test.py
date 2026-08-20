#!/usr/bin/env python3
"""Exercise a locally installed NVIDIA DSO through the compatibility loader."""

import os
from pathlib import Path
import sys
import tempfile

from run_loader_tests import prepare_fixture, run


def fail(message, result=None):
    print(f"NVIDIA loader regression failure: {message}", file=sys.stderr)
    if result is not None:
        print(f"return code: {result.returncode}", file=sys.stderr)
        print(f"stdout: {result.stdout!r}", file=sys.stderr)
        print(f"stderr: {result.stderr!r}", file=sys.stderr)
    raise SystemExit(1)


def installed_dso(directory, pattern):
    matches = sorted(
        {
            path.resolve()
            for path in directory.glob(pattern)
            if path.is_file()
        }
    )
    if not matches:
        print(
            f"SKIP: {pattern} is not installed under {directory}",
            file=sys.stderr,
        )
        raise SystemExit(77)
    if len(matches) != 1:
        fail(
            f"{pattern} is ambiguous under {directory}: "
            + ", ".join(str(path) for path in matches)
        )
    return matches[0]


def main():
    if len(sys.argv) != 6:
        fail("invalid runner arguments")
    loader, raw_target, core, facade_dir = (
        Path(value).resolve() for value in sys.argv[1:5]
    )
    patchelf = sys.argv[5]
    library_directory = os.environ.get("NVIDIA_LIBDIR")
    if not library_directory:
        print("SKIP: NVIDIA_LIBDIR is not set", file=sys.stderr)
        raise SystemExit(77)
    library_directory = Path(library_directory).resolve()
    glcore = installed_dso(library_directory, "libnvidia-glcore.so.*")
    nvidia_tls = installed_dso(library_directory, "libnvidia-tls.so.*")

    with tempfile.TemporaryDirectory(
        prefix="musl-bsd NVIDIA loader "
    ) as temp_name:
        target = Path(temp_name) / "glibc NVIDIA loader probe"
        prepare_fixture(patchelf, raw_target, target, loader)

        env = os.environ.copy()
        env["MUSL_BSD_PRELOAD_PATH"] = str(core)
        env["MUSL_BSD_LIBRARY_PATH"] = (
            f"{facade_dir}:{library_directory}:/usr/lib"
        )
        env.pop("LD_PRELOAD", None)
        env.pop("MUSL_BSD_NVIDIA_TLS_PATH", None)
        env.pop("MUSL_BSD_LOADER_STAGE", None)

        result = run(target, ["nvidia-loader", "load", str(glcore)], env)
        if (
            result.returncode == 0
            or "initial-exec TLS" not in result.stderr
        ):
            fail("missing NVIDIA TLS preload did not fail closed", result)

        env["MUSL_BSD_NVIDIA_TLS_PATH"] = str(nvidia_tls)
        result = run(target, ["nvidia-loader", "load", str(glcore)], env)
        if result.returncode != 0:
            fail("real NVIDIA DSO compatibility-loader chain", result)

        result = run(
            target,
            ["nvidia-loader", "recurse-load", str(glcore)],
            env,
        )
        if result.returncode != 0:
            fail("NVIDIA TLS policy across /proc/self/exe re-exec", result)


if __name__ == "__main__":
    main()
