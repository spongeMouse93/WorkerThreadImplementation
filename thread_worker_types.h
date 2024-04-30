#ifndef TW_TYPES_H
#define TW_TYPES_H

#include <ucontext.h>

typedef unsigned int worker_t;

typedef struct TCB{
  unsigned int id;
  int status;
  ucontext_t context;
  void *retVal;
  unsigned int joining;
} tcb;

#endif
