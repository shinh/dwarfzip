#include <dwarf.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <memory>
#include <vector>

#include "binary.h"
#include "scanner.h"

using namespace std;

static void uleb128o(uint64_t v, uint8_t*& p) {
  do {
    uint8_t b = v & 0x7f;
    v >>= 7;
    if (v)
      b |= 0x80;
    *p++ = b;
  } while (v);
}

static void sleb128o(int64_t v, uint8_t*& p) {
  bool done = false;
  while (!done) {
    uint8_t b = v & 0x7f;
    v >>= 7;
    if ((v == 0 && (b & 0x40) == 0) || (v == -1 && (b & 0x40))) {
      done = true;
    } else {
      b |= 0x80;
    }
    *p++ = b;
  }
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

    last_values_.clear();

    cu_cnt_++;
    last_offset_ = offset;
  }

  virtual void onAbbrev(uint64_t number, uint64_t offset) {
    uleb128o(number, p_);

    //fprintf(stderr, "abbr %lu @%lx\n", number, last_offset_);
    last_offset_ = offset;
  }

  virtual void onAttr(uint16_t name, uint8_t form, uint64_t value,
                      uint64_t offset) {
    switch (form) {
    case DW_FORM_strp:
    case DW_FORM_data4:
    case DW_FORM_ref4: {
      int32_t v = static_cast<int32_t>(value);
      map<int, uint64_t>::iterator iter =
        last_values_.insert(make_pair(name, 0)).first;
      int32_t diff = v - static_cast<int32_t>(iter->second);
      sleb128o(diff, p_);
      iter->second = v;
      break;
    }

    default: {
      size_t sz = offset - last_offset_;
      memcpy(p_, binary_->debug_info + last_offset_, sz);
      p_ += sz;
    }
    }

    //fprintf(stderr, "attr %d %d @%lx\n", name, form, last_offset_);

    last_offset_ = offset;
  }

  uint8_t* p_;
  uint64_t last_offset_;
  int cu_cnt_;
  map<int, uint64_t> last_values_;
};

static const int HEADER_SIZE = 8;

int main(int argc, char* argv[]) {
  const char* argv0 = argv[0];
  bool opt_d = false;
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      continue;
    }
    if (!strcmp(argv[i], "-d")) {
      opt_d = true;
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
    }
    argc--;
    argv++;
  }

  if (argc < 3) {
    fprintf(stderr, "Usage: %s [-d] binary output\n", argv0);
    exit(1);
  }

  auto_ptr<Binary> binary(readBinary(argv[1]));

  if (opt_d && !binary->is_zipped) {
    fprintf(stderr, "%s is not compressed\n", argv[1]);
    exit(1);
  } else if (!opt_d && binary->is_zipped) {
    fprintf(stderr, "%s is already compressed\n", argv[1]);
    exit(1);
  }

  int fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    err(1, "open failed: %s", argv[1]);

  if (write(fd, "\xdfZIP\0\0\0\0", HEADER_SIZE) < 0)
    err(1, "write failed");

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

  ZipScanner zip(binary.get(), p + debug_info_offset + HEADER_SIZE);
  zip.run();
  fflush(stderr);

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

  uint32_t* offset_outp = (uint32_t*)(p + 4);
  *offset_outp = binary->size - out_size + HEADER_SIZE;

  munmap(p, binary->size);
  close(fd);

  printf("%lu => %lu (%.2f%%)\n",
         binary->size, out_size,
         ((float)out_size / binary->size) * 100);
}
