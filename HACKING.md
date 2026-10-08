# HACKING

- Tests live in sibling `musl-bsd-tests`, which builds this checkout directly.
- `libmusl-bsd-core.a` is source-only: no `RTLD_NEXT` shims or runtime-DSO dependency.
