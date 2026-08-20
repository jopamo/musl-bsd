#!/usr/bin/env python3
"""Verify audited dynamic-loading policy in the NVIDIA/CUDA manifest."""

import json
from pathlib import Path
import sys


POLICIES = {
    "dlmopen": {
        "version": "GLIBC_2.3.4",
        "soname": "libdl.so.2",
        "implementation": "musl-bsd-core:dlmopen",
        "test": "compat/glibc_api",
        "quality": "UNSUPPORTED",
    },
    "dladdr1": {
        "version": "GLIBC_2.3.3",
        "soname": "libdl.so.2",
        "implementation": "musl-bsd-core:dladdr1",
        "test": "compat/glibc_api",
        "quality": "TRANSLATED",
    },
    "dlvsym": {
        "version": "GLIBC_2.2.5",
        "soname": "libdl.so.2",
        "implementation": "musl-bsd-core:dlvsym",
        "test": "compat/glibc_api",
        "quality": "DEGRADED",
    },
}


def fail(message):
    raise SystemExit(f"dlfcn manifest audit: {message}")


def main():
    if len(sys.argv) != 2:
        fail("usage: check_dlfcn_manifest.py MANIFEST")
    manifest = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    for name, policy in POLICIES.items():
        entries = [
            item for item in manifest["symbols"] if item["name"] == name
        ]
        if len(entries) != 1:
            fail(f"{name} has {len(entries)} manifest entries")
        entry = entries[0]
        for field, expected in policy.items():
            if entry[field] != expected:
                fail(
                    f"{name} {field} is {entry[field]!r}, "
                    f"expected {expected!r}"
                )


if __name__ == "__main__":
    main()
