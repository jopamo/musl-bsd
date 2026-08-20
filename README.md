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

The report records SONAMEs, undefined-symbol binding and versions, TLS
relocations, IFUNC/IRELATIVE use, relocation types, and a consolidated
compatibility requirement list. Pass explicit DSO paths to avoid automatic
NVIDIA filename discovery.

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
`DT_NEEDED` entries or mandatory provider-backed symbol requirements a command
failure. Unresolved weak imports are reported separately as optional. Malformed,
unsupported, or ambiguous input always fails.

`nvidia-symbols.json` is the checked compatibility-policy manifest for the
observed NVIDIA 610.x and CUDA 13.3 roots. It contains only their versioned
glibc imports and records each symbol's requested version, expected SONAME,
binding, selected runtime implementation, qualification test, and quality:

```sh
tools/nvidia-manifest validate nvidia-symbols.json

tools/nvidia-manifest generate nvidia-inventory.json \
  --base nvidia-symbols.json \
  --output nvidia-symbols.new.json

tools/nvidia-manifest check \
  nvidia-symbols.json nvidia-inventory.json
```

Generation requires scanner provider analysis with no unresolved mandatory
symbols. `--base` preserves audited test and quality policy for unchanged
requirements while removing stale entries and defaulting only new entries to
`UNSUPPORTED`: finding a same-name provider proves availability, not ABI or
semantic equivalence. The focused compatibility audit promotes entries to
`EXACT`, `TRANSLATED`, `DEGRADED`, or `STUB` only after their recorded tests
justify that claim. Validation rejects unknown fields, qualities, versions,
duplicate or unsorted entries, and `GLIBC_PRIVATE`; inventory checking also
rejects requirements outside the exact scanned root set. The manifest records
no proprietary binary content or absolute local paths.

The NVIDIA-required pthread subset is audited separately. Direct musl scalar
ABIs are classified `EXACT`; compatibility-core opaque-object and old-version
bridges are `TRANSLATED`. The pthread manifest test ties every promoted entry
to `compat/pthread_abi` and verifies that compatibility-core exports carry the
requested `GLIBC_*` version.

The observed `dladdr1@GLIBC_2.3.3` use is limited to
`RTLD_DL_LINKMAP` and is classified `TRANSLATED`. The adapter derives a real
musl link map through `dladdr`/`dlinfo`; it rejects `RTLD_DL_SYMENT` rather
than fabricating an `ElfW(Sym)`.

`dlvsym@GLIBC_2.2.5` is also actively used. musl cannot select among glibc
symbol definitions, so this adapter is explicitly `DEGRADED`: it resolves by
name only for the finite set of `GLIBC_*` versions observed in the qualified
NVIDIA/CUDA graph and its direct probes. Unknown versions, `GLIBC_PRIVATE`,
and null versions return `NULL` with `dlerror` set instead of silently
downgrading.

CUDA 13.3 actively requests `dlmopen(LM_ID_NEWLM, ..., RTLD_NOW)`. musl has no
link-map namespace mechanism, so that operation is classified `UNSUPPORTED`
and fails with `dlerror`. `LM_ID_BASE` remains an exact adapter to `dlopen`;
the runtime never silently widens a requested new namespace into the base
namespace.

The Xorg driver requires `__rawmemchr@GLIBC_2.2.5`. The compatibility-core
implementation is classified `EXACT` and tested across unaligned starts,
high-byte conversion, first-match behavior, and an inaccessible page boundary.

Glcore requires `fallocate64@GLIBC_2.10` for ordinary allocation and
`FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE`. On the qualified x86_64 LP64
ABI, `off64_t` and `off_t` are the same 64-bit type, so the compatibility-core
adapter delegates directly to musl `fallocate` and is classified `EXACT`.
Regression coverage includes offsets above 4 GiB, size changes, hole contents,
return values, and `errno`.

Glcore calls `backtrace@GLIBC_2.2.5` from 47 diagnostic paths, each with room
for sixteen frames. The compatibility core uses the toolchain's standard
Itanium unwinder, omits its own frame, bounds writes to the caller's capacity,
and detects a non-progressing unwind. This is classified `TRANSLATED` because
it provides glibc-compatible behavior through the musl toolchain unwinder
rather than glibc's own unwind-loading machinery.

