#include "thread_worker_types.h"
#include <stdbool.h>
#include <stdlib.h>

#ifndef GENERAL_QUEUE_H
#define GENERAL_QUEUE_H

struct QNode {
  tcb* data;
  struct QNode *next;
};

struct MyQueue {
  struct QNode *head;
  struct QNode *back;
};

struct MyQueue *createQueue() {
  struct MyQueue *queue = (struct MyQueue *)malloc(sizeof(struct MyQueue));
  queue->head = NULL;
  queue->back = NULL;
  return queue;
}

void enqueue(struct MyQueue *queue, tcb* data) {
  struct QNode *newNode = (struct QNode *)malloc(sizeof(struct QNode));
  newNode->data = data;
  newNode->next = NULL;
  if (queue->back == NULL) {
    queue->head = newNode;
    queue->back = newNode;
  } else {
    queue->head->next = newNode;
    queue->back = newNode;
  }
}

tcb* dequeue(struct MyQueue *queue) {
  if (queue->head == NULL)
    return NULL;
  tcb* data = queue->head->data;
  struct QNode *temp = queue->head;
  queue->head = queue->head->next;
  if (queue->head == NULL) 
    queue->back = NULL;
  free(temp); 
  return data;
}

tcb* peek(struct MyQueue *queue) {
  if (queue->head == NULL) 
    return NULL; 
  return queue->head->data;
}

bool isQueueEmpty(struct MyQueue *queue) { return (queue->head == NULL); }

int queueSize(struct MyQueue* queue) {
  int size = 0;
  struct QNode* current = queue->head;
  while (current != NULL) {
    size++;
    current = current->next;
  }
  return size;
}

tcb* queueGet(struct MyQueue* queue, int position) {
  if (position < 0 || position >= queueSize(queue)) 
    return NULL;
  struct QNode* current = queue->head;
  for (int i = 0; i < position; i++) 
    current = current->next;
  return current->data;
}

void clearQueue(struct MyQueue* queue) {
  while (!isQueueEmpty(queue)) 
    dequeue(queue);
}

#endif
