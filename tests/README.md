# Test suite

Tests are owned by the component they exercise:

- `abi/`: compile-time and compiler-dump ABI checks
- `argp/`: focused API tests and integration examples
- `cdefs/`: `sys/cdefs.h` behavior
- `compat/`: compatibility runtime and loader behavior
- `fts/`: file-tree traversal behavior
- `headers/`: overlay header compile/link checks
- `obstack/`: allocation and object-building behavior
- `tools/`: black-box tests for repository analysis tools

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

The `compat` suite has no loader exclusion on a qualified ABI. It includes:

- a standalone static-PIE baseline;
- static-PIE ELF metadata and relocation checks;
- IFUNC/IRELATIVE startup, dependency-ordering, and `dlopen()` checks;
- fail-closed secure-policy checks;
- a glibc-named ELF fixture traversing the compatibility interpreter and musl
  loader;
- argument, `dlopen`, preload-order, recursion, missing-target,
  missing-library, exit-status, and signal tests;
- constructor dependency ordering and reverse-order teardown with readiness
  assertions at every phase;
- all NVIDIA/CUDA-required pthread and semaphore bridge calls, direct scalar
  ABIs, compatibility-core export versions, return behavior, and error
  contracts;
- `dladdr1` public ABI and `RTLD_DL_LINKMAP` identity, including explicit
  rejection of unsupported `RTLD_DL_SYMENT`;
- bounded name-based `dlvsym` compatibility, including all observed versions,
  unknown-version rejection, `GLIBC_PRIVATE`, `dlerror`, and `errno`;
- exact `dlmopen(LM_ID_BASE)` adaptation and fail-closed rejection of
  libcudart's unsupported `LM_ID_NEWLM` namespace request;
- exact `__rawmemchr` byte conversion, first-match, alignment, return-address,
  `errno`, and guarded-page behavior;
- exact `fallocate64` LP64 offsets, allocation, hole punching, file-size,
  return-value, and `errno` behavior;
- translated `backtrace` frame ordering, caller identity, capacity bounds,
  non-positive sizes, return values, and `errno` preservation;
- fail-closed exclusion of obsolete malloc-hook state, including runtime
  lookup failure, normal allocator behavior, and real-glcore probe discovery;
- canonical musl `secure_getenv` ownership, lookup identity, environment and
  `errno` behavior, internal-alias exclusion, and compatibility-version use;
- degraded musl `_IO_getc` byte conversion, EOF, stream-error, pushback,
  `errno`, and provider-identity behavior;
- degraded musl `_IO_putc` byte conversion, persisted output, stream-error,
  `errno`, and provider-identity behavior;
- degraded musl `__assert_fail` provider identity, argument diagnostics,
  non-returning behavior, and `SIGABRT` termination;
- degraded musl `__ctype_b_loc` pointer ABI, glibc mask encoding, signed-byte
  indexing range, C/C.UTF-8 classification, `errno`, and provider identity;
- degraded musl `__ctype_get_mb_cur_max` return ABI, C and C.UTF-8 widths,
  thread-local locale switching, conversion bounds, `errno`, and provider
  identity;
- degraded musl `__cxa_atexit` registration ABI, argument delivery,
  allocation-backed capacity, reverse process-exit ordering, exactly-once
  execution, `errno`, status preservation, and provider identity;
- stub musl `__cxa_finalize` ABI, DSO-specific and all-DSO no-op behavior,
  repeated calls, delayed process-exit publication, `errno`, and both strong
  and weak manifest requirements;
- exact musl `__duplocale`/`__freelocale` lifecycle: pointer ABIs,
  source-snapshot independence, inactive construction, thread-local
  publication and withdrawal, global/static and allocated ownership, cleanup,
  `errno`, and public-alias identity;
- exact musl `__uselocale` thread-locale publication, query, restoration, and
  `LC_GLOBAL_LOCALE` ABI, `errno`, provider ownership, and public-alias
  identity through `compat/locale_ownership`;
- degraded musl `__wcscoll_l`/`wcscoll_l` wide-string collation ABI, ASCII and
  non-ASCII ordering, C/C.UTF-8 handles, `errno`, provider ownership, and
  public-alias identity through `compat/locale_ownership`;
- degraded musl `__wcsxfrm_l`/`wcsxfrm_l` wide-string transformation ABI,
  Unicode code-point keys, full copies, sizing, truncation, `errno`, provider
  ownership, and public-alias identity through `compat/locale_ownership`;
- degraded musl `__strcoll_l`/`__strxfrm_l` and public `strcoll_l`/`strxfrm_l`
  locale-comparison and transformation ABIs, C and C.UTF-8 bytewise ordering,
  transformed lengths, truncation, embedded-NUL handling, `errno`, provider
  ownership, and public-alias identity;
- exact compatibility-core `__strdup` allocation ABI, embedded-NUL and empty
  strings, copy independence, `errno`, and provider ownership through
  `compat/strdup_abi`;
- degraded compatibility-core `__strftime_l` and direct musl
  `__wcsftime_l` formatting ABIs, C and C.UTF-8 numeric formats, percent
  conversion, truncation, zero-size handling, their distinct `errno` behavior,
  provider ownership, and public-alias identity through `compat/strftime_abi`;
