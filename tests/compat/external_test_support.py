#!/usr/bin/env python3
"""Shared discovery and failure handling for external ELF tests."""

import os
from pathlib import Path
import subprocess
import sys


def fail(message, result=None):
    print(f"external ELF regression failure: {message}", file=sys.stderr)
    if result is not None:
        print(f"return code: {result.returncode}", file=sys.stderr)
        print(f"stdout: {result.stdout!r}", file=sys.stderr)
        print(f"stderr: {result.stderr!r}", file=sys.stderr)
    raise SystemExit(1)


def external_dso(variable):
    value = os.environ.get(variable)
    if not value:
        print(f"SKIP: {variable} is not set", file=sys.stderr)
        raise SystemExit(77)
    path = Path(value).resolve()
    if not path.is_file():
        print(f"SKIP: {variable} does not name a file: {path}", file=sys.stderr)
        raise SystemExit(77)
    return path


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
