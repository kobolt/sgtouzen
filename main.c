#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "z80.h"
#include "mem.h"
#include "tms9918.h"
#include "sk1100.h"
#include "console.h"
#include "gui.h"
#include "panic.h"

#define SOUND_WRITE 0x7F



static z80_t z80;
static mem_t mem;
static tms9918_t tms9918;
static sk1100_t sk1100;

static bool debugger_break = false;
static bool irq_break = false;
static char panic_msg[80];



static void debugger_help(void)
{
  fprintf(stdout, "Commands:\n");
  fprintf(stdout, "  q        - Quit\n");
  fprintf(stdout, "  h        - Help\n");
  fprintf(stdout, "  c        - Continue\n");
  fprintf(stdout, "  i        - Continue until next IRQ\n");
  fprintf(stdout, "  s        - Step\n");
  fprintf(stdout, "  w        - Warp mode toggle\n");
  fprintf(stdout, "  l <file> - Load cassette WAV file.\n");
  fprintf(stdout, "  k <file> - Save cassette WAV file.\n");
  fprintf(stdout, "  t <file> - Inject text file as keyboard input.\n");
#ifndef DISABLE_Z80_TRACE
  fprintf(stdout, "  1        - Dump CPU Trace\n");
#endif /* DISABLE_Z80_TRACE */
  fprintf(stdout, "  2        - Dump RAM\n");
  fprintf(stdout, "  3        - Dump TMS9918 Registers\n");
  fprintf(stdout, "  4        - Dump TMS9918 VRAM\n");
  fprintf(stdout, "  5        - Dump TMS9918 Name Table\n");
  fprintf(stdout, "  6        - Dump TMS9918 Patterns\n");
  fprintf(stdout, "  7        - Dump TMS9918 Sprite Patterns\n");
  fprintf(stdout, "  8        - Dump TMS9918 Sprite Attributes\n");
}



static bool debugger(void)
{
  char input[128];
  char *argv[2];
  int argc;
  int result;

  fprintf(stdout, "\n");
  while (1) {
    fprintf(stdout, "%08d:%04x> ", tms9918.frame_no, z80.pc);

    if (fgets(input, sizeof(input), stdin) == NULL) {
      if (feof(stdin)) {
        exit(EXIT_SUCCESS);
      }
      continue;
    }

    if ((strlen(input) > 0) && (input[strlen(input) - 1] == '\n')) {
      input[strlen(input) - 1] = '\0'; /* Strip newline. */
    }

    argv[0] = strtok(input, " ");
    if (argv[0] == NULL) {
      continue;
    }

    for (argc = 1; argc < 2; argc++) {
      argv[argc] = strtok(NULL, " ");
      if (argv[argc] == NULL) {
        break;
      }
    }

    if (strncmp(argv[0], "q", 1) == 0) {
      exit(EXIT_SUCCESS);

    } else if (strncmp(argv[0], "h", 1) == 0) {
      debugger_help();

    } else if (strncmp(argv[0], "c", 1) == 0) {
      return false;

    } else if (strncmp(argv[0], "i", 1) == 0) {
      irq_break = true;
      return false;

    } else if (strncmp(argv[0], "s", 1) == 0) {
      return true;

    } else if (strncmp(argv[0], "w", 1) == 0) {
      if (gui_warp_mode_get()) {
        gui_warp_mode_set(false);
        fprintf(stdout, "Warp mode disabled.\n");
      } else {
        gui_warp_mode_set(true);
        fprintf(stdout, "Warp mode enabled.\n");
      }

    } else if (strncmp(argv[0], "l", 1) == 0) {
      if (argc >= 2) {
        result = sk1100_cassette_load_file(&sk1100, argv[1]);
        if (result != 0) {
          fprintf(stdout, "Failed to load cassette WAV file! Error Code: %d\n",
            result);
        } else {
          fprintf(stdout, "File '%s' playing.\n", argv[1]);
        }
      } else {
        fprintf(stdout, "Specify filename!\n");
      }

    } else if (strncmp(argv[0], "k", 1) == 0) {
      if (argc >= 2) {
        result = sk1100_cassette_save_file(&sk1100, argv[1]);
        if (result != 0) {
          fprintf(stdout, "Failed to save cassette WAV file! Error Code: %d\n",
            result);
        } else {
          fprintf(stdout, "File '%s' recording.\n", argv[1]);
        }
      } else {
        fprintf(stdout, "Specify filename!\n");
      }

    } else if (strncmp(argv[0], "t", 1) == 0) {
      if (argc >= 2) {
        result = console_text_inject(argv[1]);
        if (result != 0) {
          fprintf(stdout, "Failed to load text file! Error Code: %d\n",
            result);
        } else {
          fprintf(stdout, "File '%s' injecting.\n", argv[1]);
        }
      } else {
        fprintf(stdout, "Specify filename!\n");
      }

#ifndef DISABLE_Z80_TRACE
    } else if (strncmp(argv[0], "1", 1) == 0) {
      z80_trace_dump(stdout);
      z80_dump(stdout, &z80, &mem);
#endif /* DISABLE_Z80_TRACE */

    } else if (strncmp(argv[0], "2", 1) == 0) {
      mem_dump(stdout, &mem, 0x8000, 0xFFFF);

    } else if (strncmp(argv[0], "3", 1) == 0) {
      tms9918_dump(stdout, &tms9918);

    } else if (strncmp(argv[0], "4", 1) == 0) {
      tms9918_dump_vram(stdout, &tms9918, 0x0, 0x3FFF);

    } else if (strncmp(argv[0], "5", 1) == 0) {
      tms9918_dump_name_table(stdout, &tms9918);

    } else if (strncmp(argv[0], "6", 1) == 0) {
      tms9918_dump_patterns(stdout, &tms9918, false);

    } else if (strncmp(argv[0], "7", 1) == 0) {
      tms9918_dump_patterns(stdout, &tms9918, true);

    } else if (strncmp(argv[0], "8", 1) == 0) {
      tms9918_dump_sprites(stdout, &tms9918);

    }
  }
}



