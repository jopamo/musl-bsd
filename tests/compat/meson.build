compat_test_inc = include_directories('.')

compat_loader_test = executable(
  'test_compat_loader',
  'loader_target.c',
  include_directories: [overlay_inc, compat_test_inc],
  c_args: c_flags,
  link_args: ['-Wl,--dynamic-linker=' + glibc_loader.full_path()],
  install: false,
)

compat_loader_env = environment()
compat_loader_env.set('MUSL_BSD_PRELOAD_PATH', libcompat.full_path())
test(
  'compat/loader',
  compat_loader_test,
  env: compat_loader_env,
  depends: [glibc_loader, libcompat],
  suite: 'compat',
)

compat_tests = [
  'runtime',
  'glibc_api',
  'mount_link',
]

foreach test_name : compat_tests
  test_exe = executable(
    'test_compat_' + test_name,
    'test_' + test_name + '.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat_static,
    c_args: c_flags,
    install: false,
  )
  test('compat/' + test_name, test_exe, suite: 'compat')
endforeach
