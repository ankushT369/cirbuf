/* A Fixed sized thread-safe circular buffer library in C 
 * Author: Ankush Mondal(ankushmondal1y2t@gmail.com) */

/*
 *      // put() operation pushes an element from the back
 *      // get() operation pops an element from the front and returns it
 *
 *      [?]  [?]  [?]  [?]
 *       0    1    2    3
 *      buffer = []
 *      head=EMPTY  tail=EMPTY  len=0
 *
 *      
 *      // put(9), put(4), put(3)
 *
 *      [9]  [4]  [3]  [?]
 *       0    1    2    3
 *       ^         ^
 *       head      tail
 *       buffer = [9, 4, 3]
 *       head=0  tail=2  len=3
 *
 *
 *      // put(1)
 *
 *      [9]  [4]  [3]  [1]
 *       0    1    2    3
 *       ^              ^
 *       head           tail
 *       buffer = [9, 4, 3, 1]
 *       head=0  tail=3  len=4
 *
 *
 *      // put(5)
 *
 *      [5]  [4]  [3]  [1]
 *       0    1    2    3
 *       ^    ^
 *       tail head
 *       buffer = [4, 3, 1, 5]
 *       head=1  tail=0  len=4
 *
 *
 *      // get()
 *
 *      [5]  [x]  [3]  [1]
 *       0    1    2    3
 *       ^         ^
 *       tail      head
 *       buffer = [3, 1, 5]
 *       head=2  tail=0  len=3
 */

#ifndef CIRBUF_H
#define CIRBUF_H

#include <stddef.h>

#define CIRBUF_EMPTY SIZE_MAX
#define MEM_FILE "cirbuf"

typedef struct cirbuf cirbuf;
typedef size_t iter;

/* API */
cirbuf* cirbuf_create(size_t max, size_t datatype);
void cirbuf_destroy(cirbuf* queue);

void cirbuf_put(cirbuf* cbuf, void* data);
void cirbuf_get(cirbuf* cbuf, void* buf);

/* Bulk operations */
void cirbuf_write(cirbuf* cbuf, void* data, size_t len);
void cirbuf_read(cirbuf* cbuf, void* buf, size_t len);

/* Utility functions */
int cirbuf_is_ok(const cirbuf *q);
int cirbuf_is_err(const cirbuf *q);
int cirbuf_is_empty(const cirbuf* q);


#endif // CIRBUF_H
