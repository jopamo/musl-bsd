obstack_test_inc = include_directories('.')

obstack_test_support = static_library(
  'obstack_test_support',
  'test_support.c',
  include_directories: [inc, obstack_test_inc],
  c_args: c_flags,
  install: false,
)

# Keep this list aligned with test_<behavior>.c.
obstack_tests = [
  'alignment',
  'begin_extra',
  'begin_plain',
  'calculate_object_size',
  'extra_allocator_oom',
  'force_newchunk',
  'free_edges',
  'free_reset',
  'free_to_object',
  'growth_boundaries',
  'int_ptr_grow',
  'memory_used',
  'printf_len_guard',
  'printf_small',
  'vprintf',
]

foreach test_name : obstack_tests
  test_exe = executable(
    'test_obstack_' + test_name,
    'test_' + test_name + '.c',
    include_directories: [inc, obstack_test_inc],
    link_with: [libobstack, obstack_test_support],
    c_args: c_flags,
    install: false,
  )
  test('obstack/' + test_name, test_exe, suite: 'obstack')
endforeach

# These tests need static production code so linker wrapping can intercept the
# allocation and formatting calls made by obstack itself.
obstack_wrapped_tests = [
  ['alloc_failed_handler', 'malloc'],
  ['vprintf_failures', 'vsnprintf'],
]

foreach wrapped_test : obstack_wrapped_tests
  test_name = wrapped_test[0]
  wrapped_symbol = wrapped_test[1]
  test_exe = executable(
    'test_obstack_' + test_name,
    'test_' + test_name + '.c',
    include_directories: [inc, obstack_test_inc],
    link_with: [libobstack_test, obstack_test_support],
    c_args: c_flags,
    link_args: ['-Wl,--wrap=' + wrapped_symbol],
    install: false,
  )
  test('obstack/' + test_name, test_exe, suite: 'obstack')
endforeach
