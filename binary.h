#ifndef BINARY_H_
#define BINARY_H_

#include <stdio.h>

class Binary {
public:
  char* head;
  size_t size;
  size_t mapped_size;
  const char* debug_info;
  const char* debug_abbrev;
  const char* debug_str;
  size_t debug_info_len;
  size_t debug_abbrev_len;
  size_t debug_str_len;
};

Binary* readBinary(const char* filename);

#endif  // BINARY_H_