Glcore also probes `__malloc_hook`, `__realloc_hook`, `__free_hook`, and
`__memalign_hook` by name. These are not ELF imports, and its probe explicitly
accepts all four names being absent. The compatibility core therefore does not
export writable hook state or claim allocator interposition that musl cannot
provide. These optional names are deliberately excluded from the symbol
manifest.

No qualified NVIDIA/CUDA object imports or probes `secure_getenv` or
`__secure_getenv`. Musl already provides the public `secure_getenv` using its
authoritative startup security state, so the compatibility core delegates to
that implementation and does not export a duplicate public function or a
fabricated internal alias. The loader’s separate launch boundary continues to
reject secure execution before reading environment-controlled policy.

Gpucomp and NVML import `_IO_getc@GLIBC_2.2.5` for ordinary locked byte input.
Musl provides `_IO_getc` as its weak alias of `getc`, matching glibc’s alias
and observed success/EOF behavior. It is classified `DEGRADED`, however,
because musl sets the stream error indicator but preserves `errno` when called
on a write-only stream, whereas glibc reports `EBADF`. The NVIDIA call sites
use readable streams. Focused coverage includes unsigned-byte conversion, EOF
and stream error state, pushback, `errno`, and provider ownership.

Gpucomp also imports `_IO_putc@GLIBC_2.2.5` for one byte-output loop. Its
observed calls pass values from 0 through 255 to a writable stream. Musl
provides `_IO_putc` as the weak alias of locked `putc` and matches that path,
including `int`-to-`unsigned char` conversion. It is `DEGRADED` because a call
on a read-only stream sets the stream error indicator but preserves `errno`
instead of reporting glibc’s `EBADF`.

Libcuda and NVML import `__assert_fail@GLIBC_2.2.5` for compiler-generated
invariant failures. Musl preserves the essential non-returning diagnostic and
`SIGABRT` behavior, so failed invariants remain fail-closed. The provider is
classified `DEGRADED` because musl uses a signed line parameter and a different
diagnostic format without glibc’s abort-message metadata.

The NVIDIA compiler stack also imports `abort@GLIBC_2.2.5` for unconditional
failure termination. Musl's canonical provider preserves the non-returning
`SIGABRT` contract and forces the default action even when the signal is
ignored. It is `EXACT`; the shared `compat/assert_fail` regression verifies
provider ownership and this fail-closed termination path.

The NVIDIA compiler stack imports `access@GLIBC_2.2.5` for real-UID filesystem
permission checks. Musl's canonical provider preserves the observed `F_OK`,
read/write/execute, symlink-following, invalid-mode, missing-path, and
successful-call `errno` behavior. It is `EXACT`; `compat/access_abi` verifies
the path/mode ABI and provider ownership.

The NVIDIA compiler stack imports `acos@GLIBC_2.2.5`, `acosf@GLIBC_2.2.5`,
and `asin@GLIBC_2.2.5` from `libm.so.6` for inverse-trigonometric
calculations. Musl's providers preserve the observed finite-domain values,
signed-zero and endpoint behavior, NaN handling, domain errors, and
successful-call `errno`, but return domain NaNs without publishing `EDOM`
where glibc does. They are `DEGRADED`; the shared `compat/invtrig_abi`
verifies all three scalar floating-point ABIs, distinct error behavior, and
provider ownership.

The NVIDIA compiler stack imports `alarm@GLIBC_2.2.5` for process timer
management. Musl's canonical provider preserves cancellation return values,
`SIGALRM` delivery, and successful-call `errno`; it is `EXACT`.
`compat/alarm_abi` isolates timer state in a child process and verifies the
timer ABI and provider ownership.

The NVIDIA compiler stack imports `alphasort@GLIBC_2.2.5` for directory-entry
ordering, while the compatibility-core adapter provides `alphasort64` for the
same x86_64 ABI. The direct and adapter providers preserve the comparator ABI,
observed ASCII and UTF-8 ordering, and successful-call `errno` under C and
C.UTF-8 handles. They are `DEGRADED` because musl's locale collation policy is
narrower; `compat/alphasort_abi` verifies both ordering and ownership paths.

