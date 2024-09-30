#pragma once

#include "message.h"

void consumer(int id, const char *group, int read_fd, int write_fd);
