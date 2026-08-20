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
    {
        "name": "__lxstat64",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:__lxstat64",
        "test": "compat/stat_abi",
        "quality": "DEGRADED",
    },
    {
        "name": "__register_atfork",
        "version": "GLIBC_2.3.2",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:__register_atfork",
        "test": "compat/atfork",
        "quality": "DEGRADED",
    },
    {
        "name": "__sched_cpualloc",
        "version": "GLIBC_2.7",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:__sched_cpualloc",
        "test": "compat/sched_abi",
        "quality": "EXACT",
    },
    {
        "name": "__sched_cpufree",
        "version": "GLIBC_2.7",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:__sched_cpufree",
        "test": "compat/sched_abi",
        "quality": "EXACT",
    },
    {
        "name": "__strdup",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:__strdup",
        "test": "compat/strdup_abi",
        "quality": "EXACT",
    },
    {
        "name": "__strftime_l",
        "version": "GLIBC_2.3",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-bsd-core:__strftime_l",
        "test": "compat/strftime_abi",
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
    {
        "name": "__lxstat",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__lxstat",
        "test": "compat/stat_abi",
        "quality": "DEGRADED",
    },
    {
        "name": "__newlocale",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__newlocale",
        "test": "compat/locale_ownership",
        "quality": "DEGRADED",
    },
    {
        "name": "__nl_langinfo_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__nl_langinfo_l",
        "test": "compat/locale_ownership",
        "quality": "DEGRADED",
    },
    {
        "name": "__progname_full",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__progname_full",
        "test": "compat/program_name",
        "quality": "EXACT",
    },
    {
        "name": "__sched_cpucount",
        "version": "GLIBC_2.6",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__sched_cpucount",
        "test": "compat/sched_abi",
        "quality": "EXACT",
    },
    {
        "name": "__strcoll_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__strcoll_l",
        "test": "compat/locale_ownership",
        "quality": "DEGRADED",
    },
    {
        "name": "__strtod_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__strtod_l",
        "test": "compat/strto_l",
        "quality": "DEGRADED",
    },
    {
        "name": "__strtof_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__strtof_l",
        "test": "compat/strto_l",
        "quality": "DEGRADED",
    },
    {
        "name": "__strxfrm_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__strxfrm_l",
        "test": "compat/locale_ownership",
        "quality": "DEGRADED",
    },
    {
        "name": "__tls_get_addr",
        "version": "GLIBC_2.3",
        "soname": "ld-linux-x86-64.so.2",
        "binding": "global",
        "implementation": "musl-libc:__tls_get_addr",
        "test": "compat/tls_abi",
        "quality": "EXACT",
    },
    {
        "name": "__towlower_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__towlower_l",
        "test": "compat/wctype_l",
        "quality": "DEGRADED",
    },
    {
        "name": "__towupper_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__towupper_l",
        "test": "compat/wctype_l",
        "quality": "DEGRADED",
    },
    {
        "name": "__uselocale",
        "version": "GLIBC_2.3",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__uselocale",
        "test": "compat/locale_ownership",
        "quality": "EXACT",
    },
    {
        "name": "__wcscoll_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__wcscoll_l",
        "test": "compat/locale_ownership",
        "quality": "DEGRADED",
    },
    {
        "name": "__wcsftime_l",
        "version": "GLIBC_2.3",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__wcsftime_l",
        "test": "compat/strftime_abi",
        "quality": "DEGRADED",
    },
    {
        "name": "__wcsxfrm_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__wcsxfrm_l",
        "test": "compat/locale_ownership",
        "quality": "DEGRADED",
    },
    {
        "name": "__wctype_l",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__wctype_l",
        "test": "compat/wctype_l",
        "quality": "DEGRADED",
    },
    {
        "name": "__xmknod",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__xmknod",
        "test": "compat/stat_abi",
        "quality": "DEGRADED",
    },
    {
        "name": "__xstat",
        "version": "GLIBC_2.2.5",
        "soname": "libc.so.6",
        "binding": "global",
        "implementation": "musl-libc:__xstat",
        "test": "compat/stat_abi",
        "quality": "DEGRADED",
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
