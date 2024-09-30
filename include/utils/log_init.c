#include "log_init.h"
int setup_logging(void) {
  FILE *log_file = fopen("file_activity.log", "w");
  if (log_file == NULL) {
    perror("Error opening log file");
    return 1;
  }
  // Add the log file for INFO level and above
  if (log_add_fp(log_file, LOG_TRACE) != 0) {
    fprintf(stderr, "Failed to add log file\n");
    fclose(log_file);
    return 1;
  }
  log_trace("Logging Initialized");
  return 0;
}
