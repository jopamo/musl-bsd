fts_test_inc = include_directories('.')

fts_test_support = static_library(
  'fts_test_support',
  [
    'test_assertions.c',
    'test_fixture.c',
    'test_walk.c',
  ],
  include_directories: [inc, fts_test_inc],
  c_args: c_flags,
  install: false,
)

# Keep this list aligned with test_<behavior>.c. The shared naming rule makes
# each test ID, executable, and source file mechanically discoverable.
fts_tests = [
  # Core API behavior.
  'children_api',
  'children_errno',
  'children_matrix',
  'children_null',
  'file_root',
  'open_invalid_flags',
  'two_roots',

  # Traversal modes and ordering.
  'concurrent_streams',
  'cwd_restore',
  'cycle_detection',
  'cycle_table_edges',
  'fd_discipline',
  'many_children_sorted',
  'seedot',
  'symlink_loop_follow',
  'traversal_order',
  'unreadable_dir',
  'walk_logical',
  'walk_logical_comfollow',
  'walk_nostat_seedot',
  'walk_physical_chdir',
  'walk_physical_nochdir',
  'whiteout',
  'xdev',

  # Fault paths and regressions.
  'changedir_failures',
  'dracut_modalias_compat',
  'fault_injection',
  'read_error_cur_reset',
]

foreach test_name : fts_tests
  test_exe = executable(
    'test_fts_' + test_name,
    'test_' + test_name + '.c',
    include_directories: [inc, fts_test_inc],
    link_with: [libfts_test, fts_test_support],
    c_args: c_flags,
    install: false,
  )
  test('fts/' + test_name, test_exe, suite: 'fts')
endforeach

# These tests need linker wrapping, so they cannot use the regular loop.
fts_wrapped_tests = [
  ['padjust_realloc', 'realloc'],
  ['read_failures', 'malloc'],
]

foreach wrapped_test : fts_wrapped_tests
  test_name = wrapped_test[0]
  wrapped_symbol = wrapped_test[1]
  test_exe = executable(
    'test_fts_' + test_name,
    'test_' + test_name + '.c',
    include_directories: [inc, fts_test_inc],
    link_with: [libfts_test, fts_test_support],
    c_args: c_flags,
    link_args: ['-Wl,--wrap=' + wrapped_symbol],
    install: false,
  )
  test('fts/' + test_name, test_exe, suite: 'fts')
endforeach
