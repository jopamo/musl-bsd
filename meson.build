project(
  'musl-bsd',
  'c',
  version: '2.0.0',
  default_options: [
    'c_std=c99',
    'warning_level=2',
    'buildtype=debug'
  ]
)

env = environment()
env.set('CC', 'clang')
env.set('CXX', 'clang++')
env.set('LD', 'ld.lld')

cc = meson.get_compiler('c')

conf_data = configuration_data()

conf_data.set('HAVE_CONFIG_H', true)
conf_data.set('HAVE_ALLOCA_H', cc.check_header('alloca.h'))
conf_data.set('HAVE_UNISTD_H', cc.check_header('unistd.h'))
conf_data.set('HAVE_EX_USAGE', cc.has_header_symbol('sysexits.h', 'EX_USAGE'))
conf_data.set('HAVE_DECL_FLOCKFILE', cc.has_function('flockfile'))
conf_data.set('HAVE_DECL_FPUTS_UNLOCKED', cc.has_function('fputs_unlocked', prefix: '#include <stdio.h>'))
conf_data.set('HAVE_DECL_FPUTC_UNLOCKED', cc.has_function('fputc_unlocked', prefix: '#include <stdio.h>'))
conf_data.set('HAVE_DECL_FWRITE_UNLOCKED', cc.has_function('fwrite_unlocked', prefix: '#include <stdio.h>'))
conf_data.set('HAVE_DECL_PUTC_UNLOCKED', cc.has_function('putc_unlocked', prefix: '#include <stdio.h>'))
conf_data.set('HAVE_MEMPCPY', cc.has_function('mempcpy', prefix: '#include <string.h>'))
conf_data.set('HAVE_ASPRINTF', cc.has_function('asprintf', prefix: '#include <stdio.h>'))
conf_data.set('HAVE_STRCHRNUL', cc.has_function('strchrnul', prefix: '#include <string.h>'))
conf_data.set('HAVE_STRNDUP', cc.has_function('strndup', prefix: '#include <string.h>'))
conf_data.set('HAVE_STRCASECMP', cc.has_function('strcasecmp', prefix: '#include <string.h>'))
conf_data.set('HAVE_DECL_PROGRAM_INVOCATION_NAME', cc.has_header_symbol('errno.h', 'program_invocation_name'))
conf_data.set('HAVE_DECL_PROGRAM_INVOCATION_SHORT_NAME', cc.has_header_symbol('errno.h', 'program_invocation_short_name'))

configure_file(input: 'meson_config.h.in', output: 'config.h', configuration: conf_data)
add_project_arguments('-DHAVE_CONFIG_H=1', language: 'c')

inc = include_directories('include', 'src/argp')
fts_test_inc = include_directories('tests/fts')

c_flags = [
  '-D_GNU_SOURCE'
]

fts_map = meson.current_source_dir() / 'src/fts/libfts.map'
obstack_map = meson.current_source_dir() / 'src/obstack/libobstack.map'
argp_map = meson.current_source_dir() / 'src/argp/libargp.map'

vs_supported = cc.has_link_argument('-Wl,--version-script=' + fts_map)

fts_link_args = []
fts_link_deps = []
if vs_supported
  fts_link_args = ['-Wl,--version-script=' + fts_map]
  fts_link_deps = [fts_map]
endif

obstack_link_args = []
obstack_link_deps = []
if vs_supported
  obstack_link_args = ['-Wl,--version-script=' + obstack_map]
  obstack_link_deps = [obstack_map]
endif

argp_link_args = []
argp_link_deps = []
if vs_supported
  argp_link_args = ['-Wl,--version-script=' + argp_map]
  argp_link_deps = [argp_map]
endif

libobstack = shared_library(
  'obstack',
  ['src/obstack.c'],
  include_directories: inc,
  install: true,
  version: '2.0.0',
  c_args: c_flags,
  link_args: obstack_link_args,
  link_depends: obstack_link_deps
)

install_headers('include/obstack.h', subdir: '')

libfts = shared_library(
  'fts',
  ['src/fts.c'],
  include_directories: inc,
  install: true,
  version: '2.0.0',
  c_args: c_flags,
  link_args: fts_link_args,
  link_depends: fts_link_deps
)

install_headers('include/fts.h', subdir: '')

argp_source = files([
    'src/argp/argp-ba.c',
    'src/argp/argp-eexst.c',
    'src/argp/argp-fmtstream.c',
    'src/argp/argp-help.c',
    'src/argp/argp-parse.c',
    'src/argp/argp-pv.c',
    'src/argp/argp-pvh.c'
])

