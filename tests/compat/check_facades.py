#!/usr/bin/env python3
from pathlib import Path
import subprocess
import sys


def output(*args):
    return subprocess.run(
        args, check=True, text=True, stdout=subprocess.PIPE
    ).stdout


def main():
    if len(sys.argv) != 10:
        raise SystemExit("usage: check_facades.py READELF CORE FACADE...")
    readelf = sys.argv[1]
    core = Path(sys.argv[2])
    expected = {
        "libc.so.6": Path(sys.argv[3]),
        "libdl.so.2": Path(sys.argv[4]),
        "libm.so.6": Path(sys.argv[5]),
        "libpthread.so.0": Path(sys.argv[6]),
        "libresolv.so.2": Path(sys.argv[7]),
        "librt.so.1": Path(sys.argv[8]),
        "libutil.so.1": Path(sys.argv[9]),
    }

    core_dynamic = output(readelf, "-dW", str(core))
    if "Library soname: [libmusl-bsd-core.so.2]" not in core_dynamic:
        raise SystemExit("core has wrong SONAME")

    for soname, path in expected.items():
        dynamic = output(readelf, "-dW", str(path))
        symbols = output(readelf, "--dyn-syms", "-W", str(path))
        versions = output(readelf, "-VW", str(path))
        if f"Library soname: [{soname}]" not in dynamic:
            raise SystemExit(f"{path}: wrong SONAME")
        if "Shared library: [libmusl-bsd-core.so.2]" not in dynamic:
            raise SystemExit(f"{path}: no dependency on project core")
        if "GLIBC_2.2.5" not in versions:
            raise SystemExit(f"{path}: no glibc version definition")
        if soname == "libutil.so.1":
            if "musl_bsd_glibc_probe@@GLIBC_2.2.5" not in symbols:
                raise SystemExit(f"{path}: missing facade-owned probe")
        elif "musl_bsd_glibc_probe" in symbols:
            raise SystemExit(f"{path}: exports another facade's symbol")


if __name__ == "__main__":
    main()