NVML imports `__ctype_b_loc@GLIBC_2.3` at three whitespace checks. Each call
indexes the returned table with a sign-extended byte and tests glibc’s
`_ISspace` mask (`0x2000`). Musl supplies the required pointer-to-pointer ABI,
the complete `-128..255` indexing range, and glibc-compatible masks, so the
observed NVML path is preserved. The provider remains `DEGRADED`: musl’s table
does not implement glibc’s per-thread, current-locale table switching for
legacy locale-specific single-byte classifications.

Gpucomp imports `__ctype_get_mb_cur_max@GLIBC_2.2.5` for two locale-facet
queries. Both temporarily select the facet’s thread-local locale; one tests
whether the maximum is one byte and the other returns the maximum width. Musl
correctly follows `uselocale`, returning one for C/POSIX and four for UTF-8,
and its four-byte result bounds its Unicode conversion behavior. This is
`DEGRADED`, not `EXACT`, because glibc’s UTF-8 charmap declares a historical
six-byte maximum and glibc supports additional legacy encodings.

Libcuda, glcore, NVML, gpucomp, and libcudart import
`__cxa_atexit@GLIBC_2.2.5` for compiler-generated static destructor
registration; gpucomp alone contains 2,485 call sites. Musl commits successful
registrations to a locked process-wide list and runs them exactly once in
reverse order at normal process exit. This is `DEGRADED`: musl does not retain
the supplied DSO handle, so it cannot provide glibc’s per-DSO finalization
ownership or unload-time callback execution. The qualified musl loader keeps
DSOs resident and NVIDIA’s process-exit path remains functional.

Glcore strongly imports `__cxa_finalize@GLIBC_2.2.5`; the other qualified
NVIDIA/CUDA objects import it weakly through compiler-generated finalizers.
Glcore also has explicit finalization calls, including one that preserves
`errno`. Musl’s implementation is an intentional no-op and is classified
`STUB`: neither a supplied DSO handle nor `NULL` publishes registered
destructors. Because musl’s `__cxa_atexit` list is process-owned and its loader
keeps DSOs resident, those callbacks remain valid and run later from the
authoritative process-exit path rather than being lost or invoked twice.

Gpucomp imports `__duplocale@GLIBC_2.2.5` through one wrapper used by two
locale-facet constructors. Each duplicates a supplied locale before retaining
it and caching time or monetary data. Musl’s internal function and public
`duplocale` are one implementation that allocates an independently owned
locale snapshot, including for `LC_GLOBAL_LOCALE`. The result remains inactive
until explicitly selected with `uselocale` and remains valid if the source is
changed or released. This contract is classified `EXACT`.

Gpucomp’s matching `__freelocale@GLIBC_2.2.5` cleanup wrapper releases those
retained facet snapshots after they are no longer active. Musl frees allocated
locale handles while recognizing its non-owned built-in handles, and its
internal symbol is the public `freelocale` implementation. Releasing one owned
snapshot does not affect independent copies or current thread state. The
duplicator and releaser are therefore covered by one `EXACT` locale-ownership
regression rather than parallel lifecycle tests.

Gpucomp imports `__newlocale@GLIBC_2.2.5` through a constructor that passes
mask `0x40`, the literal `C` locale, and no base handle. Musl's internal symbol
is the canonical public `newlocale` implementation; it returns a valid
built-in/default handle without changing the calling thread's active locale.
It is `DEGRADED` because musl supports a narrower locale database and
classification model than glibc. `compat/locale_ownership` verifies the
observed construction, alias and provider identity, in-place category
replacement, and successful `errno` preservation.

Gpucomp's locale-facet paths import `__nl_langinfo_l@GLIBC_2.2.5` and query
monetary items `0x40000`, `0x40002` through `0x40006`, and `0x40015` on the
constructed locale. Musl's internal symbol is the canonical public
`nl_langinfo_l` implementation; the observed C-locale path returns stable
empty strings, preserves `errno`, and reports the expected ASCII/UTF-8
`CODESET`. It is `DEGRADED` because musl's locale database and item coverage
are narrower than glibc's outside this observed matrix. The shared locale
regression owns the item matrix, return and pointer ABI, alias identity, and
provider check.

