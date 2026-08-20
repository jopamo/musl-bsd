#!/usr/bin/env python3
"""Black-box tests for the NVIDIA/CUDA symbol manifest tool."""

import json
from pathlib import Path
import subprocess
import sys
import tempfile


def run(*arguments):
    return subprocess.run(
        [str(argument) for argument in arguments],
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
    if len(sys.argv) != 6:
        raise SystemExit(
            "usage: test_nvidia_manifest.py TOOL SCANNER CONSUMER "
            "PROVIDER COMMITTED_MANIFEST"
        )
    tool, scanner, consumer, provider, committed = (
        Path(value).resolve() for value in sys.argv[1:]
    )

    result = run(tool, "validate", committed)
    if result.returncode != 0:
        fail("committed NVIDIA manifest is invalid", result)

    with tempfile.TemporaryDirectory(
        prefix="musl-bsd-nvidia-manifest-"
    ) as name:
        directory = Path(name)
        inventory = directory / "inventory.json"
        manifest = directory / "manifest.json"
        result = run(
            sys.executable,
            scanner,
            "--no-recursive",
            "--format",
            "json",
            "--output",
            inventory,
            "--provider",
            provider,
            "--provider-alias",
            "required_symbol=provided_symbol",
            consumer,
        )
        if result.returncode != 0:
            fail("cannot construct manifest test inventory", result)

        result = run(tool, "generate", inventory, "--output", manifest)
        if result.returncode != 0:
            fail("manifest generation failed", result)
        result = run(tool, "validate", manifest)
        if result.returncode != 0:
            fail("generated manifest failed validation", result)
        result = run(tool, "check", manifest, inventory)
        if result.returncode != 0:
            fail("generated manifest does not match its inventory", result)

        generated = json.loads(manifest.read_text(encoding="utf-8"))
        scanned = json.loads(inventory.read_text(encoding="utf-8"))
        requirements = {
            item["name"]: item
            for item in scanned["summary"]["required_glibc_symbols"]
        }
        symbols = {item["name"]: item for item in generated["symbols"]}
        if set(symbols) != {"optional_symbol", "required_symbol"}:
            fail(f"unexpected generated symbols: {symbols!r}")
        required = symbols["required_symbol"]
        optional = symbols["optional_symbol"]
        if (
            required["version"] != "GLIBC_2.2.5"
            or required["soname"] != requirements["required_symbol"]["library"]
            or required["binding"] != "global"
            or required["implementation"]
            != f"elf-provider[{provider.name}]:provided_symbol"
            or required["quality"] != "UNSUPPORTED"
        ):
            fail(f"required-symbol policy was not normalized: {required!r}")
        if (
            optional["binding"] != "weak"
            or optional["implementation"] != "optional-unresolved"
            or optional["quality"] != "UNSUPPORTED"
        ):
            fail(f"optional-symbol policy was not normalized: {optional!r}")

        promoted = dict(generated)
        promoted["symbols"] = [dict(item) for item in generated["symbols"]]
        promoted["symbols"][0]["test"] = "compat/focused"
        promoted["symbols"][0]["quality"] = "EXACT"
        promoted_path = directory / "promoted.json"
        promoted_path.write_text(json.dumps(promoted), encoding="utf-8")
        merged_path = directory / "merged.json"
        result = run(
            tool,
            "generate",
            inventory,
            "--base",
            promoted_path,
            "--output",
            merged_path,
        )
        if result.returncode != 0:
            fail("manifest policy merge failed", result)
        merged = json.loads(merged_path.read_text(encoding="utf-8"))
        if (
            merged["symbols"][0]["test"] != "compat/focused"
            or merged["symbols"][0]["quality"] != "EXACT"
        ):
            fail("manifest regeneration discarded audited policy")

        invalid = dict(generated)
        invalid["symbols"] = [dict(item) for item in generated["symbols"]]
        invalid["symbols"][0]["quality"] = "ASSUMED"
        invalid_path = directory / "invalid.json"
        invalid_path.write_text(json.dumps(invalid), encoding="utf-8")
        result = run(tool, "validate", invalid_path)
        if result.returncode != 2 or "invalid quality" not in result.stderr:
            fail("unknown manifest quality did not fail closed", result)

        private = dict(generated)
        private["symbols"] = [dict(item) for item in generated["symbols"]]
        private["symbols"][0]["version"] = "GLIBC_PRIVATE"
        private_path = directory / "private.json"
        private_path.write_text(json.dumps(private), encoding="utf-8")
        result = run(tool, "validate", private_path)
        if result.returncode != 2 or "unsupported symbol version" not in result.stderr:
            fail("GLIBC_PRIVATE manifest entry did not fail closed", result)

        wrong_provider = dict(generated)
        wrong_provider["symbols"] = [
            dict(item) for item in generated["symbols"]
        ]
        wrong_provider["symbols"][0]["implementation"] = "musl-libc:wrong"
        wrong_provider_path = directory / "wrong-provider.json"
        wrong_provider_path.write_text(
            json.dumps(wrong_provider), encoding="utf-8"
        )
        result = run(tool, "check", wrong_provider_path, inventory)
        if (
            result.returncode != 2
            or "implementation does not match" not in result.stderr
        ):
            fail("provider implementation mismatch was accepted", result)

        incomplete = dict(generated)
        incomplete["symbols"] = generated["symbols"][1:]
        incomplete_path = directory / "incomplete.json"
        incomplete_path.write_text(json.dumps(incomplete), encoding="utf-8")
        result = run(tool, "check", incomplete_path, inventory)
        if result.returncode != 2 or "missing requirements" not in result.stderr:
            fail("missing manifest requirement did not fail closed", result)

        no_analysis = json.loads(inventory.read_text(encoding="utf-8"))
        del no_analysis["provider_analysis"]
        no_analysis_path = directory / "no-analysis.json"
        no_analysis_path.write_text(json.dumps(no_analysis), encoding="utf-8")
        result = run(tool, "generate", no_analysis_path)
        if result.returncode != 2 or "provider_analysis" not in result.stderr:
            fail("inventory without provider analysis was accepted", result)


if __name__ == "__main__":
    main()
