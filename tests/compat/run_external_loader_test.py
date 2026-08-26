#!/usr/bin/env python3
"""Exercise an external DSO through the compatibility loader."""

import os
from pathlib import Path
import sys
import tempfile

from external_test_support import external_dso, fail
from run_loader_tests import prepare_fixture, run


def symbol_list(variable):
    return [
        symbol.strip()
        for symbol in os.environ.get(variable, "").split(",")
        if symbol.strip()
    ]


def main():
    if len(sys.argv) != 7:
        fail("invalid runner arguments")
    loader, raw_target, core, facade_dir = (
        Path(value).resolve() for value in sys.argv[1:5]
    )
    patchelf = sys.argv[6]
    dso = external_dso("MUSL_BSD_TEST_DSO")
    early_preload = os.environ.get("MUSL_BSD_TEST_EARLY_PRELOAD_PATH")
    if early_preload:
        early_preload = str(Path(early_preload).resolve())
    exports = symbol_list("MUSL_BSD_TEST_EXPORTS")
    weak_symbols = symbol_list("MUSL_BSD_TEST_WEAK_SYMBOLS")
    library_dir = dso.parent

    with tempfile.TemporaryDirectory(
        prefix="musl-bsd external loader "
    ) as temp_name:
        target = Path(temp_name) / "glibc external loader probe"
        prepare_fixture(patchelf, raw_target, target, loader)

        env = os.environ.copy()
        env["MUSL_BSD_PRELOAD_PATH"] = str(core)
        env["MUSL_BSD_LIBRARY_PATH"] = (
            f"{facade_dir}:{library_dir}:/usr/lib"
        )
        env.pop("LD_PRELOAD", None)
        env.pop("MUSL_BSD_EARLY_PRELOAD_PATH", None)
        env.pop("MUSL_BSD_LOADER_STAGE", None)
        if early_preload:
            env["MUSL_BSD_EARLY_PRELOAD_PATH"] = early_preload

        result = run(target, ["external-loader", "load", str(dso)], env)
        if result.returncode != 0:
            fail("external DSO compatibility-loader chain", result)

        if weak_symbols:
            result = run(
                target,
                ["external-loader", "weak-load", str(dso), *weak_symbols],
                env,
            )
            if result.returncode != 0:
                fail("external weak-symbol relocation", result)

        if exports:
            result = run(
                target,
                ["external-loader", "global-load", str(dso), *exports],
                env,
            )
            if result.returncode != 0:
                fail("external RTLD_GLOBAL publication", result)
            for scope in ("local", "global"):
                result = run(
                    target,
                    [
                        "external-loader",
                        "repeat-load",
                        scope,
                        str(dso),
                        *exports,
                    ],
                    env,
                )
                if result.returncode != 0:
                    fail(f"repeated external RTLD_{scope.upper()} loading", result)

        result = run(
            target,
            ["external-loader", "recurse-load", str(dso)],
            env,
        )
        if result.returncode != 0:
            fail("early-preload policy across /proc/self/exe re-exec", result)


if __name__ == "__main__":
    main()
