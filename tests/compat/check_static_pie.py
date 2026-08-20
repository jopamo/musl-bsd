#!/usr/bin/env python3
"""Assert the ELF properties required by the static-PIE startup baseline."""

from pathlib import Path
import subprocess
import sys


def output(readelf, *arguments, target):
    result = subprocess.run(
        [readelf, *arguments, str(target)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )
    return result.stdout


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: check_static_pie.py READELF TARGET")
    readelf = sys.argv[1]
    target = Path(sys.argv[2])

    header = output(readelf, "-hW", target=target)
    if "Type:                              DYN" not in header:
        raise SystemExit(f"{target}: -static-pie did not produce ET_DYN")

    program_headers = output(readelf, "-lW", target=target)
    if "INTERP" in program_headers or "Requesting program interpreter" in program_headers:
        raise SystemExit(f"{target}: static PIE unexpectedly has PT_INTERP")

    relocations = output(readelf, "-rW", target=target)
    if "R_X86_64_RELATIVE" not in relocations:
        raise SystemExit(
            f"{target}: static PIE has no relative relocations to apply at startup"
        )


if __name__ == "__main__":
    main()
