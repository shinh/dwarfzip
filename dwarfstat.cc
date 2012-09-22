#include <dwarf.h>
#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include <vector>

#include "binary.h"
#include "dwarfstr.h"
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

class StatScanner : public Scanner {
public:
  explicit StatScanner(Binary* binary)
    : Scanner(binary),
      names_(0x4000),
      forms_(256),
      ref4_names_(0x4000),
      ref4_sdata_names_(0x4000),
      ref4_udata_names_(0x4000),
      last_offset_(0) {
  }

  void show() const {
    printf("total size: %lu\n", last_offset_);
    printf("CU: %d %lu\n", cu_.cnt, cu_.size);
    printf("abbrev: %d %lu\n", abbrev_.cnt, abbrev_.size);
    printf("attr: %d %lu\n", attr_.cnt, attr_.size);

    for (size_t i = 0; i < names_.size(); i++) {
      if (names_[i].cnt)
        printf("%s: %d %lu\n",
               DW_AT_STR(i), names_[i].cnt, names_[i].size);
    }
    for (size_t i = 0; i < forms_.size(); i++) {
      if (forms_[i].cnt)
        printf("%s: %d %lu\n",
               DW_FORM_STR(i), forms_[i].cnt, forms_[i].size);
    }

    for (size_t i = 0; i < ref4_names_.size(); i++) {
      if (ref4_names_[i].cnt) {
        printf("ref4_%s: %d %lu\n",
               DW_AT_STR(i), ref4_names_[i].cnt, ref4_names_[i].size);
        printf("ref4_sdata_%s: %d %lu\n",
               DW_AT_STR(i),
               ref4_sdata_names_[i].cnt, ref4_sdata_names_[i].size);
        printf("ref4_udata_%s: %d %lu\n",
               DW_AT_STR(i),
               ref4_udata_names_[i].cnt, ref4_udata_names_[i].size);
      }
    }
  }

private:
  struct Stat {
    int cnt;
    size_t size;

    Stat() : cnt(0), size(0) {}

    void add(size_t s) {
      cnt++;
      size += s;
    }
  };

  virtual void onCU(CU* cu, uint64_t offset) {
    fprintf(stderr, "CU: %d @0x%lx len=%x version=%x ptrsize=%x\n",
            cu_.cnt, last_offset_, cu->length, cu->version, cu->ptrsize);

    cu_.add(offset - last_offset_);
    last_offset_ = offset;
  }

  virtual void onAbbrev(uint64_t, uint64_t offset) {
    //fprintf(stderr, "abbr %lu @%lx\n", number, last_offset_);
    abbrev_.add(offset - last_offset_);
    last_offset_ = offset;
  }

  virtual void onAttr(uint16_t name, uint8_t form,
                      uint64_t value, uint64_t offset) {
    //fprintf(stderr, "attr %d %d @%lx\n", name, form, last_offset_);
    int64_t size = offset - last_offset_;
    attr_.add(size);
    names_[name].add(size);
    forms_[form].add(size);

    if (form == DW_FORM_ref4) {
      ref4_names_[name].add(size);
      uint8_t buf[10];
      uint8_t* p;

      p = buf;
      sleb128o(value, p);
      ref4_sdata_names_[name].add(p - buf);

      p = buf;
      uleb128o(value, p);
      ref4_udata_names_[name].add(p - buf);
    }

    last_offset_ = offset;
  }

  Stat cu_;
  Stat abbrev_;
  Stat attr_;
  vector<Stat> names_;
  vector<Stat> forms_;
  vector<Stat> ref4_names_;
  vector<Stat> ref4_sdata_names_;
  vector<Stat> ref4_udata_names_;

  uint64_t last_offset_;
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s binary\n", argv[0]);
    exit(1);
  }

  initDwarfStr();

  auto_ptr<Binary> binary(readBinary(argv[1]));
  StatScanner stat(binary.get());
  stat.run();
  fflush(stderr);
  stat.show();
}
