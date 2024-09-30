#include "message.h"
#include "consumer.h"
#include "producer.h"
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *GROUP_NAMES[NUM_GROUPS] = {"file_activity", "database_activity",
                                       "file_classification",
                                       "database_classification"};

mqd_t MQ;
int dispatcher_to_consumer[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP][2];
int consumer_to_dispatcher[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP][2];
pid_t pids[NUM_GROUPS + NUM_CONSUMERS + 1]; // +1 for dispatcher

int msg_q_init() {
  MQ = get_message_queue();
  if (MQ == (mqd_t)-1) {
    return -1;
  }
  return 0;
}

mqd_t get_message_queue() {
  struct mq_attr attr;
  attr.mq_flags = 0;
  attr.mq_maxmsg = 10;
  attr.mq_msgsize = sizeof(Message);
  attr.mq_curmsgs = 0;
  return mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0644, &attr);
}

int init_pipe() {
  // Create pipes for communication between dispatcher and consumers
  for (int i = 0; i < NUM_GROUPS; i++) {
    for (int j = 0; j < NUM_CONSUMERS_PER_GROUP; j++) {
      if (pipe(dispatcher_to_consumer[i][j]) == -1 ||
          pipe(consumer_to_dispatcher[i][j]) == -1) {
        perror("pipe");
        return -1;
        ;
      }
    }
  }
  return 0;
}

int init_consumer() {
  // Create consumer processes
  int consumer_count = 0;
  for (int i = 0; i < NUM_GROUPS; i++) {
    for (int j = 0; j < NUM_CONSUMERS_PER_GROUP; j++) {
      pids[consumer_count] = fork();
      if (pids[consumer_count] < 0) {
        perror("fork");
        return -1;
        ;
      } else if (pids[consumer_count] == 0) {
        // Child (consumer) process
        close(dispatcher_to_consumer[i][j][1]); // Close write end
        close(consumer_to_dispatcher[i][j][0]); // Close read end

        // Close all other pipes
        for (int x = 0; x < NUM_GROUPS; x++) {
          for (int y = 0; y < NUM_CONSUMERS_PER_GROUP; y++) {
            if (x != i || y != j) {
              close(dispatcher_to_consumer[x][y][0]);
              close(dispatcher_to_consumer[x][y][1]);
              close(consumer_to_dispatcher[x][y][0]);
              close(consumer_to_dispatcher[x][y][1]);
            }
          }
        }

        consumer(consumer_count, GROUP_NAMES[i],
                 dispatcher_to_consumer[i][j][0],
                 consumer_to_dispatcher[i][j][1]);
        exit(0);
      }
      close(dispatcher_to_consumer[i][j][0]); // Close read end in parent
      close(consumer_to_dispatcher[i][j][1]); // Close write end in parent
      consumer_count++;
    }
  }
  return 0;
}
int init_dispatcher() {
  // Create dispatcher process
  pids[NUM_CONSUMERS] = fork();
  if (pids[NUM_CONSUMERS] < 0) {
    perror("fork");
    return -1;
  } else if (pids[NUM_CONSUMERS] == 0) {
    // Child (dispatcher) process
    int dispatcher_read_pipes[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP];
    int dispatcher_write_pipes[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP];
    for (int i = 0; i < NUM_GROUPS; i++) {
      for (int j = 0; j < NUM_CONSUMERS_PER_GROUP; j++) {
        dispatcher_read_pipes[i][j] = consumer_to_dispatcher[i][j][0];
        dispatcher_write_pipes[i][j] = dispatcher_to_consumer[i][j][1];
      }
    }
    dispatcher(dispatcher_read_pipes, dispatcher_write_pipes);
    exit(0);
  }
  return 0;
}

