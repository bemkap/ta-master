/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include"mm_alloc.h"
#include<stdlib.h>
#include<unistd.h>

void*mm_malloc(size_t size){static void*base=NULL; struct block*b;
  if(!base){
    base=(struct block*)sbrk(sizeof(struct block)+size);
    base->prev=base->next=NULL;
    base->free=0; base->size=size;
    return base->mem;
  }else{
    b=(struct block*)base;
    for(; (!b->free||b->size<(size+sizeof(struct block))&&b->next; b=b->next);
    if(!b->next){
      b->next=sbrk(sizeof(struct block)+size);
      b->next->prev=b;
      b->next->next=NULL;
      b->next->size=size;
      b->free=0;
      return b->next->mem;
    }else{
      b->free=0;
      b->next=(struct block*)b->mem+size;
      b->next->free=1;
      b->next->prev=b;
      b->next->size=b->size-size;
      b->size=size;
      return b->mem;
    }
  }    
  return NULL;
}

void*mm_realloc(void*ptr,size_t size){
  /* YOUR CODE HERE */
  return NULL;
}

void mm_free(void*ptr){
  /* YOUR CODE HERE */
}