static void sig_handler(int sig)
{
  switch (sig) {
  case SIGINT:
    debugger_break = true;
    return;
  }
}



void panic(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  vsnprintf(panic_msg, sizeof(panic_msg), format, args);
  va_end(args);

  debugger_break = true;
}



void io_dummy_write(void *cookie, uint8_t value, uint8_t upper_address)
{
  (void)cookie;
  (void)value;
  (void)upper_address;
}



static void display_help(const char *progname)
{
  fprintf(stdout, "Usage: %s <options> [rom]\n", progname);
  fprintf(stdout, "Options:\n"
    "  -h   Display this help.\n"
    "  -b   Break into debugger on start.\n"
    "  -w   Enable warp mode on start.\n"
    "  -v   Disable SDL video.\n"
    "  -c   Disable console colors.\n"
    "  -m   Mirror work RAM at 0xC000/0xDFFF to 0xE000/0xFFFF.\n"
    "\n");
}



int main(int argc, char *argv[])
{
  int c;
  char *rom_filename = NULL;
  bool disable_gui_video = false;
  bool disable_console_colors = false;
  bool work_ram_mirror = false;

  while ((c = getopt(argc, argv, "hbwvcm")) != -1) {
    switch (c) {
    case 'h':
      display_help(argv[0]);
      return EXIT_SUCCESS;

    case 'b':
      debugger_break = true;
      break;

    case 'w':
      gui_warp_mode_set(true);
      break;

    case 'v':
      disable_gui_video = true;
      break;

    case 'c':
      disable_console_colors = true;
      break;

    case 'm':
      work_ram_mirror = true;
      break;

    case '?':
    default:
      display_help(argv[0]);
      return EXIT_FAILURE;
    }
  }

#ifndef DISABLE_Z80_TRACE
  z80_trace_init();
#endif /* DISABLE_Z80_TRACE */
  z80_init(&z80);
  mem_init(&mem, work_ram_mirror);
  tms9918_init(&tms9918, &z80);
  sk1100_init(&sk1100, &z80);

  panic_msg[0] = '\0';
  signal(SIGINT, sig_handler);

  /* Assign dummy functions to unused ports. */
  z80.io_write[SOUND_WRITE].func = io_dummy_write;

  if (argc <= optind) {
    display_help(argv[0]);
    return EXIT_FAILURE;
  } else {
    rom_filename = argv[optind];
  }

  if (mem_load_from_file(&mem, rom_filename, 0) != 0) {
    fprintf(stderr, "Failed to load ROM: %s\n", rom_filename);
    return EXIT_FAILURE;
  }

  if (gui_init(&sk1100, disable_gui_video) != 0) {
    return EXIT_FAILURE;
  }

  console_init(&sk1100, disable_console_colors);

  while (1) {
    z80_execute(&z80, &mem);

    while (z80.cycles > 0) {
      tms9918_execute(&tms9918);
      tms9918_execute(&tms9918);
      tms9918_execute(&tms9918);
      sk1100_execute_sync(&sk1100);
      z80.cycles--;
    }

    if (tms9918.frame_ready) {
      console_execute();
      gui_execute();
      sk1100_execute_frame(&sk1100);
      tms9918.frame_ready = false;
    }

    if (tms9918.interrupt_enable && tms9918.interrupt_flag) {
      z80_irq(&z80, &mem);
      if (irq_break) {
        irq_break = false;
        debugger_break = true;
      }
    }

    if (debugger_break) {
      console_pause();
      if (panic_msg[0] != '\0') {
        fprintf(stdout, "%s", panic_msg);
        panic_msg[0] = '\0';
      }
      debugger_break = debugger();
      if (! debugger_break) {
        console_resume();
      }
    }
  }

  return EXIT_SUCCESS;
}



