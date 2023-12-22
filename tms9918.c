#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "tms9918.h"
#include "z80.h"
#include "console.h"
#include "gui.h"
#include "panic.h"

#define TMS9918_DATA_READ  0xBE
#define TMS9918_DATA_WRITE 0xBE
#define TMS9918_CTRL_READ  0xBF
#define TMS9918_CTRL_WRITE 0xBF

#define TMS9918_GRAPHICS_HEIGHT 192
#define TMS9918_GRAPHICS_WIDTH  256

#define TMS9918_SPRITE_TERMINATOR 208



static void tms9918_set_mode(tms9918_t *tms9918)
{
  if (tms9918->mode_bit[0] == 1 &&
      tms9918->mode_bit[1] == 0 &&
      tms9918->mode_bit[2] == 0) {
    tms9918->mode = TMS9918_MODE_TEXT;

  } else if (tms9918->mode_bit[0] == 0 &&
             tms9918->mode_bit[1] == 1 &&
             tms9918->mode_bit[2] == 0) {
    tms9918->mode = TMS9918_MODE_MULTICOLOR;

  } else if (tms9918->mode_bit[0] == 0 &&
             tms9918->mode_bit[1] == 0 &&
             tms9918->mode_bit[2] == 1) {
    tms9918->mode = TMS9918_MODE_GRAPHICS_II;

  } else if (tms9918->mode_bit[0] == 0 &&
             tms9918->mode_bit[1] == 0 &&
             tms9918->mode_bit[2] == 0) {
    tms9918->mode = TMS9918_MODE_GRAPHICS_I;

  } else {
    tms9918->mode = TMS9918_MODE_UNKNOWN;
  }
}



static uint8_t tms9918_data_read(void *tms9918, uint8_t upper_address)
{
  /* Read data from VRAM. */
  (void)upper_address;
  uint8_t value;
  value = ((tms9918_t *)tms9918)->vram[((tms9918_t *)tms9918)->vram_address];
  ((tms9918_t *)tms9918)->vram_address++;
  if (((tms9918_t *)tms9918)->vram_address >= TMS9918_VRAM_MAX) {
    ((tms9918_t *)tms9918)->vram_address = 0;
  }
  return value;
}



static uint8_t tms9918_ctrl_read(void *tms9918, uint8_t upper_address)
{
  /* Read VDP status. */
  (void)upper_address;
  uint8_t value;
  value = (((tms9918_t *)tms9918)->interrupt_flag << 7) |
          (((tms9918_t *)tms9918)->fifth_sprite_flag << 6) |
          (((tms9918_t *)tms9918)->coincidence_flag << 5) |
          (((tms9918_t *)tms9918)->fifth_sprite_number & 0x1F);
  ((tms9918_t *)tms9918)->interrupt_flag    = false;
  ((tms9918_t *)tms9918)->fifth_sprite_flag = false;
  ((tms9918_t *)tms9918)->coincidence_flag  = false;
  return value;
}



static void tms9918_data_write(void *tms9918, uint8_t value,
  uint8_t upper_address)
{
  /* Write data to VRAM. */
  (void)upper_address;
  ((tms9918_t *)tms9918)->vram[((tms9918_t *)tms9918)->vram_address] = value;
  ((tms9918_t *)tms9918)->vram_address++;
  if (((tms9918_t *)tms9918)->vram_address >= TMS9918_VRAM_MAX) {
    ((tms9918_t *)tms9918)->vram_address = 0;
  }
}



