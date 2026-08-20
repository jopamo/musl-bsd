#!/usr/bin/env python3
"""Run IRELATIVE fixtures through the selected musl dynamic linker."""

from pathlib import Path
import subprocess
import sys


def fail(message, result=None):
    print(f"IRELATIVE regression failure: {message}", file=sys.stderr)
    if result is not None:
        print(f"return code: {result.returncode}", file=sys.stderr)
        print(f"stdout: {result.stdout!r}", file=sys.stderr)
        print(f"stderr: {result.stderr!r}", file=sys.stderr)
    raise SystemExit(1)


def output(readelf, *arguments, target):
    result = subprocess.run(
        [readelf, *arguments, str(target)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )
    return result.stdout


def run(linker, library_path, target, *arguments):
    return subprocess.run(
        [
            str(linker),
            "--library-path",
            str(library_path),
            str(target),
            *arguments,
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def check_fixture(readelf, path, dependency_soname):
    relocations = output(readelf, "-rW", target=path)
    if "R_X86_64_IRELATIVE" not in relocations:
        fail(f"{path}: no R_X86_64_IRELATIVE relocation")
    symbols = output(readelf, "-sW", target=path)
    if "IFUNC" not in symbols:
        fail(f"{path}: no GNU IFUNC symbol")
    dynamic = output(readelf, "-dW", target=path)
    if f"Shared library: [{dependency_soname}]" not in dynamic:
        fail(f"{path}: resolver dependency is not in DT_NEEDED")


def main():
    if len(sys.argv) != 8:
        raise SystemExit(
            "usage: run_ifunc_tests.py LINKER READELF STARTUP "
            "DLOPEN MULTIPLE IFUNC_A IFUNC_B"
        )
    linker, readelf = sys.argv[1:3]
    startup, dlopen_probe, multiple = (
        Path(value).resolve() for value in sys.argv[3:6]
    )
    ifunc_a, ifunc_b = (
        Path(value).resolve() for value in sys.argv[6:8]
    )
    library_path = ifunc_a.parent
    dependency_soname = "libifunc-dependency.so.1"

    check_fixture(readelf, ifunc_a, dependency_soname)
    check_fixture(readelf, ifunc_b, dependency_soname)

    result = run(linker, library_path, startup)
    if result.returncode != 0:
        fail("initial-program IRELATIVE relocation", result)

    result = run(linker, library_path, dlopen_probe, ifunc_a)
    if result.returncode != 0:
        fail("dlopen IRELATIVE relocation", result)

    result = run(linker, library_path, multiple)
    if result.returncode != 0:
        fail("multiple IRELATIVE relocations and dependency ordering", result)


if __name__ == "__main__":
    main()
