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

# Binary-runtime support is deliberately narrower than source-overlay support.
# x86_64 LP64 is the only ABI qualified by the current layout and loader tests.
cpu_family = host_machine.cpu_family()
is_lp64 = cc.sizeof('void *') == 8 and cc.sizeof('long') == 8
glibc_runtime_qualified = cpu_family == 'x86_64' and is_lp64
glibc_runtime_option = get_option('glibc_runtime')

if glibc_runtime_option == 'enabled' and not glibc_runtime_qualified
  error('glibc binary runtime was explicitly enabled for unverified ABI ' + cpu_family)
endif

glibc_runtime_enabled = glibc_runtime_option == 'enabled' or (
  glibc_runtime_option == 'auto' and glibc_runtime_qualified
)

unwind_backtrace_supported = cc.links(
  '''
    #include <unwind.h>
    static _Unwind_Reason_Code trace(struct _Unwind_Context *context, void *argument) {
      (void)argument;
      return _Unwind_GetIP(context) == 0 ? _URC_END_OF_STACK : _URC_NO_REASON;
    }
    int main(void) {
      return _Unwind_Backtrace(trace, 0) == _URC_FATAL_PHASE1_ERROR;
    }
  ''',
  name: 'compiler unwinder for backtrace',
)
if glibc_runtime_enabled and not unwind_backtrace_supported
  error('Qualified glibc runtime requires the compiler unwinder for backtrace()')
endif
if unwind_backtrace_supported
  c_flags += '-DMUSL_BSD_HAVE_UNWIND=1'
endif

# This distro is usrmerge-only.  The interpreter alias is in /usr/lib, while
# all project payload and glibc facade DSOs live under a private project tree.
compat_runtime_install_dir = '/usr/lib/musl-bsd'
compat_glibc_install_dir = compat_runtime_install_dir + '/glibc'
compat_loader_payload_install_dir = compat_runtime_install_dir + '/loader'
glibc_loader_install_dir = '/usr/lib'
glibc_loader_name = get_option('glibc_loader_name')
musl_linker_path = get_option('musl_linker_path')
default_glibc_loader_name = ''
default_musl_linker_path = ''

if cpu_family == 'x86_64'
  default_glibc_loader_name = 'ld-linux-x86-64.so.2'
  default_musl_linker_path = '/lib/ld-musl-x86_64.so.1'
endif

if glibc_runtime_enabled and glibc_loader_name == ''
  glibc_loader_name = default_glibc_loader_name
endif

if glibc_runtime_enabled and musl_linker_path == ''
  musl_linker_path = default_musl_linker_path
endif

if glibc_runtime_enabled and (
  glibc_loader_name == '' or musl_linker_path == ''
)
  error('Qualified glibc runtime requires a loader name and musl linker path')
endif

compat_runtime_preload_path = compat_runtime_install_dir + '/libmusl-bsd-core.so.2'
compat_runtime_library_path = compat_glibc_install_dir + ':/usr/lib'
compat_runtime_c_args = c_flags
if glibc_runtime_enabled
  compat_runtime_c_args += [
    '-DMUSL_BSD_GLIBC_LOADER_NAME="' + glibc_loader_name + '"',
    '-DMUSL_BSD_MUSL_LINKER_PATH="' + musl_linker_path + '"',
    '-DMUSL_BSD_PRELOAD_PATH="' + compat_runtime_preload_path + '"',
    '-DMUSL_BSD_LIBRARY_PATH="' + compat_runtime_library_path + '"',
  ]
endif

runtime_metadata = configuration_data()
runtime_metadata.set('ARCHITECTURE', cpu_family)
runtime_metadata.set('ABI', glibc_runtime_qualified ? 'x86_64-lp64' : 'unqualified')
runtime_metadata.set('GLIBC_RUNTIME', glibc_runtime_enabled ? 'true' : 'false')
runtime_metadata.set('GLIBC_LOADER', glibc_loader_name)
runtime_metadata.set('MUSL_LINKER', musl_linker_path)
runtime_metadata_file = configure_file(
  input: 'compat-runtime.json.in',
  output: 'compat-runtime.json',
  configuration: runtime_metadata,
)
install_data(runtime_metadata_file, install_dir: compat_runtime_install_dir)

fts_map = meson.current_source_dir() / 'src/fts/libfts.map'
obstack_map = meson.current_source_dir() / 'src/obstack/libobstack.map'
argp_map = meson.current_source_dir() / 'src/argp/libargp.map'

# has_link_argument() links an empty probe against the supplied script.  The
# fts script names symbols that are not present in that probe, so it reports a
# false negative even though the linker supports version scripts.  The
# compatibility and ABI libraries rely on these scripts for versioned glibc
# entry points.
vs_supported = true

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
if vs_supported and glibc_runtime_enabled
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
  'src/compat/core.c',
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

compat_interpose_sources = []
if glibc_runtime_enabled
  compat_interpose_sources += [
    'src/compat/exec.c',
    'src/compat/preload_policy.c',
    'src/compat/readlink.c',
  ]
endif

pthread_abi_supported = glibc_runtime_enabled
if pthread_abi_supported
  compat_interpose_sources += 'src/compat/pthread.c'
elif glibc_runtime_enabled
  warning('glibc pthread ABI bridge is disabled: no verified fixture for ' + cpu_family)
endif

libcompat = shared_library(
  'musl-bsd-core',
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
  'musl-bsd-core',
  # Keep libc interposition shims out of the static archive: they depend on
  # RTLD_NEXT and collide with libc.a during static links.
  compat_common_sources,
  include_directories: [overlay_inc],
  install: true,
  install_dir: compat_runtime_install_dir,
  c_args: c_flags
)

glibc_facades = []
if glibc_runtime_enabled
  facade_map = meson.current_source_dir() / 'src/compat/libmusl-bsd-facade.map'
  probe_facade_map = meson.current_source_dir() / 'src/compat/libmusl-bsd-probe-facade.map'
  foreach facade : [
    ['c', '6'],
    ['dl', '2'],
    ['m', '6'],
    ['pthread', '0'],
    ['resolv', '2'],
    ['rt', '1'],
    ['util', '1'],
  ]
    facade_c_args = c_flags
    facade_version_map = facade_map
    if facade[0] == 'util'
      facade_c_args += '-DMUSL_BSD_PROBE_FACADE=1'
      facade_version_map = probe_facade_map
    endif
    facade_dso = shared_library(
      facade[0],
      'src/compat/facade.c',
      soversion: facade[1],
      install: true,
      install_dir: compat_glibc_install_dir,
      link_with: libcompat,
      c_args: facade_c_args,
      link_args: ['-Wl,--version-script=' + facade_version_map],
      link_depends: facade_version_map,
    )
    glibc_facades += facade_dso
  endforeach

  if not cc.has_link_argument('-static-pie')
    error('glibc runtime requires a working -static-pie linker mode')
  endif

  glibc_loader = executable(
    glibc_loader_name,
    [
      'src/compat/loader.c',
      'src/compat/loader_policy.c',
      'src/compat/preload_policy.c',
    ],
    include_directories: include_directories('src/compat'),
    install: true,
    install_dir: compat_loader_payload_install_dir,
    c_args: compat_runtime_c_args,
    link_args: ['-static-pie']
  )

  install_symlink(
    glibc_loader_name,
    install_dir: glibc_loader_install_dir,
    pointing_to: 'musl-bsd/loader/' + glibc_loader_name
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
