tools_python = find_program('python3')
nvidia_scan = meson.project_source_root() / 'tools/nvidia-scan'

test(
  'tools/nvidia-scan',
  tools_python,
  args: [
    files('test_nvidia_scan.py'),
    nvidia_scan,
  ],
  suite: 'tools',
  timeout: 30,
)
