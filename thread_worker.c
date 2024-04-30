#include "thread_worker.h"
#include "general_queue.h"
#include "tcb_queue.h"
#include "thread_worker_types.h"
#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ucontext.h>

#define STACK_SIZE 16 * 1024
#define QUANTUM 10 * 1000
#define MAIN_THREAD 1
#define QUEUE_NUM 4
#define SCHEDULER_THREAD 0
#define MAX_BUCKETS 100

struct Entry {
  int key;
  tcb *value;
  struct Entry *next;
};

struct my_hashmap {
  struct Entry *buckets[MAX_BUCKETS];
};

struct my_hashmap *createHashMap() {
  struct my_hashmap *map =
      (struct my_hashmap *)malloc(sizeof(struct my_hashmap));
  for (int i = 0; i < MAX_BUCKETS; i++)
    map->buckets[i] = NULL;
  return map;
}

void insert(struct my_hashmap *map, int key, tcb *value) {
  int index = key % MAX_BUCKETS;
  struct Entry *newEntry = (struct Entry *)malloc(sizeof(struct Entry));
  newEntry->key = key;
  newEntry->value = value;
  newEntry->next = NULL;
  if (map->buckets[index] == NULL)
    map->buckets[index] = newEntry;
  else {
    struct Entry *current = map->buckets[index];
    while (current->next != NULL)
      current = current->next;
    current->next = newEntry;
  }
}

tcb *get(struct my_hashmap *map, int key) {
  int index = key % MAX_BUCKETS;
  struct Entry *current = map->buckets[index];
  while (current != NULL) {
    if (current->key == key)
      return current->value;
    current = current->next;
  }
  return NULL;
}

bool containsKey(struct my_hashmap *map, int key) {
  int index = key % MAX_BUCKETS;
  struct Entry *current = map->buckets[index];
  while (current != NULL) {
    if (current->key == key)
      return true;
    current = current->next;
  }
  return false;
}

void removeKey(struct my_hashmap *map, int key) {
  int index = key % MAX_BUCKETS;
  struct Entry *current = map->buckets[index];
  struct Entry *prev = NULL;
  while (current != NULL && current->key != key) {
    prev = current;
    current = current->next;
  }
  if (current != NULL) {
    if (prev == NULL)
      map->buckets[index] = current->next;
    else
      prev->next = current->next;
    free(current);
  }
}

void clearHashMap(struct my_hashmap *map) {
  for (int i = 0; i < MAX_BUCKETS; i++) {
    struct Entry *current = map->buckets[i];
    while (current != NULL) {
      struct Entry *temp = current;
      current = current->next;
      free(temp);
    }
    map->buckets[i] = NULL;
  }
}

enum sched_options { _RR, _MLFQ };
int SCHED_TYPE = _MLFQ;
struct my_hashmap *map;
struct MyQueue *run_queue[QUEUE_NUM];
int threadCounter = 2;
bool isSchedCreated = false, isYielding = false, isDebugging = false;

typedef struct {
  unsigned int tNum;
  int qNum;
} tTuple;

tTuple currentThread = {MAIN_THREAD, 0};

tcb *get_current_tcb() { return get(map, currentThread.tNum); }
tcb *get_scheduler_tcb() { return get(map, SCHEDULER_THREAD); }

void debug_print(const char *string) {
  char msg[strlen(string) + 8];
  memset(msg, 0, strlen(string) + 8);
  strcat(msg, "DEBUG: ");
  strcat(msg, string);
  write(1, msg, strlen(msg));
}

void createContext(ucontext_t *threadContext) {
  getcontext(threadContext);
  threadContext->uc_link = NULL;
  threadContext->uc_stack.ss_sp = malloc(STACK_SIZE);
  threadContext->uc_stack.ss_size = STACK_SIZE;
  threadContext->uc_stack.ss_flags = 0;
  if (threadContext->uc_stack.ss_sp == NULL) {
    printf("Error: Unable to allocate stack memory: %d bytes in the heap\n",
           STACK_SIZE);
    exit(1);
  }
}

