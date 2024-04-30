#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#ifndef TCB_QUEUE_H
#define TCB_QUEUE_H

struct intNode {
  unsigned int data;
  struct intNode *next;
};

struct MyIntQueue {
  struct intNode *front;
  struct intNode *rear;
};

struct MyIntQueue *createIntQueue() {
  struct MyIntQueue *queue = (struct MyIntQueue *)malloc(sizeof(struct MyIntQueue));
  queue->front = NULL;
  queue->rear = NULL;
  return queue;
}

void intEnqueue(struct MyIntQueue *queue, unsigned int data) {
  struct intNode *newNode = (struct intNode *)malloc(sizeof(struct intNode));
  newNode->data = data;
  newNode->next = NULL;
  if (queue->rear == NULL) {
    queue->front = newNode;
    queue->rear = newNode;
  } else {
    queue->rear->next = newNode;
    queue->rear = newNode;
  }
}

unsigned int intDequeue(struct MyIntQueue *queue) {
  if (queue->front == NULL)
    return EINVAL;
  unsigned int data = queue->front->data;
  struct intNode *temp = queue->front;
  queue->front = queue->front->next;
  if (queue->front == NULL) 
    queue->rear = NULL;
  free(temp); 
  return data;
}

unsigned int intPeek(struct MyIntQueue *queue) {
  if (queue->front == NULL) 
    return EINVAL; 
  return queue->front->data;
}

bool isIntQueueEmpty(struct MyIntQueue *queue) { return (queue->front == NULL); }

int intQueueSize(struct MyIntQueue* queue) {
  int size = 0;
  struct intNode* current = queue->front;
  while (current != NULL) {
    size++;
    current = current->next;
  }
  return size;
}

unsigned int intQueueGet(struct MyIntQueue* queue, int position) {
  if (position < 0 || position >= intQueueSize(queue)) 
    return EINVAL;
  struct intNode* current = queue->front;
  for (int i = 0; i < position; i++) 
    current = current->next;
  return current->data;
}

void clearIntQueue(struct MyIntQueue* queue) {
  while (!isIntQueueEmpty(queue)) 
    intDequeue(queue);
}

#endif