void dispatcher(int consumer_read_pipes[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP],
                int consumer_write_pipes[NUM_GROUPS][NUM_CONSUMERS_PER_GROUP]) {

  if (MQ == (mqd_t)-1) {
    perror("global message queue not defined");
  }

  Message msg;
  Message ack_msg;
  int must_stop = 0;
  int current_consumer[NUM_GROUPS] = {
      0}; // Track the current consumer for each group
  fd_set read_fds;
  int max_fd = -1;

  // Find the maximum file descriptor for select()
  for (int i = 0; i < NUM_GROUPS; i++) {
    for (int j = 0; j < NUM_CONSUMERS_PER_GROUP; j++) {
      if (consumer_read_pipes[i][j] > max_fd) {
        max_fd = consumer_read_pipes[i][j];
      }
    }
  }
  // Set stdin to non-blocking mode
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  while (!must_stop) {
    ssize_t bytes_read = mq_receive(MQ, (char *)&msg, sizeof(Message), NULL);
    log_trace("got the message in dispatcher");
    if (bytes_read >= 0) {
      if (!strncmp(msg.text, MSG_STOP, strlen(MSG_STOP))) {
        must_stop = 1;
        log_trace("Dispatcher received stop message\n");
      } else {
        int group_index = -1;
        for (int i = 0; i < NUM_GROUPS; i++) {
          if (strcmp(msg.group, GROUP_NAMES[i]) == 0) {
            group_index = i;
            break;
          }
        }

        if (group_index != -1) {
          log_trace("send to the consumer");
          // Send message to the current consumer in the group
          if (write(consumer_write_pipes[group_index]
                                        [current_consumer[group_index]],
                    &msg, sizeof(Message)) == -1) {
            perror("write");
          }

          log_trace("waiting for the ack");
          // Wait for acknowledgement
          FD_ZERO(&read_fds);
          FD_SET(
              consumer_read_pipes[group_index][current_consumer[group_index]],
              &read_fds);

          struct timeval timeout;
          timeout.tv_sec = 5; // 5 seconds timeout
          timeout.tv_usec = 0;

          int ready = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
          if (ready == -1) {
            perror("select");
          } else if (ready == 0) {
            printf("Timeout waiting for acknowledgement from consumer %d in "
                   "group %s\n",
                   current_consumer[group_index], GROUP_NAMES[group_index]);
            usleep(500000);
          } else {
            if (read(consumer_read_pipes[group_index]
                                        [current_consumer[group_index]],
                     &ack_msg, sizeof(Message)) > 0) {
              if (!strncmp(ack_msg.text, MSG_ACK, strlen(MSG_ACK))) {
                printf(
                    "Received acknowledgement from consumer %d in group %s\n",
                    current_consumer[group_index], GROUP_NAMES[group_index]);
              }
            }
          }

          log_trace("move to the next consumer");
          // Move to the next consumer in the group (round-robin)
          current_consumer[group_index] =
              (current_consumer[group_index] + 1) % NUM_CONSUMERS_PER_GROUP;
          log_trace("move successful");
        }
      }
    } else if (errno != EAGAIN) {
      perror("mq_receive");
      break;
    }
  }

  // Send stop message to all consumers
  printf("Dispatcher sending stop messages to all consumers\n");
  Message stop_msg;
  strncpy(stop_msg.text, MSG_STOP, sizeof(stop_msg.text));
  for (int i = 0; i < NUM_GROUPS; i++) {
    strncpy(stop_msg.group, GROUP_NAMES[i], MAX_GROUP_NAME);
    for (int j = 0; j < NUM_CONSUMERS_PER_GROUP; j++) {
      if (write(consumer_write_pipes[i][j], &stop_msg, sizeof(Message)) == -1) {
        perror("write stop message");
      }
      // Wait for acknowledgement of stop message
      FD_ZERO(&read_fds);
      FD_SET(consumer_read_pipes[i][j], &read_fds);

      struct timeval timeout;
      timeout.tv_sec = 5; // 5 seconds timeout
      timeout.tv_usec = 0;

      int ready = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
      if (ready == -1) {
        perror("select");
      } else if (ready == 0) {
        printf("Timeout waiting for stop acknowledgement from consumer %d in "
               "group %s\n",
               j, GROUP_NAMES[i]);
      } else {
        if (read(consumer_read_pipes[i][j], &ack_msg, sizeof(Message)) > 0) {
          if (!strncmp(ack_msg.text, MSG_ACK, strlen(MSG_ACK))) {
            printf(
                "Received stop acknowledgement from consumer %d in group %s\n",
                j, GROUP_NAMES[i]);
          }
        }
      }
    }
  }

  // Close all consumer pipes
  for (int i = 0; i < NUM_GROUPS; i++) {
    for (int j = 0; j < NUM_CONSUMERS_PER_GROUP; j++) {
      close(consumer_read_pipes[i][j]);
      close(consumer_write_pipes[i][j]);
    }
  }
}

int send_with_retry(const char *msg_ptr, size_t msg_len,
                    unsigned int msg_prio) {
  log_trace("inside send_with_retry");
  if (MQ == (mqd_t)-1) {
    // global message queue is not initilized
    perror("global message queue not defined");
    return -1;
  }

  int retries = 0;
  while (retries < MAX_RETRIES) {
    log_trace("sending to the queue");
    if (mq_send(MQ, msg_ptr, msg_len, msg_prio) != -1) {
      return 0; // Success
    }

    if (errno != EAGAIN) {
      perror("mq_send");
      return -1; // Unrecoverable error
    }
    log_error("send faile..retrying");
    usleep(RETRY_DELAY_US);
    retries++;
  }
  fprintf(stderr, "Failed to send message after %d retries\n", MAX_RETRIES);
  return -1;
}

static int i = 0;
int pass_message(Message msg) {
  i++;
  pids[NUM_CONSUMERS + 1 + i] = fork();
  if (pids[NUM_CONSUMERS + 1 + i] < 0) {
    perror("fork");
    /* exit(1); */
    return 1;
  } else if (pids[NUM_CONSUMERS + 1 + i] == 0) {
    printf("inside pass message");
    // Child (producer) process
    producer(i, msg);
    exit(0);
  }
  return 0;
}
