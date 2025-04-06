project(
  'musl-bsd',
  'c',
  version : '2.0.0'
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

# --- libobstack ---
libobstack = shared_library(
  'obstack',
  [
    'src/obstack.c',
    'src/obstack_printf.c'
  ],
  include_directories : inc,
  install : true,
  version : '2.0.0'
)

install_headers(
  'include/obstack.h',
  subdir : ''
)

# --- libfts ---
libfts = shared_library(
  'fts',
  ['src/fts.c'],
  include_directories : inc,
  install : true,
  version : '2.0.0'
)

install_headers(
  'include/fts.h',
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
