#!/usr/bin/env python3
"""Black-box tests for the local NVIDIA ELF inventory tool."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile


def run(scanner, *arguments):
    return subprocess.run(
        [sys.executable, str(scanner), *arguments],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def fail(message, result=None):
    if result is not None:
        message += f"\nstdout={result.stdout!r}\nstderr={result.stderr!r}"
    raise SystemExit(message)


def main():
    if len(sys.argv) != 4:
        raise SystemExit(
            "usage: test_nvidia_scan.py SCANNER CONSUMER PROVIDER"
        )
    scanner = Path(sys.argv[1]).resolve()
    consumer = Path(sys.argv[2]).resolve()
    provider = Path(sys.argv[3]).resolve()

    result = run(
        scanner,
        "--no-recursive",
        "--format",
        "json",
        sys.executable,
    )
    if result.returncode != 0:
        fail("scanner rejected the host Python ELF", result)
    try:
        report = json.loads(result.stdout)
    except json.JSONDecodeError as error:
        fail(f"scanner did not produce JSON: {error}")
    if report["format"] != 1 or len(report["objects"]) != 1:
        fail(f"unexpected report envelope: {report!r}")
    object_report = report["objects"][0]
    expected_elf = {
        "class": "ELF64",
        "data": "little-endian",
        "machine": "x86_64",
        "type": object_report["elf"]["type"],
    }
    if object_report["elf"] != expected_elf:
        fail(f"unexpected ELF identity: {object_report['elf']!r}")
    for key in (
        "needed",
        "undefined_symbols",
        "glibc_versions",
        "tls",
        "ifunc_symbols",
        "irelative_relocations",
        "relocation_types",
    ):
        if key not in object_report:
            fail(f"missing per-object field: {key}")
    for key in (
        "required_glibc_sonames",
        "required_glibc_versions",
        "required_glibc_symbols",
        "tls_relocation_types",
        "compatibility_requirements",
    ):
        if key not in report["summary"]:
            fail(f"missing summary field: {key}")

    with tempfile.TemporaryDirectory(prefix="musl-bsd-nvidia-scan-") as name:
        bad = Path(name) / "not-an-elf"
        bad.write_bytes(b"not an ELF file\n")
        result = run(scanner, str(bad))
        if result.returncode != 2 or "not an ELF file" not in result.stderr:
            fail("malformed ELF input was not rejected", result)

        # Directory discovery must be local and deterministic; it must not
        # require an NVIDIA installation just to exercise the naming rules.
        discovered = Path(name) / "libcuda.so.1"
        discovered.write_bytes(Path(sys.executable).read_bytes())
        result = run(
            scanner,
            "--no-recursive",
            "--format",
            "json",
            name,
        )
        if result.returncode != 0:
            fail("directory target discovery failed", result)
        report = json.loads(result.stdout)
        if report["roots"] != [str(discovered.resolve())]:
            fail(f"unexpected discovered root: {report['roots']!r}")

    result = run(
        scanner,
        "--no-recursive",
        "--format",
        "json",
        "--provider",
        str(provider),
        "--provider-alias",
        "required_symbol=provided_symbol",
        str(consumer),
    )
    if result.returncode != 0:
        fail("explicit provider alias was rejected", result)
    analysis = json.loads(result.stdout)["provider_analysis"]
    if (
        analysis["matching_policy"] != "musl-name-based-runtime-resolution"
        or analysis["provided_count"] != 1
        or analysis["missing_count"] != 0
        or analysis["provided"][0]["resolution"] != "alias"
    ):
        fail(f"unexpected provider analysis: {analysis!r}")

    result = run(
        scanner,
        "--no-recursive",
        "--strict",
        "--format",
        "json",
        "--provider",
        str(provider),
        str(consumer),
    )
    if result.returncode != 2:
        fail("strict provider analysis did not reject a missing symbol", result)
    analysis = json.loads(result.stdout)["provider_analysis"]
    if (
        analysis["missing_count"] != 1
        or analysis["missing"][0]["name"] != "required_symbol"
    ):
        fail(f"missing provider requirement was not reported: {analysis!r}")


if __name__ == "__main__":
    main()
