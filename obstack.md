# obstack Testing Checklist

## Core API Coverage

* `_obstack_begin` initializes a usable obstack with default malloc/free
* `_obstack_begin_1` initializes an obstack with caller-supplied alloc/free functions
* `_obstack_newchunk` correctly allocates and chains new chunks when current chunk is full
* `obstack_alloc` returns aligned storage of requested size, advancing `next_free`
* `obstack_copy` and `obstack_copy0` correctly copy existing buffers, zero-terminating where required
* `obstack_grow` and `obstack_grow0` append data to current object, with and without final terminator
* `obstack_blank` and `obstack_blank_fast` reserve raw bytes with correct alignment and growth behavior
* `obstack_free` frees up to a given object or all objects when passed `NULL`
* `obstack_printf` and `obstack_vprintf` append formatted text to the current object

---

## Struct Layout and Invariants

* `struct obstack` has the expected field ordering and size matching the chosen GNU reference
* After initialization:

  * `chunk` is non-NULL
  * `chunk_limit` points to end of allocated chunk
  * `object_base` equals `next_free` (no open object yet)
* After each allocation or grow operation:

  * `object_base` stays at the beginning of the open object
  * `next_free` points to the next free byte after the object content
* After `obstack_free` to a particular object:

  * objects above that object are discarded
  * `next_free` and `object_base` reflect the surviving head object

---

## Chunk Allocation and Linking

* First allocation via `_obstack_begin`:

  * allocates an initial chunk of at least `chunk_size`
  * aligns `object_base` as required
* Filling a chunk:

  * repeated grows and allocs until just before `chunk_limit` succeed
  * a new chunk is allocated only when necessary
* `_obstack_newchunk` behavior:

  * new chunk size grows appropriately (e.g. by factor or with minimum) based on requested size
  * `chunk->prev` links to the old chunk
  * `object_base` and `next_free` are updated to point into the new chunk
* Freeing all objects with `obstack_free(obstack, NULL)`:

  * all chunks except the first are freed
  * the first chunk is retained or reinitialized per design
  * obstack returns to its initial, empty state

---

## Alignment Semantics

* Objects allocated with `obstack_alloc` respect the obstack’s alignment setting
* `obstack_blank` preserves alignment and moves `next_free` to the next aligned boundary when requested
* Misaligned sizes still yield correctly aligned pointers for subsequent allocations
* Alignment does not cause out-of-bounds access at the end of a chunk

---

## Grow vs Allocate Semantics

* `obstack_alloc` starts a new object and returns its base address
* `obstack_grow` and `obstack_grow0`:

  * append to the *currently open* object without closing it
  * handle repeated calls, even across chunk boundaries
* `obstack_copy` and `obstack_copy0`:

  * act like `alloc + memcpy` and `alloc + memcpy + terminator`
  * never reuse or corrupt previous objects
* Mixing operations:

  * sequence of `obstack_alloc` + `obstack_grow` + `obstack_blank` behaves like growing a single object
  * any new allocation after `obstack_finish` starts a distinct object

---

## `obstack_free` Behavior

* `obstack_free(obstack, NULL)`:

  * releases all objects
  * leaves obstack usable for new allocations
* `obstack_free(obstack, pointer_to_object)`:

  * correctly locates containing chunk and object
  * frees all chunks allocated after that chunk
  * uses allocator’s `free` function for each freed chunk
* Passing a pointer that is not an object base:

  * behavior matches GNU expectations (typically treated as freeing to object start or undefined, depending on your chosen compatibility stance)
  * tests should check and document the behavior you commit to
* Double free of the same object pointer is prevented by design (documented as undefined but tested for graceful failure where feasible)

---

## Custom Allocator Hooks

* `_obstack_begin_1` correctly stores function pointers for `chunk_alloc` and `chunk_free`
* All chunk allocations go through `chunk_alloc` and all frees go through `chunk_free`
* Using a custom allocator:

  * tracks the exact number and sizes of chunk allocations
  * confirms no unexpected allocations or frees happen
* When `chunk_alloc` fails (returns NULL):

  * `obstack_alloc_failed_handler` is called
  * no partial state remains that could be reused unsafely

---

## `obstack_alloc_failed_handler` and Error Paths

* Default handler is installed and called on allocation failure
* Replacing `obstack_alloc_failed_handler`:

  * handler is invoked on every failed chunk allocation
  * handler can log or longjmp without corrupting obstack state
* Tests simulate OOM:

  * custom allocator that fails after N allocations
  * verify handler is called exactly once per failure
  * verify there are no leaks or double frees when failures happen mid-grow

---

## `obstack_printf` and `obstack_vprintf`

* Simple formatted strings append correctly to the current object
* Long formatted strings that require crossing chunk boundaries:

  * allocate additional chunk(s) as needed
  * leave object contiguous in obstack memory space
* Format specifiers:

  * `%s`, `%d`, `%ld`, `%zu`, `%p`, `%%` behave correctly
  * width and precision fields are honored
* `obstack_vprintf`:

  * behaves identically to `obstack_printf` when wrapped with a `va_list`
  * does not corrupt the `va_list` for subsequent calls

---

## Multi-object Usage Patterns

* Multiple objects can be allocated sequentially:

  * `obj1 = obstack_alloc(...)`
  * `obj2 = obstack_alloc(...)`
  * both remain valid until `obstack_free` to earlier object or full reset
* Freeing back to `obj1` invalidates `obj2` but not `obj1`
* Freeing back to `obj2` has no effect beyond potential chunk merges, if any
* Pattern with interleaved growth:

  * `obj1` grows
  * `obj2` allocated after `obj1` is finished
  * growth operations on `obj2` never affect `obj1` content

---

## Thread-safety Semantics

* Documented behavior:

  * individual `struct obstack` instances are not thread-safe without external locking
* Tests:

  * single obstack used from one thread has no races or UB under TSAN
  * multiple threads using separate obstacks in parallel show no cross-contamination or shared global state issues

---

## Memory Safety and Leak Checks

* All paths:

  * normal usage
  * early exit after partial use
  * failure in allocation
    are clean under ASan and UBSan
* No heap buffer overflows in:

  * chunk header handling
  * `grow` and `grow0` when transitioning between chunks
* No memory leaks:

  * when freeing to NULL
  * when freeing to interior objects
  * when destroying obstack structure (as far as your API defines)

---

## Stress and Edge Cases

* Very large objects:

  * single object larger than default chunk size
  * very large but legal size (within `size_t` and allocator limits)
* Many small objects:

  * thousands or millions of tiny allocations
  * verify chunk reuse is efficient and free of fragmentation issues in allocator logs
* Zero-sized operations:

  * `obstack_alloc(obstack, 0)` returns a distinct pointer or consistent sentinel, matching your chosen semantics
  * `obstack_grow(obstack, ptr, 0)` is a no-op on content but maintains invariants
* Alignment corner cases:

  * alignment set to 1, 2, 4, 8, 16 all work
  * computed addresses from `obstack_alloc` satisfy `(uintptr_t)p % alignment == 0`

---

## Golden Behavior Tests

Create dedicated tests for:

* basic usage with default allocator
* basic usage with custom allocator (tracking allocations and frees)
* sequences with `alloc`, `grow`, `copy`, `blank`, `free` demonstrating typical patterns
* error scenarios with injected OOM and custom `obstack_alloc_failed_handler`

Tests must verify:

* object content matches expected strings or binary patterns
* sequence of chunk allocations and frees matches expectations from custom allocator logs
* no change in externally visible behavior across builds and platforms
