#!/usr/bin/env python3
"""Validate per-thread storage in the locally installed NVIDIA TLS DSO."""

from pathlib import Path
import subprocess
import sys

from nvidia_test_support import fail, installed_dso, library_directory


def dynamic_symbols(readelf, path):
    result = subprocess.run(
        [readelf, "--dyn-syms", "-W", str(path)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        fail(f"cannot inspect dynamic symbols in {path}", result)
    return [line.split() for line in result.stdout.splitlines()]


def tls_symbol(readelf, path):
    symbols = []
    for fields in dynamic_symbols(readelf, path):
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


def verify_destructor_model(readelf, directory):
    paths = sorted(
        {
            path.resolve()
            for pattern in ("libnvidia*.so.*", "libcuda.so.*")
            for path in directory.glob(pattern)
            if path.is_file()
        }
    )
    key_users = []
    cxa_users = []
    for path in paths:
        undefined = {
            fields[7].split("@", 1)[0]
            for fields in dynamic_symbols(readelf, path)
            if len(fields) >= 8 and fields[6] == "UND"
        }
        if any(name.startswith("__cxa_thread_atexit") for name in undefined):
            cxa_users.append(path)
        if (
            {"pthread_key_create", "pthread_setspecific"} <= undefined
            or {"__pthread_key_create", "pthread_setspecific"} <= undefined
        ):
            key_users.append(path)
    if cxa_users:
        fail(
            "unqualified __cxa_thread_atexit use in "
            + ", ".join(str(path) for path in cxa_users)
        )
    if not key_users:
        fail(f"{directory}: no NVIDIA pthread-key destructor users")


def main():
    if len(sys.argv) != 5:
        fail("invalid runner arguments")
    linker = Path(sys.argv[1]).resolve()
    readelf = sys.argv[2]
    probe = Path(sys.argv[3]).resolve()
    core = Path(sys.argv[4]).resolve()
    library_dir = library_directory()
    nvidia_tls = installed_dso(library_dir, "libnvidia-tls.so.*")
    verify_destructor_model(readelf, library_dir)
    symbol = tls_symbol(readelf, nvidia_tls)
    result = subprocess.run(
        [
            str(linker),
            "--preload",
            f"{core}:{nvidia_tls}",
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
        fail(f"multithreaded lifecycle of {symbol}", result)


if __name__ == "__main__":
    main()
