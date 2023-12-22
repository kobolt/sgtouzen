#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <curses.h>

#include "sk1100.h"

/* Using colors from the default xterm/rxvt 256 color palette. */
static const int console_color_map[16] = {
  0,   /*  0 = Transparent   */
  232, /*  1 = Black         */
  40,  /*  2 = Green         */
  84,  /*  3 = Bright Green  */
  20,  /*  4 = Blue          */
  75,  /*  5 = Bright Blue   */
  124, /*  6 = Dark Red      */
  44,  /*  7 = Cyan          */
  196, /*  8 = Red           */
  204, /*  9 = Bright Red    */
  184, /* 10 = Yellow        */
  230, /* 11 = Bright Yellow */
  28,  /* 12 = Dark Green    */
  170, /* 13 = Pink          */
  250, /* 14 = Gray          */
  255, /* 15 = White         */
};

static const uint8_t console_pattern_to_ascii[UINT8_MAX + 1] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

  ' ','!','"','#','$','%','&','\'','(',')','*','+', ',','-','.','/',
  '0','1','2','3','4','5','6', '7','8','9',':',';', '<','=','>','?',
  '@','A','B','C','D','E','F', 'G','H','I','J','K', 'L','M','N','O',
  'P','Q','R','S','T','U','V', 'W','X','Y','Z','[','\\',']','^','_',
  '`','a','b','c','d','e','f', 'g','h','i','j','k', 'l','m','n','o',
  'p','q','r','s','t','u','v', 'w','x','y','z','{', '|','}','~',  0,

  '|','-','+','+','+','+','/','\\','\\','/','/','\\','\\','/','|','-',
  '#','x','+','/','\\','/','\\','\\','/','-','-','-','-','-','-','|',
  'A','A','A','A','A','A','A','A','E','E','E','E','E','E','N','N',
  'I','I','I','I','I','I','O','O','O','O','O','O','O','U','U','U',
  'U','U','U','a','b','0','/','u','Z','0','o','c','?','!','/','L',
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  '|','|','|','|','|','#','|','-','|','-','%','o','o',' ',' ',' ',
  ' ',' ',' ',' ',' ','s','h','d','c','O','^','T','H','@','+',' ',
};

/* Need enough time for BASIC to process statements after CR. */
#define CONSOLE_TEXT_INJECT_DELAY 16

static bool console_colors = true;
static sk1100_t *console_sk1100_reference = NULL;
static FILE *console_text_inject_fh = NULL;



void console_draw_pattern(uint8_t y, uint8_t x, uint8_t pattern,
  uint8_t bg, uint8_t fg)
{
  uint8_t color_pair;

  if (console_colors && has_colors() && can_change_color()) {
    color_pair = (bg * 16) + fg + 1;
    attron(COLOR_PAIR(color_pair));
  }

  pattern = console_pattern_to_ascii[pattern];
  mvaddch(y, x, isprint(pattern) ? pattern : ' ');

  if (console_colors && has_colors() && can_change_color()) {
    attroff(COLOR_PAIR(color_pair));
  }
}



static bool console_key_is_shifted(int key)
{
  if isupper(key) {
    return true;
  }

  if (key == '!' ||
      key == '"' ||
      key == '#' ||
      key == '$' ||
      key == '%' ||
      key == '&' ||
      key == '\'' ||
      key == '(' ||
      key == ')' ||
      key == '`' ||
      key == '}' ||
      key == '{' ||
      key == '<' ||
      key == '>' ||
      key == '+' ||
      key == '*' ||
      key == '=' ||
      key == '~' ||
      key == '|' ||
      key == '?' ||
      key == '_') {
    return true;
  }

  return false;
}



static bool console_key_is_control(int key)
{
  if (key == '\r' || key == '\n') {
    return false;
  } else if (key < 0x20) {
    return true;
  } else {
    return false;
  }
}



