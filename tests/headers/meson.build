header_tests = [
  'glibc_overlay',
  'linux_mount_overlay',
]

foreach test_name : header_tests
  test_exe = executable(
    'test_headers_' + test_name,
    'test_' + test_name + '.c',
    include_directories: overlay_inc,
    link_with: libcompat_static,
    c_args: c_flags,
    install: false,
  )
  test('headers/' + test_name, test_exe, suite: 'headers')
endforeach
