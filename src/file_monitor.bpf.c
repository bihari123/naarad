
// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright (c) 2023 Tarun */
#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#define MAX_PATH_LENGTH 256
#define TASK_COMM_LEN 16

#ifndef MAY_READ
#define MAY_READ 0x01
#endif

#ifndef MAY_WRITE
#define MAY_WRITE 0x02
#endif

struct data_t {
  __u32 pid;
  __u32 uid;
  __u32 gid;
  __u64 timestamp;
  char comm[TASK_COMM_LEN];
  char filename[MAX_PATH_LENGTH];
  char new_filename[MAX_PATH_LENGTH];
  char operation[16];
};

struct {
  __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
  __uint(key_size, sizeof(u32));
  __uint(value_size, sizeof(u32));
} events SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 1024);
  __type(key, u64);
  __type(value, struct data_t);
} inode_cache SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
  __uint(max_entries, 1);
  __type(key, u32);
  __type(value, struct data_t);
} data_array SEC(".maps");

const volatile __u64 target_inode = 0;

static __always_inline int is_target_dir(struct inode *dir) {
  return (BPF_CORE_READ(dir, i_ino) == target_inode);
}

static __always_inline void submit_event(void *ctx, struct data_t *data) {
  data->pid = bpf_get_current_pid_tgid() >> 32;
  data->timestamp = bpf_ktime_get_ns();
  __u64 uid_gid = bpf_get_current_uid_gid();
  data->uid = uid_gid & 0xFFFFFFFF;
  data->gid = (uid_gid >> 32) & 0xFFFFFFFF;
  bpf_get_current_comm(&data->comm, sizeof(data->comm));
  bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, data, sizeof(*data));
}

SEC("lsm/file_open")
int BPF_PROG(file_open, struct file *file) {
  if (!is_target_dir(BPF_CORE_READ(file, f_path.dentry->d_parent->d_inode))) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));

  if (file->f_path.dentry && file->f_path.mnt) {
    bpf_d_path(&file->f_path, data->filename, sizeof(data->filename));
    u64 ino = BPF_CORE_READ(file, f_inode, i_ino);
    bpf_map_update_elem(&inode_cache, &ino, data, BPF_ANY);
  }

  __builtin_memcpy(data->operation, "open", sizeof(data->operation));
  submit_event(ctx, data);

  return 0;
}

SEC("lsm/file_permission")
int BPF_PROG(file_permission, struct file *file, int mask) {
  if (!is_target_dir(BPF_CORE_READ(file, f_path.dentry->d_parent->d_inode))) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));

  u64 ino = BPF_CORE_READ(file, f_inode, i_ino);
  struct data_t *cached = bpf_map_lookup_elem(&inode_cache, &ino);
  if (cached) {
    __builtin_memcpy(data->filename, cached->filename, sizeof(data->filename));
  }

  if (mask & MAY_READ) {
    __builtin_memcpy(data->operation, "read", sizeof(data->operation));
    submit_event(ctx, data);
  }
  if (mask & MAY_WRITE) {
    __builtin_memcpy(data->operation, "write", sizeof(data->operation));
    submit_event(ctx, data);
  }

  return 0;
}

SEC("lsm/inode_create")
int BPF_PROG(inode_create, struct inode *dir, struct dentry *dentry,
             umode_t mode) {
  if (!is_target_dir(dir)) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));
  bpf_probe_read_str(data->filename, sizeof(data->filename),
                     BPF_CORE_READ(dentry, d_name.name));
  __builtin_memcpy(data->operation, "create", sizeof(data->operation));
  submit_event(ctx, data);
  return 0;
}

SEC("lsm/inode_link")
int BPF_PROG(inode_link, struct dentry *old_dentry, struct inode *dir,
             struct dentry *new_dentry) {
  if (!is_target_dir(dir)) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));
  bpf_probe_read_str(data->filename, sizeof(data->filename),
                     BPF_CORE_READ(new_dentry, d_name.name));
  __builtin_memcpy(data->operation, "link", sizeof(data->operation));
  submit_event(ctx, data);
  return 0;
}

SEC("lsm/inode_unlink")
int BPF_PROG(inode_unlink, struct inode *dir, struct dentry *dentry) {
  if (!is_target_dir(dir)) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));
  bpf_probe_read_str(data->filename, sizeof(data->filename),
                     BPF_CORE_READ(dentry, d_name.name));
  __builtin_memcpy(data->operation, "unlink", sizeof(data->operation));
  submit_event(ctx, data);
  return 0;
}

SEC("lsm/inode_symlink")
int BPF_PROG(inode_symlink, struct inode *dir, struct dentry *dentry,
             const char *old_name) {
  if (!is_target_dir(dir)) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));
  bpf_probe_read_str(data->filename, sizeof(data->filename),
                     BPF_CORE_READ(dentry, d_name.name));
  __builtin_memcpy(data->operation, "symlink", sizeof(data->operation));
  submit_event(ctx, data);
  return 0;
}

SEC("lsm/inode_mkdir")
int BPF_PROG(inode_mkdir, struct inode *dir, struct dentry *dentry,
             umode_t mode) {
  if (!is_target_dir(dir)) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));
  bpf_probe_read_str(data->filename, sizeof(data->filename),
                     BPF_CORE_READ(dentry, d_name.name));
  __builtin_memcpy(data->operation, "mkdir", sizeof(data->operation));
  submit_event(ctx, data);
  return 0;
}

SEC("lsm/inode_rmdir")
int BPF_PROG(inode_rmdir, struct inode *dir, struct dentry *dentry) {
  if (!is_target_dir(dir)) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));
  bpf_probe_read_str(data->filename, sizeof(data->filename),
                     BPF_CORE_READ(dentry, d_name.name));
  __builtin_memcpy(data->operation, "rmdir", sizeof(data->operation));
  submit_event(ctx, data);
  return 0;
}

SEC("lsm/inode_mknod")
int BPF_PROG(inode_mknod, struct inode *dir, struct dentry *dentry,
             umode_t mode, dev_t dev) {
  if (!is_target_dir(dir)) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));
  bpf_probe_read_str(data->filename, sizeof(data->filename),
                     BPF_CORE_READ(dentry, d_name.name));
  __builtin_memcpy(data->operation, "mknod", sizeof(data->operation));
  submit_event(ctx, data);
  return 0;
}

SEC("lsm/inode_rename")
int BPF_PROG(inode_rename, struct inode *old_dir, struct dentry *old_dentry,
             struct inode *new_dir, struct dentry *new_dentry,
             unsigned int flags) {
  if (!is_target_dir(old_dir) && !is_target_dir(new_dir)) {
    return 0;
  }

  u32 zero = 0;
  struct data_t *data = bpf_map_lookup_elem(&data_array, &zero);
  if (!data)
    return 0;

  __builtin_memset(data, 0, sizeof(*data));
  bpf_probe_read_str(data->filename, sizeof(data->filename),
                     BPF_CORE_READ(old_dentry, d_name.name));
  bpf_probe_read_str(data->new_filename, sizeof(data->new_filename),
                     BPF_CORE_READ(new_dentry, d_name.name));
  __builtin_memcpy(data->operation, "rename", sizeof(data->operation));
  submit_event(ctx, data);
  return 0;
}

char LICENSE[] SEC("license") = "GPL";
