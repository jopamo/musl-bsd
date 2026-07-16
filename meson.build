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
overlay_inc = include_directories('overlay/include')

c_flags = [
  '-D_GNU_SOURCE'
]

# This distro is usrmerge-only: /lib* are path aliases owned by layout policy,
# while the real installed runtime payload lives in /usr/lib.
compat_runtime_install_dir = '/usr/lib'
glibc_loader_install_dir = '/usr/lib'
glibc_loader_name = get_option('glibc_loader_name')
musl_linker_path = get_option('musl_linker_path')
glibc_loader_symlink_dir = ''
default_glibc_loader_name = ''
default_musl_linker_path = ''
cpu_family = host_machine.cpu_family()

if cpu_family == 'x86_64'
  default_glibc_loader_name = 'ld-linux-x86-64.so.2'
  default_musl_linker_path = '/lib/ld-musl-x86_64.so.1'
elif cpu_family == 'x86'
  default_glibc_loader_name = 'ld-linux.so.2'
  default_musl_linker_path = '/lib/ld-musl-i386.so.1'
elif cpu_family == 'aarch64'
  default_glibc_loader_name = 'ld-linux-aarch64.so.1'
  default_musl_linker_path = '/lib/ld-musl-aarch64.so.1'
elif cpu_family == 'arm'
  default_glibc_loader_name = 'ld-linux-armhf.so.3'
  default_musl_linker_path = '/lib/ld-musl-armhf.so.1'
elif cpu_family == 'riscv64'
  default_glibc_loader_name = 'ld-linux-riscv64-lp64d.so.1'
  default_musl_linker_path = '/lib/ld-musl-riscv64.so.1'
elif cpu_family == 'loongarch64'
  default_glibc_loader_name = 'ld-linux-loongarch-lp64d.so.1'
  default_musl_linker_path = '/lib/ld-musl-loongarch64.so.1'
elif cpu_family == 's390x'
  default_glibc_loader_name = 'ld64.so.1'
  default_musl_linker_path = '/lib/ld-musl-s390x.so.1'
elif cpu_family == 'ppc64'
  default_glibc_loader_name = 'ld64.so.2'
  default_musl_linker_path = '/lib/ld-musl-powerpc64le.so.1'
endif

if glibc_loader_name == ''
  glibc_loader_name = default_glibc_loader_name
endif

if musl_linker_path == ''
  musl_linker_path = default_musl_linker_path
endif

if glibc_loader_name == '' or musl_linker_path == ''
  error('Unsupported architecture for automatic glibc-loader setup; set -Dglibc_loader_name=... and -Dmusl_linker_path=...')
endif

compat_runtime_preload_path = compat_runtime_install_dir + '/libmusl-bsd-compat.so.2'
compat_runtime_c_args = c_flags + [
  '-DMUSL_BSD_GLIBC_LOADER_NAME="' + glibc_loader_name + '"',
  '-DMUSL_BSD_MUSL_LINKER_PATH="' + musl_linker_path + '"',
  '-DMUSL_BSD_PRELOAD_PATH="' + compat_runtime_preload_path + '"'
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

compat_map = meson.current_source_dir() / 'src/compat/libmusl-bsd-compat.map'
compat_link_args = []
compat_link_deps = []
if vs_supported
  compat_link_args = ['-Wl,--version-script=' + compat_map]
  compat_link_deps = [compat_map]
endif

obstack_sources = ['src/obstack.c']

libobstack = shared_library(
  'obstack',
  obstack_sources,
  include_directories: inc,
  install: true,
  version: '2.0.0',
  c_args: c_flags,
  link_args: obstack_link_args,
  link_depends: obstack_link_deps
)

libobstack_static = static_library(
  'obstack',
  obstack_sources,
  include_directories: inc,
  install: true,
  c_args: c_flags
)

libobstack_test = static_library(
  'obstack_test',
  obstack_sources,
  include_directories: inc,
  c_args: c_flags,
  install: false
)

install_headers('include/obstack.h', subdir: '')

fts_sources = ['src/fts.c']

libfts = shared_library(
  'fts',
  fts_sources,
  include_directories: inc,
  install: true,
  version: '2.0.0',
  c_args: c_flags,
  link_args: fts_link_args,
  link_depends: fts_link_deps
)

libfts_static = static_library(
  'fts',
  fts_sources,
  include_directories: inc,
  install: true,
  c_args: c_flags
)

libfts_test = static_library(
  'fts_test',
  fts_sources,
  include_directories: inc,
  c_args: c_flags,
  install: false
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

libargp_static = static_library(
  'argp',
  argp_source,
  include_directories: inc,
  install: true,
  c_args: c_flags
)

compat_common_sources = [
  'src/compat/dlfcn.c',
  'src/compat/error.c',
  'src/compat/fs64.c',
  'src/compat/glibc.c',
  'src/compat/math.c',
  'src/compat/mount.c',
  'src/compat/string.c',
  'src/compat/time.c',
  'src/compat/version.c',
]

compat_interpose_sources = [
  'src/compat/exec.c',
  'src/compat/readlink.c',
]

libcompat = shared_library(
  'musl-bsd-compat',
  compat_common_sources + compat_interpose_sources,
  include_directories: [overlay_inc],
  install: true,
  install_dir: compat_runtime_install_dir,
  version: '2.0.0',
  c_args: compat_runtime_c_args,
  link_args: compat_link_args,
  link_depends: compat_link_deps
)

libcompat_static = static_library(
  'musl-bsd-compat',
  # Keep libc interposition shims out of the static archive: they depend on
  # RTLD_NEXT and collide with libc.a during static links.
  compat_common_sources,
  include_directories: [overlay_inc],
  install: true,
  install_dir: compat_runtime_install_dir,
  c_args: compat_runtime_c_args
)

glibc_loader = executable(
  glibc_loader_name,
  ['src/compat/loader.c'],
  install: true,
  install_dir: glibc_loader_install_dir,
  c_args: compat_runtime_c_args,
  link_args: ['-static-pie']
)

foreach alias_name : [
  'libc.so.6',
  'libdl.so.2',
  'libm.so.6',
  'libpthread.so.0',
  'libresolv.so.2',
  'librt.so.1',
  'libutil.so.1',
]
  install_symlink(
    alias_name,
    install_dir: compat_runtime_install_dir,
    pointing_to: 'libmusl-bsd-compat.so.2'
  )
endforeach

if glibc_loader_symlink_dir != '' and glibc_loader_symlink_dir != glibc_loader_install_dir
  install_symlink(
    glibc_loader_name,
    install_dir: glibc_loader_symlink_dir,
    pointing_to: '../lib/' + glibc_loader_name
  )
endif

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

install_subdir(
  'overlay/include',
  install_dir: get_option('libdir') / 'musl-bsd/overlay'
)

pkg = import('pkgconfig')
pkg.generate(libargp,
  version: meson.project_version(),
  name: 'libargp',
  filebase: 'libargp',
  description: 'Hierarchical argument parsing library broken out from glibc'
)

subdir('tests')