static void tms9918_ctrl_write(void *tms9918, uint8_t value,
  uint8_t upper_address)
{
  /* Write address to VRAM or write to VDP register. */
  (void)upper_address;
  if (((tms9918_t *)tms9918)->ctrl_latched) {
    ((tms9918_t *)tms9918)->ctrl_latched = false;

    if ((value >> 6) == 1 || (value >> 6) == 0) { /* VRAM address */
      ((tms9918_t *)tms9918)->vram_address = 
        ((tms9918_t *)tms9918)->ctrl_latch | ((value & 0x3F) << 8);

    } else if ((value >> 3) == 0x10) { /* VDP register */
      switch (value & 0x7) {
      case 0:
        ((tms9918_t *)tms9918)->mode_bit[2] = 
          (((tms9918_t *)tms9918)->ctrl_latch >> 1) & 1;
        tms9918_set_mode((tms9918_t *)tms9918);
        break;

      case 1:
        ((tms9918_t *)tms9918)->sprite_mag = 
           ((tms9918_t *)tms9918)->ctrl_latch       & 1;
        ((tms9918_t *)tms9918)->sprite_size = 
          (((tms9918_t *)tms9918)->ctrl_latch >> 1) & 1;
        ((tms9918_t *)tms9918)->mode_bit[1] = 
          (((tms9918_t *)tms9918)->ctrl_latch >> 3) & 1;
        ((tms9918_t *)tms9918)->mode_bit[0] = 
          (((tms9918_t *)tms9918)->ctrl_latch >> 4) & 1;
        ((tms9918_t *)tms9918)->interrupt_enable = 
          (((tms9918_t *)tms9918)->ctrl_latch >> 5) & 1;
        ((tms9918_t *)tms9918)->blank = 
          (((tms9918_t *)tms9918)->ctrl_latch >> 6) & 1;
        tms9918_set_mode((tms9918_t *)tms9918);
        break;

      case 2:
        ((tms9918_t *)tms9918)->ntba =
          (((tms9918_t *)tms9918)->ctrl_latch & 0xF) * 0x400;
        break;

      case 3:
        ((tms9918_t *)tms9918)->ctba =
          ((tms9918_t *)tms9918)->ctrl_latch * 0x40;
        break;

      case 4:
        ((tms9918_t *)tms9918)->pgba =
          (((tms9918_t *)tms9918)->ctrl_latch & 0x7) * 0x800;
        break;

      case 5:
        ((tms9918_t *)tms9918)->satba =
          (((tms9918_t *)tms9918)->ctrl_latch & 0x7F) * 0x80;
        break;

      case 6:
        ((tms9918_t *)tms9918)->spgba =
          (((tms9918_t *)tms9918)->ctrl_latch & 0x7) * 0x800;
        break;

      case 7:
        ((tms9918_t *)tms9918)->text_color_0 =
          ((tms9918_t *)tms9918)->ctrl_latch & 0xF;
        ((tms9918_t *)tms9918)->text_color_1 =
          ((tms9918_t *)tms9918)->ctrl_latch >> 4;
        break;
      }

    } else {
      panic("Unknown TMS9918 control write: %02x\n", value);
    }

  } else {
    ((tms9918_t *)tms9918)->ctrl_latched = true;
    ((tms9918_t *)tms9918)->ctrl_latch = value;
  }
}



void tms9918_init(tms9918_t *tms9918, z80_t *z80)
{
  memset(tms9918, 0, sizeof(tms9918_t));

  z80->io_read[TMS9918_DATA_READ].func   = tms9918_data_read;
  z80->io_read[TMS9918_CTRL_READ].func   = tms9918_ctrl_read;
  z80->io_write[TMS9918_DATA_WRITE].func = tms9918_data_write;
  z80->io_write[TMS9918_CTRL_WRITE].func = tms9918_ctrl_write;

  z80->io_read[TMS9918_DATA_READ].cookie   = tms9918;
  z80->io_read[TMS9918_CTRL_READ].cookie   = tms9918;
  z80->io_write[TMS9918_DATA_WRITE].cookie = tms9918;
  z80->io_write[TMS9918_CTRL_WRITE].cookie = tms9918;
}



static inline uint8_t tms9918_vram(tms9918_t *tms9918, uint16_t address)
{
  /* Protected access. */
  if (address >= TMS9918_VRAM_MAX) {
    panic("VRAM access out of bounds: 0x%04x\n", address);
    return 0xFF;
  }
  return tms9918->vram[address];
}



static void tms9918_draw_pattern(tms9918_t *tms9918, int pos)
{
  uint16_t address;
  uint16_t color_address;
  uint8_t pattern_name;
  uint8_t bg;
  uint8_t fg;
  int y;

  address = tms9918->ntba + pos;
  pattern_name = tms9918_vram(tms9918, address);

  if (tms9918->mode == TMS9918_MODE_GRAPHICS_II) {
    color_address = (tms9918->ctba & ~0x1FFF) + (pattern_name * 8);
    color_address += (0x800 * (pos / 256));
  } else {
    color_address = tms9918->ctba + (pattern_name / 8);
  }

  if (tms9918->mode == TMS9918_MODE_TEXT) {
    console_draw_pattern(pos / 40, pos % 40, pattern_name,
      tms9918->text_color_0, tms9918->text_color_1);
  } else {
    bg = tms9918_vram(tms9918, color_address);
    fg = bg >> 4;
    bg = bg & 0xF;
    console_draw_pattern(pos / 32, pos % 32, pattern_name,
      bg, fg);
  }

  if (tms9918->mode == TMS9918_MODE_GRAPHICS_II) {
    address = (tms9918->pgba & ~0x1FFF) + (pattern_name * 8);
    address += (0x800 * (pos / 256));
  } else {
    address = tms9918->pgba + (pattern_name * 8);
  }

  for (y = 0; y < 8; y++) {
    if (tms9918->mode == TMS9918_MODE_TEXT) {
      bg = tms9918->text_color_0;
      fg = tms9918->text_color_1;
    } else {
      if (tms9918->mode == TMS9918_MODE_GRAPHICS_II) {
        bg = tms9918_vram(tms9918, color_address + y);
      } else { /* tms9918->mode == TMS9918_MODE_GRAPHICS_I */
        bg = tms9918_vram(tms9918, color_address);
      }
      fg = bg >> 4;
      bg = bg & 0xF;
    }

    if (tms9918->mode == TMS9918_MODE_TEXT) {
      gui_draw_pattern_line(((pos / 40) * 8) + y, (pos % 40) * 6,
        tms9918_vram(tms9918, address + y), bg, fg);
    } else {
      gui_draw_pattern_line(((pos / 32) * 8) + y, (pos % 32) * 8,
        tms9918_vram(tms9918, address + y), bg, fg);
    }
  }
}



