#pragma once

#include "../message_queue/message.h"
#include <stdatomic.h>
#include <stdbool.h>
#define MAX_ENTRIES 60

typedef struct Node {
  Message msg;
  _Atomic(struct Node *) next;
} Node;

typedef struct {
  _Atomic(Node *) head;
  _Atomic(Node *) tail;
  atomic_int size;
} Dequeue;

extern Dequeue dq;

void initDequeue(void);
bool isEmpty(void);
bool isFull(void);
bool pushRear(Message msg);
bool popFront(Message *msg);
Message *displayEntries(void);
void freeDequeue(void);
