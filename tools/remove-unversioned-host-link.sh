#!/bin/sh
set -eu

install_path=$1
LC_ALL=C
export LC_ALL
case ${install_path} in
  ''|*[!a-zA-Z0-9_./+-]*)
    echo "invalid host linker-name path: ${install_path}" >&2
    exit 1
    ;;
esac
case /${install_path}/ in
  */../*)
    echo "invalid host linker-name path: ${install_path}" >&2
    exit 1
    ;;
esac
if [ "${install_path##*/}" != 'libmusl-bsd-glibc-host.so' ]; then
  echo "invalid host linker-name basename: ${install_path}" >&2
  exit 1
fi

case ${install_path} in
  /*) installed_path="${DESTDIR:-}${install_path}" ;;
  *) installed_path="${MESON_INSTALL_DESTDIR_PREFIX}/${install_path}" ;;
esac
if [ -L "${installed_path}" ]; then
  rm "${installed_path}"
elif [ -e "${installed_path}" ]; then
  echo "refusing to remove non-symlink host linker name: ${installed_path}" >&2
  exit 1
fi
