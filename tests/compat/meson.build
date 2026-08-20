compat_test_inc = include_directories('.')

compat_loader_test = executable(
  'compat_loader_runtime',
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

if pthread_abi_supported
  pthread_abi_test = executable(
    'test_compat_pthread_abi',
    'test_pthread_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )

  pthread_abi_env = environment()
  pthread_abi_env.set('LD_PRELOAD', libcompat.full_path())
  test(
    'compat/pthread_abi',
    pthread_abi_test,
    env: pthread_abi_env,
    depends: libcompat,
    suite: 'compat',
    timeout: 30,
  )

  if cc.get_define('__GLIBC__', prefix: '#include <features.h>') != ''
    glibc_pthread_layout_test = executable(
      'test_glibc_pthread_layout',
      'test_glibc_pthread_layout.c',
      c_args: c_flags,
      install: false,
    )
    test(
      'compat/glibc_pthread_layout',
      glibc_pthread_layout_test,
      suite: 'compat',
    )
  endif
endif

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
