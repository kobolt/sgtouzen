#ifndef _MEM_H
#define _MEM_H

#include <stdint.h>
#include <stdio.h>

#define MEM_SIZE_CART  0x8000 /* 0x0000 -> 0x7FFF */
#define MEM_SIZE_RAM   0x8000 /* 0x8000 -> 0xFFFF */

typedef struct mem_s {
  uint8_t ram[MEM_SIZE_RAM];
  uint8_t cart[MEM_SIZE_CART];
  bool work_ram_mirror; /* 0xC000 -> 0xDFFF mirrored to 0xE000 -> 0xFFFF */
} mem_t;

void mem_init(mem_t *mem, bool work_ram_mirror);
uint8_t mem_read(mem_t *mem, uint16_t address);
void mem_write(mem_t *mem, uint16_t address, uint8_t value);
int mem_load_from_file(mem_t *mem, const char *filename, uint16_t address);
void mem_dump(FILE *fh, mem_t *mem, uint16_t start, uint16_t end);

#endif /* _MEM_H */
