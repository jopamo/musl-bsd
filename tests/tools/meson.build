tools_python = find_program('python3')
elf_scan = meson.project_source_root() / 'tools/elf-scan'
compatibility_manifest = meson.project_source_root() / 'tools/compatibility-manifest'
committed_compatibility_manifest = meson.project_source_root() / 'compatibility-symbols.json'
provider_map = meson.current_source_dir() / 'provider_required.map'
required_provider = shared_library(
  'tools-required-provider',
  'provider_required.c',
  install: false,
  link_args: ['-Wl,--version-script=' + provider_map],
  link_depends: provider_map,
)
provider_consumer = shared_library(
  'tools-provider-consumer',
  'provider_consumer.c',
  install: false,
  link_with: required_provider,
)
runtime_provider = shared_library(
  'tools-runtime-provider',
  'provider_runtime.c',
  install: false,
)

test(
  'tools/elf-scan',
  tools_python,
  args: [
    files('test_elf_scan.py'),
    elf_scan,
    provider_consumer.full_path(),
    runtime_provider.full_path(),
  ],
  depends: [required_provider, provider_consumer, runtime_provider],
  suite: 'tools',
  timeout: 30,
)

test(
  'tools/compatibility-manifest',
  tools_python,
  args: [
    files('test_compatibility_manifest.py'),
    compatibility_manifest,
    elf_scan,
    provider_consumer.full_path(),
    runtime_provider.full_path(),
    committed_compatibility_manifest,
  ],
  depends: [required_provider, provider_consumer, runtime_provider],
  suite: 'tools',
  timeout: 30,
)
