#ifndef _TMS9918_H
#define _TMS9918_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "z80.h"

#define TMS9918_VRAM_MAX 0x4000

typedef enum {
  TMS9918_MODE_UNKNOWN,
  TMS9918_MODE_GRAPHICS_I,
  TMS9918_MODE_GRAPHICS_II,
  TMS9918_MODE_MULTICOLOR,
  TMS9918_MODE_TEXT,
} tms9918_mode_t;

typedef struct tms9918_s {
  uint8_t vram[TMS9918_VRAM_MAX];
  uint16_t vram_address;

  bool ctrl_latched;
  uint8_t ctrl_latch;

  bool interrupt_flag;
  bool fifth_sprite_flag;
  bool coincidence_flag;
  uint8_t fifth_sprite_number;

  bool mode_bit[3];
  tms9918_mode_t mode;

  bool blank;
  bool interrupt_enable;
  bool sprite_size;
  bool sprite_mag;

  uint16_t ntba;  /* Name Table Base Address */
  uint16_t ctba;  /* Color Table Base Address */
  uint16_t pgba;  /* Pattern Generator Base Address */
  uint16_t satba; /* Sprite Attribute Table Base Address */
  uint16_t spgba; /* Sprite Pattern Generator Base Address */

  uint8_t text_color_0;
  uint8_t text_color_1;

  uint16_t pixel_clock;
  uint16_t scanline;
  uint32_t frame_no;
  bool frame_ready;
} tms9918_t;

void tms9918_init(tms9918_t *tms9918, z80_t *z80);
void tms9918_execute(tms9918_t *tms9918);
void tms9918_dump(FILE *fh, tms9918_t *tms9918);
void tms9918_dump_vram(FILE *fh, tms9918_t *tms9918,
  uint16_t start, uint16_t end);
void tms9918_dump_patterns(FILE *fh, tms9918_t *tms9918, bool sprites);
void tms9918_dump_sprites(FILE *fh, tms9918_t *tms9918);
void tms9918_dump_name_table(FILE *fh, tms9918_t *tms9918);

#endif /* _TMS9918_H */