static bool tms9918_draw_sprite(tms9918_t *tms9918, int plane)
{
  uint16_t address;
  uint8_t color;
  int pos_y;
  int pos_x;
  int y;

  address = tms9918->satba + (plane * 4);

  pos_y = tms9918_vram(tms9918, address);
  if (pos_y == TMS9918_SPRITE_TERMINATOR) {
    return true; /* Terminate */
  }
  pos_y += 1;
  pos_x = tms9918_vram(tms9918, address + 1);
  if (tms9918_vram(tms9918, address + 3) >> 7) { /* Early Clock */
    pos_x -= 32;
  }
  color = tms9918_vram(tms9918, address + 3) & 0xF;

  address = tms9918->spgba + (tms9918_vram(tms9918, address + 2) * 8);
  if (tms9918->sprite_size) { /* 16x16 */
    for (y = 0; y < 8; y++) {
      gui_draw_pattern_line(pos_y + y, pos_x,
        tms9918_vram(tms9918, address + y), 0, color);
      gui_draw_pattern_line(pos_y + y + 8, pos_x,
        tms9918_vram(tms9918, address + y + 8), 0, color);
      gui_draw_pattern_line(pos_y + y, pos_x + 8,
        tms9918_vram(tms9918, address + y + 16), 0, color);
      gui_draw_pattern_line(pos_y + y + 8, pos_x + 8,
        tms9918_vram(tms9918, address + y + 24), 0, color);
    }

  } else { /* 8x8 */
    /* Not Implemented */
    for (y = 0; y < 8; y++) {
      gui_draw_pattern_line(pos_y + y, pos_x,
        tms9918_vram(tms9918, address + y), 0, color);
    }
  }

  return false; /* Continue */
}



void tms9918_execute(tms9918_t *tms9918)
{
  int pos;
  int plane;

  tms9918->pixel_clock++;
  if (tms9918->pixel_clock < (342 * 2)) {
    return;
  }
  tms9918->pixel_clock = 0;

  tms9918->scanline++;
  if (tms9918->scanline < 262) {
    return;
  }
  tms9918->scanline = 0;

  tms9918->frame_no++;
  tms9918->frame_ready = true;
  tms9918->interrupt_flag = true;

  /* Draw black backdrop instead of transparent. */
  gui_draw_backdrop(tms9918->text_color_0 == 0 ? 1 : tms9918->text_color_0);

  switch (tms9918->mode) {
  case TMS9918_MODE_GRAPHICS_I:
  case TMS9918_MODE_GRAPHICS_II:
    for (pos = 0; pos < 768; pos++) {
      tms9918_draw_pattern(tms9918, pos);
    }
    for (plane = 0; plane < 32; plane++) {
      if (tms9918_draw_sprite(tms9918, plane) == true) {
        break;
      }
    }
    break;

  case TMS9918_MODE_TEXT:
    for (pos = 0; pos < 960; pos++) {
      tms9918_draw_pattern(tms9918, pos);
    }
    break;

  default:
    break;
  }
}



