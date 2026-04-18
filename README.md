# musl-bsd

`musl-bsd` helps software written for BSD or glibc compile and run on musl-based systems. It provides a focused set of compatibility libraries and headers for APIs that many C projects assume are available, reducing the need for per-project portability patches.

- `libfts`: BSD file-tree traversal APIs for walking directory hierarchies (`fts_open`, `fts_read`, and related functions)
- `libobstack`: GNU obstack allocation APIs for efficient incremental object construction
- `libargp`: GNU `argp` command-line parsing APIs used by tools that depend on `argp_parse` behavior
- Compatibility headers in `include/` (installed as headers like `sys/queue.h`, `sys/tree.h`, and `sys/cdefs.h`)

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