The NVIDIA locale-facet paths import `__uselocale@GLIBC_2.3` to select and
restore per-thread locale snapshots. Musl's internal symbol is the canonical
public `uselocale` implementation and preserves query, publication,
restoration, `LC_GLOBAL_LOCALE`, return, and successful-call `errno` behavior.
It is `EXACT`; `compat/locale_ownership` verifies direct provider ownership and
public-alias identity alongside the existing locale lifecycle tests.

Gpucomp imports `__strcoll_l@GLIBC_2.2.5` for locale-facet string comparisons.
Musl's internal symbol is the canonical public `strcoll_l` implementation and
preserves the bytewise ordering, embedded-NUL behavior, return ABI, and
successful-call `errno` contract covered by the observed C and C.UTF-8 paths.
It is `DEGRADED` because musl intentionally ignores locale-specific collation.
The shared `compat/locale_ownership` regression verifies both locale handles,
provider ownership, and public-alias identity.

The NVIDIA stack imports `__strdup@GLIBC_2.2.5` through several compatibility
paths. The compatibility-core symbol delegates to musl's canonical `strdup`
allocator, preserving allocation ownership, NUL-terminated copying, empty
strings, successful-call `errno`, and the return ABI. It is `EXACT` on the
qualified x86_64 ABI; `compat/strdup_abi` verifies independent allocations and
direct provider ownership.

The NVIDIA stack imports `__strftime_l@GLIBC_2.3` for locale-aware time
formatting. The compatibility-core wrapper delegates to musl's canonical
`strftime_l` implementation and preserves the observed numeric formats,
percent conversion, buffer truncation, zero-size behavior, return ABI, and
successful-call `errno`. It is `DEGRADED` because musl's locale database and
formatting extensions are narrower than glibc's; `compat/strftime_abi` covers
the qualified C and C.UTF-8 paths and provider ownership.

The NVIDIA compiler stack imports `__wcsftime_l@GLIBC_2.3` for wide locale-facet
time formatting. Musl's internal symbol is the canonical public `wcsftime_l`
provider and preserves the observed numeric formats, percent conversion,
buffer truncation, zero-size behavior, and return ABI for C and C.UTF-8 handles.
Its wide format parser leaves `errno` as `EINVAL` for non-empty formatting
calls, while preserving an existing value for zero-size calls. It is `DEGRADED`
because musl's locale database, formatting extensions, and successful-call
`errno` behavior are narrower than glibc's; the consolidated
`compat/strftime_abi` regression verifies the direct provider and public-alias
identity.

The NVIDIA stack imports `__strtof_l@GLIBC_2.2.5` and
`__strtod_l@GLIBC_2.2.5` for locale-facet floating-point parsing. Musl's
internal symbols are the canonical public `strtof_l`/`strtod_l` providers and
preserve decimal and hexadecimal conversion, end pointers, special values,
range errors, invalid-input handling, and successful-call `errno` for the
observed C and C.UTF-8 paths. They are `DEGRADED` because musl ignores the
explicit locale and its locale-aware conversion model is narrower; the
consolidated `compat/strto_l` regression verifies both provider and public-alias
identities.

The NVIDIA/CUDA graph imports `__tls_get_addr@GLIBC_2.3` from the glibc loader
SONAME. Musl unifies the loader and libc implementation, exporting the same
x86_64 two-word TLS module/offset ABI from its runtime libc. The policy is
`EXACT`; `compat/tls_abi` verifies provider ownership and resolves a
general-dynamic TLS DSO across multiple threads while preserving the main
thread's storage and `errno`.

The NVIDIA compiler stack imports `__towlower_l@GLIBC_2.2.5` and
`__towupper_l@GLIBC_2.2.5` for locale-facet Unicode case conversion. Musl's
internal symbols are the canonical public `towlower_l`/`towupper_l` providers
and preserve the observed ASCII and Unicode mappings, `WEOF`, return ABI, and
successful-call `errno`. They are `DEGRADED` because musl applies one Unicode
case model regardless of the explicit locale; `compat/wctype_l` now verifies
both provider and public-alias identities.

Gpucomp and the NVIDIA compiler stack import `__strxfrm_l@GLIBC_2.2.5` for
locale-facet collation keys. Musl's internal symbol is the canonical public
`strxfrm_l` provider and returns identity/code-point transformation lengths,
full copies, zero-size sizing results, and the observed `errno` behavior for C
and C.UTF-8 handles. It is `DEGRADED` because locale-specific collation and
glibc's transformed-output policy are not reproduced; `compat/locale_ownership`
now verifies the transformation ABI, truncation boundary, provider ownership,
and public-alias identity.