void tms9918_dump(FILE *fh, tms9918_t *tms9918)
{
  char *mode_name;

  fprintf(fh, "Interrupt Flag      : %d\n", tms9918->interrupt_flag);
  fprintf(fh, "Coincidence Flag    : %d\n", tms9918->coincidence_flag);
  fprintf(fh, "Fifth Sprite Flag   : %d\n", tms9918->fifth_sprite_flag);
  fprintf(fh, "Fifth Sprite Number : 0x%x\n", tms9918->fifth_sprite_number);

  switch (tms9918->mode) {
  case TMS9918_MODE_GRAPHICS_I:
    mode_name = "Graphics I";
    break;

  case TMS9918_MODE_GRAPHICS_II:
    mode_name = "Graphics II";
    break;

  case TMS9918_MODE_MULTICOLOR:
    mode_name = "Multicolor";
    break;

  case TMS9918_MODE_TEXT:
    mode_name = "Text";
    break;

  case TMS9918_MODE_UNKNOWN:
  default:
    mode_name = "Unknown";
    break;
  }

  fprintf(fh, "Mode                : %d-%d-%d (%s)\n",
    tms9918->mode_bit[0],
    tms9918->mode_bit[1],
    tms9918->mode_bit[2], mode_name);

  fprintf(fh, "Blank Enable        : %d\n", tms9918->blank);
  fprintf(fh, "Interrupt Enable    : %d\n", tms9918->interrupt_enable);
  fprintf(fh, "Sprite Size         : %d (%s)\n", tms9918->sprite_size,
    tms9918->sprite_size ? "16x16" : "8x8");
  fprintf(fh, "Sprite Magnification: %d (%s)\n", tms9918->sprite_mag,
    tms9918->sprite_mag ? "2X" : "1X");

  fprintf(fh, "Name Table Base Address              : 0x%04x\n",
    tms9918->ntba);
  fprintf(fh, "Color Table Base Address             : 0x%04x\n",
    tms9918->ctba);
  fprintf(fh, "Pattern Generator Base Address       : 0x%04x\n",
    tms9918->pgba);
  fprintf(fh, "Sprite Attribute Table Base Address  : 0x%04x\n",
    tms9918->satba);
  fprintf(fh, "Sprite Pattern Generator Base Address: 0x%04x\n",
    tms9918->spgba);

  fprintf(fh, "Text Color 0: %d\n", tms9918->text_color_0);
  fprintf(fh, "Text Color 1: %d\n", tms9918->text_color_1);

  fprintf(fh, "Control Register Latch: 0x%02x (%d)\n",
    tms9918->ctrl_latch, tms9918->ctrl_latched);
  fprintf(fh, "Current VRAM Address  : 0x%04x\n",
    tms9918->vram_address);
}



