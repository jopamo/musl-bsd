# fts Testing Checklist

## Core API Coverage

* `fts_open` correctly handles single path, multiple paths, and empty path list cases
* `fts_read` returns entries in expected order for a simple tree (preorder `FTS_D`, postorder `FTS_DP` if used)
* `fts_children` lists immediate children correctly for:

  * directory at root
  * directory deep in the tree
  * `NULL` argument meaning “current directory”
* `fts_set` correctly applies:

  * `FTS_FOLLOW`
  * `FTS_SKIP`
  * `FTS_AGAIN`
* `fts_close`:

  * returns 0 on success and closes all internal file descriptors
  * propagates last error (via `errno`) if close fails

---

## Traversal Order & `fts_info` Values

* Simple tree traversal yields the expected sequence of `fts_info`:

  * `FTS_D` for preorder dirs
  * `FTS_DP` for postorder dirs when appropriate
  * `FTS_F` for regular files
  * `FTS_SL` / `FTS_SLNONE` for symlinks as per flags
  * `FTS_DNR`, `FTS_NS`, `FTS_ERR` on errors
* Sibling ordering matches comparison function when one is provided (`compar` in `fts_open`)
* Without a comparison function, order matches underlying `readdir` order and is stable within a single run
* Directories yield preorder `FTS_D` followed by postorder `FTS_DP` unless explicitly skipped

---

## Flag Behavior: Logical vs Physical Walks

* Default traversal is physical (`lstat` style)
* `FTS_LOGICAL` opt-in follows symlinks during traversal and forces `FTS_NOCHDIR` internally:

  * symbolic link to directory is treated as directory when followed
  * `fts_info` reflects logical view
* `FTS_PHYSICAL` does not follow symlinks:

  * symlink to directory is reported as `FTS_SL` / `FTS_SLNONE`
  * directory pointed to is never descended into
* `FTS_COMFOLLOW` forces a follow for the initial path when it is a symlink
* Combinations:

  * `FTS_LOGICAL | FTS_COMFOLLOW`
  * `FTS_PHYSICAL | FTS_COMFOLLOW`
    behave as expected for the first element vs deeper symlinks

---

## Flag Behavior: Miscellaneous

* `FTS_NOCHDIR`:

  * default traversal uses `chdir`/`fchdir` for speed and shorter paths
  * with `FTS_NOCHDIR`, traversal stays rooted via absolute paths without changing process `cwd`
* `FTS_XDEV`:

  * traversal does not cross filesystem boundaries (same `st_dev` only)
  * offending directories still return a postorder `FTS_DP` but are not descended into
* `FTS_SEEDOT`:

  * `.` and `..` entries appear in traversal where they exist
* `FTS_NOSTAT`:

  * `fts_statp` is omitted for non-directories (stack buffer only)
  * when walking physically, d_type is used to classify non-directories without a `stat`; logical walks still `stat`
* `FTS_WHITEOUT`:

  * on platforms exposing `DT_WHT`, entries surface as `FTS_W` with zeroed `fts_statp` (and `st_mode` set to `S_IFWHT` when available)

---

## Path Buffer & Name Handling

* `fts_path` is always:

  * null-terminated
  * valid until next `fts_read` call
* `fts_accpath` reflects the appropriate path used for system calls (`stat`, `open`, etc)
* Very deep directory structures:

  * paths longer than `PATH_MAX` (if defined) are handled via dynamic buffers without truncation
  * traversal does not crash or corrupt memory on long paths
* Pathnames with:

  * spaces
  * leading `./`
  * trailing slashes
  * empty relative components
    are normalized and traversed as expected
* Multibyte filenames:

  * UTF-8 names are returned intact in `fts_name` and `fts_path`
  * no truncation or invalid splitting of multibyte sequences

---

## Directory Stream & Descriptor Handling

* Each directory visited:

  * opens a descriptor only when needed
  * closes it after iterating children or on error
* No descriptor leaks when:

  * traversal finishes normally
  * traversal is aborted early by user (breaking out of loop)
  * `fts_close` is called after an error in `fts_read`
* `FTS_NOCHDIR` vs default:

  * both modes keep descriptor usage within tight bounds
  * no mixing of `chdir` and descriptors leads to incorrect paths

---

## Error Handling & `fts_info` Conditions

* Non-readable directory:

  * `fts_read` yields `FTS_DNR` for that entry
  * `fts_errno` contains the underlying `errno` value
* `stat` failure:

  * entry is reported as `FTS_NS`
  * `fts_errno` set appropriately
* `opendir` / `readdir` failures:

  * correct `FTS_ERR` entry produced
  * traversal continues for other branches where possible
* User-simulated failure:

  * injecting failures via test harness (e.g. fault-injection wrappers) yields consistent `FTS_*` codes and does not leak or crash

---

## Cycle Detection

* Directory self-symlink (e.g. `dir/loop -> .`) is detected as a cycle
* Parent loop (e.g. `dir/a/b/c/loop -> ../..`) is also detected
* On detection:

  * `fts_info` for the offending entry is set to cycle type (`FTS_DC` or equivalent)
  * `fts_cycle` points to the earlier ancestor `FTSENT` in the cycle chain
* Multiple distinct cycles in a tree do not interfere with each other
* Cycle detection remains correct under both `FTS_LOGICAL` and `FTS_PHYSICAL` modes

---

## `fts_children` Semantics