The NVIDIA compiler stack imports `__wcscoll_l@GLIBC_2.2.5` for wide-string
locale-facet comparisons. Musl's internal symbol is the canonical public
`wcscoll_l` provider and preserves the observed wide-string ordering, Unicode
code-point handling, return ABI, and successful-call `errno` for C and
C.UTF-8 handles. It is `DEGRADED` because locale-specific collation is not
reproduced; `compat/locale_ownership` now verifies provider and public-alias
identity.

The NVIDIA compiler stack imports `__wcsxfrm_l@GLIBC_2.2.5` for wide-string
locale-facet transformation keys. Musl's internal symbol is the canonical
public `wcsxfrm_l` provider and returns Unicode code-point transformation
lengths, full copies, zero-size sizing results, the musl truncation boundary,
and the observed `errno` behavior for C and C.UTF-8 handles. It is `DEGRADED`
because locale-specific collation and glibc's transformed-output policy are not
reproduced; `compat/locale_ownership` verifies the wide transformation ABI,
provider ownership, and public-alias identity.

NVIDIA's present and NVML DSOs import `__progname_full@GLIBC_2.2.5` as a global
object requirement. Musl's object is the authoritative storage aliased by
`program_invocation_name`; startup preserves the exact `argv[0]` pointer and
exposes its basename through `program_invocation_short_name`. This is
`EXACT`, verified by `compat/program_name` through object address identity,
startup value, aliases, `errno`, and direct-musl provider ownership.

Libcuda, libnvidia-cfg, and libnvidia-opencl import
`__register_atfork@GLIBC_2.3.2`; their static adapters pass callback triplets
and DSO handles before registering process-fork handlers. The compatibility
core delegates to `pthread_atfork` and preserves prepare reverse order and
parent/child forward order. It is `DEGRADED` because musl does not retain the
glibc DSO handle for unload-time removal; the qualified NVIDIA loader keeps
these DSOs resident. `compat/atfork` verifies ordering in both processes,
return values, `errno`, provider ownership, and the adapter boundary.

NVML imports `__sched_cpualloc@GLIBC_2.7` for dynamically sized CPU sets used
around affinity queries, including counts crossing 64-bit-word boundaries.
The compatibility-core implementation delegates to musl's zeroed `CPU_ALLOC`
storage and is `EXACT` on the qualified x86_64 ABI. `compat/sched_abi`
verifies allocation size and alignment, zero initialization, boundary-bit
operations, successful `errno` preservation, and provider ownership; cleanup
uses only the public `CPU_FREE` macro.

Gpucomp and tileiras import `__sched_cpucount@GLIBC_2.6` with the observed
128-byte CPU-set size after affinity queries. Musl's direct provider counts
every bit in the supplied byte range, including partial word boundaries, and
preserves `errno`; this is `EXACT` on the qualified x86_64 ABI.
`compat/sched_abi` verifies zero-size/null handling, counts across byte and
word boundaries, the observed 128-byte size, and provider ownership.

NVML imports `__sched_cpufree@GLIBC_2.7` to release those dynamically sized
sets. The compatibility-core implementation delegates to public `CPU_FREE`,
preserves successful-call `errno`, and is `EXACT` on the qualified x86_64 ABI.
`compat/sched_abi` verifies direct provider ownership and cleanup through the
internal symbol rather than only the public macro.

The seven qualified NVIDIA/CUDA objects that perform ordinary system work
import `__errno_location@GLIBC_2.2.5`; the inventory contains both libc and
legacy libpthread SONAME requirements. Musl’s unified libc returns the address
of `errno_val` in the current thread control block. That address is stable for
the thread, is exactly the storage used by the `errno` macro and syscall
wrappers, and is isolated from every concurrently live thread. Both manifest
requirements therefore use the canonical musl provider and are `EXACT`.

