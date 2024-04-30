#include "tcb_queue.h"
#include <stdatomic.h>
#ifndef MTX_TYPES_H
#define MTX_TYPES_H

/* mutex struct definition */
typedef struct worker_mutex_t{
  atomic_flag flag;
  unsigned int owner;
  struct MyIntQueue* queue;
} worker_mutex_t;

#endif
