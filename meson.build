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

cc = meson.get_compiler('c')

# These values cross C, JSON, pkg-config and loader separator grammars.
install_path_characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/_-.+'
foreach path_option : [
  ['prefix', get_option('prefix')],
  ['libdir', get_option('libdir')],
  ['glibc_loader_name', get_option('glibc_loader_name')],
  ['musl_linker_path', get_option('musl_linker_path')],
]
  if path_option[1].strip(install_path_characters) != ''
    error(path_option[0] + ': unsupported path characters; use ASCII letters, digits and /_.+-')
  endif
  if path_option[1] == '' and path_option[0] in ['prefix', 'libdir']
    error(path_option[0] + ' must not be empty')
  endif
  foreach component : path_option[1].split('/')
    if component == '..'
      error(path_option[0] + ': parent path components are not supported')
    endif
  endforeach
endforeach
loader_name_option = get_option('glibc_loader_name')
if loader_name_option.contains('/') or loader_name_option in ['.', '..']
  error('glibc_loader_name must be a filename, not a path or dot component')
endif
linker_path_option = get_option('musl_linker_path')
if linker_path_option != '' and (
  not linker_path_option.startswith('/') or linker_path_option.endswith('/')
)
  error('musl_linker_path must be an absolute target path without a trailing slash')
endif

conf_data = configuration_data()

conf_data.set10('HAVE_CONFIG_H', true)
conf_data.set10('HAVE_ALLOCA_H', cc.check_header('alloca.h'))
conf_data.set10('HAVE_UNISTD_H', cc.check_header('unistd.h'))
conf_data.set10('HAVE_EX_USAGE', cc.has_header_symbol('sysexits.h', 'EX_USAGE'))

# Match the feature namespace used by the library translation units.
stdio_prefix = '#define _GNU_SOURCE 1\n#include <stdio.h>'
foreach function : [
  'clearerr_unlocked', 'feof_unlocked', 'ferror_unlocked', 'fflush_unlocked',
  'fgets_unlocked', 'fputc_unlocked', 'fputs_unlocked', 'fread_unlocked',
  'fwrite_unlocked', 'getc_unlocked', 'getchar_unlocked', 'putc_unlocked',
  'putchar_unlocked',
]
  conf_data.set10('HAVE_DECL_' + function.to_upper(), cc.has_function(function, prefix: stdio_prefix))
endforeach
conf_data.set10('HAVE_FLOCKFILE', cc.has_function('flockfile', prefix: stdio_prefix))
conf_data.set10('HAVE_FUNLOCKFILE', cc.has_function('funlockfile', prefix: stdio_prefix))
conf_data.set10('HAVE_ASPRINTF', cc.has_function('asprintf', prefix: stdio_prefix))

string_prefix = '#define _GNU_SOURCE 1\n#include <string.h>'
foreach function : ['mempcpy', 'strchrnul', 'strndup']
  conf_data.set10('HAVE_' + function.to_upper(), cc.has_function(function, prefix: string_prefix))
endforeach
errno_prefix = '#define _GNU_SOURCE 1\n#include <errno.h>'
foreach symbol : ['program_invocation_name', 'program_invocation_short_name']
  conf_data.set10('HAVE_DECL_' + symbol.to_upper(), cc.has_header_symbol('errno.h', symbol, prefix: errno_prefix))
endforeach

configure_file(input: 'meson_config.h.in', output: 'config.h', configuration: conf_data)
add_project_arguments('-DHAVE_CONFIG_H=1', language: 'c')

inc = include_directories('include', 'src/argp')
overlay_inc = include_directories('overlay/include')
compat_inc = include_directories('src/compat')
musl_bsd_source_root = meson.current_source_dir()
musl_bsd_build_root = meson.current_build_dir()

c_flags = [
  '-D_GNU_SOURCE'
]

# Binary-runtime support is deliberately narrower than source-overlay support.
# x86_64 LP64 is the only ABI qualified by the current layout and loader tests.
cpu_family = host_machine.cpu_family()
is_lp64 = cc.sizeof('void *') == 8 and cc.sizeof('long') == 8
glibc_runtime_abi_qualified = (
  host_machine.system() == 'linux' and cpu_family == 'x86_64' and is_lp64
)
glibc_runtime_option = get_option('glibc_runtime')
musl_libc_verified = false

