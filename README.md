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
- **Linux libc wrappers** — including the file-descriptor mount and pidfd APIs
- **Byte-preserving multibyte input** — for consumers that cannot rely on glibc stdio recovery semantics
- **Passphrase input** — `<musl-bsd/readpassphrase.h>` and `readpassphrase()`
  through `musl-bsd-source`, without the foreign-binary runtime

The passphrase implementation is derived from OpenBSD via libbsd commit
`5d2dcb1b729d1faebc09aec5e78909efe10eed1d`, retaining its ISC license.
It retains the BSD flags and attempts terminal and signal restoration before
redelivery. Setup failures are rejected before prompting or reading. Cleanup
failures return `NULL` without returning input; the first input/setup error
takes precedence over a later cleanup error. A refused restoration can leave
process state changed. Signals are resent only after their original action
is restored. Input uses deferred cancellation; setup and restoration disable
cancellation. Cancellation attempts the same restoration, closes only owned
descriptors, and wipes the caller's buffer. Normal return restores the caller's
cancellation state and type. Prompt and final-newline writes handle short
writes and reject errors without returning input.
Like the BSD interface, it changes process-wide signal handlers; callers
must serialize its use and exclusively own input while it runs. Do not read
from the same input, close/reassign its descriptor, or change terminal settings
concurrently. In a multithreaded process, other threads must block `SIGALRM`,
`SIGHUP`, `SIGINT`, `SIGPIPE`, `SIGQUIT`, `SIGTERM`, `SIGTSTP`, `SIGTTIN` and
`SIGTTOU` so process-directed signals reach the reader. The reader preserves
its incoming mask and uses `ppoll()` to atomically allow those unblocked by
that mask while waiting for input. Terminal restoration uses the incoming
mask to retain job-control behavior. It is not a cryptographic primitive or an RNG.

The source-compatibility layer remains portable independently of the optional glibc binary runtime.

The multibyte reader owns byte input from its `FILE` and retains partial bytes
after an I/O error. Call `clearerr()` before retrying; the retry starts conversion
from the retained bytes. Malformed input reports `EILSEQ`, consuming one byte
only when `invalid_byte` is non-null. Incomplete EOF is malformed input; a
decoded NUL is `L'\0'`, not EOF. Keep the effective `LC_CTYPE` locale fixed for
the reader's lifetime and do not interleave other reads or seeks on its stream.

GNU-style `error()` and `error_at_line()` lock the whole stderr message and
flush stdout before reporting. Per-line suppression owns its cached filename;
allocation failure discards that cache rather than suppressing a diagnostic.
Cancellation stays disabled until output is flushed and stderr is unlocked.
Synchronize external count reads and diagnostic-global changes with calls,
and serialize locale changes separately. Custom program-name callbacks run under stderr's
recursive stream lock with cancellation disabled.

Obstack chunk sizes are hints, raised when necessary to fit the header and
alignment padding. Zero selects defaults; nonzero alignment must be a power
of two. Chunk spans are limited to `PTRDIFF_MAX`, since object and room sizes
use pointer differences. Invalid alignment or unrepresentable growth invokes
the allocation-failure handler with `EINVAL` or `EOVERFLOW`. That handler must
exit or perform a non-local return; if it returns normally, the library aborts.
The `_fast` macros remain unchecked and require callers to reserve room.

FTS path buffers are bounded by both `__fts_length_t` and `PTRDIFF_MAX`.
Paths must leave space for the terminator within that capacity. Padding and
power-of-two growth stop at the bound; `FTS.fts_pathlen` records the actual
allocation size rather than a capped larger allocation. Unrepresentable
paths fail with `ENAMETOOLONG`. Root and sibling lists cannot exceed the
public count representation or `PTRDIFF_MAX / sizeof(FTSENT *)`; oversized
lists fail with `EOVERFLOW`. Sort-array padding clips to that bound, and
`FTS.fts_nitems` records the exact array capacity. Entry allocation checks
name representation and the complete name/stat span before allocating.
`FTS_NOSTAT` skips metadata for known non-directory entry types, but stats
directories and `DT_UNKNOWN` entries. Directory link counts do not establish
that no child directories remain. `FTS_NAMEONLY` still skips child metadata.

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

