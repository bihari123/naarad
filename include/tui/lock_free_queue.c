#include "lock_free_queue.h"

#include "../utils/logs.h"
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
Dequeue dq;

void initDequeue(void) {
  Node *dummy = malloc(sizeof(Node));
  if (!dummy) {
    log_error("Memory allocation failed\n");
    exit(1);
  }
  dummy->next = NULL;
  atomic_store(&dq.head, dummy);
  atomic_store(&dq.tail, dummy);
  atomic_store(&dq.size, 0);
}

bool isEmpty(void) { return atomic_load(&dq.size) == 0; }

bool isFull(void) { return atomic_load(&dq.size) >= MAX_ENTRIES; }

bool pushRear(Message msg) {
  Node *new_node = malloc(sizeof(Node));
  if (!new_node) {
    return false;
  }
  new_node->msg = msg;
  new_node->next = NULL;

  while (true) {
    Node *tail = atomic_load(&dq.tail);
    Node *next = atomic_load(&tail->next);

    if (tail == atomic_load(&dq.tail)) {
      if (next == NULL) {
        if (atomic_compare_exchange_weak(&tail->next, &next, new_node)) {
          atomic_compare_exchange_weak(&dq.tail, &tail, new_node);
          int old_size = atomic_fetch_add(&dq.size, 1);
          if (old_size >= MAX_ENTRIES) {
            // If we've exceeded MAX_ENTRIES, try to remove from front
            Message dummy;
            popFront(&dummy);
          }
          return true;
        }
      } else {
        atomic_compare_exchange_weak(&dq.tail, &tail, next);
      }
    }
  }
}

bool popFront(Message *msg) {
  while (true) {
    Node *head = atomic_load(&dq.head);
    Node *tail = atomic_load(&dq.tail);
    Node *next = atomic_load(&head->next);

    if (head == atomic_load(&dq.head)) {
      if (head == tail) {
        if (next == NULL) {
          return false; // Queue is empty
        }
        atomic_compare_exchange_weak(&dq.tail, &tail, next);
      } else {
        *msg = next->msg;
        if (atomic_compare_exchange_weak(&dq.head, &head, next)) {
          atomic_fetch_sub(&dq.size, 1);
          free(head);
          return true;
        }
      }
    }
  }
}

Message *displayEntries(void) {
  Message *array = (Message *)malloc(sizeof(Message) * MAX_ENTRIES);
  if (array == NULL) {
    return NULL;
  }

  Node *current = atomic_load(&dq.head)->next;
  int index = 0;
  log_info("Current entries in the dequeue (newest last):\n");
  while (current != NULL && index < MAX_ENTRIES) {
    array[index].op = current->msg.op;
    array[index].gid = current->msg.gid;
    array[index].uid = current->msg.uid;
    snprintf(array[index].comm, sizeof(current->msg.comm), "%s",
             current->msg.comm);

    snprintf(array[index].file, sizeof(current->msg.file), "%s",
             current->msg.file);
    if (current->msg.op == OP_RENAME) {
      snprintf(array[index].new_file, sizeof(current->msg.new_file), "%s",
               current->msg.new_file);
    }
    current = atomic_load(&current->next);
    index++;
  }
  if (index == 0) {
    return NULL;
  }
  array[index].op = STOP_MESSAG;

  log_info("Total entries: %d\n", atomic_load(&dq.size));

  return array;
}

void freeDequeue(void) {
  Node *current = atomic_load(&dq.head);
  while (current != NULL) {
    Node *next = atomic_load(&current->next);
    free(current);
    current = next;
  }
  atomic_store(&dq.head, NULL);
  atomic_store(&dq.tail, NULL);
  atomic_store(&dq.size, 0);
}
