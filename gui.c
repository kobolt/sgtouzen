#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#include "sk1100.h"

#define GUI_WIDTH 256
#define GUI_HEIGHT 192

#define GUI_W_SCALE 2
#define GUI_H_SCALE 2

#define GUI_PALETTE_TRANSPARENT 0

static const uint8_t gui_palette[16][3] = {
  {0xFF, 0xFF, 0xFF}, /*  0 = Transparent   */
  {0x00, 0x00, 0x00}, /*  1 = Black         */
  {0x20, 0xC0, 0x20}, /*  2 = Green         */
  {0x60, 0xE0, 0x60}, /*  3 = Bright Green  */
  {0x20, 0x20, 0xE0}, /*  4 = Blue          */
  {0x40, 0x60, 0xE0}, /*  5 = Bright Blue   */
  {0xA0, 0x20, 0x20}, /*  6 = Dark Red      */
  {0x40, 0xC0, 0xE0}, /*  7 = Cyan          */
  {0xE0, 0x20, 0x20}, /*  8 = Red           */
  {0xE0, 0x60, 0x60}, /*  9 = Bright Red    */
  {0xC0, 0xC0, 0x20}, /* 10 = Yellow        */
  {0xC0, 0xC0, 0x80}, /* 11 = Bright Yellow */
  {0x20, 0x80, 0x20}, /* 12 = Dark Green    */
  {0xC0, 0x40, 0xA0}, /* 13 = Pink          */
  {0xA0, 0xA0, 0xA0}, /* 14 = Gray          */
  {0xE0, 0xE0, 0xE0}, /* 15 = White         */
};

static SDL_Window *gui_window = NULL;
static SDL_Renderer *gui_renderer = NULL;
static SDL_Texture *gui_texture = NULL;
static SDL_PixelFormat *gui_pixel_format = NULL;
static Uint32 *gui_pixels = NULL;
static int gui_pixel_pitch = 0;
static Uint32 gui_ticks = 0;

static bool gui_warp_mode = false;
static sk1100_t *gui_sk1100_reference = NULL;



void gui_draw_backdrop(uint8_t color)
{
  int x;
  int y;
  int scale_x;
  int scale_y;
  int out_x;
  int out_y;

  if (gui_renderer == NULL) {
    return;
  }

  for (y = 0; y < GUI_HEIGHT; y++) {
    for (x = 0; x < GUI_WIDTH; x++) {
      for (scale_y = 0; scale_y < GUI_H_SCALE; scale_y++) {
        for (scale_x = 0; scale_x < GUI_W_SCALE; scale_x++) {
          out_y = (y * GUI_H_SCALE) + scale_y;
          out_x = (x * GUI_W_SCALE) + scale_x;
          gui_pixels[(out_y * GUI_WIDTH * GUI_W_SCALE) + out_x] =
            SDL_MapRGB(gui_pixel_format,
            gui_palette[color % 16][0],
            gui_palette[color % 16][1],
            gui_palette[color % 16][2]);
        }
      }
    }
  }
}



void gui_draw_pattern_line(int y, int start_x, uint8_t line,
  uint8_t bg, uint8_t fg)
{
  int x;
  int shift;
  int scale_x;
  int scale_y;
  int out_x;
  int out_y;
  bool active;

  if (gui_renderer == NULL) {
    return;
  }

  if (y < 0 || y >= GUI_HEIGHT) {
    return;
  }

  for (x = start_x, shift = 7; x < (start_x + 8); x++, shift--) {
    if (x < 0 || x >= GUI_WIDTH) {
      continue;
    }
    active = ((line >> shift) & 1) ? true : false;
    for (scale_y = 0; scale_y < GUI_H_SCALE; scale_y++) {
      for (scale_x = 0; scale_x < GUI_W_SCALE; scale_x++) {
        out_y = (y * GUI_H_SCALE) + scale_y;
        out_x = (x * GUI_W_SCALE) + scale_x;

        if (active) {
          if (fg != GUI_PALETTE_TRANSPARENT) {
            gui_pixels[(out_y * GUI_WIDTH * GUI_W_SCALE) + out_x] =
              SDL_MapRGB(gui_pixel_format,
              gui_palette[fg % 16][0],
              gui_palette[fg % 16][1],
              gui_palette[fg % 16][2]);
          }
        } else {
          if (bg != GUI_PALETTE_TRANSPARENT) {
            gui_pixels[(out_y * GUI_WIDTH * GUI_W_SCALE) + out_x] =
              SDL_MapRGB(gui_pixel_format,
              gui_palette[bg % 16][0],
              gui_palette[bg % 16][1],
              gui_palette[bg % 16][2]);
          }
        }
      }
    }
  }
}



