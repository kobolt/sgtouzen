#ifndef _CONSOLE_H
#define _CONSOLE_H

#include <stdint.h>
#include <stdbool.h>
#include "sk1100.h"

void console_draw_pattern(uint8_t y, uint8_t x, uint8_t pattern,
  uint8_t bg, uint8_t fg);
void console_pause(void);
void console_resume(void);
void console_init(sk1100_t *sk1100, bool disable_colors);
int console_text_inject(const char *filename);
void console_execute(void);

#endif /* _CONSOLE_H */
