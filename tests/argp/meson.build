# Keep this list aligned with test_<behavior>.c.
argp_tests = [
  'ambiguous_options',
  'basic_options',
  'child_parsers',
  'fmtstream',
  'help_error_paths',
  'help_fmt_dup_args',
  'help_matrix',
  'parse_edge_cases',
]

foreach test_name : argp_tests
  test_exe = executable(
    'test_argp_' + test_name,
    'test_' + test_name + '.c',
    include_directories: inc,
    link_with: libargp,
    c_args: c_flags,
    install: false,
  )
  test('argp/' + test_name, test_exe, suite: 'argp')
endforeach

argp_example_minimal = executable(
  'argp_example_minimal',
  'examples/minimal.c',
  include_directories: inc,
  link_with: libargp,
  c_args: c_flags,
  install: false,
)

argp_example_options = executable(
  'argp_example_options_and_arguments',
  'examples/options_and_arguments.c',
  include_directories: inc,
  link_with: libargp,
  c_args: c_flags,
  install: false,
)

argp_example_advanced = executable(
  'argp_example_advanced_options',
  'examples/advanced_options.c',
  include_directories: inc,
  link_with: libargp,
  c_args: c_flags,
  install: false,
)

argp_example_nested = executable(
  'argp_example_nested_parser',
  'examples/nested_parser.c',
  include_directories: inc,
  link_with: libargp,
  c_args: c_flags,
  install: false,
)

test('argp/example/minimal', argp_example_minimal, suite: 'argp')
test('argp/example/nested_parser', argp_example_nested, suite: 'argp')

test(
  'argp/example/options_and_arguments',
  argp_example_options,
  args: ['arg1', 'arg2'],
  suite: 'argp',
)
test(
  'argp/example/options_and_arguments_options',
  argp_example_options,
  args: ['-q', '-v', '--output=FILE', 'arg1', 'arg2'],
  suite: 'argp',
)
test(
  'argp/example/options_and_arguments_silent_alias',
  argp_example_options,
  args: ['-s', 'arg1', 'arg2'],
  suite: 'argp',
)
test(
  'argp/example/options_and_arguments_missing_arg',
  argp_example_options,
  args: ['arg1'],
  should_fail: true,
  suite: 'argp',
)
test(
  'argp/example/options_and_arguments_too_many_args',
  argp_example_options,
  args: ['arg1', 'arg2', 'arg3'],
  should_fail: true,
  suite: 'argp',
)

test(
  'argp/example/advanced_options',
  argp_example_advanced,
  args: ['-v', '--output=FILE', 'arg1_', 'STRING1', 'STRING2', 'STRING3'],
  suite: 'argp',
)
test(
  'argp/example/advanced_options_quiet_repeat',
  argp_example_advanced,
  args: ['-q', '--repeat=2', 'arg1_', 'STRING1'],
  suite: 'argp',
)
test(
  'argp/example/advanced_options_abort',
  argp_example_advanced,
  args: ['--abort', 'arg1_'],
  should_fail: true,
  suite: 'argp',
)
test(
  'argp/example/advanced_options_no_args',
  argp_example_advanced,
  should_fail: true,
  suite: 'argp',
)
