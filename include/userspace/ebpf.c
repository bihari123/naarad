
#include "ebpf.h"
#include "../message_queue/message.h"
#include "../message_queue/producer.h"
#include "../tui/hashmap.h"
#include "../tui/lock_free_queue.h"
#include "../tui/tui.h"
#include "../utils/string_helper.h"
#include <stdio.h>
static const char *target_dir = "/media/tarun/ITB/target";

static int libbpf_print_fn(enum libbpf_print_level level, const char *format,
                           va_list args) {
  return vfprintf(stderr, format, args);
}
void handle_event(void *ctx, int cpu, void *data, __u32 data_sz) {
  const struct data_t *e = data;
  /*
  struct tm *tm;
  char ts[32];
  time_t t;

  struct passwd *pw;
  struct group *gr;
  time(&t);
  tm = localtime(&t);
  strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm);

  pw = getpwuid(e->uid);
  gr = getgrgid(e->gid);

  printf("Time: %s\n", ts);
  printf("PID: %u, Command: %s\n", e->pid, e->comm);
  printf("User: %s (UID: %u), Group: %s (GID: %u)\n",
         pw ? pw->pw_name : "UNKNOWN", e->uid, gr ? gr->gr_name : "UNKNOWN",
         e->gid);
  printf("Operation: %s\n", e->operation);
  printf("File: %s\n", e->filename);

  if (strcmp(e->operation, "rename") == 0) {
    printf("New File: %s\n", e->new_filename);
  }

  printf("------------------------\n");
*/
  Message msg;
  msg.gid = e->gid;
  msg.uid = e->uid;
  msg.category = FILE_ACTIVITY_MONITORING;
  log_info("the operations is %s", e->operation);
  HashMapResult result = get(e->operation);
  log_info("the operation code is %d", result.value);
  if (result.found) {

    if (result.value == OP_RENAME) {
      snprintf(msg.new_file, sizeof(msg.new_file), "%s", e->new_filename);
    }
    msg.op = result.value;
  } else {
    log_info("operation %s not found in the hashmap", e->operation);
    msg.op = OP_UNKNOWN;
  }

  log_info("the operation code for message is %d", msg.op);
  snprintf(msg.comm, sizeof(msg.comm), "%s", e->comm);
  snprintf(msg.file, sizeof(msg.file), "%s", e->filename);
  increment_counter(msg.op);
  pushRear(msg);
  producer(FILE_ACTIVITY_MONITORING, msg);
}

struct file_monitor_bpf *setup_and_load_bpf(void) {
  struct file_monitor_bpf *skel;
  int err;

  libbpf_set_strict_mode(LIBBPF_STRICT_ALL);
  libbpf_set_print(libbpf_print_fn);

  skel = file_monitor_bpf__open();
  if (!skel) {
    fprintf(stderr, "Failed to open BPF skeleton\n");
    return NULL;
  }

  struct stat st;
  if (stat(target_dir, &st) == -1) {
    fprintf(stderr, "Failed to get directory inode: %s\n", strerror(errno));
    file_monitor_bpf__destroy(skel);
    return NULL;
  }
  skel->rodata->target_inode = st.st_ino;

  err = file_monitor_bpf__load(skel);
  if (err) {
    fprintf(stderr, "Failed to load and verify BPF skeleton\n");
    file_monitor_bpf__destroy(skel);
    return NULL;
  }

  err = file_monitor_bpf__attach(skel);
  if (err) {
    fprintf(stderr, "Failed to attach BPF skeleton\n");
    file_monitor_bpf__destroy(skel);
    return NULL;
  }

  return skel;
}

struct perf_buffer *setup_perf_buffer(struct file_monitor_bpf *skel) {
  struct perf_buffer *pb = perf_buffer__new(bpf_map__fd(skel->maps.events), 64,
                                            handle_event, NULL, NULL, NULL);
  if (libbpf_get_error(pb)) {
    fprintf(stderr, "Failed to open perf buffer\n");
    return NULL;
  }
  return pb;
}