libargp = shared_library(
  'argp',
  argp_source,
  include_directories: inc,
  install: true,
  version: '2.0.0',
  c_args: c_flags,
  link_args: argp_link_args,
  link_depends: argp_link_deps
)

install_headers(
  'src/argp/argp.h'
)

install_headers(
  [
    'include/cdefs.h',
    'include/queue.h',
    'include/tree.h'
  ],
  subdir: 'sys'
)

pkg = import('pkgconfig')
pkg.generate(libargp,
  version: meson.project_version(),
  name: 'libargp',
  filebase: 'libargp',
  description: 'Hierarchical argument parsing library broken out from glibc'
)

fts_test_common = static_library(
  'fts_test_common',
  ['tests/fts/fts_test_common.c'],
  include_directories: [inc, fts_test_inc],
  c_args: c_flags,
  install: false
)

fts_tests = [
  ['fts_cwd_restore', 'test_cwd_restore.c'],
  ['fts_file_root', 'test_file_root.c'],
  ['fts_seedot', 'test_seedot.c'],
  ['fts_fd_discipline', 'test_fd_discipline.c'],
  ['fts_symlink_loop_follow', 'test_symlink_loop_follow.c'],
  ['fts_traversal_order', 'test_traversal_order.c'],
  ['fts_cycle_detection', 'test_cycle_detection.c'],
  ['fts_many_children_sorted', 'test_many_children_sorted.c'],
  ['fts_walk_physical_chdir', 'test_walk_physical_chdir.c'],
  ['fts_walk_physical_nochdir', 'test_walk_physical_nochdir.c'],
  ['fts_walk_logical', 'test_walk_logical.c'],
  ['fts_walk_logical_comfollow', 'test_walk_logical_comfollow.c'],
  ['fts_two_roots', 'test_two_roots.c'],
  ['fts_walk_nostat_seedot', 'test_walk_nostat_seedot.c'],
  ['fts_children_api', 'test_children_api.c'],
  ['fts_children_null', 'test_children_null.c'],
  ['fts_fault_injection', 'test_fault_injection.c'],
  ['fts_unreadable_dir', 'test_unreadable_dir.c'],
  ['fts_whiteout', 'test_whiteout.c'],
  ['fts_xdev', 'test_xdev.c']
]

foreach t : fts_tests
  exe = executable(
    t[0],
    ['tests/fts/' + t[1]],
    include_directories: [inc, fts_test_inc],
    link_with: [libfts, fts_test_common],
    c_args: c_flags,
    install: false
  )
  test(t[0], exe)
endforeach

test_obstack_exe = executable(
  'test_obstack',
  ['tests/test_obstack.c'],
  include_directories: inc,
  link_with: libobstack,
  c_args: c_flags,
  install: false
)

ex1 = executable('ex1', 'src/argp/testsuite/ex1.c', link_with: [libargp])
ex3 = executable('ex3', 'src/argp/testsuite/ex3.c', link_with: [libargp])
ex4 = executable('ex4', 'src/argp/testsuite/ex4.c', link_with: [libargp])

argp_tests = [
  ['test_argp_ambiguous', 'tests/argp/test_ambiguous.c'],
  ['test_argp_basic_options', 'tests/argp/test_basic_options.c'],
  ['test_argp_child_parsers', 'tests/argp/test_child_parsers.c'],
  ['test_argp_help_fmt_dup_args', 'tests/argp/test_help_fmt_dup_args.c']
]

foreach t : argp_tests
  exe = executable(
    t[0],
    [t[1]],
    include_directories: inc,
    link_with: libargp,
    c_args: c_flags,
    install: false
  )
  test(t[0], exe)
endforeach

test('ex1', ex1)
test('ex3', ex3, args: ['arg1', 'arg2'])
test('ex4', ex4, args: ['-v', '--output=FILE', 'arg1_', 'STRING1', 'STRING2', 'STRING3'])
test('test_obstack', test_obstack_exe)

abi_inc = include_directories('.')

if host_machine.cpu_family() == 'x86_64'
  layout_cc = cc.cmd_array()[0]
  clang_prog = find_program('clang', required: false)
  if clang_prog.found()
    layout_cc = clang_prog.full_path()
  endif

  abi_layout_static = executable(
    'abi_layout_static',
    'tests/abi/test_layout_static.c',
    include_directories: [inc, abi_inc],
    c_args: c_flags,
    install: false
  )
  test('abi_layout_static', abi_layout_static)

  layout_script = find_program('tests/abi/check_record_layouts.sh')
  test(
    'abi_record_layouts',
    layout_script,
    env: {
      'MESON_SOURCE_ROOT': meson.project_source_root(),
      'MESON_BUILD_ROOT': meson.current_build_dir(),
      'CC': layout_cc
    }
  )
endif
