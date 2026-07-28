/* A Fixed sized circular buffer library in C
 * Author: Ankush Mondal(ankushmondal1y2t@gmail.com) */
#include "cirbuf.h"

#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MFD_CLOEXEC
#define MFD_CLOEXEC 0x0001U
#endif

typedef struct cirbuf cirbuf;
typedef struct bufmem bufmem;

/* A global memory used for returning errors
 * this global variable is thread safe.
 * Each thread carries its own copy of this
 * global variable. */
_Thread_local cirbuf e_buffer;

/* bufmem holds the mirrored memory mapping details for a cirbuf. */
struct bufmem
{
    void *addr;
    size_t size;
    size_t usable;
};

/* cirbuf represents the internal state
 * of the buffer and also it holds the data */
struct cirbuf
{
    bufmem mem; // bufmem holds the actual address of the
                // memory location and also holds the other
                // variables like size(virtual size),
                // usable size.

    iter head; // head points to the first element in the buffer
    iter tail; // tail point to the last element in the buffer

    size_t cap;
    size_t datatype;

    int status; // status holds either 0 or -1 on success it will be 0
                // on error it will be -1
};

static void *cirbuf_get_block_addr(void *base, size_t index, size_t size)
{
    if (base == NULL)
        return NULL;

    if (size == 0 || index > SIZE_MAX / size)
        return NULL;

    return (char *)base + index * size;
}

static size_t min_pages(size_t tot_size, size_t unit_page_size)
{
    return (tot_size + unit_page_size - 1) / unit_page_size;
}

