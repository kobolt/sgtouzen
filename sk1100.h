#ifndef _SK1100_H
#define _SK1100_H

#include <stdint.h>
#include <stdio.h>
#include "z80.h"

typedef enum {
  SK1100_KEY_NONE = 0,
  SK1100_KEY_1,
  SK1100_KEY_2,
  SK1100_KEY_3,
  SK1100_KEY_4,
  SK1100_KEY_5,
  SK1100_KEY_6,
  SK1100_KEY_7,
  SK1100_KEY_8,
  SK1100_KEY_9,
  SK1100_KEY_0,
  SK1100_KEY_A,
  SK1100_KEY_B,
  SK1100_KEY_C,
  SK1100_KEY_D,
  SK1100_KEY_E,
  SK1100_KEY_F,
  SK1100_KEY_G,
  SK1100_KEY_H,
  SK1100_KEY_I,
  SK1100_KEY_J,
  SK1100_KEY_K,
  SK1100_KEY_L,
  SK1100_KEY_M,
  SK1100_KEY_N,
  SK1100_KEY_O,
  SK1100_KEY_P,
  SK1100_KEY_Q,
  SK1100_KEY_R,
  SK1100_KEY_S,
  SK1100_KEY_T,
  SK1100_KEY_U,
  SK1100_KEY_V,
  SK1100_KEY_W,
  SK1100_KEY_X,
  SK1100_KEY_Y,
  SK1100_KEY_Z,
  SK1100_KEY_AT,
  SK1100_KEY_LEFT_BRACKET,
  SK1100_KEY_RIGHT_BRACKET,
  SK1100_KEY_COMMA,
  SK1100_KEY_PERIOD,
  SK1100_KEY_SEMICOLON,
  SK1100_KEY_COLON,
  SK1100_KEY_MINUS,
  SK1100_KEY_CARET,
  SK1100_KEY_YEN, /* Convenient alias for backslash. */
  SK1100_KEY_SLASH,
  SK1100_KEY_PI,
  SK1100_KEY_SPACE,
  SK1100_KEY_FUNC,
  SK1100_KEY_CTRL,
  SK1100_KEY_SHIFT,
  SK1100_KEY_GRAPH,
  SK1100_KEY_KANA, /* Dieresis on international version. */
  SK1100_KEY_BREAK,
  SK1100_KEY_CR,
  SK1100_KEY_INS_DEL,
  SK1100_KEY_HOME_CLR,
  SK1100_KEY_UP,
  SK1100_KEY_DOWN,
  SK1100_KEY_LEFT,
  SK1100_KEY_RIGHT,
  SK1100_KEY_JOYPAD_1_UP,
  SK1100_KEY_JOYPAD_1_DOWN,
  SK1100_KEY_JOYPAD_1_LEFT,
  SK1100_KEY_JOYPAD_1_RIGHT,
  SK1100_KEY_JOYPAD_1_BUTTON_1,
  SK1100_KEY_JOYPAD_1_BUTTON_2,
  SK1100_KEY_JOYPAD_2_UP,
  SK1100_KEY_JOYPAD_2_DOWN,
  SK1100_KEY_JOYPAD_2_LEFT,
  SK1100_KEY_JOYPAD_2_RIGHT,
  SK1100_KEY_JOYPAD_2_BUTTON_1,
  SK1100_KEY_JOYPAD_2_BUTTON_2,
} SK1100_KEY;

typedef struct sk1100_s {
  SK1100_KEY key_pressed;
  bool shift_pressed;
  bool ctrl_pressed;
  bool func_pressed;
  int key_hold;

  uint8_t ppi_port_a;
  uint8_t ppi_port_b;
  uint8_t ppi_port_c;
  uint8_t ppi_ctrl;

  FILE *cassette_load_fh;
  uint32_t cassette_load_cycle;
  uint8_t cassette_load_sample;

  FILE *cassette_save_fh;
  uint32_t cassette_save_cycle;
  uint32_t cassette_save_sample_count;
  uint32_t cassette_save_idle_count;
  bool cassette_save_high_seen;
} sk1100_t;

void sk1100_init(sk1100_t *sk1100, z80_t *z80);
void sk1100_execute_sync(sk1100_t *sk1100);
void sk1100_execute_frame(sk1100_t *sk1100);
void sk1100_key_press(sk1100_t *sk1100, SK1100_KEY key,
  bool shift, bool ctrl, bool func);
int sk1100_cassette_load_file(sk1100_t *sk1100, const char *filename);
int sk1100_cassette_save_file(sk1100_t *sk1100, const char *filename);

#endif /* _SK1100_H */
