#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#include "mem.h"
#include "panic.h"



void mem_init(mem_t *mem, bool work_ram_mirror)
{
  int i;

  for (i = 0; i < MEM_SIZE_CART; i++) {
    mem->cart[i] = 0x00;
  }
  for (i = 0; i < MEM_SIZE_RAM; i++) {
    mem->ram[i] = 0x00;
  }

  mem->work_ram_mirror = work_ram_mirror;
}



uint8_t mem_read(mem_t *mem, uint16_t address)
{
  if (address < 0x8000) {
    return mem->cart[address];
  } else if (mem->work_ram_mirror && address >= 0xE000) {
    return mem->ram[address - 0x8000 - 0x2000];
  } else {
    return mem->ram[address - 0x8000];
  }
}



void mem_write(mem_t *mem, uint16_t address, uint8_t value)
{
  if (address < 0x8000) {
    panic("Memory write to read-only cartridge area: 0x%04x\n", address);
  } else if (mem->work_ram_mirror && address >= 0xE000) {
    mem->ram[address - 0x8000 - 0x2000] = value;
  } else {
    mem->ram[address - 0x8000] = value;
  }
}



int mem_load_from_file(mem_t *mem, const char *filename, uint16_t address)
{
  FILE *fh;
  int c;

  fh = fopen(filename, "rb");
  if (fh == NULL) {
    return -1;
  }

  while ((c = fgetc(fh)) != EOF) {
    mem->cart[address] = c;
    address++;
    if (address >= MEM_SIZE_CART) {
      break;
    }
  }

  fclose(fh);
  return 0;
}



static void mem_dump_16(FILE *fh, mem_t *mem, uint16_t start, uint16_t end)
{
  int i;
  uint16_t address;
  uint8_t value;

  fprintf(fh, "%04x   ", start & 0xFFF0);

  /* Hex */
  for (i = 0; i < 16; i++) {
    address = (start & 0xFFF0) + i;
    value = mem_read(mem, address);
    if ((address >= start) && (address <= end)) {
      fprintf(fh, "%02x ", value);
    } else {
      fprintf(fh, "   ");
    }
    if (i % 4 == 3) {
      fprintf(fh, " ");
    }
  }

  /* Character */
  for (i = 0; i < 16; i++) {
    address = (start & 0xFFF0) + i;
    value = mem_read(mem, address);
    if ((address >= start) && (address <= end)) {
      if (isprint(value)) {
        fprintf(fh, "%c", value);
      } else {
        fprintf(fh, ".");
      }
    } else {
      fprintf(fh, " ");
    }
  }

  fprintf(fh, "\n");
}



void mem_dump(FILE *fh, mem_t *mem, uint16_t start, uint16_t end)
{
  int i;
  mem_dump_16(fh, mem, start, end);
  for (i = (start & 0xFFF0) + 16; i <= end; i += 16) {
    mem_dump_16(fh, mem, i, end);
  }
}