void createMainContext() {
  tcb *mainTCB = (tcb *)malloc(sizeof(tcb));
  mainTCB->id = 1;
  mainTCB->status = READY;
  insert(map, MAIN_THREAD, mainTCB);
}

bool isLastQueue(int queueNum) { return queueNum >= QUEUE_NUM - 1; }

bool isThreadInactive(int queueNum) {
  return peek(run_queue[queueNum])->status == FINISHED ||
         peek(run_queue[queueNum])->status == BLOCKED_MUTEX ||
         peek(run_queue[queueNum])->status == BLOCKED_JOIN;
}

static void sched_rr(int queueNum) {
  if (isQueueEmpty(run_queue[queueNum]))
    return;
  if (SCHED_TYPE == _RR) {
    if (isThreadInactive(queueNum))
      dequeue(run_queue[queueNum]);
    else
      enqueue(run_queue[queueNum], dequeue(run_queue[queueNum]));
  }
  tcb *currTCB = peek(run_queue[queueNum]);
  currentThread.tNum = currTCB->id;
  currentThread.qNum = queueNum;
  setcontext(&currTCB->context);
}

static void sched_mlfq() {
  int qNum = currentThread.qNum;
  if (isThreadInactive(qNum))
    dequeue(run_queue[qNum]);
  else if (isYielding || isLastQueue(qNum))
    enqueue(run_queue[qNum], dequeue(run_queue[qNum]));
  else
    enqueue(run_queue[qNum + 1], dequeue(run_queue[qNum]));
  isYielding = false;
  int currQueue = 0;
  while (currQueue < QUEUE_NUM) {
    sched_rr(currQueue);
    ++currQueue;
  }
}

/* scheduler */
static void schedule() {
  // - every time a timer interrupt occurs, your worker thread library
  // should be contexted switched from a thread context to this
  // schedule() function

  // - invoke scheduling algorithms according to the policy (RR or MLFQ)

  // - schedule policy
  currentThread.tNum = SCHEDULER_THREAD;
  if (SCHED_TYPE == _MLFQ)
    sched_mlfq();
  else if (SCHED_TYPE == _RR)
    sched_rr(0);
  else if (isDebugging)
    debug_print("!!!INVALID SCHEDULE TYPE!!!");

#ifndef MLFQ
    // Choose RR

#else
    // Choose MLFQ

#endif
}

void createSchedulerContext() {
  tcb *schedTCB = (tcb *)malloc(sizeof(tcb));
  schedTCB->id = 0;
  createContext(&schedTCB->context);
  makecontext(&schedTCB->context, (void (*)()) & schedule, 0);
  insert(map, SCHEDULER_THREAD, schedTCB);
}

void timer_handler(int signum) {
  if (isDebugging)
    debug_print("Running setupTimer handler\n");
  if (currentThread.tNum != SCHEDULER_THREAD)
    swapcontext(&get_current_tcb()->context, &get_scheduler_tcb()->context);
}

void setupTimer() {
  struct itimerval it_val;
  if (signal(SIGALRM, (void (*)(int))timer_handler) == SIG_ERR) {
    perror("Unable to catch SIGALRM");
    exit(1);
  }
  it_val.it_value.tv_sec = QUANTUM / 1000;
  it_val.it_value.tv_usec = (QUANTUM * 1000) % 1000000;
  it_val.it_interval = it_val.it_value;
  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
    perror("error calling setitimer()");
    exit(1);
  }
  bool isTimerFiredOnce = false;
  while (!isTimerFiredOnce) {
    pause();
    isTimerFiredOnce = true;
  }
}

void startTimer() {
  struct itimerval it_val;
  it_val.it_value.tv_sec = QUANTUM / 1000;
  it_val.it_value.tv_usec = (QUANTUM * 1000) % 1000000;
  it_val.it_interval = it_val.it_value;
  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
    perror("error calling setitimer()");
    exit(1);
  }
}

void stopTimer() {
  struct itimerval it_val;
  it_val.it_interval.tv_usec = 0;
  it_val.it_interval.tv_sec = 0;
  it_val.it_value.tv_usec = 0;
  it_val.it_value.tv_sec = 0;
  setitimer(ITIMER_REAL, &it_val, NULL);
}

