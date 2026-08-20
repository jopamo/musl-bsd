# musl-bsd

`musl-bsd` helps software written for BSD or glibc compile and run on musl-based systems. It provides a focused set of compatibility libraries and headers for APIs that many C projects assume are available, reducing the need for per-project portability patches.

- `libfts`: BSD file-tree traversal APIs for walking directory hierarchies (`fts_open`, `fts_read`, and related functions)
- `libobstack`: GNU obstack allocation APIs for efficient incremental object construction
- `libargp`: GNU `argp` command-line parsing APIs used by tools that depend on `argp_parse` behavior
- Compatibility headers in `include/` (installed as headers like `sys/queue.h`, `sys/tree.h`, and `sys/cdefs.h`)

## glibc Binary Runtime

The source overlay remains portable, but the glibc binary-runtime bridge has a
separate qualification gate:

```sh
meson setup build -Dglibc_runtime=auto
```

`auto` enables the runtime only for x86_64 LP64. `enabled` fails configuration
on every unqualified ABI, and `disabled` omits the glibc-named interpreter and
facade artifacts. The generated
`/usr/lib/musl-bsd/compat-runtime.json` records the architecture, ABI,
qualification result, interpreter name, and musl linker path.

Runtime payload is private:

```text
/usr/lib/musl-bsd/
├── compat-runtime.json
├── glibc/
│   ├── libc.so.6
│   ├── libdl.so.2
│   ├── libm.so.6
│   ├── libpthread.so.0
│   ├── libresolv.so.2
│   ├── librt.so.1
│   └── libutil.so.1
├── libmusl-bsd-core.so.2
└── loader/
    └── ld-linux-x86-64.so.2
```

The glibc-named interpreter alias points into `loader/`. It executes musl's
loader with an explicit trusted core preload and private library path while
preserving the target's `argv[0]`, remaining arguments, exit status, and signal
termination. User `LD_PRELOAD` is not rewritten; its entries follow the core
in the explicit musl preload list.

NVIDIA's initial-exec TLS library must exist in the process's initial static
TLS set. Set `MUSL_BSD_NVIDIA_TLS_PATH` to its absolute installed path when
launching an NVIDIA compatibility target. Preload order is fixed:

```text
musl-bsd core → NVIDIA TLS → user LD_PRELOAD
```

The loader does not guess driver versions or search for NVIDIA TLS. Secure
execution is rejected before this environment-controlled path is read.

### Security boundary

Secure execution is intentionally unsupported. The interpreter reads
`AT_SECURE` before any environment-controlled compatibility state. If
`AT_SECURE` is nonzero, or real and effective credentials differ, it exits
immediately with:

```text
musl-bsd loader: secure execution is unsupported (AT_SECURE); refusing to continue
```

No environment option enables or weakens secure execution.

### Current musl SONAME behavior

musl 1.2.x resolves integrated legacy names such as `libc.so.6`,
`libdl.so.2`, `libpthread.so.0`, `librt.so.1`, and `libutil.so.1` to its own
loader/libc object instead of opening a same-named file from
`--library-path`. The project installs real, correctly named private facade
DSOs and verifies their metadata and explicit `dlopen()` ownership, but a
normal `DT_NEEDED` edge does not map those facades without a separately
qualified musl loader change. musl-bsd does not inject all facades as a
workaround.

### Static-PIE baseline

`compat/static_pie_baseline` is a standalone, libc-minimal static PIE. It must
run before the loader path can be accepted. A compiler wrapper that combines a
Clang executable with GCC-only `-specs` files, or injects an installed
compatibility DSO into test programs, is a test-toolchain construction defect;
the runtime does not compensate for it. The compatibility suite also checks
that the result is ET_DYN, has no `PT_INTERP`, and contains relative
relocations for static-PIE startup.

## NVIDIA ELF Inventory

`tools/nvidia-scan` inspects locally installed NVIDIA DSOs and recursively
follows their `DT_NEEDED` dependencies. It does not download drivers, invoke a
package manager, or treat unresolved dependencies as satisfied:

```sh
NVIDIA_LIBDIR=/usr/lib \
  tools/nvidia-scan --format json --output nvidia-inventory.json
```

The report records SONAMEs, versioned undefined symbols, TLS relocations,
IFUNC/IRELATIVE use, relocation types, and a consolidated compatibility
requirement list. Pass explicit DSO paths to avoid automatic NVIDIA filename
discovery.

Provider analysis compares those requirements with explicit runtime ELFs:

