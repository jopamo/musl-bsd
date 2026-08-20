tools_python = find_program('python3')
nvidia_scan = meson.project_source_root() / 'tools/nvidia-scan'
nvidia_manifest = meson.project_source_root() / 'tools/nvidia-manifest'
committed_nvidia_manifest = meson.project_source_root() / 'nvidia-symbols.json'
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
  'tools/nvidia-scan',
  tools_python,
  args: [
    files('test_nvidia_scan.py'),
    nvidia_scan,
    provider_consumer.full_path(),
    runtime_provider.full_path(),
  ],
  depends: [required_provider, provider_consumer, runtime_provider],
  suite: 'tools',
  timeout: 30,
)

test(
  'tools/nvidia-manifest',
  tools_python,
  args: [
    files('test_nvidia_manifest.py'),
    nvidia_manifest,
    nvidia_scan,
    provider_consumer.full_path(),
    runtime_provider.full_path(),
    committed_nvidia_manifest,
  ],
  depends: [required_provider, provider_consumer, runtime_provider],
  suite: 'tools',
  timeout: 30,
)
