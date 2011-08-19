#include "binary.h"

#include <elf.h>
#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr

Binary::Binary()
  : head(NULL),
    size(0),
    mapped_head(NULL),
    mapped_size(0),
    debug_info(NULL),
    debug_abbrev(NULL),
    debug_str(NULL),
    debug_info_len(0),
    debug_abbrev_len(0),
    debug_str_len(0),
    is_zipped(false),
    reduced_size(0) {
}

class ELFBinary : public Binary {
public:
  explicit ELFBinary(const char* filename) {
    fd_ = open(filename, O_RDONLY);
    if (fd_ < 0)
      err(1, "open failed: %s", filename);

    size = lseek(fd_, 0, SEEK_END);
    mapped_size = (size + 0xfff) & ~0xfff;

    char* p = (char*)mmap(NULL, mapped_size,
                          PROT_READ, MAP_SHARED,
                          fd_, 0);
    if (p == MAP_FAILED)
      err(1, "mmap failed: %s", filename);

    mapped_head = p;

    reduced_size = 0;
    if (!strncmp(p, "\xdfZIP", 4)) {
      is_zipped = true;
      reduced_size = *(uint32_t*)(p + 4);
      p += 8;
    }

    head = p;

    if (strncmp(p, ELFMAG, SELFMAG)) {
      err(1, "not ELF: %s", filename);
    }

    if (p[EI_CLASS] != ELFCLASS64) {
      err(1, "not 64bit: %s", filename);
    }

    Elf_Ehdr* ehdr = (Elf_Ehdr*)p;
    if (!ehdr->e_shoff || !ehdr->e_shnum)
      err(1, "no section header: %s", filename);
    if (!ehdr->e_shstrndx)
      err(1, "no section name: %s", filename);

    Elf_Shdr* shdr = (Elf_Shdr*)(p + ehdr->e_shoff - reduced_size);
    const char* shstr = (const char*)(p + shdr[ehdr->e_shstrndx].sh_offset);
    shstr -= reduced_size;
    bool debug_info_seen = false;
    for (int i = 0; i < ehdr->e_shnum; i++) {
      Elf_Shdr* sec = shdr + i;
      const char* pos = p + sec->sh_offset;
      if (debug_info_seen)
        pos -= reduced_size;
      size_t sz = sec->sh_size;
      if (!strcmp(shstr + sec->sh_name, ".debug_info")) {
        debug_info = pos;
        debug_info_len = sz - reduced_size;
        debug_info_seen = true;
      } else if (!strcmp(shstr + sec->sh_name, ".debug_abbrev")) {
        debug_abbrev = pos;
        debug_abbrev_len = sz;
      } else if (!strcmp(shstr + sec->sh_name, ".debug_str")) {
        debug_str = pos;
        debug_str_len = sz;
      }
    }

    if (!debug_info || !debug_abbrev || !debug_str)
      err(1, "no debug info: %s", filename);
  }

  ~ELFBinary() {
    munmap(head, mapped_size);
    close(fd_);
  }

private:
  int fd_;
};

Binary* readBinary(const char* filename) {
  return new ELFBinary(filename);
}
