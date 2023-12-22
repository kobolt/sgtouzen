#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "sk1100.h"
#include "z80.h"

#define SK1100_PPI_PORT_A_READ  0xDC
#define SK1100_PPI_PORT_B_READ  0xDD
#define SK1100_PPI_PORT_C_READ  0xDE
#define SK1100_PPI_PORT_C_WRITE 0xDE
#define SK1100_PPI_CTRL_WRITE   0xDF

#define SK1100_CASSETTE_INTERNAL_SAMPLE_RATE 3579545 /* CPU Clock Speed */
#define SK1100_CASSETTE_WAV_SAMPLE_RATE 44100
#define SK1100_CASSETTE_SAVE_IDLE_STOP 4000000 /* Header->Data Compensate */

typedef struct wav_header_s {
  uint8_t riff_string[4];
  uint32_t chunk_size;
  uint8_t wave_string[4];
  uint8_t fmt_string[4];
  uint32_t subchunk1_size;
  uint16_t audio_format;
  uint16_t channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t block_align;
  uint16_t bits_per_sample;
  uint8_t data_string[4];
  uint32_t subchunk2_size;
} wav_header_t;



static void sk1100_ppi_port_c_write(void *sk1100, uint8_t value,
  uint8_t upper_address)
{
  (void)upper_address;
  ((sk1100_t *)sk1100)->ppi_port_c = value;

  /* Default is that no key is pressed. */
  ((sk1100_t *)sk1100)->ppi_port_a = 0xFF;
  ((sk1100_t *)sk1100)->ppi_port_b = 0xFF;

  switch (value & 0x7) {
  case 0:
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_1) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_Q) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_A) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_Z) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xF7;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_KANA) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xEF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_COMMA) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xDF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_K) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xBF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_I) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0x7F;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_8) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFE;
    }
    break;

  case 1:
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_2) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_W) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_S) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_X) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xF7;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_SPACE) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xEF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_PERIOD) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xDF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_L) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xBF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_O) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0x7F;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_9) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFE;
    }
    break;

  case 2:
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_3) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_E) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_D) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_C) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xF7;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_HOME_CLR) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xEF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_SLASH) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xDF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_SEMICOLON) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xBF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_P) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0x7F;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_0) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFE;
    }
    break;

  case 3:
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_4) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_R) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_F) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_V) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xF7;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_INS_DEL) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xEF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_PI) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xDF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_COLON) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xBF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_AT) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0x7F;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_MINUS) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFE;
    }
    break;

  case 4:
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_5) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_T) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_G) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_B) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xF7;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_DOWN) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xDF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_RIGHT_BRACKET) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xBF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_LEFT_BRACKET) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0x7F;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_CARET) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFE;
    }
    break;

  case 5:
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_6) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_Y) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_H) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_N) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xF7;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_LEFT) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xDF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_CR) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xBF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_YEN) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_FUNC ||
        ((sk1100_t *)sk1100)->func_pressed) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xF7;
    }
    break;

  case 6:
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_7) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_U) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_J) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_M) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xF7;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_RIGHT) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xDF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_UP) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xBF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_BREAK) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_GRAPH) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_CTRL ||
        ((sk1100_t *)sk1100)->ctrl_pressed) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_SHIFT ||
        ((sk1100_t *)sk1100)->shift_pressed) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xF7;
    }
    break;

  case 7:
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_1_UP) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_1_DOWN) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_1_LEFT) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_1_RIGHT) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xF7;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_1_BUTTON_1) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xEF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_1_BUTTON_2) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xDF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_2_UP) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0xBF;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_2_DOWN) {
      ((sk1100_t *)sk1100)->ppi_port_a &= 0x7F;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_2_LEFT) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFE;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_2_RIGHT) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFD;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_2_BUTTON_1) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xFB;
    }
    if (((sk1100_t *)sk1100)->key_pressed == SK1100_KEY_JOYPAD_2_BUTTON_2) {
      ((sk1100_t *)sk1100)->ppi_port_b &= 0xF7;
    }
    break;
  }
}



static uint8_t sk1100_ppi_port_a_read(void *sk1100, uint8_t upper_address)
{
  (void)upper_address;
  return ((sk1100_t *)sk1100)->ppi_port_a;
}



static uint8_t sk1100_ppi_port_b_read(void *sk1100, uint8_t upper_address)
{
  (void)upper_address;
  return ((sk1100_t *)sk1100)->ppi_port_b;
}



static uint8_t sk1100_ppi_port_c_read(void *sk1100, uint8_t upper_address)
{
  (void)upper_address;
  return ((sk1100_t *)sk1100)->ppi_port_c;
}



void sk1100_ppi_ctrl_write(void *sk1100, uint8_t value, uint8_t upper_address)
{
  (void)upper_address;
  ((sk1100_t *)sk1100)->ppi_ctrl = value;
}



