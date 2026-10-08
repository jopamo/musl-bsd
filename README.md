# musl-bsd

BSD and GNU interfaces for programs built against musl: fts, obstack, argp,
`sys/queue.h`, `sys/tree.h`, `sys/cdefs.h`, and a source archive with Linux
mount/pidfd wrappers, multibyte input and `readpassphrase()`.

There is also a limited runtime for glibc-targeted x86_64 LP64 binaries.
Rebuilding against musl and running an existing glibc binary are different
jobs. The source libraries do not depend on that runtime.

## Build

Use Meson, Ninja and a C99 toolchain. Shared libraries need an ELF linker
with symbol version-script support. Select the compiler and linker at setup;
the build does not replace your selection.

```sh
CC=clang CC_LD=lld meson setup build -Dglibc_runtime=disabled
meson compile -C build
meson install -C build
```

For the runtime, use `-Dglibc_runtime=enabled`. The default, `auto`, enables
it only when setup verifies a Linux musl x86_64 LP64 target. Verification
executes the selected toolchain's libc and requires musl's banner and usage
exit status 1. Meson's raw probe prints `NO (1)`; read the separate
`Musl target libc verified` result. Unknown targets disable `auto` and fail
`enabled`. Runtime builds also need the compiler unwinder and static-PIE
support. Cross runtime builds need an execution wrapper; source-only builds
do not run this libc probe.

`glibc_loader_name` overrides the interpreter filename; `musl_linker_path`
overrides its absolute target path. Configured paths accept ASCII letters,
digits and `/_.+-`, without `..` components. Loader names cannot contain
slashes; interpreter paths cannot end in a slash. `DESTDIR` stages an
install without changing the configured target paths. Relative and absolute
`libdir` values are supported.

The runtime host installs in the ABI library directory. The source archive,
header overlay, glibc-named facades and loader stay under `libdir/musl-bsd`.
There is no unversioned host-DSO linker name.

## Link a rebuilt program

Use the interface that matches the program:

| pkg-config name | What it supplies |
| --- | --- |
| `musl-bsd-headers` | Header overlay, no library |
| `musl-bsd-source` | Overlay and PIC, hidden-visibility `libmusl-bsd-core.a` |
| `musl-bsd-glibc-host` | Versioned runtime host DSO, retained with `--no-as-needed` |
| `musl-bsd-glibc-startup` | Host and explicit private facade inputs, with a private runtime search path |

```sh
cc program.c $(pkg-config --cflags --libs musl-bsd-source) -o program
```

The source archive link is exact: it does not fall back to a runtime DSO.
Large-file aliases in the overlay resolve to musl's libc interfaces.
Startup's explicit DSO inputs honor `PKG_CONFIG_SYSROOT_DIR`; its runtime
search path remains a target path. The standalone fts, obstack and argp
libraries and their public headers are installed too.

Some source interfaces have caller obligations:

- FTS entries borrow the walk's path buffer. Use `FTS_NOCHDIR` for independent
  concurrent walks; CHDIR mode changes process cwd. Paths and entry counts
  are bounded by their public types and allocation limits. Oversized paths
  fail with `ENAMETOOLONG`, lists with `EOVERFLOW`. `FTS_NOSTAT` still stats
  directories and unknown entry types; `FTS_NAMEONLY` skips child metadata.
- Obstack alignment must be zero/default or a power of two. Chunks cannot
  exceed `PTRDIFF_MAX`; invalid alignment or growth invokes the failure
  handler with `EINVAL` or `EOVERFLOW`. That handler must not return normally.
  The `_fast` macros are unchecked: reserve room first.
- Unless `HAVE_QSORT_R` is defined, `sys/cdefs.h` supplies a GNU-signature
  `qsort_r` macro with per-thread, nestable comparator contexts. Comparators
  must return normally, not escape by exception, `longjmp` or cancellation.
  Define `HAVE_QSORT_R` only for a matching libc declaration.
- Mount/pidfd feature macros promise declarations and archive symbols, not
  kernel support or permission. Missing syscall numbers return `ENOSYS`;
  other calls forward the kernel result. Linux mount headers are required;
  older headers may leave `struct mount_attr` incomplete. Pidfd signal
  declarations need POSIX.1-2008 or GNU signal types.
- The multibyte reader needs exclusive access to its `FILE` and a fixed
  `LC_CTYPE`. After an I/O error, call `clearerr()` before retrying retained
  partial bytes. Invalid input reports `EILSEQ`; a non-null `invalid_byte`
  consumes one byte, while a null pointer leaves it pending. Incomplete EOF
  is invalid input; a decoded NUL is not EOF.
- `readpassphrase()` changes process signal handlers. Serialize calls and
  exclusively own the input and terminal settings. Other threads must block
  `SIGALRM`, `SIGHUP`, `SIGINT`, `SIGPIPE`, `SIGQUIT`, `SIGTERM`, `SIGTSTP`,
  `SIGTTIN` and `SIGTTOU`. Setup, output or cleanup failures return `NULL`
  without input; the first error wins. Deferred input cancellation attempts
  restoration and wipes the buffer. Normal return preserves the caller's
  cancellation state/type. Failed restoration can leave process state
  changed. See `<musl-bsd/readpassphrase.h>` for flags.
- `error()` and `error_at_line()` hold stderr's stream lock through output
  and disable cancellation until flushing and unlocking. Synchronize
  external accesses to diagnostic globals and serialize locale changes.
  Program-name callbacks run under that lock with cancellation disabled.

## Binary runtime limits

