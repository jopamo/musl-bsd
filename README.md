# musl-bsd

`musl-bsd` provides compatibility libraries, headers, and an optional binary-runtime bridge for software that expects BSD or glibc interfaces on musl-based systems.

The project has two distinct roles:

1. **Source compatibility** for software that can be rebuilt against musl.
2. **Optional glibc binary compatibility** for explicitly qualified x86_64
   LP64 binaries.

> [!IMPORTANT]
> The glibc binary-runtime bridge is intentionally limited and qualified. It is not a blanket replacement for glibc, and secure execution is not supported.

## Features

### Source compatibility

`musl-bsd` provides:

- **libfts** — BSD file-tree traversal APIs such as `fts_open()` and `fts_read()`
- **libobstack** — GNU obstack allocation APIs
- **libargp** — GNU `argp` command-line parsing APIs
- **BSD compatibility headers** — including `sys/queue.h`, `sys/tree.h`, and `sys/cdefs.h`

The source-compatibility layer remains portable independently of the optional glibc binary runtime.

### Optional glibc binary runtime

The runtime bridge provides:

- a qualified compatibility host DSO in the standard ABI library directory
- glibc-named facade DSOs
- a glibc-named interpreter alias
- explicit runtime qualification metadata
- audited, manifest-backed compatibility policy
- explicit early-dependency preload handling
- symbol/provider auditing tools

The qualified runtime currently targets **x86_64 LP64**.

---

## Quick Start

### Source compatibility only

Configure without the glibc binary runtime:

```sh
meson setup build -Dglibc_runtime=disabled
```

Run the test suite:

```sh
meson test -C build --print-errorlogs
```

### Enable the glibc binary runtime automatically

```sh
meson setup build -Dglibc_runtime=auto
```

`auto` enables the runtime only on the qualified **x86_64 LP64** ABI.

Available modes:

| Mode | Behavior |
|---|---|
| `auto` | Enable the runtime only on a qualified ABI |
| `enabled` | Require the runtime; configuration fails on an unqualified ABI |
| `disabled` | Omit the glibc-named interpreter and facade artifacts |

The generated runtime qualification report is installed as:

```text
/usr/lib/musl-bsd/compat-runtime.json
```

It records the architecture, ABI, qualification result, interpreter name, and musl linker path.

### Explicit build interfaces

Consumers opt into exactly one compatibility contract through pkg-config:

| Module | Contract |
|---|---|
| `musl-bsd-headers` | Overlay headers only |
| `musl-bsd-source` | Overlay headers plus the PIC, hidden-visibility `libmusl-bsd-core.a` |
| `musl-bsd-glibc-host` | Force the qualified host DSO into `DT_NEEDED`; installed only for a qualified ABI |
| `musl-bsd-glibc-startup` | Force the host DSO and every private glibc facade into the initial dependency graph; installed only for a qualified ABI |

`musl-bsd-source` names the archive exactly and cannot fall back to a shared
object. `musl-bsd-glibc-host` names the versioned host DSO exactly, scopes
`--no-as-needed` with linker push/pop state, and cannot fall back to the source
archive. `musl-bsd-glibc-startup` uses exact DSO paths, publishes no ambient
library search path, and records only the private facade directory in the
consumer RUNPATH.

The source overlay maps ABI-identical glibc large-file names such as
`pread64`, `pwrite64`, and `open64` onto musl's native 64-bit interfaces.
Musl-native shared libraries must therefore retain native libc imports; real
glibc `*64` ABI symbols are exported only by the qualified foreign-DSO host.

---

## Runtime Layout

Source compatibility and glibc facade payload remain private. The qualified
host DSO is installed in the normal ABI library directory so musl can resolve
its SONAME without a consumer RUNPATH:

```text
/usr/lib/
├── libmusl-bsd-glibc-host.so.2
└── musl-bsd/
    ├── compat-runtime.json
    ├── libmusl-bsd-core.a
    ├── overlay/include/
    ├── glibc/
    │   ├── libc.so.6
    │   ├── libdl.so.2
    │   ├── libm.so.6
    │   ├── libpthread.so.0
    │   ├── libresolv.so.2
    │   ├── librt.so.1
    │   └── libutil.so.1
    └── loader/
        └── ld-linux-x86-64.so.2
```

There is no unversioned `libmusl-bsd-glibc-host.so` linker name.

The glibc-named interpreter alias points into `loader/`. It launches musl's loader with:

- the trusted `musl-bsd` glibc-host DSO preloaded
- the private compatibility library path
- the original target `argv[0]`
- the original arguments
- normal exit-status and signal propagation

User `LD_PRELOAD` entries are preserved rather than rewritten.

---

## Security Model

Secure execution is intentionally unsupported.

