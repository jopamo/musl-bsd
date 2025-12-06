# argp testing checklist

## current coverage (tests/argp)

- `test_basic_options.c`: short/long options, positional delivery, `arg_num`, `KEY_END`, `KEY_SUCCESS`, and `ARGP_NO_EXIT` flow
- `test_child_parsers.c`: chained parsers via `.children`, `child_inputs` propagation, parent/child `KEY_INIT`/`KEY_END` ordering
- `test_help_fmt_dup_args.c`: `$ARGP_HELP_FMT` parsing, column overrides, `dup-args` behavior, help output contains option docs
- `test_ambiguous.c`: ambiguous long option resolution with `ARGP_NO_EXIT|ARGP_NO_ERRS` returning `EINVAL`

Keep adding scenarios below as individual tests (one file per behavior) and register them in Meson.

## near-term gaps to cover

- Key lifecycle: explicit `KEY_ARGS` and `KEY_FINI` delivery, `KEY_ERROR` propagation on parser failures
- Flag matrix: `ARGP_IN_ORDER`, `ARGP_NO_ARGS`, `ARGP_LONG_ONLY` ambiguity rules, `ARGP_NO_HELP` vs `ARGP_HELP_ONLY`, `ARGP_PARSE_ARGV0`, `ARGP_SILENT`, `ARGP_ARGV_ERR_UNKNOWN`
- Built-ins: `--help`/`--usage` exit codes and printed program name, `argp_program_version`/`argp_program_version_hook`, `argp_program_bug_address`
- Error helpers: `argp_usage`, `argp_error`, `argp_failure` (exit codes, errnum passthrough, `ARGP_NO_ERRS`/`ARGP_SILENT` interaction)
- State integrity: `argp_state->next` and `argv` mutation when parsers decrement `next`; `quoted` handling around `--`
- Option surface: `OPTION_ALIAS`, `OPTION_DOC`, `OPTION_NO_USAGE`, and grouping rules in help output
- `ARGP_HELP_FMT`: boolean toggles (e.g., `no-dup-args`), width/indent tuning, stable wrapping (golden outputs)

## golden output plan

- Capture deterministic help/usage/error outputs for:
  - simple parser help
  - chained parser help with headers
  - usage line only
  - unknown option error
  - missing required argument
- Store under `tests/argp/golden/` and compare with tolerant diff (e.g., normalize trailing spaces, allow wrapped lines if widths match configured margins).

## stress and compatibility

- Large argv and multibyte arguments keep state consistent (no overflow, no corruption)
- Minimal parser (no options) still emits `KEY_SUCCESS`/`KEY_END` and respects `ARGP_NO_ARGS`
- Repeated options accumulate as user code expects (e.g., counter or last-value wins) without leaking
- glibc parity: behavior matches manual examples and `argp-standalone` for the same inputs, including `--opt=value` and long-only mode resolution

## memory-safety expectations

- Clean under ASan/UBSan for: normal parse, early exit via help/usage, parser returning errors, and repeated help/error calls
- No leaked allocations from help formatting buffers or `child_inputs` handling
