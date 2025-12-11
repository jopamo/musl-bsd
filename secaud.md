# Security Audit Report: musl-bsd FTS Implementation

**Date**: 2025-12-10
**Auditor**: Claude Code
**Version**: Current git head (664febc)

## Executive Summary

The musl-bsd FTS (File Tree Stream) implementation demonstrates strong security consciousness with comprehensive protection against common directory traversal vulnerabilities. The code includes robust TOCTOU protection, systematic memory safety checks, and thorough resource management. Primary security considerations center around platform-dependent symlink race protection and the need for caller-level sandboxing when processing untrusted directory trees.

---

## 1. Threat Model Assessment

**Context**: Library implementation of FTS API for directory traversal.

| Aspect | Assessment |
|--------|------------|
| **Untrusted input influence** | Paths come from caller arguments (`argv`), which could be user-controlled |
| **Privilege level** | Library runs with caller's permissions; no privilege elevation |
| **Operations performed** | Read-only traversal by default; callers may delete/overwrite based on results |
| **Symlink handling** | Configurable: `FTS_LOGICAL` (follow) vs `FTS_PHYSICAL` (no follow) |
| **Security criticality** | High for security-sensitive callers (package managers, backup tools, scanners) |

---

## 2. Path Flow Analysis

### Path Sources
- `fts_open()` accepts `argv` array from caller
- No internal path canonicalization using `realpath()`

### Path Construction
- `fts_build()`: concatenates parent path + `/` + entry name (`fts.c:651-682`)
- `fts_read()`: updates path buffer via string manipulation (`fts.c:439-443`)
- **No canonicalization**: Paths like `foo/../bar` remain as-is; traversal follows filesystem semantics

### Buffer Safety
- Dynamic buffer allocation via `fts_palloc()` with overflow checks (`fts.c:944-961`)
- Length capping via `fts_length_cap()` prevents integer overflow (`fts.c:27-30`)
- Path length overflow detection: `pathlen < len || pathlen > fts_length_max()` (`fts.c:687`)

### Root Containment
- No `realpath()`-based prefix checking
- Caller must ensure traversal boundaries via:
  - `FTS_XDEV` (filesystem boundary)
  - External sandboxing (chroot, namespaces)

---

## 3. Directory Iteration Logic Audit

### Symlink Safety
- `FTS_PHYSICAL` uses `O_NOFOLLOW` when opening directories (`fts.c:597-601`)
- Platform-dependent: `#warning` if `O_NOFOLLOW` unsupported (`fts.c:47`)
- `fts_stat()` uses `AT_SYMLINK_NOFOLLOW` for non-following stats (`fts.c:829`)

### Cycle Detection
**Two-layer detection mechanism**:
1. **Parent chain comparison** (`fts.c:843-848`): Linear ancestor check
2. **Hash table across symlinks** (`fts.c:1050-1156`): Tracks visited `(dev,ino)` pairs
3. **Cycle reporting**: `FTS_DC` with `fts_cycle` pointer to ancestor

### Depth Limits
- Maximum level: `SHRT_MAX` (32767) (`fts.c:590`)
- Iterative state machine (no recursion) prevents stack overflow

### Entry Validation
- `readdir()` entries treated as untrusted
- `d_name` length checked before copy
- Memory allocation failure handling

---

## 4. TOCTOU (Time-of-Check-Time-of-Use) Safety

### Strong Protection in `fts_safe_changedir()` (`fts.c:994-1045`)
1. Open directory (with `O_NOFOLLOW` when available)
2. `fstat()` on FD to get `(dev,ino)`
3. Compare with expected `(p->fts_dev, p->fts_ino)`
4. `fchdir()` using FD
5. `fstat()` again on same FD to verify directory unchanged
6. Returns error if any mismatch detected

### Potential Gaps
- `O_NOFOLLOW` requirement for full symlink-race protection
- Stat-to-open race in `fts_build()` mitigated by `O_NOFOLLOW` when available

### Mitigation Added (2025-12-10)
- Stat-to-open race in `fts_build()` now protected by `fstat()` verification of `(dev,ino)` after opening, providing protection even when `O_NOFOLLOW` is unavailable.

### File Descriptor Discipline
- Uses `openat()`/`fstatat()` patterns where possible (`fts_stat()` with dfd parameter)
- FD tracking and cleanup in `fts_close()`

