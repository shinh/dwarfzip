#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include <vector>

#include "binary.h"
#include "dwarfstr.h"
#include "scanner.h"

using namespace std;

class StatScanner : public Scanner {
public:
  explicit StatScanner(Binary* binary)
    : Scanner(binary),
      names_(0x4000),
      forms_(256),
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

  virtual void onAttr(uint16_t name, uint8_t form, uint64_t, uint64_t offset) {
    //fprintf(stderr, "attr %d %d @%lx\n", name, form, last_offset_);
    attr_.add(offset - last_offset_);
    names_[name].add(offset - last_offset_);
    forms_[form].add(offset - last_offset_);

    last_offset_ = offset;
  }

  Stat cu_;
  Stat abbrev_;
  Stat attr_;
  vector<Stat> names_;
  vector<Stat> forms_;

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