if (
  glibc_runtime_option != 'disabled' and glibc_runtime_abi_qualified and
  meson.can_run_host_binaries()
)
  libc_identity_dl = cc.find_library('dl', required: false)
  libc_identity = cc.run(
    '''
      #define _GNU_SOURCE 1
      #include <dlfcn.h>
      #include <unistd.h>
      int musl_bsd_libc_identity(void) {
        Dl_info information;
        void *symbol = dlsym(RTLD_DEFAULT, "fopen");
        if (!symbol || !dladdr(symbol, &information) || !information.dli_fname)
          return 2;
        execl(information.dli_fname, information.dli_fname, (char *)0);
        return 3;
      }
      int main(void) { return musl_bsd_libc_identity(); }
    ''',
    dependencies: libc_identity_dl,
    name: 'libc provider banner (musl exits 1)',
  )
  musl_libc_verified = (
    libc_identity.compiled() and libc_identity.returncode() == 1 and
    libc_identity.stderr().startswith('musl libc (x86_64)\n')
  )
endif
if glibc_runtime_option != 'disabled'
  message('Musl target libc verified: ' + musl_libc_verified.to_string('yes', 'no'))
endif
glibc_runtime_qualified = glibc_runtime_abi_qualified and musl_libc_verified

if glibc_runtime_option == 'enabled' and not glibc_runtime_abi_qualified
  error('glibc binary runtime requires Linux x86_64 LP64; got ' + host_machine.system() + '/' + cpu_family)
endif
if glibc_runtime_option == 'enabled' and not musl_libc_verified
  error('glibc binary runtime requires a runnable, verified musl libc target')
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

# This distro is usrmerge-only.  Keep source-compatibility payload private,
# but install the qualified host DSO in the normal ABI library directory so
# musl can resolve its SONAME without an executable-specific RUNPATH.
abi_library_install_dir = get_option('prefix') / get_option('libdir')
compat_runtime_install_dir = abi_library_install_dir / 'musl-bsd'
compat_glibc_install_dir = compat_runtime_install_dir + '/glibc'
compat_loader_payload_install_dir = compat_runtime_install_dir + '/loader'
glibc_loader_install_dir = abi_library_install_dir
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

compat_runtime_preload_path = abi_library_install_dir + '/libmusl-bsd-glibc-host.so.2'
compat_runtime_library_path = compat_glibc_install_dir + ':' + abi_library_install_dir
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
runtime_metadata.set('ABI', glibc_runtime_abi_qualified ? 'x86_64-lp64' : 'unqualified')
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

version_script_probe = meson.current_source_dir() / 'meson/version-script-probe.map'
if not cc.links(
  '''
    int musl_bsd_version_script_probe(void) { return 0; }
    int main(void) { return musl_bsd_version_script_probe(); }
  ''',
  args: ['-Wl,--version-script=' + version_script_probe],
  name: 'ELF symbol version scripts',
)
  error('musl-bsd shared libraries require ELF symbol version-script support')
endif

fts_link_args = ['-Wl,--version-script=' + fts_map]
fts_link_deps = [fts_map]
obstack_link_args = ['-Wl,--version-script=' + obstack_map]
obstack_link_deps = [obstack_map]
argp_link_args = ['-Wl,--version-script=' + argp_map]
argp_link_deps = [argp_map]

compat_map = meson.current_source_dir() / 'src/compat/libmusl-bsd-compat.map'
compat_link_args = []
compat_link_deps = []
if glibc_runtime_enabled
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

install_headers('include/obstack.h', subdir: '')

fts_sources = files('src/fts.c')

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

compat_glibc_source = files('src/compat/glibc.c')
compat_mount_source = files('src/compat/mount.c')
source_pidfd_source = files('src/source/pidfd.c')
compat_common_sources = [
  'src/compat/core.c',
  'src/compat/dlfcn.c',
  'src/compat/error.c',
  'src/compat/fs64.c',
  compat_glibc_source,
  'src/compat/math.c',
  compat_mount_source,
  'src/compat/string.c',
  'src/compat/time.c',
  'src/compat/version.c',
]

