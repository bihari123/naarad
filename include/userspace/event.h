#pragma once
#include "file_monitor.skel.h"

#include <bpf/libbpf.h>
struct data_t {
  __u32 pid;
  __u32 uid;
  __u32 gid;
  __u64 timestamp;
  char comm[16];
  char filename[256];
  char new_filename[256];
  char operation[16];
};
