// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
/* Copyright (c) 2023 Tarun */
#include "../include/message_queue/message.h"
#include "../include/userspace/ebpf.h"
#include "../include/userspace/event.h"
#include "../include/utils/log_init.h"
#include "file_monitor.skel.h"
#include <bpf/libbpf.h>
#include <errno.h>
#include <grp.h>
#include <mqueue.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stop;

static void sig_int(int signo) { stop = 1; }

int run_main_loop(struct perf_buffer *pb) {
  int err = 0;
  /* printf("File monitoring started for directory: %s\n", target_dir); */
  printf("Ctrl+C to exit.\n");

  while (!stop) {
    err = perf_buffer__poll(pb, 100);
    if (err < 0 && err != -EINTR) {
      fprintf(stderr, "Error polling perf buffer: %s\n", strerror(-err));
      break;
    }
  }
  return err;
}
void cleanup(struct file_monitor_bpf *skel, struct perf_buffer *pb) {
  if (pb)
    perf_buffer__free(pb);
  if (skel)
    file_monitor_bpf__destroy(skel);
}

int main(int argc, char **argv) {

  if (msg_q_init() != 0) {
    perror("msg_q_init");
    return 1;
  }

  if (init_pipe() != 0) {
    perror("pipe_init");
    return 1;
  }

  if (init_consumer() != 0) {
    perror("init_consumer");
    return 1;
  }
  if (init_dispatcher() != 0) {
    perror("init_dispathcer");
    return 1;
  }

  printf("dispatcher to parent process closed\n");
  // Close all pipes in the parent process
  for (int i = 0; i < NUM_GROUPS; i++) {
    for (int j = 0; j < NUM_CONSUMERS_PER_GROUP; j++) {
      close(dispatcher_to_consumer[i][j][0]);
      close(dispatcher_to_consumer[i][j][1]);
      close(consumer_to_dispatcher[i][j][0]);
      close(consumer_to_dispatcher[i][j][1]);
    }
  }

  struct file_monitor_bpf *skel;
  struct perf_buffer *pb = NULL;
  int err;

  if (setup_logging() != 0) {
    return 1;
  }

  skel = setup_and_load_bpf();
  if (!skel) {
    return 1;
  }

  pb = setup_perf_buffer(skel);
  if (!pb) {
    err = 1;
    goto cleanup;
  }

  if (signal(SIGINT, sig_int) == SIG_ERR) {
    fprintf(stderr, "can't set signal handler: %s\n", strerror(errno));
    err = 1;
    goto cleanup;
  }

  err = run_main_loop(pb);

cleanup:
  cleanup(skel, pb);
  return err != 0;
}
