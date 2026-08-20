#!/usr/bin/env python3
"""Verify pthread manifest policy against the compatibility core exports."""

import json
from pathlib import Path
import subprocess
import sys


PREFIXES = ("__pthread_", "pthread_", "sem_")


def fail(message):
    raise SystemExit(f"pthread manifest audit: {message}")


def versioned_exports(readelf, core):
    result = subprocess.run(
        [readelf, "--dyn-syms", "-W", str(core)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        fail(f"cannot inspect {core}: {result.stderr}")
    exports = set()
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) < 8 or fields[6] == "UND":
            continue
        name, separator, version = fields[7].partition("@@")
        if separator:
            exports.add((name, version))
    return exports


def main():
    if len(sys.argv) != 4:
        fail("usage: check_pthread_manifest.py READELF MANIFEST CORE")
    readelf = sys.argv[1]
    manifest_path = Path(sys.argv[2]).resolve()
    core = Path(sys.argv[3]).resolve()
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    entries = [
        item
        for item in manifest["symbols"]
        if item["name"].startswith(PREFIXES)
    ]
    if not entries:
        fail("manifest contains no NVIDIA/CUDA pthread requirements")

    exports = versioned_exports(readelf, core)
    for item in entries:
        name = item["name"]
        version = item["version"]
        implementation = item["implementation"]
        if item["test"] != "compat/pthread_abi":
            fail(f"{name}@{version} has no focused pthread test")
        if implementation == f"musl-bsd-core:{name}":
            if item["quality"] != "TRANSLATED":
                fail(f"{name}@{version} bridge is not TRANSLATED")
            if (name, version) not in exports:
                fail(f"{name}@{version} is absent from compatibility core")
        elif implementation == f"musl-libc:{name}":
            if item["quality"] != "EXACT":
                fail(f"{name}@{version} direct ABI is not EXACT")
        else:
            fail(f"{name}@{version} has unexpected implementation")


if __name__ == "__main__":
    main()