- degraded musl `__strtof_l`/`__strtod_l` and public `strtof_l`/`strtod_l`
  floating-point conversion ABIs, decimal and hexadecimal input, end pointers,
  special values, range errors, invalid input, `errno`, provider ownership,
  and public-alias identity through `compat/strto_l`;
- exact musl `__tls_get_addr` loader/TLS ABI, shared general-dynamic TLS
  relocation, provider ownership, per-thread storage isolation, and `errno`
  through `compat/tls_abi`;
- degraded musl `__newlocale`/`__nl_langinfo_l` construction and query ABIs,
  observed monetary items, C and C.UTF-8 handles, inactive construction,
  in-place category replacement, `errno`, and internal/public provider
  identity;
- exact musl `__progname_full` object ABI, `program_invocation_name`
  storage identity, startup `argv[0]` value, basename alias, `errno`, and
  provider ownership;
- degraded compatibility-core `__register_atfork` callback ABI, prepare
  reverse ordering, parent/child forward ordering, process isolation,
  `errno`, provider ownership, and DSO-handle adapter behavior;
- exact compatibility-core `__sched_cpualloc`/`__sched_cpufree` allocation and
  cleanup ABIs, dynamic word-boundary sizing, zero initialization,
  boundary-bit operations, `errno`, provider ownership, and public
  `CPU_FREE` semantics;
- exact musl `__sched_cpucount` byte-range count ABI, zero-size/null handling,
  byte and word boundaries, observed 128-byte sizing, `errno`, and provider
  ownership;
- exact musl `__errno_location` pointer ABI, stable address, macro and direct
  mutation visibility, syscall updates, simultaneous thread isolation,
  provider identity, and libc/libpthread manifest requirements;
- degraded musl `__fxstat`/`__fxstatat`/`__lxstat` and compatibility-core
  `__fxstat64`/`__lxstat64` x86_64 layout, accepted version selectors, complete
  regular-file metadata above 4 GiB, directory/FIFO descriptors, relative and
  absolute paths, followed/no-follow/dangling symlinks, empty-path flags,
  path/descriptor/flag failures, `errno`, and distinct provider identities;
- degraded musl `__getdelim` pointer/return ABI, public-alias identity,
  delimiter and embedded-NUL behavior, allocation growth and reuse,
  unterminated records, EOF and stream errors, invalid arguments, `errno`, and
  provider identity;
- degraded musl `__isoc99_fscanf`/`__isoc99_sscanf` variadic ABIs and
  public-alias identities, all NVIDIA-observed formatted stream/string
  conversions, C99 hexadecimal floating input, widths, length modifiers,
  suppression, assignment counts, stream position, matching/input failures,
  write-only stream errors, `errno`, and provider identities;
- degraded musl `__wctype_l`/`wctype_l` descriptor construction and
  `__iswctype_l`/`iswctype_l` wide-character classification ABIs, all 12
  standard classes, ASCII and non-ASCII behavior with C/C.UTF-8 handles,
  current-versus-explicit locale selection, `WEOF`, unknown descriptors,
  `errno`, provider identity, and public-alias identity through
  `compat/wctype_l`;
- degraded musl `__towlower_l`/`__towupper_l` Unicode case-conversion ABIs,
  ASCII and non-ASCII mappings, C/C.UTF-8 handles, `WEOF`, `errno`, provider
  ownership, and public-alias identity through `compat/wctype_l`;
- translated musl `__libc_current_sigrtmin` integer ABI, stable realtime
  range, signal-set operations, queued payload and direct `tgkill` delivery,
  handler execution, `errno`, provider identity, and legacy-libpthread
  manifest requirement;
- absent and provider-backed weak symbol relocation;
- fail-closed and provider-backed name resolution for a `GLIBC_*` undefined
  symbol;
- schema, normalization, provider consistency, and fail-closed coverage for
  the checked NVIDIA/CUDA symbol manifest;
- facade SONAME, dependency, export, and symbol-version inspection.

When `NVIDIA_LIBDIR` is set, the `nvidia` suite also exercises the real local
`libnvidia-glcore` dependency graph through the compatibility interpreter. It
proves fail-closed behavior without early NVIDIA TLS and successful loading
with `MUSL_BSD_NVIDIA_TLS_PATH`. It also verifies `RTLD_LOCAL` isolation and
`RTLD_GLOBAL` publication for exports from glcore and its gpucomp dependency;
32 repeated cycles per scope exercise overlapping handle ownership and
close-order safety. The same real-binary test inventories weak imports and
requires both unresolved optional probes and runtime-provided weak symbols to
relocate successfully. It also requires strong `GLIBC_*` imports in the
installed graph before the `RTLD_NOW` load, proving that the load exercises
NVIDIA's actual versioned undefined symbols. The runner also verifies that
glcore and its gpucomp and TLS dependencies all carry initializers and that
the corresponding `DT_NEEDED` edges exist before loading the graph.
Proprietary binaries are never copied into the repository. A second local test
discovers an exported `_nv*TLS` object and checks per-thread address and value
isolation across eight concurrent threads. It also gates on the installed
NVIDIA graph's destructor ABI and verifies pthread-key destructor delivery
through the compatibility core.
