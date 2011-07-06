#include <stdio.h>
#include <stdlib.h>

#include <memory>

#include "binary.h"

using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s binary\n", argv[0]);
    exit(1);
  }

  auto_ptr<Binary> binary(readBinary(argv[1]));
}
