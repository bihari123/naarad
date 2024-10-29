#pragma once

#include "../utils/logs.h"
#include <mqueue.h>
#include <stdbool.h>
#include <stdint.h>

#define QUEUE_NAME "/test_queue"
// #define MAX_SIZE 1024
#define MSG_STOP "exit"
#define MSG_ACK "ack"
#define NUM_GROUPS 4
#define NUM_CONSUMERS_PER_GROUP 2
#define NUM_CONSUMERS (NUM_GROUPS * NUM_CONSUMERS_PER_GROUP)
#define MSGS_PER_PRODUCER 10
#define MAX_GROUP_NAME 20
#define MAX_RETRIES 5
#define RETRY_DELAY_US 100000 // 100 ms
#define MAX_SIZE 1024

enum MessageType {
  FILE_ACTIVITY_MONITORING = 0,
  DATABASE_MONITORING = 1,
  FILE_CLASSIFICATION = 2,
  DATABASE_CLASSIFICATION = 3,
  ACK_MESSAGE = 253,
  STOP_MESSAG = 254
};

extern int dispatcher_to_consumer[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP][2];
extern int consumer_to_dispatcher[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP][2];
extern pid_t pids[NUM_GROUPS + NUM_CONSUMERS + 1]; // +1 for dispatcher

typedef struct {
  uint32_t gid;
  uint32_t uid;
  uint8_t category;
  uint8_t op;
  uint64_t timestamp;
  char comm[16];
  char file[128];
  char new_file[128];
} Message;

int msg_q_init();
mqd_t get_message_queue();

void dispatcher(int consumer_read_pipes[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP],
                int consumer_write_pipes[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP]);

int send_with_retry(const char *msg_ptr, size_t msg_len, unsigned int msg_prio);
int init_pipe();
int init_consumer();
int init_dispatcher();
int pass_message(Message msg);