static bufmem allocate_buffer(size_t page_nos, size_t datatype, size_t cap)
{
    bufmem mem = (bufmem){
        .addr = NULL,
        .size = 0,
        .usable = 0,
    };
    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t tot_phy_size = pagesize * page_nos;
    size_t tot_vir_size = tot_phy_size * 2;

    // Validate that requested capacity fits
    if (cap * datatype > tot_phy_size)
    {
        fprintf(stderr, "Requested capacity exceeds physical memory\n");
        return mem;
    }

    // Create an anonymous in-memory file
    int fd = memfd_create(MEM_FILE, MFD_CLOEXEC);
    if (fd == -1)
    {
        perror("memfd_create");
        return mem;
    }

    // Give it tot_phy_size(total physical memory size) of storage
    if (ftruncate(fd, tot_phy_size) == -1)
    {
        perror("ftruncate");
        return mem;
    }

    // Reserve TWO pages of virtual address space
    void *reserve = mmap(NULL, tot_vir_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (reserve == MAP_FAILED)
    {
        perror("reserve");
        return mem;
    }

    // Map the file into the FIRST half
    void *first = mmap(reserve, tot_phy_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

    if (first == MAP_FAILED)
    {
        perror("first mmap");
        return mem;
    }

    // Map the SAME file into the SECOND half
    void *second =
        mmap((char *)reserve + tot_phy_size, tot_phy_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0);

    if (second == MAP_FAILED)
    {
        perror("second mmap");
        return mem;
    }

    close(fd);

    mem = (bufmem){
        .addr = reserve,
        .size = tot_vir_size,
        .usable = cap * datatype,
    };

    return mem;
}

/* cirbuf_create allocates a circular buffer with given capacity
 * and element size. Returns a valid cirbuf on success, or the
 * thread-local error buffer on failure. Caller must free with
 * cirbuf_destroy(). */
cirbuf *cirbuf_create(size_t cap, size_t datatype)
{
    size_t pagesize = sysconf(_SC_PAGESIZE);

    cirbuf *cbuf = (cirbuf *)malloc(sizeof(cirbuf));
    if (cbuf == NULL)
    {
        e_buffer = (cirbuf){
            .mem = (bufmem){NULL, 0, 0},
            .head = CIRBUF_EMPTY,
            .tail = CIRBUF_EMPTY,
            .cap = 0,
            .datatype = 0,
            .status = -1,
        };

        return &e_buffer;
    }

    if (datatype == 0 || cap > (SIZE_MAX / datatype))
    {
        free(cbuf);
        e_buffer = (cirbuf){
            .mem = (bufmem){NULL, 0, 0},
            .head = CIRBUF_EMPTY,
            .tail = CIRBUF_EMPTY,
            .cap = 0,
            .datatype = 0,
            .status = -1,
        };

        return &e_buffer;
    }

    size_t pages = min_pages(cap * datatype, pagesize);
    bufmem mem = allocate_buffer(pages, datatype, cap);
    if (mem.addr == NULL)
    {
        free(cbuf);
        e_buffer = (cirbuf){
            .mem = (bufmem){NULL, 0, 0},
            .head = CIRBUF_EMPTY,
            .tail = CIRBUF_EMPTY,
            .cap = 0,
            .datatype = 0,
            .status = -1,
        };

        return &e_buffer;
    }

    *cbuf = (cirbuf){
        .mem = mem,
        .head = CIRBUF_EMPTY,
        .tail = CIRBUF_EMPTY,
        .cap = cap,
        .datatype = datatype,
        .status = 0,
    };

    return cbuf;
}

/* cirbuf_put pushes a single element to the back of the
 * circular buffer. If the buffer is full, it overwrites
 * the oldest element at the front and advances the head.
 * The element is deep-copied into the buffer using memcpy.
 * On error (NULL buffer or error state), it returns
 * silently without any side effects. */
void cirbuf_put(cirbuf *cbuf, void *data)
{
    if (!cbuf || cbuf == &e_buffer)
        return;

    if (!data)
        return;

    if (cbuf->head == CIRBUF_EMPTY && cbuf->tail == CIRBUF_EMPTY)
    {
        cbuf->head = 0;
        cbuf->tail = 0;
    }
    else
    {
        cbuf->tail = (cbuf->tail + 1) % cbuf->cap;

        if (cbuf->head == cbuf->tail)
            cbuf->head = (cbuf->head + 1) % cbuf->cap;
    }

    void *addr = cirbuf_get_block_addr(cbuf->mem.addr, cbuf->tail, cbuf->datatype);
    if (!addr)
        return;

    memcpy(addr, data, cbuf->datatype);
}

/* cirbuf_get pops the oldest element from the front of
 * the circular buffer and copies it into the user-provided
 * void* buf. The popped slot is zeroed out with memset.
 * If the buffer is empty, it resets head and tail to
 * CIRBUF_EMPTY and returns without modifying buf.
 * On error (NULL buffer, NULL buf, or error state),
 * it returns silently without any side effects. */
void cirbuf_get(cirbuf *cbuf, void *buf)
{
    if (!cbuf || !buf || (cbuf == &e_buffer))
        return;

    void *addr = cirbuf_get_block_addr(cbuf->mem.addr, cbuf->head, cbuf->datatype);
    if (addr == NULL)
        return;

    memcpy(buf, addr, cbuf->datatype);
    memset(addr, 0, cbuf->datatype);

    if (cbuf->head == cbuf->tail)
    {
        cbuf->head = CIRBUF_EMPTY;
        cbuf->tail = CIRBUF_EMPTY;
    }
    else
    {
        cbuf->head = (cbuf->head + 1) % cbuf->cap;
    }

    return;
}

/* cirbuf_destroy frees the mirrored memory mapping and the cirbuf
 * structure. Safe to call on NULL or error buffers (no-op).
 * Setting status=-1 after free guards against double-free. */
void cirbuf_destroy(cirbuf *cbuf)
{
    if (!cbuf || cbuf->status)
        return;

    if (cbuf->mem.addr)
    {
        munmap(cbuf->mem.addr, cbuf->mem.size);
        cbuf->mem.addr = NULL;
        cbuf->mem.size = 0;
        cbuf->mem.usable = 0;
    }

    cbuf->status = -1;
    free(cbuf);
}

int cirbuf_is_ok(const cirbuf *cbuf)
{
    return cbuf->status == 0;
}

int cirbuf_is_err(const cirbuf *cbuf)
{
    return cbuf->status == 1;
}
