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
        "weak_undefined_symbols",
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
        discovered = [
            Path(name) / "libcuda.so.1",
            Path(name) / "libcudart.so.13",
        ]
        for path in discovered:
            path.write_bytes(Path(sys.executable).read_bytes())
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
        expected_roots = sorted(str(path.resolve()) for path in discovered)
        if report["roots"] != expected_roots:
            fail(f"unexpected discovered root: {report['roots']!r}")

    result = run(
        scanner,
        "--no-recursive",
        "--format",
        "json",
        str(consumer),
        str(provider),
    )
    if result.returncode != 0:
        fail("mixed versioned/unversioned weak symbols were rejected", result)
    mixed_weak = [
        symbol
        for symbol in json.loads(result.stdout)["summary"][
            "weak_undefined_symbols"
        ]
        if symbol["name"] == "optional_symbol"
    ]
    if {symbol["version"] for symbol in mixed_weak} != {
        None,
        "GLIBC_2.2.5",
    }:
        fail(f"mixed weak-symbol metadata was lost: {mixed_weak!r}")

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
    report = json.loads(result.stdout)
    undefined = {
        symbol["name"]: symbol
        for symbol in report["objects"][0]["undefined_symbols"]
    }
    if undefined["required_symbol"]["binding"] != "global":
        fail(f"mandatory symbol binding was lost: {undefined!r}")
    if undefined["optional_symbol"]["binding"] != "weak":
        fail(f"weak symbol binding was lost: {undefined!r}")
    weak_names = [
        symbol["name"]
        for symbol in report["summary"]["weak_undefined_symbols"]
    ]
    if (
        "optional_symbol" not in weak_names
        or len(weak_names) != len(set(weak_names))
    ):
        fail(f"weak symbol summary was not consolidated: {report!r}")
    analysis = report["provider_analysis"]
    if (
        analysis["matching_policy"] != "musl-name-based-runtime-resolution"
        or analysis["provided_count"] != 1
        or analysis["missing_count"] != 0
        or analysis["unresolved_weak_count"] != 1
        or analysis["unresolved_weak"][0]["name"] != "optional_symbol"
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
