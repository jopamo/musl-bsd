#!/usr/bin/env python3
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: run_preloaded_test.py TEST CORE PATCHELF")
    test, core = map(lambda value: Path(value).resolve(), sys.argv[1:3])
    patchelf = sys.argv[3]

    with tempfile.TemporaryDirectory(prefix="musl-bsd-preload-") as temp:
        fixture = Path(temp) / test.name
        shutil.copy2(test, fixture)
        names = subprocess.run(
            [patchelf, "--print-needed", str(fixture)],
            check=True,
            text=True,
            stdout=subprocess.PIPE,
        ).stdout.splitlines()
        for name in names:
            if name.startswith("libmusl-bsd-"):
                subprocess.run(
                    [patchelf, "--remove-needed", name, str(fixture)],
                    check=True,
                )

        env = os.environ.copy()
        env["LD_PRELOAD"] = str(core)
        result = subprocess.run([str(fixture)], env=env, check=False)
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
