# Test suite

Tests are owned by the component they exercise:

- `abi/`: compile-time and compiler-dump ABI checks
- `argp/`: focused API tests and integration examples
- `cdefs/`: `sys/cdefs.h` behavior
- `compat/`: compatibility runtime and loader behavior
- `fts/`: file-tree traversal behavior
- `headers/`: overlay header compile/link checks
- `obstack/`: allocation and object-building behavior

Each component has its own `meson.build`. The root test build file only
dispatches to those component files.

## Naming

- Focused C tests are named `test_<behavior>.c`.
- Shared fixtures and assertions are named by responsibility, such as
  `test_fixture.c`, `test_assertions.c`, and `test_support.h`.
- Meson test IDs are `<component>/<behavior>`, without a redundant `test_`
  prefix.
- Every component sets a matching Meson suite, so a subsystem can be run in
  isolation.
- Example programs use descriptive names under `argp/examples/`; opaque
  upstream names such as `ex3` are not exposed as test IDs.

## Running tests

From the repository root:

```sh
meson setup build
meson test -C build --print-errorlogs
meson test -C build --suite fts --print-errorlogs
meson test -C build 'argp/*' --print-errorlogs
```

The compatibility overlay targets are intended for a musl build environment.
On a glibc host, run component suites whose targets do not include that
overlay, or build in the distro's normal musl environment.