void sk1100_init(sk1100_t *sk1100, z80_t *z80)
{
  sk1100->key_pressed = SK1100_KEY_NONE;
  sk1100->shift_pressed = false;
  sk1100->ctrl_pressed  = false;
  sk1100->func_pressed  = false;
  sk1100->key_hold = 0;

  sk1100->ppi_port_a = 0xFF;
  sk1100->ppi_port_b = 0x7F;
  sk1100->ppi_port_c = 0xFF;
  sk1100->ppi_ctrl = 0x00;

  sk1100->cassette_load_fh = NULL;
  sk1100->cassette_load_cycle = 0;
  sk1100->cassette_load_sample = 0;

  sk1100->cassette_save_fh = NULL;
  sk1100->cassette_save_cycle = 0;
  sk1100->cassette_save_sample_count = 0;
  sk1100->cassette_save_idle_count = 0;
  sk1100->cassette_save_high_seen = false;

  z80->io_read[SK1100_PPI_PORT_A_READ].func   = sk1100_ppi_port_a_read;
  z80->io_read[SK1100_PPI_PORT_B_READ].func   = sk1100_ppi_port_b_read;
  z80->io_read[SK1100_PPI_PORT_C_READ].func   = sk1100_ppi_port_c_read;
  z80->io_write[SK1100_PPI_PORT_C_WRITE].func = sk1100_ppi_port_c_write;
  z80->io_write[SK1100_PPI_CTRL_WRITE].func   = sk1100_ppi_ctrl_write;

  z80->io_read[SK1100_PPI_PORT_A_READ].cookie   = sk1100;
  z80->io_read[SK1100_PPI_PORT_B_READ].cookie   = sk1100;
  z80->io_read[SK1100_PPI_PORT_C_READ].cookie   = sk1100;
  z80->io_write[SK1100_PPI_PORT_C_WRITE].cookie = sk1100;
  z80->io_write[SK1100_PPI_CTRL_WRITE].cookie   = sk1100;
}



int sk1100_cassette_load_file(sk1100_t *sk1100, const char *filename)
{
  wav_header_t header;

  if (sk1100->cassette_load_fh != NULL) {
    return -2; /* Load already in progress. */
  }

  sk1100->cassette_load_fh = fopen(filename, "rb");
  if (sk1100->cassette_load_fh == NULL) {
    return -1; /* File not found. */
  }

  if (fread(&header, sizeof(wav_header_t), 1, sk1100->cassette_load_fh) != 1) {
    fclose(sk1100->cassette_load_fh);
    sk1100->cassette_load_fh = NULL;
    return -3; /* Unable to read header. */
  }

  if (header.riff_string[0] != 'R') {
    fclose(sk1100->cassette_load_fh);
    sk1100->cassette_load_fh = NULL;
    return -4; /* Not a WAV file? */
  }

  if (header.sample_rate != SK1100_CASSETTE_WAV_SAMPLE_RATE) {
    fclose(sk1100->cassette_load_fh);
    sk1100->cassette_load_fh = NULL;
    return -5; /* Unsupported sample rate. */
  }

  if (header.channels != 1) {
    fclose(sk1100->cassette_load_fh);
    sk1100->cassette_load_fh = NULL;
    return -6; /* Unsupported channels. */
  }

  if (header.bits_per_sample != 8) {
    fclose(sk1100->cassette_load_fh);
    sk1100->cassette_load_fh = NULL;
    return -7; /* Unsupported BPS. */
  }

  return 0;
}



int sk1100_cassette_save_file(sk1100_t *sk1100, const char *filename)
{
  wav_header_t header;

  if (sk1100->cassette_save_fh != NULL) {
    return -2; /* Save already in progress. */
  }

  sk1100->cassette_save_fh = fopen(filename, "wb");
  if (sk1100->cassette_save_fh == NULL) {
    return -1; /* File not found. */
  }

  sk1100->cassette_save_sample_count = 0;

  /* Prepare and write WAV header: */
  header.riff_string[0] = 'R';
  header.riff_string[1] = 'I';
  header.riff_string[2] = 'F';
  header.riff_string[3] = 'F';
  header.chunk_size = 0; /* Unknown until finished. */
  header.wave_string[0] = 'W';
  header.wave_string[1] = 'A';
  header.wave_string[2] = 'V';
  header.wave_string[3] = 'E';
  header.fmt_string[0] = 'f';
  header.fmt_string[1] = 'm';
  header.fmt_string[2] = 't';
  header.fmt_string[3] = ' ';
  header.subchunk1_size = 16;
  header.audio_format = 1; /* PCM */
  header.channels = 1; /* Mono */
  header.sample_rate = SK1100_CASSETTE_WAV_SAMPLE_RATE;
  header.byte_rate = SK1100_CASSETTE_WAV_SAMPLE_RATE;
  header.block_align = 1;
  header.bits_per_sample = 8;
  header.data_string[0] = 'd';
  header.data_string[1] = 'a';
  header.data_string[2] = 't';
  header.data_string[3] = 'a';
  header.subchunk2_size = 0; /* Unknown until finished. */

  fwrite(&header, sizeof(wav_header_t), 1, sk1100->cassette_save_fh);
  return 0;
}



