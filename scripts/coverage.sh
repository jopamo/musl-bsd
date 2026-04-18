#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-coverage}"

if [[ ! -f "${BUILD_DIR}/build.ninja" ]]; then
  meson setup "${BUILD_DIR}" -Db_coverage=true -Db_lundef=false
fi

meson test -C "${BUILD_DIR}" --print-errorlogs

gcovr -r . "${BUILD_DIR}" --txt "${BUILD_DIR}/meson-logs/coverage.txt"
gcovr -r . "${BUILD_DIR}" --html-details "${BUILD_DIR}/coverage/index.html"
gcovr -r . "${BUILD_DIR}" --xml "${BUILD_DIR}/coverage/coverage.xml" --xml-pretty
gcovr -r . "${BUILD_DIR}" --json-summary "${BUILD_DIR}/coverage/summary.json" --json-summary-pretty
gcovr -r . "${BUILD_DIR}" --filter '^src/' --json-summary "${BUILD_DIR}/coverage/src-summary.json" --json-summary-pretty

echo "coverage text : ${BUILD_DIR}/meson-logs/coverage.txt"
echo "coverage html : ${BUILD_DIR}/coverage/index.html"
echo "coverage xml  : ${BUILD_DIR}/coverage/coverage.xml"
echo "summary json  : ${BUILD_DIR}/coverage/summary.json"
echo "src summary   : ${BUILD_DIR}/coverage/src-summary.json"
