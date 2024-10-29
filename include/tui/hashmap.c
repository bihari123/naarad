#include "hashmap.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct HashItem {
  char *key;
  uint8_t value;
  bool is_valid;
};

// Global HashMap variable definition
HashMap OperationMap;

// Flag to check if the map has been initialized
static bool initialized = false;

unsigned int hash(const char *key) {
  unsigned long int value = 0;
  unsigned int i = 0;
  unsigned int key_len = strlen(key);

  for (; i < key_len; ++i) {
    value = value * 37 + key[i];
  }

  value = value % TABLE_SIZE;
  return value;
}

static void free_item(HashItem *item) {
  free(item->key);
  free(item);
}

void init_operation_map() {
  if (!initialized) {
    for (int i = 0; i < TABLE_SIZE; i++) {
      OperationMap.items[i] = NULL;
    }
    initialized = true;
    insert("open", OP_OPEN);
    insert("read", OP_READ);
    insert("write", OP_WRITE);
    insert("create", OP_CREATE);
    insert("link", OP_LINK);
    insert("unlink", OP_UNLINK);
    insert("syslink", OP_SYSLINK);
    insert("mkdir", OP_MKDIR);
    insert("rmdir", OP_RMDIR);
    insert("mknod", OP_MKNOD);
    insert("rename", OP_RENAME);
  }
}

void cleanup_operation_map() {
  if (initialized) {
    for (int i = 0; i < TABLE_SIZE; i++) {
      HashItem *item = OperationMap.items[i];
      if (item != NULL) {
        free_item(item);
      }
    }
    initialized = false;
  }
}

void insert(const char *key, uint8_t value) {
  if (!initialized) {
    printf("Error: OperationMap not initialized\n");
    return;
  }

  unsigned int index = hash(key);
  HashItem *item = OperationMap.items[index];

  if (item == NULL) {
    item = (HashItem *)malloc(sizeof(HashItem));
    item->key = strdup(key);
    item->value = value;
    item->is_valid = true;
    OperationMap.items[index] = item;
  } else {
    if (strcmp(item->key, key) == 0) {
      item->value = value;
      item->is_valid = true;
    } else {
      // Handle collision (e.g., linear probing)
      printf("Collision occurred for key: %s\n", key);
    }
  }
}

HashMapResult get(const char *key) {
  HashMapResult result = {0, false};

  if (!initialized) {
    printf("Error: OperationMap not initialized\n");
    return result;
  }

  unsigned int index = hash(key);
  HashItem *item = OperationMap.items[index];

  if (item != NULL && item->is_valid && strcmp(item->key, key) == 0) {
    result.value = item->value;
    result.found = true;
  }

  return result;
}

bool is_initialized() { return initialized; }
