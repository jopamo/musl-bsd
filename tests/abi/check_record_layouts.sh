#!/usr/bin/env bash
set -euo pipefail

srcdir=${MESON_SOURCE_ROOT:-$(cd -- "$(dirname "$0")/../.." && pwd)}
builddir=${MESON_BUILD_ROOT:-"$srcdir/build"}
arch=$(uname -m)
baseline="${srcdir}/tests/abi/record_layouts.${arch}.txt"

if [ ! -f "$baseline" ]; then
    echo "record layout baseline missing for ${arch}, skipping"
    exit 0
fi

cc=${CC:-cc}
tmp_obj=$(mktemp)
tmp_dump=$(mktemp)
tmp_filtered=$(mktemp)
trap 'rm -f "$tmp_obj" "$tmp_dump" "$tmp_filtered"' EXIT

if ! "$cc" -std=c99 -Wall -Wextra -Werror -D_GNU_SOURCE \
    -I"${srcdir}/include" -I"${srcdir}/src/argp" -I"$builddir" \
    -c "${srcdir}/tests/abi/record_layouts.c" \
    -o "$tmp_obj" -Xclang -fdump-record-layouts >"$tmp_dump" 2>&1; then
    if grep -qi -- "-fdump-record-layouts" "$tmp_dump"; then
        echo "compiler ${cc} does not support record layout dumps, skipping"
        exit 0
    fi
    cat "$tmp_dump"
    exit 1
fi

python3 - "$tmp_dump" "$tmp_filtered" "$srcdir" <<'PY'
import sys

dump_path, out_path, src_root = sys.argv[1:]
allowed = {
    "struct _fts",
    "struct _ftsent",
    "struct _obstack_chunk",
    "struct obstack",
    "struct argp_option",
    "struct argp_child",
    "struct argp",
    "struct argp_state",
}

lines = open(dump_path, encoding="utf-8").read().splitlines()
out = []
i = 0
while i < len(lines):
    if lines[i].startswith("*** Dumping AST Record Layout"):
        i += 1
        block = []
        while i < len(lines) and lines[i].strip():
            normalized = lines[i].replace(src_root + "/", "").strip()
            block.append(normalized)
            i += 1
        if block and any(prefix in block[0] for prefix in allowed):
            out.extend(block)
            out.append("")
    i += 1

with open(out_path, "w", encoding="utf-8") as f:
    f.write("\n".join(out).rstrip() + "\n")
PY

if ! diff -u "$baseline" "$tmp_filtered"; then
    echo "record layout drift detected for ${arch}"
    exit 1
fi
