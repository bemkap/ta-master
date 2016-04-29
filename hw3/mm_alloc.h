/*
 * mm_alloc.h
 *
 * A clone of the interface documented in "man 3 malloc".
 */

#pragma once

#include<stdlib.h>

struct block {
  struct block*prev,*next;
  char free; size_t size; char mem[0];
};

void*mm_malloc(size_t);
void*mm_realloc(void*,size_t);
void mm_free(void*);