void worker_start(tcb *currTCB, void (*function)(void *), void *arg) {
  function(arg);
  currTCB->status = FINISHED;
  free(currTCB->context.uc_stack.ss_sp);
  setcontext(&get_scheduler_tcb()->context);
}

int worker_create(worker_t *thread, pthread_attr_t *attr, void *(*function)(void *), void *arg) {
  *thread = threadCounter;
  tcb *newThread = (tcb *)malloc(sizeof(tcb));
  newThread->id = threadCounter;
  newThread->status = READY;
  newThread->joining = 0;
  createContext(&newThread->context);
  getcontext(&newThread->context);
  makecontext(&newThread->context, (void (*)()) & worker_start, 3, newThread, function, arg);
  insert(map, threadCounter++, newThread);
  if (!isSchedCreated) {
    createSchedulerContext();
    createMainContext();
    enqueue(run_queue[0], get(map, MAIN_THREAD));
    enqueue(run_queue[0], newThread);
    isSchedCreated = true;
    #ifndef MLFQ
      SCHED_TYPE = _RR;
    #else
      SCHED_TYPE = _MLFQ;
    #endif
    setupTimer();
  } else
    enqueue(run_queue[0], newThread);
  return 0;
}

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
  tcb *currTCB = get_current_tcb();
  currTCB->status = READY;
  isYielding = true;
  swapcontext(&currTCB->context, &get_scheduler_tcb()->context);
  return 0;
}

/* terminate a thread */
void worker_exit(void *value_ptr) {
  tcb *currTCB = get_current_tcb();
  currTCB->retVal = value_ptr;
  free(currTCB->context.uc_stack.ss_sp);
  if (currTCB->joining != 0) {
    tcb *joinedTCB = get(map, currTCB->joining);
    joinedTCB->status = READY;
    enqueue(run_queue[0], joinedTCB);
  }
  currTCB->status = FINISHED;
  setcontext(&get_scheduler_tcb()->context);
}

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr) {
  tcb *currTCB = get_current_tcb();
  tcb *joinedTCB = get(map, thread);
  if (joinedTCB->status != FINISHED) {
    joinedTCB->joining = currTCB->id;
    currTCB->status = BLOCKED_JOIN;
    swapcontext(&currTCB->context, &get_scheduler_tcb()->context);
  }
  if (value_ptr != NULL)
    *value_ptr = joinedTCB->retVal;
  return 0;
}

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
  if (mutex == NULL)
    return EINVAL;
  mutex->queue = (struct MyIntQueue *)malloc(sizeof(struct MyIntQueue *));
  mutex->queue->front = mutex->queue->rear = NULL;
  mutex->owner = 0;
  return 0;
}

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {
  if (mutex == NULL)
    return EINVAL;
  while (__sync_lock_test_and_set((int *)&(mutex->flag), 1)) {
    tcb *currTCB = get_current_tcb();
    intEnqueue(mutex->queue, currTCB->id);
    currTCB->status = BLOCKED_MUTEX;
    swapcontext(&currTCB->context, &get_scheduler_tcb()->context);
  }
  mutex->owner = get_current_tcb()->id;
  return 0;
}

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
  if (mutex == NULL)
    return EINVAL;
  if (get_current_tcb()->id != mutex->owner) {
    printf("Tried unlocking thread without ownership!");
    exit(1);
  }
  if (!isIntQueueEmpty(mutex->queue)) {
    for (int i = 0; i < intQueueSize(mutex->queue); i++) {
      int x = intQueueGet(mutex->queue, i);
      tcb t = *(tcb *)get(map, x);
      t.status = READY;
      insert(map, x, &t);
      enqueue(run_queue[0], get(map, x));
    }
    clearIntQueue(mutex->queue);
  }
  atomic_flag_clear(&mutex->flag);
  return 0;
}

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
  if (mutex == NULL)
    return EINVAL;
  clearIntQueue(mutex->queue);
  return 0;
}
