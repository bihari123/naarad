#include "consumer.h"
#include <string.h>
#include <unistd.h>

void consumer(int id, const char *group, int read_fd, int write_fd) {
  Message msg;
  Message ack_msg;
  int must_stop = 0;

  log_info("Consumer %d started (Group %s)\n", id, group);

  while (!must_stop) {
    ssize_t bytes_read = read(read_fd, &msg, sizeof(Message));

    if (bytes_read > 0) {
      if (!strncmp(msg.text, MSG_STOP, strlen(MSG_STOP))) {
        must_stop = 1;
        log_info("Consumer %d (Group %s) received stop message\n", id, group);
      } else {
        /* log_trace("Consumer %d (Group %s) received: %s\n", id, group,
         * msg.text); */
        printf("Consumer %d (Group %s) received: %s\n", id, group, msg.text);
      }

      // Send acknowledgement
      strncpy(ack_msg.group, group, MAX_GROUP_NAME);
      strncpy(ack_msg.text, MSG_ACK, sizeof(ack_msg.text));
      if (write(write_fd, &ack_msg, sizeof(Message)) == -1) {
        perror("write ack message");
      }
    } else if (bytes_read == 0) {
      printf("Consumer %d (Group %s) pipe closed\n", id, group);
      break;
    } else {
      perror("read");
      break;
    }
  }

  close(read_fd);
  close(write_fd);
}
