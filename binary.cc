#include "binary.h"

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

#include <elf.h>

#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr

Binary::Binary(int fd, char* p, size_t sz, size_t msz)
  : head(NULL),
    size(sz),
    mapped_head(p),
    mapped_size(msz),
    debug_info(NULL),
    debug_abbrev(NULL),
    debug_str(NULL),
    debug_info_len(0),
    debug_abbrev_len(0),
    debug_str_len(0),
    is_zipped(false),
    reduced_size(0),
    fd_(fd) {
}

static bool isDwarfZip(char* p) {
  return !strncmp(p, "\xdfZIP", 4);
}

class ELFBinary : public Binary {
public:
  explicit ELFBinary(const char* filename,
                     int fd, char* p, size_t sz, size_t msz)
    : Binary(fd, p, sz, msz) {
    reduced_size = 0;
    if (isDwarfZip(p)) {
      is_zipped = true;
      reduced_size = *(uint32_t*)(p + 4);
      p += 8;
    }

    head = p;

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

  static bool isELF(const char* p) {
    if (strncmp(p, ELFMAG, SELFMAG)) {
      return false;
    }
    if (p[EI_CLASS] != ELFCLASS64) {
      err(1, "non 64bit ELF isn't supported yet");
    }
    return true;
  }
};

#include <mach-o/loader.h>

class MachOBinary : public Binary {
public:
  explicit MachOBinary(const char* filename,
                       int fd, char* p, size_t sz, size_t msz)
    : Binary(fd, p, sz, msz) {
    reduced_size = 0;
    if (isDwarfZip(p)) {
      is_zipped = true;
      reduced_size = *(uint32_t*)(p + 4);
      p += 8;
    }

    head = p;

    mach_header* header = reinterpret_cast<mach_header*>(p);
    p += sizeof(mach_header_64);
    struct load_command* cmds_ptr = reinterpret_cast<struct load_command*>(p);

    for (uint32_t i = 0; i < header->ncmds; i++) {
      switch (cmds_ptr->cmd) {
      case LC_SEGMENT_64: {
        segment_command_64* segment =
          reinterpret_cast<segment_command_64*>(cmds_ptr);

        section_64* sections = reinterpret_cast<section_64*>(
          reinterpret_cast<char*>(cmds_ptr) + sizeof(segment_command_64));

        for (uint32_t j = 0; j < segment->nsects; j++) {
          const section_64& sec = sections[j];
          if (strcmp(sec.segname, "__DWARF"))
            continue;
          const char* pos = head + sec.offset;
          size_t sz = sec.size;
          if (!strcmp(sec.sectname, "__debug_info")) {
            debug_info = pos;
            debug_info_len = sz;
          } else if (!strcmp(sec.sectname, "__debug_abbrev")) {
            debug_abbrev = pos;
            debug_abbrev_len = sz;
          } else if (!strcmp(sec.sectname, "__debug_str")) {
            debug_str = pos;
            debug_str_len = sz;
          }
        }

        break;
      }
      }

      cmds_ptr = reinterpret_cast<load_command*>(
        reinterpret_cast<char*>(cmds_ptr) + cmds_ptr->cmdsize);
    }

    if (!debug_info || !debug_abbrev || !debug_str)
      err(1, "no debug info: %s", filename);
  }

  static bool isMachO(const char* p) {
    const mach_header* header = reinterpret_cast<const mach_header*>(p);
    if (header->magic == MH_MAGIC_64) {
      return true;
    }
    if (header->magic == MH_MAGIC) {
      err(1, "non 64bit Mach-O isn't supported yet");
    }
    return false;
  }
};

Binary* readBinary(const char* filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0)
    err(1, "open failed: %s", filename);

  size_t size = lseek(fd, 0, SEEK_END);
  if (size < 8 + 16)
    err(1, "too small file: %s", filename);

  size_t mapped_size = (size + 0xfff) & ~0xfff;

  char* p = (char*)mmap(NULL, mapped_size,
                        PROT_READ, MAP_SHARED,
                        fd, 0);
  if (p == MAP_FAILED)
    err(1, "mmap failed: %s", filename);

  char* header = p;
  if (isDwarfZip(header)) {
    header += 8;
  }
  if (ELFBinary::isELF(header)) {
    return new ELFBinary(filename, fd, p, size, mapped_size);
  } else if (MachOBinary::isMachO(header)) {
    return new MachOBinary(filename, fd, p, size, mapped_size);
  }
  err(1, "unknown file format: %s", filename);
}