static SK1100_KEY console_key_map(int key)
{
  switch (key) {
  case '1':
  case '!':
    return SK1100_KEY_1;
  case '2':
  case '"':
    return SK1100_KEY_2;
  case '3':
  case '#':
    return SK1100_KEY_3;
  case '4':
  case '$':
    return SK1100_KEY_4;
  case '5':
  case '%':
    return SK1100_KEY_5;
  case '6':
  case '&':
    return SK1100_KEY_6;
  case '7':
  case '\'':
    return SK1100_KEY_7;
  case '8':
  case '(':
    return SK1100_KEY_8;
  case '9':
  case ')':
    return SK1100_KEY_9;
  case '0':
    return SK1100_KEY_0;

  case 'a':
  case 'A':
  case 0x01: /* Ctrl+A */
    return SK1100_KEY_A;
  case 'b':
  case 'B':
    return SK1100_KEY_B;
  case 'c':
  case 'C':
  case 0x03: /* Ctrl+C */
    return SK1100_KEY_C;
  case 'd':
  case 'D':
    return SK1100_KEY_D;
  case 'e':
  case 'E':
  case 0x05: /* Ctrl+E */
    return SK1100_KEY_E;
  case 'f':
  case 'F':
    return SK1100_KEY_F;
  case 'g':
  case 'G':
  case 0x07: /* Ctrl+G */
    return SK1100_KEY_G;
  case 'h':
  case 'H':
  case 0x08: /* Ctrl+H */
    return SK1100_KEY_H;
  case 'i':
  case 'I':
  case '\t':
    return SK1100_KEY_I;
  case 'j':
  case 'J':
    return SK1100_KEY_J;
  case 'k':
  case 'K':
  case 0x0B: /* Ctrl+K */
    return SK1100_KEY_K;
  case 'l':
  case 'L':
  case 0x0C: /* Ctrl+L */
    return SK1100_KEY_L;
  case 'm':
  case 'M':
    return SK1100_KEY_M;
  case 'n':
  case 'N':
  case 0x0E: /* Ctrl+N */
    return SK1100_KEY_N;
  case 'o':
  case 'O':
  case 0x0F: /* Ctrl+O */
    return SK1100_KEY_O;
  case 'p':
  case 'P':
  case 0x10: /* Ctrl+P */
    return SK1100_KEY_P;
  case 'q':
  case 'Q':
  case 0x11: /* Ctrl+Q */
    return SK1100_KEY_Q;
  case 'r':
  case 'R':
  case 0x12: /* Ctrl+R */
    return SK1100_KEY_R;
  case 's':
  case 'S':
  case 0x13: /* Ctrl+S */
    return SK1100_KEY_S;
  case 't':
  case 'T':
  case 0x14: /* Ctrl+T */
    return SK1100_KEY_T;
  case 'u':
  case 'U':
  case 0x15: /* Ctrl+U */
    return SK1100_KEY_U;
  case 'v':
  case 'V':
  case 0x16: /* Ctrl+V */
    return SK1100_KEY_V;
  case 'w':
  case 'W':
  case 0x17: /* Ctrl+W */
    return SK1100_KEY_W;
  case 'x':
  case 'X':
  case 0x18: /* Ctrl+X */
    return SK1100_KEY_X;
  case 'y':
  case 'Y':
    return SK1100_KEY_Y;
  case 'z':
  case 'Z':
    return SK1100_KEY_Z;

  case '@':
  case '`':
    return SK1100_KEY_AT;
  case '[':
  case '{':
    return SK1100_KEY_LEFT_BRACKET;
  case ']':
  case '}':
    return SK1100_KEY_RIGHT_BRACKET;
  case ',':
  case '<':
    return SK1100_KEY_COMMA;
  case '.':
  case '>':
    return SK1100_KEY_PERIOD;
  case ';':
  case '+':
    return SK1100_KEY_SEMICOLON;
  case ':':
  case '*':
    return SK1100_KEY_COLON;
  case '-':
  case '=':
    return SK1100_KEY_MINUS;
  case '^':
  case '~':
    return SK1100_KEY_CARET;
  case '\\':
  case '|':
    return SK1100_KEY_YEN;
  case '/':
  case '?':
    return SK1100_KEY_SLASH;
  case '_':
    return SK1100_KEY_PI;
  case ' ':
    return SK1100_KEY_SPACE;

  case KEY_F(1):
    return SK1100_KEY_BREAK;
  case KEY_F(2):
    return SK1100_KEY_GRAPH;
  case KEY_F(3):
    return SK1100_KEY_KANA;

  case '\r':
  case '\n':
  case KEY_ENTER:
    return SK1100_KEY_CR;
  case KEY_BACKSPACE:
    return SK1100_KEY_INS_DEL;
  case KEY_HOME:
    return SK1100_KEY_HOME_CLR;
  case KEY_UP:
    return SK1100_KEY_UP;
  case KEY_DOWN:
    return SK1100_KEY_DOWN;
  case KEY_LEFT:
    return SK1100_KEY_LEFT;
  case KEY_RIGHT:
    return SK1100_KEY_RIGHT;

  case ERR:
  default:
    return SK1100_KEY_NONE;
  }
}



void console_pause(void)
{
  endwin();
  timeout(-1);
}



void console_resume(void)
{
  timeout(0);
  refresh();
}



static void console_exit(void)
{
  curs_set(1);
  endwin();
}



void console_init(sk1100_t *sk1100, bool disable_colors)
{
  int bg;
  int fg;

  console_sk1100_reference = sk1100;

  initscr();
  atexit(console_exit);
  noecho();
  keypad(stdscr, TRUE);
  timeout(0);
  curs_set(0);

  if (disable_colors) {
    console_colors = false;

  } else {
    console_colors = true;

    if (has_colors() && can_change_color()) {
      start_color();
      use_default_colors();

      for (bg = 0; bg < 16; bg++) {
        for (fg = 0; fg < 16; fg++) {
          init_pair((bg * 16) + fg + 1,
            console_color_map[fg], console_color_map[bg]);
        }
      }
    }
  }
}



int console_text_inject(const char *filename)
{
  if (console_text_inject_fh != NULL) {
    return -2; /* Inject already in progress. */
  }

  console_text_inject_fh = fopen(filename, "rb");
  if (console_text_inject_fh == NULL) {
    return -1; /* File not found. */
  }

  return 0;
}



void console_execute(void)
{
  static bool func_toggle = false;
  static int inject_delay = 0;
  int key = ERR;

  if (console_text_inject_fh != NULL) {
    if (inject_delay > 0) {
      inject_delay--;
    } else {
      key = fgetc(console_text_inject_fh);
      if (key == EOF) {
        key = ERR;
        fclose(console_text_inject_fh);
        console_text_inject_fh = NULL;
      }
      inject_delay = CONSOLE_TEXT_INJECT_DELAY;
    }
  } else {
    key = getch();
  }

  if (key == KEY_F(4)) {
    func_toggle = !func_toggle;
  }

  if (key != ERR && console_sk1100_reference != NULL) {
    sk1100_key_press(console_sk1100_reference,
      console_key_map(key),
      console_key_is_shifted(key),
      console_key_is_control(key),
      func_toggle);
  }

  refresh();
}