Seven qualified NVIDIA/CUDA objects import `__fxstat@GLIBC_2.2.5` at 27 call
sites, all passing x86_64 `_STAT_VER_LINUX` value `1`. Musl’s provider drops
the version argument and delegates to `fstat`; its `struct stat` layout matches
glibc x86_64 and the observed path is exact, including 64-bit sizes and
timestamps. The broader adapter is `DEGRADED` because glibc accepts only its
known x86_64 selectors (`0` and `1`), while musl also accepts unknown values.

Gpucomp separately imports `__fxstat64@GLIBC_2.2.5` at two sites, also passing
selector `1`. On x86_64 LP64, `struct stat64` and `struct stat` are the same
144-byte ABI, so the compatibility-core adapter shares the same table-driven
metadata and error tests as direct musl `__fxstat`. It is likewise
`DEGRADED` only because the adapter accepts unknown selectors. Provider checks
keep direct-musl and compatibility-core ownership distinct.

NVML imports `__fxstatat@GLIBC_2.4` through one ABI wrapper reached by six
internal callers. It fixes the selector to `1`; all callers use flags zero.
Musl delegates directly to `fstatat`, matching that observed relative-directory
path and the x86_64 stat layout. The shared stat regression additionally
verifies absolute paths, symlink follow/no-follow, empty-path descriptor
lookup, invalid paths, descriptors and flags, complete metadata, and `errno`.
The symbol remains `DEGRADED` because musl accepts unknown version selectors
that glibc rejects.

Glcore, NVML, libcuda, and libcudart import
`__getdelim@GLIBC_2.2.5` at 20 call sites. All pass newline as the delimiter,
valid line/capacity pointers, and readable native streams; their loops exercise
both fresh allocation and buffer reuse. Musl’s internal symbol is the canonical
public `getdelim` implementation and matches those paths, including embedded
NUL handling, delimiter retention, final unterminated records, EOF, and
`errno`. It is `DEGRADED` because read attempts on write-only streams preserve
`errno`, whereas glibc reports `EBADF`.

Glcore, NVML, and libcuda import `__isoc99_fscanf@GLIBC_2.7` at 23 sites.
Their five formats cover signed and unsigned decimal integers, bounded
scansets, bounded strings with suppressed tails, and `%lu`, all on readable
native streams. Musl’s symbol aliases its canonical C99 `fscanf` and matches
those conversions, assignment counts, widths, suppression, stream position,
and successful `errno` behavior. It is `DEGRADED`: an initial integer mismatch
sets `EINVAL` instead of preserving `errno`, and a write-only stream preserves
`errno` rather than reporting glibc’s `EBADF`.

Glcore, NVML, libcuda, and gpucomp import
`__isoc99_sscanf@GLIBC_2.7` at 98 sites using 23 unique formats. Those formats
cover PCI identifiers, CUDA versions, names, characters, scansets, suppression,
fixed literals, and signed and unsigned integers across the observed widths and
bases. Musl aliases the symbol to the same C99 scanning engine as public
`sscanf`, and the successful paths match. It is `DEGRADED` because integer
matching failures set `EINVAL` where glibc preserves `errno`. Both ISO C99
entry points share `compat/scanf_abi` rather than maintaining parallel
conversion tests.

Gpucomp imports `__iswctype_l@GLIBC_2.2.5` through three wide-character
locale-facet paths that consume all 12 standard class descriptors. Musl
matches the complete ASCII classification set and uses the same descriptor and
return ABI. It is `DEGRADED`, however, because it ignores the explicit locale
and applies one Unicode classification model: non-ASCII letters remain
alphabetic even with a C locale handle, unlike glibc’s C tables. Focused tests
cover C/C.UTF-8 handles, every class, `WEOF`, unknown descriptors, `errno`, and
provider and public-alias identity.

The NVIDIA compiler stack also imports `__wctype_l@GLIBC_2.2.5` to construct
those wide-character class descriptors. Musl's internal symbol is the
canonical public `wctype_l` provider and returns all 12 standard descriptors,
preserves the descriptor ABI and observed `errno` behavior, and ignores the
explicit locale consistently with `__iswctype_l`. It is `DEGRADED` because
locale-specific descriptor construction is not reproduced;
`compat/wctype_l` verifies descriptor construction, classification, provider
ownership, and public-alias identity together.