static void sk1100_cassette_save_file_stop(sk1100_t *sk1100)
{
  uint32_t chunk_size;
  uint32_t subchunk2_size;

  subchunk2_size = sk1100->cassette_save_sample_count;
  chunk_size = subchunk2_size + 36;

  /* Update WAV header with chunk sizes before closing: */
  fseek(sk1100->cassette_save_fh, 4, SEEK_SET);
  fwrite(&chunk_size, sizeof(uint32_t), 1, sk1100->cassette_save_fh);
  fseek(sk1100->cassette_save_fh, 40, SEEK_SET);
  fwrite(&subchunk2_size, sizeof(uint32_t), 1, sk1100->cassette_save_fh);

  fclose(sk1100->cassette_save_fh);
  sk1100->cassette_save_fh = NULL;
}



static bool sk1100_cassette_load_sample(sk1100_t *sk1100)
{
  sk1100->cassette_load_cycle++;

  if (sk1100->cassette_load_cycle %
    (SK1100_CASSETTE_INTERNAL_SAMPLE_RATE /
     SK1100_CASSETTE_WAV_SAMPLE_RATE) == 0) {

    if (fread(&sk1100->cassette_load_sample, sizeof(uint8_t), 1,
      sk1100->cassette_load_fh) != 1) {
      fclose(sk1100->cassette_load_fh);
      sk1100->cassette_load_fh = NULL;
      sk1100->cassette_load_cycle = 0;
      sk1100->cassette_load_sample = 0;
    }
  }

  if (sk1100->cassette_load_sample > 128) {
    return true;
  } else {
    return false;
  }
}



static void sk1100_cassette_save_sample(sk1100_t *sk1100, bool level)
{
  uint8_t sample;

  sk1100->cassette_save_cycle++;

  if (sk1100->cassette_save_cycle %
    (SK1100_CASSETTE_INTERNAL_SAMPLE_RATE /
     SK1100_CASSETTE_WAV_SAMPLE_RATE) == 0) {
    if (level == true) {
      sample = UINT8_MAX;
    } else {
      sample = 0;
    }
    fwrite(&sample, sizeof(uint8_t), 1, sk1100->cassette_save_fh);
    sk1100->cassette_save_sample_count++;
  }
}



void sk1100_execute_sync(sk1100_t *sk1100)
{
  /* Cassette Loading */
  if (sk1100->cassette_load_fh != NULL) {
    if (sk1100_cassette_load_sample(sk1100) == true) {
      sk1100->ppi_port_b |= ~0x7F;
    } else {
      sk1100->ppi_port_b &= 0x7F;
    }
  }

  /* Cassette Saving */
  if (sk1100->cassette_save_fh != NULL) {
    if (sk1100->ppi_ctrl == 0x09) { /* High */
      sk1100_cassette_save_sample(sk1100, true);
      sk1100->cassette_save_idle_count = 0;
      sk1100->cassette_save_high_seen = true;

    } else if (sk1100->ppi_ctrl == 0x08) { /* Low */
      /* Don't save initial low samples. */
      if (sk1100->cassette_save_high_seen) {
        sk1100_cassette_save_sample(sk1100, false);
        sk1100->cassette_save_idle_count++;
        if (sk1100->cassette_save_idle_count > SK1100_CASSETTE_SAVE_IDLE_STOP) {
          sk1100_cassette_save_file_stop(sk1100);
          sk1100->cassette_save_high_seen = false;
        }
      }
    }
  }
}



void sk1100_execute_frame(sk1100_t *sk1100)
{
  if (sk1100->key_hold > 0) {
    sk1100->key_hold--;
    return;
  }

  sk1100->key_pressed   = SK1100_KEY_NONE;
  sk1100->shift_pressed = false;
  sk1100->ctrl_pressed  = false;
  sk1100->func_pressed  = false;
}



void sk1100_key_press(sk1100_t *sk1100, SK1100_KEY key,
  bool shift, bool ctrl, bool func)
{
  sk1100->key_pressed   = key;
  sk1100->shift_pressed = shift;
  sk1100->ctrl_pressed  = ctrl;
  sk1100->func_pressed  = func;

  /* Hold keypress for 2 frames, to compensate for software that does
     not scan the keyboard on every frame. */
  sk1100->key_hold = 2;
}



