#!/usr/bin/env python3
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile


def fail(message, result=None):
    print(f"loader regression failure: {message}", file=sys.stderr)
    if result is not None:
        print(f"return code: {result.returncode}", file=sys.stderr)
        print(f"stdout: {result.stdout!r}", file=sys.stderr)
        print(f"stderr: {result.stderr!r}", file=sys.stderr)
    raise SystemExit(1)


def run(executable, argv, env, cwd=None):
    return subprocess.run(
        argv,
        executable=str(executable),
        cwd=cwd,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def needed(patchelf, path):
    result = subprocess.run(
        [patchelf, "--print-needed", str(path)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True,
    )
    return result.stdout.splitlines()


def prepare_fixture(patchelf, source, destination, loader):
    shutil.copy2(source, destination)
    subprocess.run(
        [patchelf, "--set-interpreter", str(loader), str(destination)],
        check=True,
    )
    names = needed(patchelf, destination)
    if "libc.so" in names:
        subprocess.run(
            [patchelf, "--replace-needed", "libc.so", "libc.so.6",
             str(destination)],
            check=True,
        )
    for name in needed(patchelf, destination):
        if name.startswith("libmusl-bsd-"):
            subprocess.run(
                [patchelf, "--remove-needed", name, str(destination)],
                check=True,
            )
    names = needed(patchelf, destination)
    if "libc.so.6" not in names:
        fail(f"fixture is not glibc-named: DT_NEEDED={names}")


def main():
    if len(sys.argv) != 9:
        fail("invalid runner arguments")

    loader, raw_target, core, facade_dir, plugin, user_preload, musl_linker, patchelf = (
        Path(value).resolve() if index < 7 else value
        for index, value in enumerate(sys.argv[1:])
    )
    patchelf = sys.argv[8]

    with tempfile.TemporaryDirectory(prefix="musl-bsd loader ") as temp_name:
        temp = Path(temp_name)
        target = temp / "glibc target with spaces"
        prepare_fixture(patchelf, raw_target, target, loader)

        env = os.environ.copy()
        env["MUSL_BSD_PRELOAD_PATH"] = str(core)
        env["MUSL_BSD_LIBRARY_PATH"] = f"{facade_dir}:/usr/lib"
        env.pop("LD_PRELOAD", None)

        for argv0, executable, cwd in (
            ("ordinary-program", target, None),
            ("", target, None),
            ("relative argv0", "./glibc target with spaces", temp),
        ):
            case_env = env.copy()
            case_env["MUSL_BSD_EXPECT_ARGV0"] = argv0
            result = run(executable, [argv0, "startup", "alpha", "two words"],
                         case_env, cwd)
            if result.returncode != 0:
                fail(f"startup/argv0 case {argv0!r}", result)

        result = run(
            target,
            ["owner", "facade-owner", str(facade_dir / "libutil.so.1")],
            env,
        )
        if result.returncode != 0:
            fail("dladdr facade ownership", result)

        trace = temp / "lifecycle.trace"
        lifecycle_env = env.copy()
        lifecycle_env["MUSL_BSD_TEST_TRACE"] = str(trace)
        result = run(target, ["lifecycle", "lifecycle", str(plugin)],
                     lifecycle_env)
        if result.returncode != 0:
            fail("constructor/destructor/dlopen lifecycle", result)
        expected = [
            "target-constructor",
            "main",
            "plugin-constructor",
            "plugin-entry",
            "after-dlclose",
            "plugin-destructor",
            "target-destructor",
        ]
        actual = trace.read_text().splitlines()
        if actual != expected:
            fail(f"lifecycle ordering: expected {expected}, got {actual}")

        result = run(target, ["exit", "exit", "42"], env)
        if result.returncode != 42:
            fail("exit-status propagation", result)

        result = run(target, ["signal", "signal"], env)
        if result.returncode != -signal.SIGTERM:
            fail("signal-termination propagation", result)

        result = run(target, ["recurse", "recurse"], env)
        if result.returncode != 0:
            fail("recursion-safe /proc/self/exe re-execution", result)

        preload_trace = temp / "preload.trace"
        preload_env = env.copy()
        preload_env["LD_PRELOAD"] = str(user_preload)
        preload_env["MUSL_BSD_TEST_PRELOAD_TRACE"] = str(preload_trace)
        result = run(target, ["preload", "preload-order"], preload_env)
        if result.returncode != 0:
            fail("core/user preload ordering", result)
        if preload_trace.read_text().splitlines() != [
            "user-preload-constructor"
        ]:
            fail("user LD_PRELOAD was not preserved exactly once")

        missing = temp / "missing-dependency"
        shutil.copy2(target, missing)
        missing_name = "libmusl-bsd-definitely-missing.so.0"
        subprocess.run(
            [patchelf, "--add-needed", missing_name, str(missing)],
            check=True,
        )
        result = run(missing, ["missing", "startup", "alpha", "two words"],
                     env)
        if result.returncode == 0 or missing_name not in result.stderr:
            fail("missing-library diagnostic", result)

        missing_target = temp / "target-does-not-exist"
        result = subprocess.run(
            [
                str(musl_linker),
                "--preload",
                str(core),
                "--library-path",
                env["MUSL_BSD_LIBRARY_PATH"],
                "--argv0",
                "missing",
                "--",
                str(missing_target),
            ],
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        if result.returncode == 0 or str(missing_target) not in result.stderr:
            fail("missing-target diagnostic", result)

        result = subprocess.run(
            [str(loader)],
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
        if result.returncode != 126 or "direct or recursive" not in result.stderr:
            fail("direct loader recursion rejection", result)


if __name__ == "__main__":
    main()