void gui_warp_mode_set(bool value)
{
  gui_warp_mode = value;
}



bool gui_warp_mode_get(void)
{
  return gui_warp_mode;
}



static void gui_exit_handler(void)
{
  if (gui_pixel_format != NULL) {
    SDL_FreeFormat(gui_pixel_format);
  }
  if (gui_texture != NULL) {
    SDL_UnlockTexture(gui_texture);
    SDL_DestroyTexture(gui_texture);
  }
  if (gui_renderer != NULL) {
    SDL_DestroyRenderer(gui_renderer);
  }
  if (gui_window != NULL) {
    SDL_DestroyWindow(gui_window);
  }
  SDL_Quit();
}



int gui_init(sk1100_t *sk1100, bool disable_video)
{
  Uint32 flags = 0;

  gui_sk1100_reference = sk1100;

  if (! disable_video) {
    flags |= SDL_INIT_VIDEO;
  }

  if (SDL_Init(flags) != 0) {
    fprintf(stderr, "Unable to initalize SDL: %s\n", SDL_GetError());
    return -1;
  }
  atexit(gui_exit_handler);

  if (! disable_video) {
    if ((gui_window = SDL_CreateWindow("SG-Touzen",
      SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      GUI_WIDTH * GUI_W_SCALE, GUI_HEIGHT * GUI_H_SCALE, 0)) == NULL) {
      fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
      return -1;
    }

    if ((gui_renderer = SDL_CreateRenderer(gui_window, -1, 0)) == NULL) {
      fprintf(stderr, "Unable to create renderer: %s\n", SDL_GetError());
      return -1;
    }

    if ((gui_texture = SDL_CreateTexture(gui_renderer, 
      SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
      GUI_WIDTH * GUI_W_SCALE, GUI_HEIGHT * GUI_H_SCALE)) == NULL) {
      fprintf(stderr, "Unable to create texture: %s\n", SDL_GetError());
      return -1;
    }

    if (SDL_LockTexture(gui_texture, NULL,
      (void **)&gui_pixels, &gui_pixel_pitch) != 0) {
      fprintf(stderr, "Unable to lock texture: %s\n", SDL_GetError());
      return -1;
    }

    if ((gui_pixel_format = SDL_AllocFormat(
      SDL_PIXELFORMAT_ARGB8888)) == NULL) {
      fprintf(stderr, "Unable to create pixel format: %s\n", SDL_GetError());
      return -1;
    }
  }

  return 0;
}



