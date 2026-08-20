#!/usr/bin/env python3
"""Shared discovery and failure handling for local NVIDIA tests."""

import os
from pathlib import Path
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
