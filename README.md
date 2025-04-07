![musl-bsd logo](.github/musl-bsd.png "musl-bsd Logo")

**musl-bsd** is a lightweight compatibility library offering BSD-style functionality and macros for systems using [musl libc](https://musl.libc.org/). It includes portable implementations of common BSD APIs—like `fts(3)` for directory traversal and `obstack(3)` for efficient stack-based memory allocation—plus supporting headers (`queue.h`, `tree.h`, `cdefs.h`) derived from BSD sources.

This project is especially useful when **porting BSD-licensed software** or building **legacy Unix code** on Linux systems that rely on musl.

---

## Features

- **`fts(3)`** – Directory traversal API (`fts_open`, `fts_read`, `fts_children`, etc.)
- **`obstack(3)`** – Stack-like memory allocation (from GNU)
- **`queue.h`, `tree.h`** – BSD-style macros for linked lists, tail queues, and binary trees
- **`cdefs.h`** – Macros for compiler attributes, visibility, and more

---

## Installation

Using **Meson**:

```sh
meson setup build --prefix=/usr
meson compile -C build
sudo meson install -C build
```

This installs:
- **Libraries**: `libfts.so`, `libobstack.so` → `/usr/lib/`
- **Headers**: `fts.h`, `obstack.h`, `queue.h`, `tree.h`, `cdefs.h` → `/usr/include/`

---

## Usage

**Include** them in your code:

```c
#include <fts.h>
#include <obstack.h>
#include <queue.h>
#include <tree.h>
#include <cdefs.h>
```

Then compile and link against both libraries:

```sh
cc myapp.c -lfts -lobstack
```

---

## Compatibility

- Designed for **musl-based** Linux systems
- C99- and POSIX.1-2008-compatible

---

## Licensing

- **`fts.c` / `fts.h`**: BSD 2-Clause License
- **`obstack.*`**: GNU LGPL (2.1+)
- **`queue.h`, `tree.h`, `cdefs.h`**: BSD-derived

See each file’s header for specific license details.

---

## Credits

- Adapted from NetBSD/OpenBSD for `queue.h`, `tree.h`, and `cdefs.h`
- Based on GNU `obstack` for the obstack API
- Inspired by various musl patches from projects like [Void Linux](https://github.com/void-linux)

---

## Contributing

Pull requests and patches are welcome! If adding new BSD or musl-specific functionality, please ensure:

1. The code remains compatible with musl and POSIX.
2. The licensing is maintained (BSD or similarly permissive).

For major changes, open an issue first so we can discuss design and scope.