static SK1100_KEY gui_key_map(SDL_Scancode key)
{
  switch (key) {
  case SDL_SCANCODE_1: return SK1100_KEY_1;
  case SDL_SCANCODE_2: return SK1100_KEY_2;
  case SDL_SCANCODE_3: return SK1100_KEY_3;
  case SDL_SCANCODE_4: return SK1100_KEY_4;
  case SDL_SCANCODE_5: return SK1100_KEY_5;
  case SDL_SCANCODE_6: return SK1100_KEY_6;
  case SDL_SCANCODE_7: return SK1100_KEY_7;
  case SDL_SCANCODE_8: return SK1100_KEY_8;
  case SDL_SCANCODE_9: return SK1100_KEY_9;
  case SDL_SCANCODE_0: return SK1100_KEY_0;

  case SDL_SCANCODE_A: return SK1100_KEY_A;
  case SDL_SCANCODE_B: return SK1100_KEY_B;
  case SDL_SCANCODE_C: return SK1100_KEY_C;
  case SDL_SCANCODE_D: return SK1100_KEY_D;
  case SDL_SCANCODE_E: return SK1100_KEY_E;
  case SDL_SCANCODE_F: return SK1100_KEY_F;
  case SDL_SCANCODE_G: return SK1100_KEY_G;
  case SDL_SCANCODE_H: return SK1100_KEY_H;
  case SDL_SCANCODE_I: return SK1100_KEY_I;
  case SDL_SCANCODE_J: return SK1100_KEY_J;
  case SDL_SCANCODE_K: return SK1100_KEY_K;
  case SDL_SCANCODE_L: return SK1100_KEY_L;
  case SDL_SCANCODE_M: return SK1100_KEY_M;
  case SDL_SCANCODE_N: return SK1100_KEY_N;
  case SDL_SCANCODE_O: return SK1100_KEY_O;
  case SDL_SCANCODE_P: return SK1100_KEY_P;
  case SDL_SCANCODE_Q: return SK1100_KEY_Q;
  case SDL_SCANCODE_R: return SK1100_KEY_R;
  case SDL_SCANCODE_S: return SK1100_KEY_S;
  case SDL_SCANCODE_T: return SK1100_KEY_T;
  case SDL_SCANCODE_U: return SK1100_KEY_U;
  case SDL_SCANCODE_V: return SK1100_KEY_V;
  case SDL_SCANCODE_W: return SK1100_KEY_W;
  case SDL_SCANCODE_X: return SK1100_KEY_X;
  case SDL_SCANCODE_Y: return SK1100_KEY_Y;
  case SDL_SCANCODE_Z: return SK1100_KEY_Z;

  case SDL_SCANCODE_LEFTBRACKET:
    return SK1100_KEY_AT;
  case SDL_SCANCODE_RIGHTBRACKET:
    return SK1100_KEY_LEFT_BRACKET;
  case SDL_SCANCODE_BACKSLASH:
    return SK1100_KEY_RIGHT_BRACKET;
  case SDL_SCANCODE_COMMA:
    return SK1100_KEY_COMMA;
  case SDL_SCANCODE_PERIOD:
    return SK1100_KEY_PERIOD;
  case SDL_SCANCODE_SEMICOLON:
    return SK1100_KEY_SEMICOLON;
  case SDL_SCANCODE_APOSTROPHE:
    return SK1100_KEY_COLON;
  case SDL_SCANCODE_MINUS:
    return SK1100_KEY_MINUS;
  case SDL_SCANCODE_EQUALS:
    return SK1100_KEY_CARET;
  case SDL_SCANCODE_GRAVE:
    return SK1100_KEY_YEN;
  case SDL_SCANCODE_SLASH:
    return SK1100_KEY_SLASH;
  case SDL_SCANCODE_NONUSBACKSLASH:
    return SK1100_KEY_PI;
  case SDL_SCANCODE_SPACE:
    return SK1100_KEY_SPACE;

  case SDL_SCANCODE_F1:
    return SK1100_KEY_BREAK;
  case SDL_SCANCODE_F2:
    return SK1100_KEY_GRAPH;
  case SDL_SCANCODE_F3:
    return SK1100_KEY_KANA;

  case SDL_SCANCODE_RETURN:
    return SK1100_KEY_CR;
  case SDL_SCANCODE_BACKSPACE:
    return SK1100_KEY_INS_DEL;
  case SDL_SCANCODE_HOME:
    return SK1100_KEY_HOME_CLR;
  case SDL_SCANCODE_UP:
    return SK1100_KEY_UP;
  case SDL_SCANCODE_DOWN:
    return SK1100_KEY_DOWN;
  case SDL_SCANCODE_LEFT:
    return SK1100_KEY_LEFT;
  case SDL_SCANCODE_RIGHT:
    return SK1100_KEY_RIGHT;

  default:
    return SK1100_KEY_NONE;
  }
}



void gui_execute(void)
{
  SDL_Event event;
  SDL_Keymod keymod;

  while (SDL_PollEvent(&event) == 1) {
    switch (event.type) {
    case SDL_QUIT:
      exit(EXIT_SUCCESS);
      break;

    case SDL_KEYDOWN:
      if (gui_sk1100_reference != NULL) {
        keymod = SDL_GetModState();
        sk1100_key_press(gui_sk1100_reference,
          gui_key_map(event.key.keysym.scancode),
          (keymod & KMOD_LSHIFT) || (keymod & KMOD_RSHIFT),
          (keymod & KMOD_LCTRL),
          (keymod & KMOD_RCTRL));
      }
      break;
    }
  }

  if (gui_renderer != NULL) {
    SDL_UnlockTexture(gui_texture);

    SDL_RenderCopy(gui_renderer, gui_texture, NULL, NULL);

    if (SDL_LockTexture(gui_texture, NULL,
      (void **)&gui_pixels, &gui_pixel_pitch) != 0) {
      fprintf(stderr, "Unable to lock texture: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
    }
  }

  if (! gui_warp_mode) {
    /* Force 60 Hz (NTSC) */
    while ((SDL_GetTicks() - gui_ticks) < 16) {
      SDL_Delay(1);
    }
  }

  if (gui_renderer != NULL) {
    SDL_RenderPresent(gui_renderer);
  }

  gui_ticks = SDL_GetTicks();
}



