compat_test_inc = include_directories('.')

if glibc_runtime_enabled
  python = find_program('python3')
  readelf = find_program('readelf')

  static_pie_baseline = executable(
    'static_pie_baseline',
    'static_pie_baseline.c',
    c_args: c_flags,
    link_args: ['-static-pie'],
    install: false,
  )
  test(
    'compat/static_pie_baseline',
    static_pie_baseline,
    suite: 'compat',
  )
  static_pie_checker = files('check_static_pie.py')
  test(
    'compat/static_pie_metadata',
    python,
    args: [
      static_pie_checker,
      readelf.full_path(),
      static_pie_baseline.full_path(),
    ],
    depends: static_pie_baseline,
    suite: 'compat',
  )

  ifunc_dependency = shared_library(
    'ifunc-dependency',
    'ifunc_dependency.c',
    install: false,
    soversion: '1',
  )
  ifunc_a = shared_library(
    'ifunc-a',
    'ifunc_a.S',
    link_with: ifunc_dependency,
    install: false,
    soversion: '1',
    link_args: ['-Wl,-z,now'],
  )
  ifunc_b = shared_library(
    'ifunc-b',
    'ifunc_b.S',
    link_with: ifunc_dependency,
    install: false,
    soversion: '1',
    link_args: ['-Wl,-z,now'],
  )
  ifunc_startup = executable(
    'ifunc_startup',
    'ifunc_startup.c',
    link_with: ifunc_a,
    install: false,
    link_args: ['-Wl,-z,now'],
  )
  ifunc_multiple = executable(
    'ifunc_multiple',
    'ifunc_multiple.c',
    link_with: [ifunc_a, ifunc_b],
    install: false,
    link_args: ['-Wl,-z,now'],
  )
  ifunc_dlopen = executable(
    'ifunc_dlopen',
    'ifunc_dlopen.c',
    install: false,
  )
  ifunc_runner = files('run_ifunc_tests.py')
  test(
    'compat/ifunc_irelative',
    python,
    args: [
      ifunc_runner,
      musl_linker_path,
      readelf.full_path(),
      ifunc_startup.full_path(),
      ifunc_dlopen.full_path(),
      ifunc_multiple.full_path(),
      ifunc_a.full_path(),
      ifunc_b.full_path(),
    ],
    depends: [
      ifunc_dependency,
      ifunc_a,
      ifunc_b,
      ifunc_startup,
      ifunc_multiple,
      ifunc_dlopen,
    ],
    suite: 'compat',
    timeout: 30,
  )

  loader_policy_test = executable(
    'test_loader_policy',
    ['test_loader_policy.c', '../../src/compat/loader_policy.c'],
    include_directories: include_directories('../../src/compat'),
    c_args: c_flags,
    install: false,
  )
  test('compat/loader_policy', loader_policy_test, suite: 'compat')

  preload_policy_test = executable(
    'test_preload_policy',
    ['test_preload_policy.c', '../../src/compat/preload_policy.c'],
    include_directories: include_directories('../../src/compat'),
    c_args: c_flags,
    install: false,
  )
  test('compat/preload_policy', preload_policy_test, suite: 'compat')

  compat_loader_test = executable(
    'compat_loader_runtime',
    'loader_target.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    link_args: ['-Wl,--dynamic-linker=' + glibc_loader.full_path()],
    install: false,
  )

  loader_plugin = shared_library(
    'loader-plugin',
    'loader_plugin.c',
    c_args: c_flags,
    install: false,
  )

  loader_user_preload = shared_library(
    'loader-user-preload',
    'loader_user_preload.c',
    c_args: c_flags,
    install: false,
  )

  patchelf = find_program('patchelf')
  loader_runner = files('run_loader_tests.py')
  test(
    'compat/loader',
    python,
    args: [
      loader_runner,
      glibc_loader.full_path(),
      compat_loader_test.full_path(),
      libcompat.full_path(),
      meson.project_build_root(),
      loader_plugin.full_path(),
      loader_user_preload.full_path(),
      musl_linker_path,
      patchelf.full_path(),
    ],
    depends: [
      glibc_loader,
      libcompat,
      compat_loader_test,
      loader_plugin,
      loader_user_preload,
    ] + glibc_facades,
    suite: 'compat',
    timeout: 60,
  )

  nvidia_loader_runner = files('run_nvidia_loader_test.py')
  test(
    'nvidia/loader_glcore',
    python,
    args: [
      nvidia_loader_runner,
      glibc_loader.full_path(),
      compat_loader_test.full_path(),
      libcompat.full_path(),
      meson.project_build_root(),
      patchelf.full_path(),
    ],
    depends: [
      glibc_loader,
      libcompat,
      compat_loader_test,
    ] + glibc_facades,
    suite: ['compat', 'nvidia'],
    timeout: 60,
  )

  facade_checker = files('check_facades.py')
  test(
    'compat/facades',
    python,
    args: [
      facade_checker,
      readelf.full_path(),
      libcompat.full_path(),
      glibc_facades[0].full_path(),
      glibc_facades[1].full_path(),
      glibc_facades[2].full_path(),
      glibc_facades[3].full_path(),
      glibc_facades[4].full_path(),
      glibc_facades[5].full_path(),
      glibc_facades[6].full_path(),
    ],
    depends: [libcompat] + glibc_facades,
    suite: 'compat',
  )
endif

if pthread_abi_supported
  pthread_abi_test = executable(
    'test_compat_pthread_abi',
    'test_pthread_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )

  pthread_runner = files('run_preloaded_test.py')
  test(
    'compat/pthread_abi',
    python,
    args: [
      pthread_runner,
      pthread_abi_test.full_path(),
      libcompat.full_path(),
      patchelf.full_path(),
    ],
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