static void tms9918_vram_dump_16(FILE *fh, tms9918_t *tms9918,
  uint16_t start, uint16_t end)
{
  int i;
  uint16_t address;
  uint8_t value;

  fprintf(fh, "%04x   ", start & 0xFFF0);

  /* Hex */
  for (i = 0; i < 16; i++) {
    address = (start & 0xFFF0) + i;
    if (address >= TMS9918_VRAM_MAX) {
      break;
    }
    value = tms9918->vram[address];
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
    if (address >= TMS9918_VRAM_MAX) {
      break;
    }
    value = tms9918->vram[address];
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



void tms9918_dump_vram(FILE *fh, tms9918_t *tms9918,
  uint16_t start, uint16_t end)
{
  int i;
  tms9918_vram_dump_16(fh, tms9918, start, end);
  for (i = (start & 0xFFF0) + 16; i <= end; i += 16) {
    tms9918_vram_dump_16(fh, tms9918, i, end);
  }
}



static void tms9918_print_pattern(FILE *fh, tms9918_t *tms9918,
  uint16_t address)
{
  uint8_t value;

  if (address >= TMS9918_VRAM_MAX) {
    return;
  }

  value = tms9918->vram[address];
  fprintf(fh, "%c", ((value >> 7) & 1) ? '#' : ' ');
  fprintf(fh, "%c", ((value >> 6) & 1) ? '#' : ' ');
  fprintf(fh, "%c", ((value >> 5) & 1) ? '#' : ' ');
  fprintf(fh, "%c", ((value >> 4) & 1) ? '#' : ' ');
  fprintf(fh, "%c", ((value >> 3) & 1) ? '#' : ' ');
  fprintf(fh, "%c", ((value >> 2) & 1) ? '#' : ' ');
  fprintf(fh, "%c", ((value >> 1) & 1) ? '#' : ' ');
  fprintf(fh, "%c", ( value       & 1) ? '#' : ' ');
}



void tms9918_dump_patterns(FILE *fh, tms9918_t *tms9918, bool sprites)
{
  int pattern;
  int pattern_limit;
  int y;
  uint16_t address;
  uint16_t generator_address;

  switch (tms9918->mode) {
  case TMS9918_MODE_GRAPHICS_I:
  case TMS9918_MODE_MULTICOLOR:
  case TMS9918_MODE_TEXT:
    pattern_limit = 256;
    if (sprites) {
      generator_address = tms9918->spgba;
    } else {
      generator_address = tms9918->pgba;
    }
    break;

  case TMS9918_MODE_GRAPHICS_II:
    if (sprites) {
      pattern_limit = 256;
      generator_address = tms9918->spgba;
    } else {
      pattern_limit = (256 * 3);
      generator_address = tms9918->pgba & ~0x1FFF;
    }
    break;

  case TMS9918_MODE_UNKNOWN:
  default:
    fprintf(fh, "TMS9918 mode not set!\n");
    return;
  }

  for (pattern = 0; pattern < pattern_limit; pattern++) {
    address = generator_address + (pattern * 8);
    for (y = 0; y < 8; y++) {
      fprintf(fh, "0x%03x.%d | ", pattern, y);
      tms9918_print_pattern(fh, tms9918, address + y);
      fprintf(fh, "\n");
    }
  }
}



void tms9918_dump_sprites(FILE *fh, tms9918_t *tms9918)
{
  int plane;
  uint16_t address;
  uint8_t y;
  uint8_t x;
  uint8_t name;
  uint8_t color;
  bool ec; /* Early Clock */

  for (plane = 0; plane < 32; plane++) {
    address = tms9918->satba + (plane * 4);
    if (address >= TMS9918_VRAM_MAX) {
      break;
    }

    y     = tms9918->vram[address];
    x     = tms9918->vram[address+1];
    name  = tms9918->vram[address+2];
    color = tms9918->vram[address+3] & 0xF;
    ec    = tms9918->vram[address+3] >> 7;

    fprintf(fh, "%02d, y=%03d, x=%03d, name=0x%02x color=%02d, ec=%d\n",
      plane, y, x, name, color, ec);

    address = tms9918->spgba + (name * 8);
    if (tms9918->sprite_size) { /* 16x16 */
      for (y = 0; y < 8; y++) {
        fprintf(fh, "0x%03x.%d | ", name, y);
        tms9918_print_pattern(fh, tms9918, address + y);
        tms9918_print_pattern(fh, tms9918, address + y + 16);
        fprintf(fh, "\n");
      }
      for (y = 0; y < 8; y++) {
        fprintf(fh, "0x%03x.%d | ", name, y);
        tms9918_print_pattern(fh, tms9918, address + y + 8);
        tms9918_print_pattern(fh, tms9918, address + y + 24);
        fprintf(fh, "\n");
      }

    } else { /* 8x8 */
      for (y = 0; y < 8; y++) {
        fprintf(fh, "0x%03x.%d | ", name, y);
        tms9918_print_pattern(fh, tms9918, address + y);
        fprintf(fh, "\n");
      }
    }
  }
}



void tms9918_dump_name_table(FILE *fh, tms9918_t *tms9918)
{
  uint16_t address;
  int y_pos;
  int x_pos;

  switch (tms9918->mode) {
  case TMS9918_MODE_GRAPHICS_I:
  case TMS9918_MODE_GRAPHICS_II:
    fprintf(fh, "   |");
    for (x_pos = 0; x_pos < 32; x_pos++) {
      fprintf(fh, "%-2d", x_pos / 10);
    }
    fprintf(fh, "\n");
    fprintf(fh, "   |");
    for (x_pos = 0; x_pos < 32; x_pos++) {
      fprintf(fh, "%-2d", x_pos % 10);
    }
    fprintf(fh, "\n");
    fprintf(fh,
      "---|------------------------------"
      "----------------------------------\n");

    for (y_pos = 0; y_pos < 24; y_pos++) {
      if ((tms9918->mode == TMS9918_MODE_GRAPHICS_II) &&
         ((y_pos % 8) == 0) && (y_pos != 0)) {
        fprintf(fh,
          "   |------------------------------"
          "----------------------------------\n");
      }
      fprintf(fh, "%02d |", y_pos);

      for (x_pos = 0; x_pos < 32; x_pos++) {
        address = tms9918->ntba + (y_pos * 32) + x_pos;
        if (address >= TMS9918_VRAM_MAX) {
          break;
        }
        fprintf(fh, "%02x", tms9918->vram[address]);
      }
      fprintf(fh, "\n");
    }
    break;

  case TMS9918_MODE_TEXT:
    for (y_pos = 0; y_pos < 24; y_pos++) {
      for (x_pos = 0; x_pos < 40; x_pos++) {
        address = tms9918->ntba + (y_pos * 40) + x_pos;
        if (address >= TMS9918_VRAM_MAX) {
          break;
        }
        fprintf(fh, "%02x", tms9918->vram[address]);
      }
      fprintf(fh, "\n");
    }
    break;

  default:
    break;
  }
}



