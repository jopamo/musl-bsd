#!/usr/bin/env python3
"""Validate per-thread storage in the locally installed NVIDIA TLS DSO."""

from pathlib import Path
import subprocess
import sys

from nvidia_test_support import fail, installed_dso, library_directory


def tls_symbol(readelf, path):
    result = subprocess.run(
        [readelf, "--dyn-syms", "-W", str(path)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )
    symbols = []
    for line in result.stdout.splitlines():
        fields = line.split()
        if (
            len(fields) >= 8
            and fields[3] == "TLS"
            and fields[4] == "GLOBAL"
            and int(fields[2]) >= 8
            and fields[7].startswith("_nv")
        ):
            symbols.append(fields[7].split("@", 1)[0])
    if not symbols:
        fail(f"{path}: no writable pointer-sized NVIDIA TLS symbol")
    return sorted(set(symbols))[0]


def main():
    if len(sys.argv) != 4:
        fail("invalid runner arguments")
    linker = Path(sys.argv[1]).resolve()
    readelf = sys.argv[2]
    probe = Path(sys.argv[3]).resolve()
    library_dir = library_directory()
    nvidia_tls = installed_dso(library_dir, "libnvidia-tls.so.*")
    symbol = tls_symbol(readelf, nvidia_tls)
    result = subprocess.run(
        [
            str(linker),
            "--preload",
            str(nvidia_tls),
            "--library-path",
            f"{library_dir}:/usr/lib",
            str(probe),
            str(nvidia_tls),
            symbol,
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        fail(f"multithreaded access to {symbol}", result)


if __name__ == "__main__":
    main()
