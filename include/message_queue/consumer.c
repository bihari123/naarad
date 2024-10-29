#include "consumer.h"
#include "../tui/tui.h"
#include "message.h"
#include <unistd.h>

void consumer(int id, const char *group, int read_fd, int write_fd) {
  Message msg;
  Message ack_msg;
  int must_stop = 0;

  log_info("Consumer %d started (Group %s)\n", id, group);

  while (!must_stop) {
    ssize_t bytes_read = read(read_fd, &msg, sizeof(Message));

    if (bytes_read > 0) {
      if (msg.op == STOP_MESSAG) {
        must_stop = 1;
        log_info("Consumer %d (Group %s) received stop message\n", id, group);
      } else {
        log_info("Consumer %d (Group %s) received: %s %s %d  \n", id, group,
                 msg.file, msg.comm, msg.op);
        increment_counter(msg.op);
      }

      // Send acknowledgement
      ack_msg.op = ACK_MESSAGE;
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
