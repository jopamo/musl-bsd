test_exe = executable(
  'test_cdefs_qsort_r_reentrant',
  'test_qsort_r_reentrant.c',
  include_directories: inc,
  c_args: c_flags,
  install: false,
)
test('cdefs/qsort_r_reentrant', test_exe, suite: 'cdefs')
