#ifndef BINARY_H_
#define BINARY_H_

class Binary {
public:
  const char* debug_info;
  const char* debug_abbrev;
  const char* debug_str;
};

Binary* readBinary(const char* filename);

#endif  // BINARY_H_
