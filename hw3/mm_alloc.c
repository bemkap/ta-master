/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include"mm_alloc.h"
#include<stdlib.h>
#include<unistd.h>
#include<string.h>

#define ER (void*)-1
#define SZ sizeof(block)

void*mm_malloc(size_t size){
  static block*base=NULL; block*b,*t;
  if(!size) return NULL;
  if(!base){
    if(ER==(t=sbrk(SZ+size))) return NULL;
    base=t;
    base->prev=base->next=NULL;
    base->free=0; base->size=size;
    memset(base->mem,0,size);
    return base->mem;
  }else{
    for(b=base; (!b->free||b->size<(SZ+size))&&b->next; b=b->next);
    if(!b->next){
      if(ER==(t=sbrk(SZ+size))) return NULL;
      b->next=t;
      b->next->prev=b;
      b->next->next=NULL;
      b->next->free=0;
      b->next->size=size;
      memset(b->next->mem,0,size);
      return b->next->mem;
    }else{
      b->free=0;
      t=b->next;
      b->next=(block*)b->mem+size;
      b->next->prev=b;
      b->next->next=t;
      b->next->size=b->size-size;
      b->size=size;
      b->next->free=1;
      memset(b->mem,0,size);
      return b->mem;
    }
  }
}

void*mm_realloc(void*ptr,size_t size){
  block*np,*b=ptr-SZ;
  mm_free(ptr);
  if(NULL==(np=mm_malloc(size))){
    b->free=0;
    return NULL;
  }
  if(NULL!=ptr)
    memcpy(np->mem,ptr,size);
  return np->mem;
}

void mm_free(void*ptr){
  block*b=ptr-SZ;
  b->free=1;
}
