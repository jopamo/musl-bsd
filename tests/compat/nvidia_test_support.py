#!/usr/bin/env python3
"""Shared discovery and failure handling for local NVIDIA tests."""

import os
from pathlib import Path
import subprocess
import sys


def fail(message, result=None):
    print(f"NVIDIA regression failure: {message}", file=sys.stderr)
    if result is not None:
        print(f"return code: {result.returncode}", file=sys.stderr)
        print(f"stdout: {result.stdout!r}", file=sys.stderr)
        print(f"stderr: {result.stderr!r}", file=sys.stderr)
    raise SystemExit(1)


def library_directory():
    value = os.environ.get("NVIDIA_LIBDIR")
    if not value:
        print("SKIP: NVIDIA_LIBDIR is not set", file=sys.stderr)
        raise SystemExit(77)
    return Path(value).resolve()


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


def readelf_output(readelf, path, *options):
    result = subprocess.run(
        [readelf, *options, str(path)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        fail(f"cannot inspect ELF metadata in {path}", result)
    return result.stdout


def dynamic_symbols(readelf, path):
    return [
        line.split()
        for line in readelf_output(
            readelf, path, "--dyn-syms", "-W"
        ).splitlines()
    ]


def verify_constructor_dependencies(readelf, parent, *dependencies):
    dynamic_sections = {
        path: readelf_output(readelf, path, "-dW")
        for path in (parent, *dependencies)
    }
    for path, dynamic in dynamic_sections.items():
        if "(INIT)" not in dynamic and "(INIT_ARRAY)" not in dynamic:
            fail(f"{path}: NVIDIA constructor dependency has no initializer")
    parent_dynamic = dynamic_sections[parent]
    for dependency in dependencies:
        if f"Shared library: [{dependency.name}]" not in parent_dynamic:
            fail(f"{parent}: no DT_NEEDED edge to {dependency.name}")


def versioned_undefined_symbols(readelf, *paths):
    requirements = set()
    for path in paths:
        for fields in dynamic_symbols(readelf, path):
            if (
                len(fields) >= 8
                and fields[4] == "GLOBAL"
                and fields[5] == "DEFAULT"
                and fields[6] == "UND"
            ):
                name, separator, version = fields[7].partition("@")
                if separator and version.startswith("GLIBC_"):
                    requirements.add((name, version))
    return sorted(requirements)
