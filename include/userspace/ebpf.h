#pragma once
#include "event.h"
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

struct file_monitor_bpf *setup_and_load_bpf(void);
struct perf_buffer *setup_perf_buffer(struct file_monitor_bpf *skel);
