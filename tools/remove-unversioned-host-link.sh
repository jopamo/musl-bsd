#!/bin/sh
set -eu

relative_path=$1
case ${relative_path} in
  /*|*..*)
    echo "invalid host linker-name path: ${relative_path}" >&2
    exit 1
    ;;
esac

installed_path="${MESON_INSTALL_DESTDIR_PREFIX}/${relative_path}"
if [ -L "${installed_path}" ]; then
  rm "${installed_path}"
elif [ -e "${installed_path}" ]; then
  echo "refusing to remove non-symlink host linker name: ${installed_path}" >&2
  exit 1
fi
