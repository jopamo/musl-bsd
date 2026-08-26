#!/usr/bin/env python3
"""Validate per-thread storage in an external TLS DSO."""

from pathlib import Path
import subprocess
import sys

from external_test_support import dynamic_symbols, external_dso, fail


def tls_symbol(readelf, path):
    symbols = []
    for fields in dynamic_symbols(readelf, path):
        if (
            len(fields) >= 8
            and fields[3] == "TLS"
            and fields[4] == "GLOBAL"
            and int(fields[2]) >= 8
        ):
            symbols.append(fields[7].split("@", 1)[0])
    if not symbols:
        fail(f"{path}: no exported writable pointer-sized TLS symbol")
    return sorted(set(symbols))[0]


def main():
    if len(sys.argv) != 5:
        fail("invalid runner arguments")
    linker = Path(sys.argv[1]).resolve()
    readelf = sys.argv[2]
    probe = Path(sys.argv[3]).resolve()
    core = Path(sys.argv[4]).resolve()
    tls_dso = external_dso("MUSL_BSD_TEST_TLS_DSO")
    symbol = tls_symbol(readelf, tls_dso)
    result = subprocess.run(
        [
            str(linker),
            "--preload",
            f"{core}:{tls_dso}",
            "--library-path",
            f"{tls_dso.parent}:/usr/lib",
            str(probe),
            str(tls_dso),
            symbol,
        ],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        fail(f"multithreaded lifecycle of {symbol}", result)


if __name__ == "__main__":
    main()