```sh
tools/nvidia-scan --format json \
  --provider /lib/libc.so \
  --provider build/libmusl-bsd-core.so.2.0.0 \
  --provider-alias ftruncate64=ftruncate \
  --provider-alias statfs64=statfs
```

Aliases are accepted only when the configured providers export the target
symbol. The report labels this as musl's name-based runtime resolution; it does
not claim glibc version-quality equivalence. `--strict` makes unresolved
`DT_NEEDED` entries or provider-backed symbol requirements a command failure.
Malformed, unsupported, or ambiguous input always fails.

The local proprietary-driver loader check is opt-in:

```sh
NVIDIA_LIBDIR=/usr/lib \
  meson test -C build --suite nvidia --print-errorlogs
```

It verifies that loading `libnvidia-glcore` fails without early NVIDIA TLS and
succeeds through the complete compatibility-interpreter path when the explicit
TLS policy is enabled.

## API At A Glance

- `libfts` types: `FTS`, `FTSENT`
- `libfts` functions: `fts_open`, `fts_read`, `fts_children`, `fts_set`, `fts_close`
- `libfts` traversal/config constants: `FTS_LOGICAL`, `FTS_PHYSICAL`, `FTS_NOCHDIR`, `FTS_XDEV`, `FTS_SEEDOT`
- `libfts` entry/result constants: `FTS_D`, `FTS_DP`, `FTS_F`, `FTS_SL`, `FTS_ERR`, `FTS_NS`

- `libobstack` type: `struct obstack`
- `libobstack` setup/lifecycle: `obstack_init`, `obstack_begin`, `obstack_specify_allocation`, `obstack_free`
- `libobstack` object building helpers: `obstack_grow`, `obstack_grow0`, `obstack_1grow`, `obstack_blank`, `obstack_finish`, `obstack_copy`, `obstack_copy0`
- `libobstack` inspection/helpers: `obstack_base`, `obstack_object_size`, `obstack_memory_used`, `obstack_printf`, `obstack_vprintf`, `obstack_calculate_object_size`

- `libargp` core types: `struct argp_option`, `struct argp`, `struct argp_state`, `argp_parser_t`
- `libargp` parsing/help functions: `argp_parse`, `argp_help`, `argp_state_help`, `argp_usage`, `argp_error`, `argp_failure`
- `libargp` parser-key constants: `ARGP_KEY_ARG`, `ARGP_KEY_ARGS`, `ARGP_KEY_INIT`, `ARGP_KEY_END`, `ARGP_KEY_SUCCESS`, `ARGP_KEY_ERROR`
- `libargp` option flags: `OPTION_ARG_OPTIONAL`, `OPTION_HIDDEN`, `OPTION_ALIAS`, `OPTION_DOC`, `OPTION_NO_USAGE`

- `sys/queue.h` macro families: `SLIST_*`, `LIST_*`, `STAILQ_*`, `TAILQ_*`, `CIRCLEQ_*`
- `sys/tree.h` macro families: `RB_*`, `SPLAY_*`
- `sys/cdefs.h` attributes/visibility macros: `__dead`, `__pure`, `__packed`, `__aligned`, `__BEGIN_DECLS`, `__END_DECLS`


## Testing and Coverage

Tests are grouped by component under `tests/`. Meson test IDs follow the
`component/behavior` convention, and each component is also a Meson suite:

```sh
meson setup build
meson test -C build --print-errorlogs
meson test -C build --suite fts --print-errorlogs
```

See [`tests/README.md`](tests/README.md) for the test layout and naming rules.

Use the canonical coverage command:

```sh
./scripts/coverage.sh build-coverage
```

This runs the full Meson test suite and writes reports to:

- `build-coverage/meson-logs/coverage.txt`
- `build-coverage/coverage/index.html`
- `build-coverage/coverage/coverage.xml`
- `build-coverage/coverage/summary.json`
- `build-coverage/coverage/src-summary.json`

Coverage collection is configured via `gcovr.cfg`. It excludes `libfts.so.*.p` object directories so `gcovr` does not merge duplicate `src/fts.c` instrumentation from both shared and test-static targets.

## License

This repository is mixed-license. Canonical license texts have been added under `LICENSES/`:

- `LICENSES/LGPL-2.1-or-later.txt` for `src/argp/*` (per file headers)
- `LICENSES/GPL-2.0-or-later.txt` as the GPL text referenced by LGPL-2.1 terms
- `LICENSES/BSD-2-Clause.txt` and `LICENSES/BSD-3-Clause.txt` for BSD-family licensing coverage
- `include/fts.h` now uses explicit `SPDX-License-Identifier: BSD-3-Clause`