Build the libraries:

```sh
meson compile -C build
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

The source interface defines `HAVE_MUSL_BSD_MOUNT_API` and
`HAVE_MUSL_BSD_PIDFD_API` for consumers that provide their own syscall
fallbacks. These macros indicate that the interface supplies declarations and
linkable wrappers, so consumers must not emit competing static definitions.

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

The interpreter resolves the target to an absolute path before application
code runs. The `/proc/self/exe` and re-execution adapters reuse that path even
after a working-directory change; `argv[0]` remains the caller's original value.
Re-execution with an empty argument vector supplies an empty `argv[0]`, as
Linux does for a direct exec. A truncated executable path is rejected with
`ENAMETOOLONG` rather than passed to the loader.
For `/proc/self/exe`, `execve` selects compatibility paths and preloads from
its supplied `envp`, then forwards that environment unchanged. `execv` and
`execvp` use `environ`. Missing or empty path overrides use configured paths;
early-preload validation and core → early → user ordering apply to both startup
and re-execution.

The executable-path adapter reports the startup pathname for the process's
lifetime. Renaming or unlinking that file does not change its result; a later
re-exec still asks musl's loader to open the original name and fails if it is
gone. The runtime does not retain an executable fd or promise object identity.
An fd-based launch requires its `/proc/self/fd` or `/dev/fd` name to remain
resolvable during interpreter startup. A close-on-exec descriptor cannot
satisfy that requirement.

The runtime exec and executable-path adapters are not async-signal-safe:
they can allocate and resolve libc symbols. Do not use them in a signal
handler or between fork and exec in the child of a multithreaded process.
Publication retry/fork tests do not qualify that POSIX contract. These
limitations do not apply to source-archive consumers, which use libc directly.

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

### Pthread resolver contract

The pthread bridge publishes its libc function table all at once. Any missing
entry makes resolution permanently fail with `ENOSYS`; later-loaded providers
do not retry or enable a partial table. Pthread adapters return the error code,
while semaphore adapters return `-1` and set `errno`.

The initializing thread disables cancellation before claiming resolver
ownership and restores its previous state after publishing success or failure.
Cold lookup preserves the caller's `errno`. Recursive lookup returns `ENOSYS`
instead of waiting for itself. A child can reclaim another thread's inherited
in-progress resolver using the PID tag. This state recovery does not qualify
loader calls after a multithreaded fork or async-signal-safe use.

### Optional early preload

Some ELF binaries require a dependency to be present in the process's initial
static TLS or constructor set. Set an explicit early DSO when needed:

```sh
export MUSL_BSD_EARLY_PRELOAD_PATH=/absolute/path/to/required-early.so
```

The path must be absolute, contain no colon or ASCII whitespace, and identify
one DSO. The loader does not guess
package versions or search for this dependency. Preload order is fixed:

```text
musl-bsd glibc host → optional early DSO → user LD_PRELOAD
```

The secure-execution check runs before this environment variable is read.

### Scan ELF files

`tools/elf-scan` inspects explicit ELF files or directories and recursively
follows their `DT_NEEDED` dependencies. It does not download packages or invoke
a package manager.
Dynamic symbol inventory requires `DT_SYMTAB`, the ELF64 `DT_SYMENT` size,
and a file-backed SysV or GNU hash table to bound the symbol count. Section
headers are not required; table order is not used to guess missing bounds.
Inputs must be regular files. Size checks and reads use one opened descriptor;
reads stop with an error if input exceeds 512 MiB, including growth after
the size check. GNU hash inspection caps aggregate bucket/chain work at
16,777,216 steps per table and checks file-backed prefix/chain ranges.
Each invocation permits at most 4,096 ELF reads and 1 GiB of input bytes
across discovery probes, roots, dependencies and providers. Re-reading a graph object
as a provider counts again. Limit failures do not emit a partial inventory.
Parser/resolver work is capped at 1,048,576 units across structure reads,
string reads, address-mapping candidates, search-path splitting, configured
path canonicalization, discovery candidates, expansion passes and dependency
candidates. String searches and decodes share a 64 MiB source-byte allowance,
including terminators and repeated names. Path processing shares a separate
64 Mi-character allowance, charging expansion inputs/results and candidate joins before
allocation. Searches stop after a match rather than materializing every
candidate. Search order and sequential ORIGIN replacement remain unchanged.
Discovery streams directory entries and glob components, allowing at most
65,536 entries (including nonmatches and explicit/literal candidates) before
sorting bounded results. Configured path splitting is charged before allocation.
These caps do not bound filesystem operation latency.
This does not freeze in-place file changes or snapshot a
dependency graph.

```sh
tools/elf-scan --format json --output inventory.json /path/to/root.so
```

File output uses a same-directory temporary and atomic replacement after
writing, flushing and closing it. Existing regular-file permission bits are
preserved; new reports use mode 0600. Symlink and non-regular destinations are
rejected, and the parent directory must already exist. Failed writes attempt
temporary-file cleanup without truncating the old report. Output ownership
belongs to the writer. This does not provide fsync-based crash durability.
Standard-output behavior is unchanged.

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
provider-backed GLIBC symbol requirements. Unresolved weak imports remain optional.
JSON `scope` and text reports identify dependency resolution as a filesystem
search approximation, not runtime qualification. Provider analysis checks
exported-name membership, not loader scope/order, symbol-version selection
or semantic ABI compatibility. The historical `matching_policy` identifier
remains for manifest interoperability; it does not establish those contracts.
`--strict` does not check every non-GLIBC import.

For target-only inspection, select `--sysroot DIR`. Input and provider
arguments are host filesystem paths within that directory. Absolute dependency
names, default library directories and explicit `--library-path` directories
are target paths rooted there. ORIGIN uses the containing object's target
directory; relative slash-bearing dependencies use that object's directory.
Relative library/RPATH/RUNPATH directories are rejected in this mode.
`ELF_LIBRARY_PATH` and `ELF_SCAN_PATHS` are ignored.

```sh
tools/elf-scan --sysroot /work/target --library-path /opt/vendor/lib \
  --format json /work/target/opt/vendor/bin/application
