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

libobstack = shared_library(
  'obstack',
  ['src/obstack.c'],
  include_directories: inc,
  install: true,
  version: '2.0.0',
  c_args: c_flags
)

install_headers('include/obstack.h', subdir: '')

libfts = shared_library(
  'fts',
  ['src/fts.c'],
  include_directories: inc,
  install: true,
  version: '2.0.0',
  c_args: c_flags
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
  c_args: c_flags
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
  ['fts_unreadable_dir', 'test_unreadable_dir.c'],
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
test_argp_ambiguous_exe = executable(
  'test_argp_ambiguous',
  ['tests/test_argp_ambiguous.c'],
  include_directories: inc,
  link_with: libargp,
  c_args: c_flags,
  install: false
)

test('ex1', ex1)
test('ex3', ex3, args: ['arg1', 'arg2'])
test('ex4', ex4, args: ['-v', '--output=FILE', 'arg1_', 'STRING1', 'STRING2', 'STRING3'])
test('test_argp_ambiguous', test_argp_ambiguous_exe)
test('test_obstack', test_obstack_exe)
