#!/usr/bin/env python3
"""Verify audited runtime compatibility policy in the NVIDIA/CUDA manifest."""

import json
from pathlib import Path
import subprocess
import sys


CORE_POLICIES = [
    {
        "name": "__rawmemchr",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:__rawmemchr",
        "test": "compat/rawmemchr",
        "quality": "EXACT",
    },
    {
        "name": "backtrace",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:backtrace",
        "test": "compat/backtrace",
        "quality": "TRANSLATED",
    },
    {
        "name": "fallocate64",
        "version": "GLIBC_2.10",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:fallocate64",
        "test": "compat/fallocate64",
        "quality": "EXACT",
    },
    {
        "name": "__fxstat64",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:__fxstat64",
        "test": "compat/stat_abi",
        "quality": "DEGRADED",
    },
]

LIBC_POLICIES = [
    {
        "name": "_IO_getc",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:_IO_getc",
        "test": "compat/io_getc",
        "quality": "DEGRADED",
    },
    {
        "name": "_IO_putc",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:_IO_putc",
        "test": "compat/io_putc",
        "quality": "DEGRADED",
    },
    {
        "name": "__assert_fail",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__assert_fail",
        "test": "compat/assert_fail",
        "quality": "DEGRADED",
    },
    {
        "name": "__ctype_b_loc",
        "version": "GLIBC_2.3",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__ctype_b_loc",
        "test": "compat/ctype_b_loc",
        "quality": "DEGRADED",
    },
    {
        "name": "__ctype_get_mb_cur_max",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__ctype_get_mb_cur_max",
        "test": "compat/ctype_get_mb_cur_max",
        "quality": "DEGRADED",
    },
    {
        "name": "__cxa_atexit",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__cxa_atexit",
        "test": "compat/cxa_atexit",
        "quality": "DEGRADED",
    },
    {
        "name": "__cxa_finalize",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__cxa_finalize",
        "test": "compat/cxa_finalize",
        "quality": "STUB",
    },
    {
        "name": "__cxa_finalize",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "weak",
        "implementation": "musl-libc:__cxa_finalize",
        "test": "compat/cxa_finalize",
        "quality": "STUB",
    },
    {
        "name": "__duplocale",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__duplocale",
        "test": "compat/locale_ownership",
        "quality": "EXACT",
    },
    {
        "name": "__errno_location",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__errno_location",
        "test": "compat/errno_location",
        "quality": "EXACT",
    },
    {
        "name": "__errno_location",
        "version": "GLIBC_2.2.5",
        "soname": "libpthread.so.0",
        "binding": "global",
        "implementation": "musl-libc:__errno_location",
        "test": "compat/errno_location",
        "quality": "EXACT",
    },
    {
        "name": "__freelocale",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__freelocale",
        "test": "compat/locale_ownership",
        "quality": "EXACT",
    },
    {
        "name": "__fxstat",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__fxstat",
        "test": "compat/stat_abi",
        "quality": "DEGRADED",
    },
    {
        "name": "__fxstatat",
        "version": "GLIBC_2.4",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__fxstatat",
        "test": "compat/stat_abi",
        "quality": "DEGRADED",
    },
    {
        "name": "__getdelim",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__getdelim",
        "test": "compat/getdelim",
        "quality": "DEGRADED",
    },
    {
        "name": "__isoc99_fscanf",
        "version": "GLIBC_2.7",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__isoc99_fscanf",
        "test": "compat/scanf_abi",
        "quality": "DEGRADED",
    },
    {
        "name": "__isoc99_sscanf",
        "version": "GLIBC_2.7",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__isoc99_sscanf",
        "test": "compat/scanf_abi",
        "quality": "DEGRADED",
    },
    {
        "name": "__iswctype_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__iswctype_l",
        "test": "compat/wctype_l",
        "quality": "DEGRADED",
    },
    {
        "name": "__libc_current_sigrtmin",
        "version": "GLIBC_2.2.5",
        "soname": "libpthread.so.0",
        "binding": "global",
        "implementation": "musl-libc:__libc_current_sigrtmin",
        "test": "compat/sigrtmin",
        "quality": "TRANSLATED",
    },
]

CORE_EXCLUDED_SYMBOLS = {
    "__free_hook",
    "__malloc_hook",
    "__memalign_hook",
    "__realloc_hook",
    "__secure_getenv",
    "secure_getenv",
}


def fail(message):
    raise SystemExit(f"runtime manifest audit: {message}")


def exported_symbols(readelf, core):
    result = subprocess.run(
        [readelf, "--dyn-syms", "--wide", str(core)],
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
        exports.add(fields[7].split("@", 1)[0])
    return exports


def verify_policies(manifest, policies, exports):
    identity_fields = ("name", "version", "soname", "binding")
    for policy in policies:
        entries = [
            item
            for item in manifest["symbols"]
            if all(item[field] == policy[field] for field in identity_fields)
        ]
        identity = (
            f"{policy['name']}@{policy['version']} "
            f"{policy['soname']} {policy['binding']}"
        )
        if len(entries) != 1:
            fail(f"{identity} has {len(entries)} manifest entries")
        entry = entries[0]
        for field, expected in policy.items():
            if entry[field] != expected:
                fail(
                    f"{identity} {field} is {entry[field]!r}, "
                    f"expected {expected!r}"
                )
        name = policy["name"]
        if name not in exports:
            fail(f"{name} is absent from its runtime provider")


def main():
    if len(sys.argv) != 5:
        fail("usage: check_runtime_manifest.py READELF MANIFEST CORE LIBC")
    manifest = json.loads(Path(sys.argv[2]).read_text(encoding="utf-8"))
    core_exports = exported_symbols(
        sys.argv[1], Path(sys.argv[3]).resolve()
    )
    libc_exports = exported_symbols(
        sys.argv[1], Path(sys.argv[4]).resolve()
    )
    manifest_names = {item["name"] for item in manifest["symbols"]}
    for name in CORE_EXCLUDED_SYMBOLS:
        if name in manifest_names:
            fail(f"{name} excluded symbol must not be a manifest requirement")
        if name in core_exports:
            fail(f"{name} excluded symbol is unexpectedly exported")
    verify_policies(manifest, CORE_POLICIES, core_exports)
    verify_policies(manifest, LIBC_POLICIES, libc_exports)


if __name__ == "__main__":
    main()
