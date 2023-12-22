#ifndef _GUI_H
#define _GUI_H

#include <stdint.h>
#include <stdbool.h>
#include "sk1100.h"

void gui_draw_backdrop(uint8_t color);
void gui_draw_pattern_line(int y, int start_x, uint8_t line,
  uint8_t bg, uint8_t fg);
void gui_warp_mode_set(bool value);
bool gui_warp_mode_get(void);
int gui_init(sk1100_t *sk1100, bool disable_video);
void gui_execute(void);

#endif /* _GUI_H */
