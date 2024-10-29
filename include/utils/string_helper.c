#include "string_helper.h"

void copy_last_128_bits(const char *source, char *destination) {
  // Assuming the source string is null-terminated and at least 256 bits (32
  // bytes) long
  size_t source_length = strlen(source);

  // Check if the source is long enough
  if (source_length >= 32) {
    // Copy the last 16 bytes (128 bits) from source to destination
    memcpy(destination, source + source_length - 16, 16);
    destination[16] = '\0'; // Null-terminate the destination string
  } else {
    // Handle the case where the source string is too short
    /* destination[0] = '\0'; */
  }
}
