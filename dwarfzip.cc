#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <vector>

#include "binary.h"
#include "scanner.h"

using namespace std;

static void uleb128o(uint64_t v, uint8_t*& p) {
  do {
    *p++ = v & 0x7f;
    v >>= 7;
  } while (v);
}

class ZipScanner : public Scanner {
public:
  ZipScanner(Binary* binary, uint8_t* out)
    : Scanner(binary),
      p_(out),
      last_offset_(0),
      cu_cnt_(0) {
  }

  const uint8_t* cur() const {
    return p_;
  }

private:
  virtual void onCU(CU* cu, uint64_t offset) {
    fprintf(stderr, "CU: %d @0x%lx len=%x version=%x ptrsize=%x\n",
            cu_cnt_, last_offset_, cu->length, cu->version, cu->ptrsize);

    memcpy(p_, cu, sizeof(CU));
    p_ += sizeof(CU);

    cu_cnt_++;
    last_offset_ = offset;
  }

  virtual void onAbbrev(uint64_t number, uint64_t offset) {
    uleb128o(number, p_);

    //fprintf(stderr, "abbr %lu @%lx\n", number, last_offset_);
    last_offset_ = offset;
  }

  virtual void onAttr(uint16_t, uint8_t, uint64_t, uint64_t offset) {
    size_t sz = offset - last_offset_;
    memcpy(p_, binary_->debug_info + last_offset_, sz);
    p_ += sz;

    //fprintf(stderr, "attr %d %d @%lx\n", name, form, last_offset_);

    last_offset_ = offset;
  }

  uint8_t* p_;
  uint64_t last_offset_;
  int cu_cnt_;
};

int main(int argc, char* argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s binary output\n", argv[0]);
    exit(1);
  }

  auto_ptr<Binary> binary(readBinary(argv[1]));

  int fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    err(1, "open failed: %s", argv[1]);

  size_t debug_info_offset = binary->debug_info - binary->head;
  if (write(fd, binary->head, debug_info_offset) < 0)
    err(1, "write failed");

  if (pwrite(fd, "", 1, binary->size - 1) < 0)
    err(1, "pwrite failed");

  uint8_t* p = (uint8_t*)mmap(NULL, binary->mapped_size,
                              PROT_READ | PROT_WRITE, MAP_SHARED,
                              fd, 0);
  if (p == MAP_FAILED)
    err(1, "mmap failed");

  ZipScanner zip(binary.get(), p + debug_info_offset);
  zip.run();
  fflush(stderr);

  munmap(p, binary->size);

  size_t out_size = zip.cur() - p;
  if (lseek(fd, out_size, SEEK_SET) < 0)
    err(1, "lseek failed");

  const char* rest = binary->debug_info + binary->debug_info_len;
  size_t rest_size = binary->size - (rest - binary->head);
  if (write(fd, rest, rest_size) < 0)
    err(1, "write failed");

  out_size += rest_size;
  if (ftruncate(fd, out_size) < 0)
    err(1, "ftruncate failed");

  close(fd);

  printf("%lu => %lu\n", binary->size, out_size);
}
