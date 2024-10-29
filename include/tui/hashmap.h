#pragma once

#include <stdbool.h>
#include <stdint.h>

#define TABLE_SIZE 100
#define OP_UNKNOWN 255
/*
 * insert("open", 0);
    insert("read", 1);
    insert("write", 2);
    insert("create", 3);
    insert("link", 4);
    insert("unlink", 5);
    insert("syslink", 6);
    insert("mkdir", 7);
    insert("rmdir", 8);
    insert("mknod", 9);
    insert("rename", OP_RENAME);
* */
#define OP_OPEN 0
#define OP_READ 1
#define OP_WRITE 2
#define OP_CREATE 3
#define OP_LINK 4
#define OP_UNLINK 5
#define OP_SYSLINK 6
#define OP_MKDIR 7
#define OP_RMDIR 8
#define OP_MKNOD 9
#define OP_RENAME 10

typedef struct HashItem HashItem;

typedef struct {
  HashItem *items[TABLE_SIZE];
} HashMap;

typedef struct {
  uint8_t value;
  bool found;
} HashMapResult;

// Global HashMap variable
extern HashMap OperationMap;

// Function to initialize the OperationMap
void init_operation_map();

// Function to clean up the OperationMap
void cleanup_operation_map();

void insert(const char *key, uint8_t value);
HashMapResult get(const char *key);
bool is_initialized();