compat_source_sources = [
  'src/source/mb-reader.c',
  source_pidfd_source,
  'src/source/readpassphrase.c',
]

loader_policy_source = files('src/compat/loader_policy.c')
preload_policy_source = files('src/compat/preload_policy.c')
compat_exec_source = files('src/compat/exec.c')
compat_readlink_source = files('src/compat/readlink.c')
compat_pthread_source = files('src/compat/pthread.c')
compat_interpose_sources = []
if glibc_runtime_enabled
  compat_interpose_sources += [
    compat_exec_source,
    preload_policy_source,
    compat_readlink_source,
  ]
endif

pthread_abi_supported = glibc_runtime_enabled
if pthread_abi_supported
  compat_interpose_sources += compat_pthread_source
elif glibc_runtime_enabled
  warning('glibc pthread ABI bridge is disabled: no verified fixture for ' + cpu_family)
endif

libcompat_static = static_library(
  'musl-bsd-core',
  # Keep libc interposition shims out of the static archive: they depend on
  # RTLD_NEXT and collide with libc.a during static links.
  compat_common_sources + compat_source_sources,
  include_directories: [overlay_inc],
  install: true,
  install_dir: compat_runtime_install_dir,
  c_args: c_flags + ['-fvisibility=hidden'],
  pic: true
)

glibc_facades = []
if glibc_runtime_enabled
  # This DSO is a qualified foreign-DSO host, not the dynamic counterpart of
  # libmusl-bsd-core.a.  Its distinct basename prevents ordinary
  # -lmusl-bsd-core links from silently selecting runtime-hosting semantics.
  libcompat = shared_library(
    'musl-bsd-glibc-host',
    compat_common_sources + compat_interpose_sources,
    include_directories: [overlay_inc],
    install: true,
    install_dir: abi_library_install_dir,
    version: '2.0.0',
    c_args: compat_runtime_c_args,
    link_args: compat_link_args,
    link_depends: compat_link_deps
  )

  # The versioned SONAME is selected only through musl-bsd-glibc-host.pc.
  # Do not publish an ordinary unversioned linker name.
  meson.add_install_script(
    'tools/remove-unversioned-host-link.sh',
    get_option('libdir') / 'libmusl-bsd-glibc-host.so',
  )

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
    ] + loader_policy_source + preload_policy_source,
    include_directories: compat_inc,
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

interface_pc_data = configuration_data()
interface_pc_data.set('PREFIX', get_option('prefix'))
interface_pc_data.set(
  'LIBDIR',
  get_option('libdir').startswith('/') ? get_option('libdir') : '${prefix}/' + get_option('libdir'),
)
interface_pc_data.set('VERSION', meson.project_version())
interface_pc_install_dir = get_option('libdir') / 'pkgconfig'

configure_file(
  input: 'musl-bsd-headers.pc.in',
  output: 'musl-bsd-headers.pc',
  configuration: interface_pc_data,
  install: true,
  install_dir: interface_pc_install_dir,
)

configure_file(
  input: 'musl-bsd-source.pc.in',
  output: 'musl-bsd-source.pc',
  configuration: interface_pc_data,
  install: true,
  install_dir: interface_pc_install_dir,
)

if glibc_runtime_enabled
  configure_file(
    input: 'musl-bsd-glibc-host.pc.in',
    output: 'musl-bsd-glibc-host.pc',
    configuration: interface_pc_data,
    install: true,
    install_dir: interface_pc_install_dir,
  )

  configure_file(
    input: 'musl-bsd-glibc-startup.pc.in',
    output: 'musl-bsd-glibc-startup.pc',
    configuration: interface_pc_data,
    install: true,
    install_dir: interface_pc_install_dir,
  )
endif

pkg = import('pkgconfig')
pkg.generate(libargp,
  version: meson.project_version(),
  name: 'libargp',
  filebase: 'libargp',
  description: 'Hierarchical argument parsing library broken out from glibc'
)
