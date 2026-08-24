python = find_program('python3')
pkg_config = find_program('pkg-config')
readelf = find_program('readelf')

interface_checker_args = [
  files('check_interfaces.py'),
  pkg_config.full_path(),
  cc.cmd_array()[0],
  readelf.full_path(),
  meson.project_source_root(),
  meson.project_build_root(),
  get_option('prefix'),
  get_option('libdir'),
  glibc_runtime_enabled ? 'enabled' : 'disabled',
]

test(
  'interfaces/contracts',
  python,
  args: interface_checker_args,
  depends: glibc_runtime_enabled ? [libcompat_static, libcompat] : libcompat_static,
  suite: 'interfaces',
  timeout: 60,
)
