#include "producer.h"
#include <unistd.h>
void producer(int group_index, Message msg) {

  if (send_with_retry((char *)&msg, sizeof(Message), 0) == -1) {
    /* fprintf(stderr, "Producer %s failed to send message\n", msg.group); */
    fprintf(stderr, "Producer %s failed to send message\n", "ss");
  }
}