The compatibility interpreter checks `AT_SECURE` before reading environment-controlled compatibility state. It also rejects execution when real and effective credentials differ.

When secure execution is detected, the loader exits immediately:

```text
musl-bsd loader: secure execution is unsupported (AT_SECURE); refusing to continue
```

No environment variable or runtime option can weaken this check.

> [!WARNING]
> Do not use the compatibility runtime for setuid, setgid, or other secure-execution workloads.

---

## ELF Inventory and Compatibility Policy

### Optional early preload

Some ELF binaries require a dependency to be present in the process's initial
static TLS or constructor set. Set an explicit early DSO when needed:

```sh
export MUSL_BSD_EARLY_PRELOAD_PATH=/absolute/path/to/required-early.so
```

The path must be absolute and must identify one DSO. The loader does not guess
package versions or search for this dependency. Preload order is fixed:

```text
musl-bsd glibc host → optional early DSO → user LD_PRELOAD
```

The secure-execution check runs before this environment variable is read.

### Scan ELF files

`tools/elf-scan` inspects explicit ELF files or directories and recursively
follows their `DT_NEEDED` dependencies. It does not download packages or invoke
a package manager.

```sh
tools/elf-scan --format json --output inventory.json /path/to/root.so
```

The report includes SONAMEs, undefined symbols, symbol bindings and versions,
TLS relocations, IFUNC/IRELATIVE use, relocation types, unresolved
dependencies, and consolidated compatibility requirements. Directory discovery
considers ELF files directly within the named directory; it does not perform a
vendor-shaped system search.

Requirements can be checked against explicit runtime ELF providers:

```sh
tools/elf-scan --format json \
  --provider /lib/libc.so \
  --provider build/libmusl-bsd-glibc-host.so.2.0.0 \
  --provider-alias ftruncate64=ftruncate \
  --provider-alias statfs64=statfs \
  /path/to/root.so
```

Provider aliases are accepted only when a configured provider exports the
aliased symbol. Use `--strict` to fail on unresolved mandatory dependencies or
provider-backed symbol requirements. Unresolved weak imports remain optional.

### Compatibility manifest

`compatibility-symbols.json` is the checked runtime compatibility-policy
manifest. Validate it with:

```sh
tools/compatibility-manifest validate compatibility-symbols.json
```

Generate or check policy for any scanner inventory:

```sh
tools/compatibility-manifest generate inventory.json \
  --base compatibility-symbols.json \
  --output compatibility-symbols.new.json

tools/compatibility-manifest check \
  compatibility-symbols.new.json inventory.json
```

New requirements default to `UNSUPPORTED` until their ABI and behavior have
been audited. Policy quality levels are `EXACT`, `TRANSLATED`, `DEGRADED`,
`STUB`, and `UNSUPPORTED`.

## Important Runtime Limitations

### musl legacy SONAME resolution

Current musl 1.2.x behavior resolves integrated legacy names such as:

- `libc.so.6`
- `libdl.so.2`
- `libpthread.so.0`
- `librt.so.1`
- `libutil.so.1`

to musl's own loader/libc object instead of opening a same-named DSO from `--library-path`.

`musl-bsd` installs real private facade DSOs and verifies their metadata and explicit `dlopen()` ownership, but ordinary `DT_NEEDED` edges do not map those facades without a separately qualified musl loader change.

The runtime deliberately does **not** inject every facade as a workaround.

### `dlmopen()`

`LM_ID_BASE` is adapted to `dlopen()`.

`LM_ID_NEWLM` is **unsupported** because musl has no equivalent link-map namespace mechanism. Requests for a new namespace fail with `dlerror()` rather than silently falling back to the base namespace.

### `dlvsym()`

`dlvsym()` support is **degraded**.

musl cannot select among multiple glibc symbol-version definitions, so the adapter resolves by name only for the finite `GLIBC_*` version set covered by the compatibility manifest and direct probes.

Unknown versions, `GLIBC_PRIVATE`, and null versions fail with `dlerror()`.

### glibc allocator hooks

Some glibc binaries probe the historical allocator-hook names:

- `__malloc_hook`
- `__realloc_hook`
- `__free_hook`
- `__memalign_hook`

The observed probes allow all four to be absent.

`musl-bsd` therefore does not export writable hook state or pretend to provide allocator interposition that musl cannot support.

### Locale and libc differences

Some qualified compatibility paths are intentionally classified `DEGRADED` where musl and glibc differ in areas such as:

- locale databases
- collation behavior
- wide-character locale behavior
- selected `errno` details
- per-DSO finalization semantics
- legacy version-selector handling

The authoritative per-symbol policy is `compatibility-symbols.json`.

---

## Static PIE Qualification

