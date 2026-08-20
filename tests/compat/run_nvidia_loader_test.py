#!/usr/bin/env python3
"""Exercise a locally installed NVIDIA DSO through the compatibility loader."""

import os
from pathlib import Path
import sys
import tempfile

from nvidia_test_support import (
    dynamic_symbols,
    fail,
    installed_dso,
    library_directory,
)
from run_loader_tests import prepare_fixture, run


def exported_symbol(readelf, path, predicate):
    symbols = {
        fields[7].split("@", 1)[0]
        for fields in dynamic_symbols(readelf, path)
        if (
            len(fields) >= 8
            and fields[3] in {"FUNC", "OBJECT"}
            and fields[4] == "GLOBAL"
            and fields[5] == "DEFAULT"
            and fields[6] != "UND"
            and predicate(fields[7].split("@", 1)[0])
        )
    }
    if not symbols:
        fail(f"{path}: no suitable visibility-test export")
    return sorted(symbols)[0]


def main():
    if len(sys.argv) != 7:
        fail("invalid runner arguments")
    loader, raw_target, core, facade_dir = (
        Path(value).resolve() for value in sys.argv[1:5]
    )
    readelf = sys.argv[5]
    patchelf = sys.argv[6]
    library_dir = library_directory()
    glcore = installed_dso(library_dir, "libnvidia-glcore.so.*")
    gpucomp = installed_dso(library_dir, "libnvidia-gpucomp.so.*")
    nvidia_tls = installed_dso(library_dir, "libnvidia-tls.so.*")
    glcore_symbol = exported_symbol(
        readelf, glcore, lambda name: name.startswith("_nv") and name.endswith("glcore")
    )
    gpucomp_symbol = exported_symbol(
        readelf, gpucomp, lambda name: name == "nvGetCompilerInterface"
    )

    with tempfile.TemporaryDirectory(
        prefix="musl-bsd NVIDIA loader "
    ) as temp_name:
        target = Path(temp_name) / "glibc NVIDIA loader probe"
        prepare_fixture(patchelf, raw_target, target, loader)

        env = os.environ.copy()
        env["MUSL_BSD_PRELOAD_PATH"] = str(core)
        env["MUSL_BSD_LIBRARY_PATH"] = (
            f"{facade_dir}:{library_dir}:/usr/lib"
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
            [
                "nvidia-loader",
                "global-load",
                str(glcore),
                glcore_symbol,
                gpucomp_symbol,
            ],
            env,
        )
        if result.returncode != 0:
            fail("RTLD_GLOBAL publication of NVIDIA dependency graph", result)

        for scope in ("local", "global"):
            result = run(
                target,
                [
                    "nvidia-loader",
                    "repeat-load",
                    scope,
                    str(glcore),
                    glcore_symbol,
                    gpucomp_symbol,
                ],
                env,
            )
            if result.returncode != 0:
                fail(f"repeated RTLD_{scope.upper()} NVIDIA loading", result)

        result = run(
            target,
            ["nvidia-loader", "recurse-load", str(glcore)],
            env,
        )
        if result.returncode != 0:
            fail("NVIDIA TLS policy across /proc/self/exe re-exec", result)


if __name__ == "__main__":
    main()