Glcore imports `__libc_current_sigrtmin@GLIBC_2.2.5` from libpthread at 23
sites and uses the result consistently for handler installation, signal masks,
and direct `tgkill` delivery. Glibc returns 34 after reserving two internal
realtime signals. Musl returns 35 because it reserves a third signal for
synchronous thread coordination. This is `TRANSLATED`, not numerically exact:
35 is musl’s authoritative first application-usable realtime signal. The
focused test proves stable range reporting, queued payloads, mask operations,
and direct handler delivery.

Gpucomp imports `__lxstat@GLIBC_2.2.5` through two static paths, and gpucomp
plus nvoptix import `__lxstat64@GLIBC_2.2.5` through static pathname-stat
adapters; all observed paths pass selector `1`. Musl's `lstat` implementation
and the compatibility-core `__lxstat64` adapter match the qualified x86_64
LP64 metadata and no-follow pathname behavior. The shared stat regression
covers both providers, regular files, symlinks, dangling links, missing paths,
non-directory components, complete metadata, return values, and `errno`.
Both symbols are `DEGRADED` only because musl accepts unknown version
selectors that glibc rejects.

The NVIDIA compiler stack imports `__xmknod@GLIBC_2.2.5` for special-file
creation. Musl's direct provider drops the version argument and delegates to
`mknod`, preserving the observed FIFO mode and device metadata, successful
`errno`, and missing-parent failure behavior. It is `DEGRADED` because musl
accepts unknown version selectors that glibc rejects; `compat/stat_abi`
verifies the selector, creation, metadata, failure, and provider-ownership
contracts.

The NVIDIA compiler stack imports `__xstat@GLIBC_2.2.5` for followed pathname
metadata. Musl's direct provider drops the version argument and delegates to
`stat`, preserving the observed regular-file metadata, symlink-following
behavior, failures, and `errno`. It is `DEGRADED` because musl accepts unknown
version selectors that glibc rejects; the shared `compat/stat_abi` regression
now verifies the distinct follow-versus-no-follow provider behavior.

Gpucomp and nvoptix import `__xstat64@GLIBC_2.2.5` through followed pathname
adapters. The compatibility-core provider delegates to `stat` with the same
qualified x86_64 LP64 layout and behavior as `__xstat`, preserving the
implementation ownership boundary. It is `DEGRADED` because the adapter
accepts unknown version selectors that glibc rejects; `compat/stat_abi`
verifies both providers and their distinct follow-versus-no-follow paths.

The NVIDIA compiler stack imports `_exit@GLIBC_2.2.5` for non-returning process
termination. Musl's canonical provider terminates the process with the
requested low-byte status without running `atexit` handlers, matching the
observed ABI and lifecycle behavior. It is `EXACT`; `compat/exit_abi` verifies
provider ownership, status propagation, and handler suppression in forked
children.

The NVIDIA compiler stack imports `_setjmp@GLIBC_2.2.5` for non-local control
flow. Musl's canonical provider preserves the x86_64 jump-buffer register and
return-value ABI while leaving the signal mask unchanged, as required by
`_setjmp`; it also preserves the observed `errno`. It is `EXACT`;
`compat/setjmp_abi` verifies provider ownership, the return path, and
non-restoration of a changed signal mask through `_longjmp`.

The local proprietary-driver loader check is opt-in:

```sh
NVIDIA_LIBDIR=/usr/lib \
  meson test -C build --suite nvidia --print-errorlogs
```

It verifies that loading `libnvidia-glcore` fails without early NVIDIA TLS and
succeeds through the complete compatibility-interpreter path when the explicit
TLS policy is enabled. The loader test also proves that glcore and gpucomp
exports remain private under `RTLD_LOCAL`, then become visible at their original
addresses after glcore is promoted with `RTLD_GLOBAL`. It runs 32 local and 32
global open/close cycles with overlapping handles, verifying that closing one
reference cannot invalidate the other or corrupt symbol scope. Weak NVIDIA
imports are classified by ELF binding; the suite verifies both absent optional
probes and weak imports supplied by the runtime. A separate test verifies that
a discovered pointer-sized `_nv*TLS` object has distinct, stable storage in
eight concurrent threads and remains isolated from the main thread. The same
workers exercise NVIDIA's observed pthread-key teardown model through the
compatibility core and require all registered destructors to run.

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