`compat/static_pie_baseline` is a standalone libc-minimal static PIE used as a runtime qualification gate.

The compatibility suite verifies that it:

- is `ET_DYN`
- has no `PT_INTERP`
- contains the relative relocations required for static-PIE startup

Toolchain construction errors are treated as test/toolchain defects rather than hidden by the runtime.

---

## External ELF Loader Tests

The optional external suite can exercise any explicitly selected DSO:

```sh
MUSL_BSD_TEST_DSO=/absolute/path/to/root.so \
  meson test -C build --suite external --print-errorlogs
```

Set `MUSL_BSD_TEST_EARLY_PRELOAD_PATH` when that DSO needs an early dependency.
Set `MUSL_BSD_TEST_TLS_DSO` to opt into the generic multithreaded TLS probe.
Optional comma-separated `MUSL_BSD_TEST_EXPORTS` and
`MUSL_BSD_TEST_WEAK_SYMBOLS` lists enable publication, repeated-load, and weak
relocation checks for symbols selected by the user.
External binaries are never copied into the repository.

## API at a Glance

### libfts

Types:

- `FTS`
- `FTSENT`

Functions:

- `fts_open`
- `fts_read`
- `fts_children`
- `fts_set`
- `fts_close`

Traversal/configuration constants:

- `FTS_LOGICAL`
- `FTS_PHYSICAL`
- `FTS_NOCHDIR`
- `FTS_XDEV`
- `FTS_SEEDOT`

Entry/result constants:

- `FTS_D`
- `FTS_DP`
- `FTS_F`
- `FTS_SL`
- `FTS_ERR`
- `FTS_NS`

### libobstack

Type:

- `struct obstack`

Setup and lifecycle:

- `obstack_init`
- `obstack_begin`
- `obstack_specify_allocation`
- `obstack_free`

Object construction:

- `obstack_grow`
- `obstack_grow0`
- `obstack_1grow`
- `obstack_blank`
- `obstack_finish`
- `obstack_copy`
- `obstack_copy0`

Inspection and helpers:

- `obstack_base`
- `obstack_object_size`
- `obstack_memory_used`
- `obstack_printf`
- `obstack_vprintf`
- `obstack_calculate_object_size`

### libargp

Core types:

- `struct argp_option`
- `struct argp`
- `struct argp_state`
- `argp_parser_t`

Parsing and help:

- `argp_parse`
- `argp_help`
- `argp_state_help`
- `argp_usage`
- `argp_error`
- `argp_failure`

Parser keys:

- `ARGP_KEY_ARG`
- `ARGP_KEY_ARGS`
- `ARGP_KEY_INIT`
- `ARGP_KEY_END`
- `ARGP_KEY_SUCCESS`
- `ARGP_KEY_ERROR`

Option flags:

- `OPTION_ARG_OPTIONAL`
- `OPTION_HIDDEN`
- `OPTION_ALIAS`
- `OPTION_DOC`
- `OPTION_NO_USAGE`

### BSD headers

`sys/queue.h` provides:

- `SLIST_*`
- `LIST_*`
- `STAILQ_*`
- `TAILQ_*`
- `CIRCLEQ_*`

`sys/tree.h` provides:

- `RB_*`
- `SPLAY_*`

`sys/cdefs.h` provides compatibility macros including:

- `__dead`
- `__pure`
- `__packed`
- `__aligned`
- `__BEGIN_DECLS`
- `__END_DECLS`

---

## Testing

Tests are grouped by component under `tests/`.

Meson test IDs use the:

```text
component/behavior
```

convention, and each component is also a Meson suite.

Run all tests:

```sh
meson setup build
meson test -C build --print-errorlogs
```

Run a single component suite:

```sh
meson test -C build --suite fts --print-errorlogs
```

See [`tests/README.md`](tests/README.md) for the test layout and naming rules.

---

## Coverage

Use the canonical coverage command:

```sh
./scripts/coverage.sh build-coverage
```

Reports are written to:

```text
build-coverage/meson-logs/coverage.txt
build-coverage/coverage/index.html
build-coverage/coverage/coverage.xml
build-coverage/coverage/summary.json
build-coverage/coverage/src-summary.json
```

Coverage is configured through `gcovr.cfg`.

---

## License

This repository is mixed-license.

Canonical license texts are under `LICENSES/`:

- `LICENSES/LGPL-2.1-or-later.txt` — `src/argp/*`
- `LICENSES/GPL-2.0-or-later.txt` — GPL text referenced by LGPL-2.1 terms
- `LICENSES/BSD-2-Clause.txt`
- `LICENSES/BSD-3-Clause.txt`

`include/fts.h` uses:

```text
SPDX-License-Identifier: BSD-3-Clause
```