Do not use the runtime for setuid, setgid or other secure-execution workloads.
The interpreter rejects missing or nonzero `AT_SECURE` and mismatched
real/effective credentials before reading environment overrides. There is
no option to bypass this.

The interpreter runs musl's loader with the host DSO, private library path
and original arguments. Preload order is host → optional early DSO → user
`LD_PRELOAD`. `MUSL_BSD_EARLY_PRELOAD_PATH` selects one absolute DSO path,
without colon or ASCII whitespace; nothing is searched or downloaded.
`MUSL_BSD_PRELOAD_PATH` and `MUSL_BSD_LIBRARY_PATH` override configured paths.
Missing or empty path overrides use the defaults.

Executable-path adapters retain the resolved startup pathname, not an open
file or object identity. Re-exec fails if that name no longer exists. An
fd-based launch needs its `/proc/self/fd` or `/dev/fd` name to remain resolvable
through interpreter startup; a close-on-exec fd does not suffice.
`execve("/proc/self/exe", ...)` uses the supplied environment unchanged;
`execv`/`execvp` use `environ`. These adapters allocate and resolve symbols:
they are not async-signal-safe or suitable between fork and exec in a
multithreaded child. Source-archive callers use libc directly.

Other limits matter even when a symbol is exported:

- musl 1.2.x resolves several legacy SONAMEs, including `libc.so.6`, to its
  own libc rather than the private facade. Ordinary `DT_NEEDED` edges do
  not establish facade ownership. The runtime does not preload every facade
  to work around this.
- `dlmopen(LM_ID_BASE, ...)` uses `dlopen()`. `LM_ID_NEWLM` fails; there is
  no separate namespace.
- `dlvsym()` resolves by name only for the tested finite `GLIBC_*` version
  set. Unknown, null and `GLIBC_PRIVATE` versions fail.
- Writable glibc allocator hooks are not exported. Locale, errno and
  finalization behavior can differ; consult the per-symbol policy.
- Pthread function-table resolution is all-or-nothing. Missing entries
  permanently fail with `ENOSYS`; recursive lookup also returns `ENOSYS`.
  PID-tagged child recovery does not make loader calls safe after a
  multithreaded fork. Semaphore wrappers return `-1` and set errno.

## Inspect ELF files and policy

`tools/elf-scan` reads x86_64 little-endian ELF64 files and follows
`DT_NEEDED` through explicit search roots. It never executes inspected files
or fetches dependencies. Dynamic symbol inventory needs `DT_SYMTAB`, the
ELF64 `DT_SYMENT` size and a file-backed SysV or GNU hash table; section
headers are not required.

```sh
tools/elf-scan --format json --output inventory.json /path/to/root.so
tools/compatibility-manifest validate compatibility-symbols.json
tools/compatibility-manifest generate inventory.json \
  --base compatibility-symbols.json --output compatibility-symbols.new.json
tools/compatibility-manifest check compatibility-symbols.new.json inventory.json
```

`compatibility-symbols.json` is the per-symbol policy. New or changed
implementations default to `UNSUPPORTED`; other levels are `EXACT`,
`TRANSLATED`, `DEGRADED` and `STUB`. Exporting a name is not ABI qualification.

Scanner `--provider` arguments check exported names. `--provider-alias`
requires an actual provider export. `--strict` fails unresolved mandatory
dependencies or provider-backed GLIBC requirements, not every non-GLIBC
import. Weak unresolved imports remain optional. The search is a filesystem
approximation, not loader scope, version selection or semantic ABI checking.

For target-only inspection:

```sh
tools/elf-scan --sysroot /work/target --library-path /opt/vendor/lib \
  --format json /work/target/opt/vendor/bin/application
```

Input/provider arguments are host paths inside the sysroot; dependency and
library paths are target paths. Relative library/RPATH/RUNPATH directories
are rejected, and `ELF_LIBRARY_PATH`/`ELF_SCAN_PATHS` are ignored. Canonical
paths cannot escape the sysroot. Use target-internal relative symlinks;
absolute symlinks are not reinterpreted. This is not filesystem isolation
against concurrent path replacement or a snapshot of changing files.

Inputs must be regular files. ELF reads are limited to 512 MiB per file,
4,096 reads and 1 GiB total per invocation, with separate parser, string,
hash and path-work budgets. JSON inputs are limited to 1 GiB, nesting 64 and
16,777,216 structural units; malformed types, duplicate keys and inconsistent
counts are rejected. Limits do not bound filesystem latency.

Report files use same-directory temporary files and atomic replacement.
Existing regular-file modes are preserved; new files use 0600. Symlink and
non-regular destinations are rejected; the parent must exist. This protects
an old report from a failed write, not power loss or competing writers.

## Tests

Tests live in the sibling `musl-bsd-tests` repo, which builds this checkout
as a Meson subproject. No installed copy is needed:

```sh
cd ../musl-bsd-tests
ln -s ../../musl-bsd subprojects/musl-bsd  # once
python3 scripts/validate.py source-release build-source --cc clang --cxx clang++
```

Its README covers runtime and sanitizer profiles, component suites, coverage
and optional external DSO tests. Runtime tests include a static-PIE baseline
check; passing it does not qualify an arbitrary glibc binary.

## License

File notices govern this mixed-license repository. License texts are in
`LICENSES/`: BSD-2-Clause, BSD-3-Clause, LGPL-2.1-or-later and the GPL text
referenced by the LGPL. Argp is LGPL; fts headers are BSD-3-Clause.
`readpassphrase()` is ISC-derived OpenBSD code, imported through libbsd commit
`5d2dcb1b729d1faebc09aec5e78909efe10eed1d`, with its notice retained.