* Called after `fts_read` returns a directory entry:

  * returns a linked list of direct children with correct `fts_parent` and `fts_level`
  * respects `FTS_NOCHDIR`, `FTS_PHYSICAL`, `FTS_LOGICAL`, etc
* With `FTS_NAMEONLY` flag:

  * `fts_children` omits `stat` data where expected
  * populates only name-related fields while leaving others as agreed
* `fts_children` called on a non-directory entry returns `NULL` and leaves `errno` unchanged
* `fts_children(NULL, ...)` enumerates children of the “current” directory as defined by the last `FTS_D` entry

---

## Memory Layout & ABI Expectations

* `FTSENT`:

  * field offsets, sizes, and alignment match the chosen BSD reference
  * `fts_parent`, `fts_link`, `fts_cycle`, `fts_statp`, and `fts_path` behave as expected for linked traversal
* `FTS` internal structure is opaque to users but stable across your own library versions unless SONAME is bumped
* ABI test:

  * `sizeof(FTSENT)` and offsets of key fields (`fts_info`, `fts_name`, `fts_path`, `fts_parent`, `fts_level`) match the reference layout verified in separate ABI tests

---

## Stress & Edge Cases

* Very large directory with thousands of entries:

  * traversal is complete and deterministic
  * no excessive memory growth beyond what is expected for entry lists and path buffers
* Very deep directory nesting:

  * no stack overflow (avoid recursion in implementation)
  * no buffer overflow in `fts_path` or `fts_accpath`
* Mix of file types:

  * regular files, dirs, symlinks, sockets, fifos, devices are all classified correctly in `fts_info`
* Empty directories:

  * directories with no entries still get visited correctly as `FTS_D` (and `FTS_DP` if applicable)
* Stopping conditions:

  * user breaks out of traversal early, calls `fts_close`, and no leaks or use-after-free occur

---

## Golden Behavior Tests

Create a fixed fixture tree (like `tests/fixtures/fts/basic/`) and record:

* expected traversal sequence (type, path, level) for:

  * default flags
  * `FTS_PHYSICAL`
  * `FTS_LOGICAL | FTS_COMFOLLOW`
  * `FTS_NOCHDIR`
* expected `fts_info` and `fts_level` for each entry
* expected cycle detection cases
* expected behavior for permission errors and simulated `stat` failures

Tests must assert:

* traversal sequence matches golden expectations
* `fts_info` and `fts_level` values match the golden log
* no memory errors under ASan/UBSan and no descriptor leaks detected by tools like `lsof` or custom wrappers

---

# Design Notes (engine + nodes + policy)

* **Layering:** treat `FTSENT` as the only user-visible node model, a traversal engine that emits nodes, and a policy layer that interprets flags (`LOGICAL`, `PHYSICAL`, `NOCHDIR`, `XDEV`, `NOSTAT`, `NAMEONLY`, `COMFOLLOW`, `SEEDOT`, `WHITEOUT`). Tests should “fall out” of these three pieces.
* **ABI-first `FTSENT`:** mirror the NetBSD layout byte-for-byte; user-visible fields are immutable snapshots. Keep extra engine state inside opaque `FTS`. Maintain offset/size tests (`pahole`/`clang -fdump-record-layouts`).
* **Directory frames:** keep an explicit stack of frames (dirfd/`DIR*`, children list + index, flags for preorder/postorder owed, skip/XDEV state). `fts_read` is a state machine over this stack; no recursion. `FTS_AGAIN` reuses the current entry; `FTS_DP` is emitted unless skipped.
* **Path handling:** a single resizable path buffer owned by `FTS`; `fts_padjust` rewrites pointers after realloc. Default mode uses `chdir`/`fchdir` with a stack of dirfds; `FTS_NOCHDIR` sticks to absolute (or rooted relative) paths and never changes cwd. `fts_accpath` is the syscall target in either mode.
* **Policy hooks:** centralize decisions: (1) classification (`stat` vs `lstat`, `NOSTAT`/`NAMEONLY`, `d_type` hints), (2) descent (`XDEV`, `FTS_SKIP`, errors), (3) sibling order (`compar` → sort buffered children, else raw `readdir`). This keeps traversal, flag semantics, and ordering deterministic.
* **Cycle detection:** on promoting an entry to `FTS_D`, compare `(st_dev, st_ino)` against ancestors (and a small hash set for speed). On hit, set `fts_info = FTS_DC` and `fts_cycle` to the ancestor entry.
* **Errors as entries:** wrap syscalls so failures yield `FTS_DNR` / `FTS_NS` / `FTS_ERR` with `fts_errno` set; engine keeps running. Fault injection is just swapping the ops table.
* **`fts_children`:** returns the already-buffered child list for the current directory; `NAMEONLY` builds that list without stats. `fts_children(NULL, …)` is “children of the last `FTS_D`”. Never re-scan the directory.
* **FD discipline:** frame lifecycle defines when `DIR*`/dirfd are open; close on `FTS_DP` or errors, and fchdir back up in chdir mode. Guard against leaks when users exit early or on errors.
* **Portability order:** prioritize musl correctness, match BSD semantics/ABI, and stay compatible on glibc. Whiteouts (`FTS_W`) only surface where `DT_WHT` exists; everything else relies only on POSIX.1-2008 calls (`openat`, `fstatat`, `readlinkat`, `fdopendir`, etc.).
