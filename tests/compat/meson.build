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

  loader_dependency_leaf = shared_library(
    'loader-dependency-leaf',
    'loader_dependency_leaf.c',
    c_args: c_flags,
    install: false,
  )

  loader_dependency_middle = shared_library(
    'loader-dependency-middle',
    'loader_dependency_middle.c',
    link_with: loader_dependency_leaf,
    c_args: c_flags,
    install: false,
  )

  loader_plugin = shared_library(
    'loader-plugin',
    'loader_plugin.c',
    link_with: loader_dependency_middle,
    c_args: c_flags,
    install: false,
  )

  loader_user_preload = shared_library(
    'loader-user-preload',
    'loader_user_preload.c',
    c_args: c_flags,
    install: false,
  )

  loader_weak_consumer = shared_library(
    'loader-weak-consumer',
    'loader_weak_consumer.c',
    c_args: c_flags,
    install: false,
  )

  loader_weak_provider = shared_library(
    'loader-weak-provider',
    'loader_weak_provider.c',
    c_args: c_flags,
    install: false,
  )

  loader_versioned_map = meson.current_source_dir() / 'loader_versioned_provider.map'
  loader_versioned_reference = shared_library(
    'loader-versioned-reference',
    'loader_versioned_provider.c',
    c_args: c_flags,
    link_args: ['-Wl,--version-script=' + loader_versioned_map],
    link_depends: loader_versioned_map,
    install: false,
  )

  loader_versioned_consumer = shared_library(
    'loader-versioned-consumer',
    'loader_versioned_consumer.c',
    link_with: loader_versioned_reference,
    c_args: c_flags,
    install: false,
  )

  loader_versioned_runtime = shared_library(
    'loader-versioned-runtime',
    'loader_versioned_provider.c',
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
      loader_weak_consumer.full_path(),
      loader_weak_provider.full_path(),
      loader_versioned_consumer.full_path(),
      loader_versioned_runtime.full_path(),
      musl_linker_path,
      readelf.full_path(),
      patchelf.full_path(),
    ],
    depends: [
      glibc_loader,
      libcompat,
      compat_loader_test,
      loader_dependency_leaf,
      loader_dependency_middle,
      loader_plugin,
      loader_user_preload,
      loader_weak_consumer,
      loader_weak_provider,
      loader_versioned_reference,
      loader_versioned_consumer,
      loader_versioned_runtime,
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
      readelf.full_path(),
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

  nvidia_tls_threads = executable(
    'nvidia_tls_threads',
    'nvidia_tls_threads.c',
    install: false,
  )
  nvidia_tls_runner = files('run_nvidia_tls_test.py')
  test(
    'nvidia/tls_threads',
    python,
    args: [
      nvidia_tls_runner,
      musl_linker_path,
      readelf.full_path(),
      nvidia_tls_threads.full_path(),
      libcompat.full_path(),
    ],
    depends: [nvidia_tls_threads, libcompat],
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

  dlfcn_manifest_checker = files('check_dlfcn_manifest.py')
  test(
    'compat/dlfcn_manifest',
    python,
    args: [
      dlfcn_manifest_checker,
      meson.project_source_root() / 'nvidia-symbols.json',
    ],
    suite: 'compat',
  )

  runtime_manifest_checker = files('check_runtime_manifest.py')
  test(
    'compat/runtime_manifest',
    python,
    args: [
      runtime_manifest_checker,
      readelf.full_path(),
      meson.project_source_root() / 'nvidia-symbols.json',
      libcompat.full_path(),
      musl_linker_path,
    ],
    depends: libcompat,
    suite: 'compat',
  )

  backtrace_test = executable(
    'test_compat_backtrace',
    'test_backtrace.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    link_args: ['-Wl,--export-dynamic'],
    install: false,
  )
  test('compat/backtrace', backtrace_test, suite: 'compat')

  malloc_hook_scope_test = executable(
    'test_compat_malloc_hook_scope',
    'test_malloc_hook_scope.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    install: false,
  )
  test(
    'compat/malloc_hook_scope',
    malloc_hook_scope_test,
    suite: 'compat',
  )

  secure_getenv_scope_test = executable(
    'test_compat_secure_getenv_scope',
    'test_secure_getenv_scope.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    install: false,
  )
  test(
    'compat/secure_getenv_scope',
    secure_getenv_scope_test,
    suite: 'compat',
  )

  io_getc_test = executable(
    'test_compat_io_getc',
    'test_io_getc.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/io_getc', io_getc_test, suite: 'compat')

  io_putc_test = executable(
    'test_compat_io_putc',
    'test_io_putc.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/io_putc', io_putc_test, suite: 'compat')

  assert_fail_test = executable(
    'test_compat_assert_fail',
    'test_assert_fail.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/assert_fail', assert_fail_test, suite: 'compat')

  access_abi_test = executable(
    'test_compat_access_abi',
    'test_access_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/access_abi', access_abi_test, suite: 'compat')

  acos_abi_test = executable(
    'test_compat_acos_abi',
    'test_acos_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/acos_abi', acos_abi_test, suite: 'compat')

  alarm_abi_test = executable(
    'test_compat_alarm_abi',
    'test_alarm_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/alarm_abi', alarm_abi_test, suite: 'compat')

  alphasort_abi_test = executable(
    'test_compat_alphasort_abi',
    'test_alphasort_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    install: false,
  )
  test('compat/alphasort_abi', alphasort_abi_test, suite: 'compat')

  exit_abi_test = executable(
    'test_compat_exit_abi',
    'test_exit_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/exit_abi', exit_abi_test, suite: 'compat')

  setjmp_abi_test = executable(
    'test_compat_setjmp_abi',
    'test_setjmp_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/setjmp_abi', setjmp_abi_test, suite: 'compat')

  strdup_abi_test = executable(
    'test_compat_strdup',
    'test_strdup.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    install: false,
  )
  test('compat/strdup_abi', strdup_abi_test, suite: 'compat')

  strftime_abi_test = executable(
    'test_compat_strftime',
    'test_strftime.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    install: false,
  )
  test('compat/strftime_abi', strftime_abi_test, suite: 'compat')

  strto_l_test = executable(
    'test_compat_strto_l',
    'test_strto_l.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/strto_l', strto_l_test, suite: 'compat')

  tls_abi_dso = shared_library(
    'tls-abi',
    'tls_abi.c',
    c_args: c_flags,
    install: false,
  )
  tls_abi_test = executable(
    'test_compat_tls_abi',
    'test_tls_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test(
    'compat/tls_abi',
    tls_abi_test,
    args: tls_abi_dso.full_path(),
    depends: tls_abi_dso,
    suite: 'compat',
  )

  ctype_b_loc_test = executable(
    'test_compat_ctype_b_loc',
    'test_ctype_b_loc.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/ctype_b_loc', ctype_b_loc_test, suite: 'compat')

  ctype_get_mb_cur_max_test = executable(
    'test_compat_ctype_get_mb_cur_max',
    'test_ctype_get_mb_cur_max.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test(
    'compat/ctype_get_mb_cur_max',
    ctype_get_mb_cur_max_test,
    suite: 'compat',
  )

  wctype_l_test = executable(
    'test_compat_wctype_l',
    'test_wctype_l.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/wctype_l', wctype_l_test, suite: 'compat')

  sigrtmin_test = executable(
    'test_compat_sigrtmin',
    'test_sigrtmin.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/sigrtmin', sigrtmin_test, suite: 'compat')

  cxa_atexit_test = executable(
    'test_compat_cxa_atexit',
    'test_cxa_atexit.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/cxa_atexit', cxa_atexit_test, suite: 'compat')

  cxa_finalize_test = executable(
    'test_compat_cxa_finalize',
    'test_cxa_finalize.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/cxa_finalize', cxa_finalize_test, suite: 'compat')

  atfork_test = executable(
    'test_compat_atfork',
    'test_atfork.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    install: false,
  )
  test('compat/atfork', atfork_test, suite: 'compat')

  sched_abi_test = executable(
    'test_compat_sched_abi',
    'test_sched_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    install: false,
  )
  test('compat/sched_abi', sched_abi_test, suite: 'compat')

  locale_ownership_test = executable(
    'test_compat_locale_ownership',
    'test_locale_ownership.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test(
    'compat/locale_ownership',
    locale_ownership_test,
    suite: 'compat',
  )

  program_name_test = executable(
    'test_compat_program_name',
    'test_program_name.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/program_name', program_name_test, suite: 'compat')

  errno_location_test = executable(
    'test_compat_errno_location',
    'test_errno_location.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/errno_location', errno_location_test, suite: 'compat')

  stat_abi_test = executable(
    'test_compat_stat_abi',
    'test_stat_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    link_with: libcompat,
    c_args: c_flags,
    install: false,
  )
  test('compat/stat_abi', stat_abi_test, suite: 'compat')

  getdelim_test = executable(
    'test_compat_getdelim',
    'test_getdelim.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/getdelim', getdelim_test, suite: 'compat')

  scanf_abi_test = executable(
    'test_compat_scanf_abi',
    'test_scanf_abi.c',
    include_directories: [overlay_inc, compat_test_inc],
    c_args: c_flags,
    install: false,
  )
  test('compat/scanf_abi', scanf_abi_test, suite: 'compat')
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
    depends: [pthread_abi_test, libcompat],
    suite: 'compat',
    timeout: 30,
  )

  pthread_manifest_checker = files('check_pthread_manifest.py')
  test(
    'compat/pthread_manifest',
    python,
    args: [
      pthread_manifest_checker,
      readelf.full_path(),
      meson.project_source_root() / 'nvidia-symbols.json',
      libcompat.full_path(),
    ],
    depends: libcompat,
    suite: 'compat',
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
  'fallocate64',
  'glibc_api',
  'mount_link',
  'rawmemchr',
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
