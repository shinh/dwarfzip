#include "binary.h"

#include <elf.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr

class ELFBinary : public Binary {
public:
  explicit ELFBinary(const char* filename) {
    fd_ = open(filename, O_RDONLY);
    if (fd_ < 0)
      err(1, "open failed: %s", filename);

    size_ = lseek(fd_, 0, SEEK_END);
    size_ = (size_ + 0xfff) & ~0xfff;

    char* p = (char*)mmap(NULL, size_,
                          PROT_READ, MAP_SHARED,
                          fd_, 0);
    if (p == MAP_FAILED)
      err(1, "mmap failed: %s", filename);

    mapped_ = p;

    Elf_Ehdr* ehdr = (Elf_Ehdr*)p;
    if (!ehdr->e_shoff || !ehdr->e_shnum)
      err(1, "no section header: %s", filename);
    if (!ehdr->e_shstrndx)
      err(1, "no section name: %s", filename);

    Elf_Shdr* shdr = (Elf_Shdr*)(p + ehdr->e_shoff);
    const char* shstr = (const char*)(p + shdr[ehdr->e_shstrndx].sh_offset);
    for (int i = 0; i < ehdr->e_shnum; i++) {
      Elf_Shdr* sec = shdr + i;
      if (!strcmp(shstr + sec->sh_name, ".debug_info")) {
        debug_info = p + sec->sh_offset;
      } else if (!strcmp(shstr + sec->sh_name, ".debug_abbrev")) {
        debug_abbrev = p + sec->sh_offset;
      } else if (!strcmp(shstr + sec->sh_name, ".debug_str")) {
        debug_str = p + sec->sh_offset;
      }
    }

    if (!debug_info || !debug_abbrev || !debug_str)
      err(1, "no debug info: %s", filename);
  }

  ~ELFBinary() {
    munmap(mapped_, size_);
    close(fd_);
  }

private:
  int fd_;
  off_t size_;
  char* mapped_;
};

Binary* readBinary(const char* filename) {
  return new ELFBinary(filename);
}