```

Canonical paths escaping the sysroot are rejected, including outward-pointing
symlinks in roots, providers, discovery and dependencies. Use target-internal
relative symlinks; absolute symlinks are not reinterpreted as target paths.
This remains an inspection approximation, not loader emulation or a filesystem
isolation boundary against concurrent path replacement. Reports record the
canonical sysroot. Without this option, existing host search behavior remains.

### Compatibility manifest

`compatibility-symbols.json` is the checked runtime compatibility-policy
manifest. The tool accepts inventory format 1 and manifest schema 1 as
integers, not booleans or floating-point equivalents. JSON must use UTF-8,
unique object keys and standard numeric literals. Provider outcome counts
must match their arrays; unresolved outcomes must be weak. Invalid nested
types and duplicate object reports are rejected before policy generation.
The manifest's exact-field schema remains unchanged. Validate it with:

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

Regeneration preserves base test and quality policy only when the symbol key
and implementation descriptor still match. A changed implementation requires
renewed qualification and defaults to `UNSUPPORTED`.

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

The optional external suite in the `musl-bsd-tests` repository can exercise any
explicitly selected DSO. Run there after configuring its build:

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

Tests and coverage tooling live in the separate `musl-bsd-tests` repository.
This repository builds only the production libraries, headers, and runtime.

For sibling checkouts, configure and run the tests with:

```sh
cd ../musl-bsd-tests
ln -s ../../musl-bsd subprojects/musl-bsd
meson setup build
meson test -C build --print-errorlogs
```

The test repository builds this checkout as a local Meson subproject. Use
`-Dmusl-bsd:glibc_runtime=disabled` for source-only testing. Its README documents
component suites, external DSO tests, and `scripts/coverage.sh`.

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
