/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include "mm_alloc.h"
#include <stdlib.h>

void *mm_malloc(size_t size) {
  static block*base=NULL;
  struct block*b;
  for(b=base;b!=NULL&&b->next!=NULL;b=b->next);

  sbrk(size);
  return NULL;
}

void *mm_realloc(void *ptr, size_t size) {
  /* YOUR CODE HERE */
  return NULL;
}

void mm_free(void *ptr) {
  /* YOUR CODE HERE */
}