---

## 5. Error Handling & Resource Limits

### Error Reporting
System call failures yield appropriate `FTS_*` codes:
- `FTS_DNR`: Unreadable directory
- `FTS_NS`: `stat()` failure
- `FTS_ERR`: Other errors with `fts_errno` set

### Traversal Continuation
- Errors in one branch don't abort entire walk
- Fault isolation preserves traversal of unaffected subtrees

### Resource Limits
| Resource | Limit | Implementation |
|----------|-------|----------------|
| **Depth** | `SHRT_MAX` (32767) | `fts.c:590` |
| **Memory** | Dynamic with checks | `safe_recallocarray()` with overflow detection |
| **Descriptors** | Tracked and closed | Cleanup in `fts_close()` |
| **DoS Potential** | Linear growth with entries | No per-directory entry limit |

---

## 6. Privilege Boundaries

### Library Responsibility
- Runs with caller's privileges
- No privilege escalation/dropping internally

### Caller Considerations for Security-Sensitive Use
1. Drop privileges before walking untrusted trees
2. Use `FTS_XDEV` to prevent crossing filesystem boundaries
3. Consider external sandboxing (namespaces, chroot) for hostile environments
4. Validate input paths before passing to `fts_open()`

---

## 7. Security Strengths

1. **Robust TOCTOU protection** in directory changes
2. **Comprehensive cycle detection** across symlinks
3. **Memory safety**: Systematic overflow checks, length capping, safe allocations
4. **Clean resource management**: FD tracking, guaranteed cleanup
5. **Fault isolation**: Errors don't abort entire traversal
6. **Test coverage**: Includes symlink loops, cycle detection, fault injection

---

## 8. Recommendations for Security-Sensitive Callers

### Immediate Actions
1. **Use `FTS_PHYSICAL`** unless symlink following is explicitly required
2. **Combine with `FTS_XDEV`** to prevent filesystem boundary crossing
3. **Drop privileges** before traversing untrusted directories

### Additional Protections
4. **External sandboxing**: Use namespaces, chroot, or seccomp for hostile environments
5. **Memory monitoring**: Watch memory usage when processing potentially hostile directories
6. **Input validation**: Sanitize paths before passing to `fts_open()`

### Platform Considerations
- On systems without `O_NOFOLLOW`, employ additional sandboxing
- Consider compile-time warnings about symlink race protection

---

## 9. Test Coverage Validation

### Existing Test Coverage
- `test_cycle_detection.c`: Cycle detection with symlink loops
- `test_symlink_loop_follow.c`: Symlink following behavior
- `test_fd_discipline.c`: File descriptor leak prevention
- `test_fault_injection.c`: Error handling and fault injection
- `test_whiteout.c`: Whiteout entry handling

### Recommended Additional Tests
1. **TOCTOU race condition simulation**
2. **Memory exhaustion** with directories containing millions of entries
3. **Path length overflow** edge cases
4. **Unicode normalization** and path traversal attempts
5. **Concurrent modification** during traversal

---

## 10. Overall Assessment

### Code Quality
- **Well-structured**: Clear separation of concerns
- **Documented**: Good comments and design documentation (`fts.md`)
- **Security boundaries**: Clear distinction between library and caller responsibilities

### Security Posture
- **Strong**: Comprehensive protection against common traversal vulnerabilities
- **Platform-aware**: Conditional compilation for `O_NOFOLLOW` support
- **Resource-conscious**: Systematic memory and FD management

### Primary Remaining Risk
**Platform-dependent symlink race protection** when `O_NOFOLLOW` unavailable. In such environments, security-critical callers should employ additional sandboxing measures.

### Suitability
**Recommended for security-sensitive applications** when used with the precautions outlined in Section 8. The implementation provides a solid foundation for secure directory traversal with appropriate caller-level protections.

---

## Appendix: File References

| File | Purpose |
|------|---------|
| `src/fts.c` | Core FTS implementation |
| `include/fts.h` | Public API header |
| `include/musl-bsd/fts_ops.h` | Internal operations interface |
| `tests/fts/` | Test suite |
| `fts.md` | Design documentation |

---

*Audit conducted following the methodology outlined in "Practical procedure for auditing file-hierarchy traversal implementations" (2025-12-10).*