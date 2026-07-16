if host_machine.cpu_family() == 'x86_64'
  static_layout_test = executable(
    'test_abi_static_layout',
    'test_static_layout.c',
    include_directories: inc,
    c_args: c_flags,
    install: false,
  )
  test('abi/static_layout', static_layout_test, suite: 'abi')

  layout_cc = cc.cmd_array()[0]
  clang_prog = find_program('clang', required: false)
  if clang_prog.found()
    layout_cc = clang_prog.full_path()
  endif

  record_layout_script = find_program('check_record_layouts.sh')
  test(
    'abi/record_layouts',
    record_layout_script,
    env: {
      'MESON_SOURCE_ROOT': meson.project_source_root(),
      'MESON_BUILD_ROOT': meson.project_build_root(),
      'CC': layout_cc,
    },
    suite: 'abi',
  )
endif
