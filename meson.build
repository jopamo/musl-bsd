project(
  'musl-bsd',
  'c',
  version : '2.0.0',
  default_options : [
    'c_std=c11',
    'warning_level=2'
  ]
)

cc = meson.get_compiler('c')

config_data = configuration_data()

foreach hdr : [
  'assert.h',
  'dirent.h',
  'errno.h',
  'fcntl.h',
  'stdlib.h',
  'string.h',
  'unistd.h',
  'sys/param.h',
  'sys/stat.h'
]
  if cc.has_header(hdr)
    define_name = 'HAVE_' + hdr.to_upper().underscorify()
    config_data.set(define_name, 1)
  endif
endforeach

if cc.has_function('dirfd')
  config_data.set('HAVE_DIRFD', 1)
endif

inc = include_directories('include')

# Libraries
libobstack = shared_library(
  'obstack',
  ['src/obstack.c'],
  include_directories : inc,
  install : true,
  version : '2.0.0'
)

install_headers(
  'include/obstack.h',
  subdir : ''
)

libfts = shared_library(
  'fts',
  ['src/fts.c'],
  include_directories : inc,
  install : true,
  version : '2.0.0'
)

install_headers(
  'include/fts.h',
  'include/error.h',
  subdir : ''
)

libargp = shared_library(
  'argp',
  ['src/argp.c'],
  include_directories : inc,
  install : true,
  version : '2.0.0'
)

install_headers(
  'include/argp.h',
  subdir : ''
)

install_headers(
  [
    'include/cdefs.h',
    'include/queue.h',
    'include/tree.h'
  ],
  subdir : 'sys'
)

# Test Executables
test_argp_exe = executable(
  'test_argp',
  ['tests/test_argp.c'],
  include_directories : inc,
  link_with : libargp,
  c_args : [
    '-D_XOPEN_SOURCE=700',
    '-D_DEFAULT_SOURCE'
  ],
  install : false
)

test_fts_exe = executable(
  'test_fts',
  ['tests/test_fts.c'],
  include_directories : inc,
  link_with : libfts,
  c_args : [
    '-D_XOPEN_SOURCE=700',
    '-D_DEFAULT_SOURCE'
  ],
  install : false
)

test_obstack_exe = executable(
  'test_obstack',
  ['tests/test_obstack.c'],
  include_directories : inc,
  link_with : libobstack,
  c_args : [
    '-D_XOPEN_SOURCE=700',
    '-D_DEFAULT_SOURCE'
  ],
  install : false
)

# Tests
test('test_argp', test_argp_exe)
test('test_fts', test_fts_exe)
test('test_obstack', test_obstack_exe)
