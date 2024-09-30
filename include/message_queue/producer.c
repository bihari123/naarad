#include "producer.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
void producer(int group_index, Message msg) {
  /* srand(time(NULL) ^ (group_index << 16)); */

  /* for (int i = 0; i < MSGS_PER_PRODUCER; i++) { */

  if (send_with_retry((char *)&msg, sizeof(Message), 0) == -1) {
    fprintf(stderr, "Producer %s failed to send message\n", msg.group);
  }
  usleep(rand() % 1000000); // Sleep up to 1 second
  /* } */
}
