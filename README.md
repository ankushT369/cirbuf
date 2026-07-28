# cirbuf

A fixed-size circular buffer library written in C.

The buffer stores copies of elements and automatically overwrites the oldest data when it becomes full.

Internally it uses mirrored memory mapping on Linux so the buffer can be accessed as one continuous memory region.

## Features

* Fixed-size circular buffer
* Stores any data type
* Automatically overwrites old data when full
* Opaque API (internal structure is hidden)
* Linux implementation using `memfd_create()` and `mmap()`

## Build

```sh
make
```

This builds:

* `libcirbuf.a` (static library)
* `libcirbuf.so` (shared library)

## Example

```c
#include <stdio.h>
#include "cirbuf.h"

int main(void)
{
    cirbuf *buf = cirbuf_create(4, sizeof(int));

    if (!cirbuf_is_ok(buf))
        return 1;

    int value;

    value = 10;
    cirbuf_put(buf, &value);

    value = 20;
    cirbuf_put(buf, &value);

    cirbuf_get(buf, &value);
    printf("%d\n", value);

    cirbuf_destroy(buf);

    return 0;
}
```

Compile:

```sh
gcc main.c -L. -lcirbuf
```

## API

```c
cirbuf *cirbuf_create(size_t capacity, size_t element_size);

void cirbuf_destroy(cirbuf *buf);

void cirbuf_put(cirbuf *buf, void *data);

void cirbuf_get(cirbuf *buf, void *data);

int cirbuf_is_ok(const cirbuf *buf);

int cirbuf_is_err(const cirbuf *buf);
```

## Project Status

### Implemented

* [x] Create and destroy a buffer
* [x] Put one element
* [x] Get one element
* [x] Automatic overwrite when full
* [x] Hidden internal structure
* [x] Mirrored memory mapping

### Planned

* [ ] Bulk read
* [ ] Bulk write
* [ ] Buffer size function
* [ ] Empty/full check
* [ ] Better error handling
* [ ] More examples
* [ ] Tests
