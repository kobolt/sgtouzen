#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "z80.h"
#include "mem.h"
#include "panic.h"



typedef enum {
  /* 8-Bit registers */
  DT_R_B,
  DT_R_C,
  DT_R_D,
  DT_R_E,
  DT_R_H,
  DT_R_L,
  DT_R_HLI,
  DT_R_A,
  DT_R_IXH,
  DT_R_IXL,
  DT_R_IXI,
  DT_R_IYH,
  DT_R_IYL,
  DT_R_IYI,

  /* Register pairs */
  DT_RP_BC,
  DT_RP_DE,
  DT_RP_HL,
  DT_RP_SP,
  DT_RP_AF,
  DT_RP_IX,
  DT_RP_IY,

  /* Conditions */
  DT_CC_NZ,
  DT_CC_Z,
  DT_CC_NC,
  DT_CC_C,
  DT_CC_PO,
  DT_CC_PE,
  DT_CC_P,
  DT_CC_M,

  /* Arithmetic/Logic operations */
  DT_ALU_ADD,
  DT_ALU_ADC,
  DT_ALU_SUB,
  DT_ALU_SBC,
  DT_ALU_AND,
  DT_ALU_XOR,
  DT_ALU_OR,
  DT_ALU_CP,

  /* Rotation/Shift operations */
  DT_ROT_RLC,
  DT_ROT_RRC,
  DT_ROT_RL,
  DT_ROT_RR,
  DT_ROT_SLA,
  DT_ROT_SRA,
  DT_ROT_SLL,
  DT_ROT_SRL,

  /* Interrupt modes */
  DT_IM_0,
  DT_IM_01,
  DT_IM_1,
  DT_IM_2,

  /* Block instructions */
  DT_BLI_LDI,
  DT_BLI_LDD,
  DT_BLI_LDIR,
  DT_BLI_LDDR,
  DT_BLI_CPI,
  DT_BLI_CPD,
  DT_BLI_CPIR,
  DT_BLI_CPDR,
  DT_BLI_INI,
  DT_BLI_IND,
  DT_BLI_INIR,
  DT_BLI_INDR,
  DT_BLI_OUTI,
  DT_BLI_OUTD,
  DT_BLI_OTIR,
  DT_BLI_OTDR,
} dt_t;



static dt_t dt_r[8] =
  {DT_R_B, DT_R_C, DT_R_D, DT_R_E, DT_R_H, DT_R_L, DT_R_HLI, DT_R_A};

static dt_t dt_rix[8] =
  {DT_R_B, DT_R_C, DT_R_D, DT_R_E, DT_R_IXH, DT_R_IXL, DT_R_IXI, DT_R_A};

static dt_t dt_riy[8] =
  {DT_R_B, DT_R_C, DT_R_D, DT_R_E, DT_R_IYH, DT_R_IYL, DT_R_IYI, DT_R_A};

static dt_t dt_rp[4] =
  {DT_RP_BC, DT_RP_DE, DT_RP_HL, DT_RP_SP};

static dt_t dt_rpaf[4] =
  {DT_RP_BC, DT_RP_DE, DT_RP_HL, DT_RP_AF};

static dt_t dt_rpix[4] =
  {DT_RP_BC, DT_RP_DE, DT_RP_IX, DT_RP_SP};

static dt_t dt_rpiy[4] =
  {DT_RP_BC, DT_RP_DE, DT_RP_IY, DT_RP_SP};

static dt_t dt_cc[8] =
  {DT_CC_NZ, DT_CC_Z, DT_CC_NC, DT_CC_C, DT_CC_PO, DT_CC_PE, DT_CC_P, DT_CC_M};

static dt_t dt_alu[8] = 
  {DT_ALU_ADD, DT_ALU_ADC, DT_ALU_SUB, DT_ALU_SBC,
   DT_ALU_AND, DT_ALU_XOR, DT_ALU_OR,  DT_ALU_CP};

static dt_t dt_rot[8] =
  {DT_ROT_RLC, DT_ROT_RRC, DT_ROT_RL,  DT_ROT_RR,
   DT_ROT_SLA, DT_ROT_SRA, DT_ROT_SLL, DT_ROT_SRL};

static dt_t dt_im[8] =
  {DT_IM_0, DT_IM_01, DT_IM_1, DT_IM_2, DT_IM_0, DT_IM_01, DT_IM_1, DT_IM_2};

static dt_t dt_bli[4][4] =
  {{DT_BLI_LDI,  DT_BLI_CPI,  DT_BLI_INI,  DT_BLI_OUTI},
   {DT_BLI_LDD,  DT_BLI_CPD,  DT_BLI_IND,  DT_BLI_OUTD},
   {DT_BLI_LDIR, DT_BLI_CPIR, DT_BLI_INIR, DT_BLI_OTIR},
   {DT_BLI_LDDR, DT_BLI_CPDR, DT_BLI_INDR, DT_BLI_OTDR}};



#ifdef DISABLE_Z80_TRACE
#define z80_trace(...)
#else

#define Z80_TRACE_BUFFER_SIZE 1024
#define Z80_TRACE_BUFFER_ENTRY 80

static char z80_trace_buffer[Z80_TRACE_BUFFER_SIZE][Z80_TRACE_BUFFER_ENTRY];
static int z80_trace_buffer_index = 0;

static char *dt_text(dt_t sym)
{
  switch (sym) {
  case DT_R_B:      return "B";
  case DT_R_C:      return "C";
  case DT_R_D:      return "D";
  case DT_R_E:      return "E";
  case DT_R_H:      return "H";
  case DT_R_L:      return "L";
  case DT_R_HLI:    return "(HL)";
  case DT_R_A:      return "A";
  case DT_R_IXH:    return "IXH";
  case DT_R_IXL:    return "IXL";
  case DT_R_IXI:    return "(IX)";
  case DT_R_IYH:    return "IYH";
  case DT_R_IYL:    return "IYL";
  case DT_R_IYI:    return "(IY)";
  case DT_RP_BC:    return "BC";
  case DT_RP_DE:    return "DE";
  case DT_RP_HL:    return "HL";
  case DT_RP_SP:    return "SP";
  case DT_RP_AF:    return "AF";
  case DT_RP_IX:    return "IX";
  case DT_RP_IY:    return "IY";
  case DT_CC_NZ:    return "NZ";
  case DT_CC_Z:     return "Z";
  case DT_CC_NC:    return "NC";
  case DT_CC_C:     return "C";
  case DT_CC_PO:    return "PO";
  case DT_CC_PE:    return "PE";
  case DT_CC_P:     return "P";
  case DT_CC_M:     return "M";
  case DT_ALU_ADD:  return "ADD A,";
  case DT_ALU_ADC:  return "ADC A,";
  case DT_ALU_SUB:  return "SUB A,";
  case DT_ALU_SBC:  return "SBC A,";
  case DT_ALU_AND:  return "AND A,";
  case DT_ALU_XOR:  return "XOR A,";
  case DT_ALU_OR:   return "OR A,";
  case DT_ALU_CP:   return "CP A,";
  case DT_ROT_RLC:  return "RLC";
  case DT_ROT_RRC:  return "RRC";
  case DT_ROT_RL:   return "RL";
  case DT_ROT_RR:   return "RR";
  case DT_ROT_SLA:  return "SLA";
  case DT_ROT_SRA:  return "SRA";
  case DT_ROT_SLL:  return "SLL";
  case DT_ROT_SRL:  return "SRL";
  case DT_IM_0:     return "0";
  case DT_IM_01:    return "0/1";
  case DT_IM_1:     return "1";
  case DT_IM_2:     return "2";
  case DT_BLI_LDI:  return "LDI";
  case DT_BLI_LDD:  return "LDD";
  case DT_BLI_LDIR: return "LDIR";
  case DT_BLI_LDDR: return "LDDR";
  case DT_BLI_CPI:  return "CPI";
  case DT_BLI_CPD:  return "CPD";
  case DT_BLI_CPIR: return "CPIR";
  case DT_BLI_CPDR: return "CPDR";
  case DT_BLI_INI:  return "INI";
  case DT_BLI_IND:  return "IND";
  case DT_BLI_INIR: return "INIR";
  case DT_BLI_INDR: return "INDR";
  case DT_BLI_OUTI: return "OUTI";
  case DT_BLI_OUTD: return "OUTD";
  case DT_BLI_OTIR: return "OTIR";
  case DT_BLI_OTDR: return "OTDR";
  default:
    return "?";
  }
}



static void z80_trace(z80_t *z80, mem_t *mem, int no, const char *format, ...)
{
  va_list args;
  char buffer[Z80_TRACE_BUFFER_ENTRY + 2];
  int n = 0;

  n += snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n,
    "%04x   ", z80->pc);

  switch (no) {
  case 1:
    n += snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n,
      "%02x            ",
      mem_read(mem, z80->pc));
    break;
  case 2:
    n += snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n,
      "%02x %02x         ",
      mem_read(mem, z80->pc), mem_read(mem, z80->pc + 1));
    break;
  case 3:
    n += snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n,
      "%02x %02x %02x      ",
      mem_read(mem, z80->pc), mem_read(mem, z80->pc + 1),
      mem_read(mem, z80->pc + 2));
    break;
  case 4:
    n += snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n,
      "%02x %02x %02x %02x   ",
      mem_read(mem, z80->pc), mem_read(mem, z80->pc + 1),
      mem_read(mem, z80->pc + 2), mem_read(mem, z80->pc + 3));
    break;
  case 0:
  default:
    n += snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n,
      "              ");
    break;
  }

  va_start(args, format);
  n += vsnprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n, format, args);
  va_end(args);

  while (36 - n > 0) {
    buffer[n] = ' ';
    n++;
    if (n >= Z80_TRACE_BUFFER_ENTRY) {
      break;
    }
  }

  n += snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n,
    "%02x %04x %04x %04x %04x %04x %04x ",
    z80->a, z80->bc, z80->de, z80->hl, z80->sp, z80->ix, z80->iy);
    
  n += snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n, "%c%c%c%c%c%c%c",
    z80->flag.s  ? 'S' : '.',
    z80->flag.z  ? 'Z' : '.',
    z80->flag.h  ? 'H' : '.',
    z80->flag.pv ? 'P' : '.',
    z80->flag.n  ? 'N' : '.',
    z80->flag.c  ? 'C' : '.',
    z80->iff1    ? 'I' : '.');

  snprintf(&buffer[n], Z80_TRACE_BUFFER_ENTRY - n, "\n");

  strncpy(z80_trace_buffer[z80_trace_buffer_index],
    buffer, Z80_TRACE_BUFFER_ENTRY);
  z80_trace_buffer_index++;
  if (z80_trace_buffer_index >= Z80_TRACE_BUFFER_SIZE) {
    z80_trace_buffer_index = 0;
  }
}



void z80_trace_init(void)
{
  for (int i = 0; i < Z80_TRACE_BUFFER_SIZE; i++) {
    z80_trace_buffer[i][0] = '\0';
  }
  z80_trace_buffer_index = 0;
}



void z80_trace_dump(FILE *fh)
{
  fprintf(fh, "PC:    Code:         Mnemonics:     "
              "A: BC:  DE:  HL:  SP:  IX:  IY:  Flags:\n");
  for (int i = z80_trace_buffer_index; i < Z80_TRACE_BUFFER_SIZE; i++) {
    if (z80_trace_buffer[i][0] != '\0') {
      fprintf(fh, z80_trace_buffer[i]);
    }
  }
  for (int i = 0; i < z80_trace_buffer_index; i++) {
    if (z80_trace_buffer[i][0] != '\0') {
      fprintf(fh, z80_trace_buffer[i]);
    }
  }
}



void z80_dump(FILE *fh, z80_t *z80, mem_t *mem)
{
  fprintf(fh,
    "%04x   %02x %02x %02x %02x   -              "
    "%02x %04x %04x %04x %04x %04x %04x ",
    z80->pc, mem_read(mem, z80->pc),     mem_read(mem, z80->pc + 1),
             mem_read(mem, z80->pc + 2), mem_read(mem, z80->pc + 3),
    z80->a, z80->bc, z80->de, z80->hl, z80->sp, z80->ix, z80->iy);

  fprintf(fh, "%c%c%c%c%c%c%c",
    z80->flag.s  ? 'S' : '.',
    z80->flag.z  ? 'Z' : '.',
    z80->flag.h  ? 'H' : '.',
    z80->flag.pv ? 'P' : '.',
    z80->flag.n  ? 'N' : '.',
    z80->flag.c  ? 'C' : '.',
    z80->iff1    ? 'I' : '.');

  fprintf(fh, "\n");
}
#endif /* DISABLE_Z80_TRACE */



static inline uint8_t reset_bit(uint8_t value, int bit_no)
{
  return value & ~(1 << bit_no);
}



static inline uint8_t set_bit(uint8_t value, int bit_no)
{
  return value | (1 << bit_no);
}



static inline bool parity_even(uint8_t value)
{
  value ^= value >> 4;
  value ^= value >> 2;
  value ^= value >> 1;
  return (~value) & 1;
}



static uint8_t z80_io_read(z80_t *z80, uint8_t port, uint8_t upper_address)
{
  if (z80->io_read[port].func != NULL) {
    return (z80->io_read[port].func)(z80->io_read[port].cookie, upper_address);
  } else {
    panic("Unknown IO port read: %02x (upper: %02x)\n", port, upper_address);
    return 0;
  }
}



static uint8_t z80_io_read_with_flag(z80_t *z80, uint8_t port,
  uint8_t upper_address)
{
  uint8_t value;
  value = z80_io_read(z80, port, upper_address);
  z80->flag.pv = parity_even(value);
  z80->flag.n = 0;
  z80->flag.h = 0;
  z80->flag.s = (value & 0x80) ? 1 : 0;
  z80->flag.z = (value == 0) ? 1 : 0;
  return value;
}



static void z80_io_write(z80_t *z80, uint8_t port, uint8_t upper_address,
  uint8_t value)
{
  if (z80->io_write[port].func != NULL) {
    (z80->io_write[port].func)(z80->io_write[port].cookie,
      value, upper_address);
  } else {
    panic("Unknown IO port write: %02x (upper: %02x) (value: %02x)\n",
      port, upper_address, value);
  }
}



static inline void z80_add(z80_t *z80, uint8_t value)
{
  uint16_t result = z80->a + value;
  z80->flag.h = ((z80->a & 0xF) + (value & 0xF)) & 0x10 ? 1 : 0;
  z80->flag.pv = ((z80->a & 0x80) == (value  & 0x80)) &&
                 ((value  & 0x80) != (result & 0x80)) ? 1 : 0;
  z80->flag.n = 0;
  z80->flag.c = (result & 0x100) ? 1 : 0;
  z80->a = result;
  z80->flag.s = (z80->a & 0x80) ? 1 : 0;
  z80->flag.z = (z80->a == 0) ? 1 : 0;
}



static inline void z80_adc(z80_t *z80, uint8_t value)
{
  uint16_t result = z80->a + value + z80->flag.c;
  z80->flag.h = (((z80->a & 0xF) + (value & 0xF)) + z80->flag.c) & 0x10 ? 1 : 0;
  z80->flag.pv = ((z80->a & 0x80) == (value  & 0x80)) &&
                 ((value  & 0x80) != (result & 0x80)) ? 1 : 0;
  z80->flag.n = 0;
  z80->flag.c = (result & 0x100) ? 1 : 0;
  z80->a = result;
  z80->flag.s = (z80->a & 0x80) ? 1 : 0;
  z80->flag.z = (z80->a == 0) ? 1 : 0;
}



static inline void z80_sub(z80_t *z80, uint8_t value)
{
  uint16_t result = z80->a - value;
  z80->flag.h = ((z80->a & 0xF) - (value & 0xF)) & 0x10 ? 1 : 0;
  z80->flag.pv = ((z80->a & 0x80) != (value  & 0x80)) &&
                 ((value  & 0x80) == (result & 0x80)) ? 1 : 0;
  z80->flag.n = 1;
  z80->flag.c = (result & 0x100) ? 1 : 0;
  z80->a = result;
  z80->flag.s = (z80->a & 0x80) ? 1 : 0;
  z80->flag.z = (z80->a == 0) ? 1 : 0;
}



static inline void z80_sbc(z80_t *z80, uint8_t value)
{
  uint16_t result = z80->a - value - z80->flag.c;
  z80->flag.h = (((z80->a & 0xF) - (value & 0xF)) - z80->flag.c) & 0x10 ? 1 : 0;
  z80->flag.pv = ((z80->a & 0x80) != (value  & 0x80)) &&
                 ((value  & 0x80) == (result & 0x80)) ? 1 : 0;
  z80->flag.n = 1;
  z80->flag.c = (result & 0x100) ? 1 : 0;
  z80->a = result;
  z80->flag.s = (z80->a & 0x80) ? 1 : 0;
  z80->flag.z = (z80->a == 0) ? 1 : 0;
}



static inline void z80_and(z80_t *z80, uint8_t value)
{
  z80->a &= value;
  z80->flag.s = (z80->a & 0x80) ? 1 : 0;
  z80->flag.z = (z80->a == 0) ? 1 : 0;
  z80->flag.h = 1;
  z80->flag.pv = parity_even(z80->a);
  z80->flag.n = 0;
  z80->flag.c = 0;
}



static inline void z80_or(z80_t *z80, uint8_t value)
{
  z80->a |= value;
  z80->flag.s = (z80->a & 0x80) ? 1 : 0;
  z80->flag.z = (z80->a == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(z80->a);
  z80->flag.n = 0;
  z80->flag.c = 0;
}



static inline void z80_xor(z80_t *z80, uint8_t value)
{
  z80->a ^= value;
  z80->flag.s = (z80->a & 0x80) ? 1 : 0;
  z80->flag.z = (z80->a == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(z80->a);
  z80->flag.n = 0;
  z80->flag.c = 0;
}



static inline void z80_compare(z80_t *z80, uint8_t value)
{
  uint16_t result = z80->a - value;
  z80->flag.h = ((z80->a & 0xF) - (value & 0xF)) & 0x10 ? 1 : 0;
  z80->flag.pv = ((z80->a & 0x80) != (value  & 0x80)) &&
                 ((value  & 0x80) == (result & 0x80)) ? 1 : 0;
  z80->flag.n = 1;
  z80->flag.c = (result & 0x100) ? 1 : 0;
  z80->flag.s = (result & 0x80) ? 1 : 0;
  z80->flag.z = (result == 0) ? 1 : 0;
}



static inline void z80_add_16(z80_t *z80, uint16_t *reg, uint16_t value)
{
  uint32_t result = *reg + value;
  z80->flag.h = ((*reg & 0xFFF) + (value & 0xFFF)) & 0x1000 ? 1 : 0;
  z80->flag.n = 0;
  z80->flag.c = (result & 0x10000) ? 1 : 0;
  *reg = result;
}



static inline void z80_adc_16(z80_t *z80, uint16_t *reg, uint16_t value)
{
  uint32_t result = *reg + value + z80->flag.c;
  z80->flag.h = ((*reg & 0xFFF) + (value & 0xFFF) + z80->flag.c) &
                 0x1000 ? 1 : 0;
  z80->flag.pv = ((*reg  & 0x8000) == (value  & 0x8000)) &&
                 ((value & 0x8000) != (result & 0x8000)) ? 1 : 0;
  z80->flag.n = 0;
  z80->flag.c = (result & 0x10000) ? 1 : 0;
  *reg = result;
  z80->flag.s = (*reg & 0x8000) ? 1 : 0;
  z80->flag.z = (*reg == 0) ? 1 : 0;
}



static inline void z80_sbc_16(z80_t *z80, uint16_t *reg, uint16_t value)
{
  uint32_t result = *reg - value - z80->flag.c;
  z80->flag.h = ((*reg & 0xFFF) - (value & 0xFFF) - z80->flag.c) &
                 0x1000 ? 1 : 0;
  z80->flag.pv = ((*reg  & 0x8000) != (value  & 0x8000)) &&
                 ((value & 0x8000) == (result & 0x8000)) ? 1 : 0;
  z80->flag.n = 1;
  z80->flag.c = (result & 0x10000) ? 1 : 0;
  *reg = result;
  z80->flag.s = (*reg & 0x8000) ? 1 : 0;
  z80->flag.z = (*reg == 0) ? 1 : 0;
}



static inline void z80_bit(z80_t *z80, uint8_t value, int bit_no)
{
  z80->flag.z = !((value >> bit_no) & 0x1) ? 1 : 0;
  z80->flag.h = 1;
  z80->flag.n = 0;
}



static inline void z80_cpd(z80_t *z80, mem_t *mem)
{
  bool old_c = z80->flag.c;
  z80_compare(z80, mem_read(mem, z80->hl));
  z80->hl--;
  z80->bc--;
  z80->flag.pv = (z80->bc != 0) ? 1 : 0;
  z80->flag.c = old_c;
}



static inline void z80_cpi(z80_t *z80, mem_t *mem)
{
  bool old_c = z80->flag.c;
  z80_compare(z80, mem_read(mem, z80->hl));
  z80->hl++;
  z80->bc--;
  z80->flag.pv = (z80->bc != 0) ? 1 : 0;
  z80->flag.c = old_c;
}



static inline void z80_ldd(z80_t *z80, mem_t *mem)
{
  mem_write(mem, z80->de, mem_read(mem, z80->hl));
  z80->de--;
  z80->hl--;
  z80->bc--;
  z80->flag.h = 0;
  z80->flag.pv = (z80->bc != 0) ? 1 : 0;
  z80->flag.n = 0;
}



static inline void z80_ldi(z80_t *z80, mem_t *mem)
{
  mem_write(mem, z80->de, mem_read(mem, z80->hl));
  z80->de++;
  z80->hl++;
  z80->bc--;
  z80->flag.h = 0;
  z80->flag.pv = (z80->bc != 0) ? 1 : 0;
  z80->flag.n = 0;
}



static inline void z80_ind(z80_t *z80, mem_t *mem)
{
  uint8_t value;
  value = z80_io_read(z80, z80->c, z80->b);
  z80->b--;
  mem_write(mem, z80->hl, value);
  z80->hl--;
  z80->flag.n = 1;
  z80->flag.z = (z80->b == 0) ? 1 : 0;
}



static inline void z80_ini(z80_t *z80, mem_t *mem)
{
  uint8_t value;
  value = z80_io_read(z80, z80->c, z80->b);
  z80->b--;
  mem_write(mem, z80->hl, value);
  z80->hl++;
  z80->flag.n = 1;
  z80->flag.z = (z80->b == 0) ? 1 : 0;
}



static inline void z80_outd(z80_t *z80, mem_t *mem)
{
  uint8_t value;
  value = mem_read(mem, z80->hl);
  z80->b--;
  z80_io_write(z80, z80->c, z80->b, value);
  z80->hl--;
  z80->flag.n = 1;
  z80->flag.z = (z80->b == 0) ? 1 : 0;
}



static inline void z80_outi(z80_t *z80, mem_t *mem)
{
  uint8_t value;
  value = mem_read(mem, z80->hl);
  z80->b--;
  z80_io_write(z80, z80->c, z80->b, value);
  z80->hl++;
  z80->flag.n = 1;
  z80->flag.z = (z80->b == 0) ? 1 : 0;
}



static inline uint8_t z80_inc(z80_t *z80, uint8_t input)
{
  uint8_t output;
  output = input + 1;
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = !(output & 0xF) ? 1 : 0;
  z80->flag.pv = (input == 0x7F) ? 1 : 0;
  z80->flag.n = 0;
  return output;
}



static inline uint8_t z80_dec(z80_t *z80, uint8_t input)
{
  uint8_t output;
  output = input - 1;
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = !(input & 0xF) ? 1 : 0;
  z80->flag.pv = (input == 0x80) ? 1 : 0;
  z80->flag.n = 1;
  return output;
}



static inline uint8_t z80_rlc(z80_t *z80, uint8_t input)
{
  uint8_t output = input << 1;
  if (input & 0x80) {
    output |= 0x01;
  }
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(output);
  z80->flag.n = 0;
  z80->flag.c = (input & 0x80) ? 1 : 0;
  return output;
}



static inline uint8_t z80_rrc(z80_t *z80, uint8_t input)
{
  uint8_t output = input >> 1;
  if (input & 0x01) {
    output |= 0x80;
  }
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(output);
  z80->flag.n = 0;
  z80->flag.c = (input & 0x01) ? 1 : 0;
  return output;
}



static inline uint8_t z80_rl(z80_t *z80, uint8_t input)
{
  uint8_t output = input << 1;
  if (z80->flag.c) {
    output |= 0x01;
  }
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(output);
  z80->flag.n = 0;
  z80->flag.c = (input & 0x80) ? 1 : 0;
  return output;
}



static inline uint8_t z80_rr(z80_t *z80, uint8_t input)
{
  uint8_t output = input >> 1;
  if (z80->flag.c) {
    output |= 0x80;
  }
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(output);
  z80->flag.n = 0;
  z80->flag.c = (input & 0x01) ? 1 : 0;
  return output;
}



static inline uint8_t z80_sla(z80_t *z80, uint8_t input)
{
  uint8_t output = input << 1;
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(output);
  z80->flag.n = 0;
  z80->flag.c = (input & 0x80) ? 1 : 0;
  return output;
}



static inline uint8_t z80_sra(z80_t *z80, uint8_t input)
{
  uint8_t output = input >> 1;
  output |= input & 0x80;
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(output);
  z80->flag.n = 0;
  z80->flag.c = (input & 0x01) ? 1 : 0;
  return output;
}



static inline uint8_t z80_sll(z80_t *z80, uint8_t input)
{
  uint8_t output = input << 1;
  output |= 0x01;
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(output);
  z80->flag.n = 0;
  z80->flag.c = (input & 0x80) ? 1 : 0;
  return output;
}



static inline uint8_t z80_srl(z80_t *z80, uint8_t input)
{
  uint8_t output = input >> 1;
  z80->flag.s = (output & 0x80) ? 1 : 0;
  z80->flag.z = (output == 0) ? 1 : 0;
  z80->flag.h = 0;
  z80->flag.pv = parity_even(output);
  z80->flag.n = 0;
  z80->flag.c = (input & 0x01) ? 1 : 0;
  return output;
}



void z80_init(z80_t *z80)
{
  memset(z80, 0, sizeof(z80_t));
}



void z80_execute(z80_t *z80, mem_t *mem)
{
  /* Implementing decoding logic described at: http://z80.info/decoding.htm */
  uint8_t mc[4];

  if (z80->halted) {
    z80->r = (z80->r + 1) & 0x7F;
    z80->cycles += 4;
    return;
  }

  mc[0] = mem_read(mem, z80->pc);
  mc[1] = mem_read(mem, z80->pc + 1);
  mc[2] = mem_read(mem, z80->pc + 2);
  mc[3] = mem_read(mem, z80->pc + 3);

  if (mc[0] == 0xDD && mc[1] == 0xCB) {
    /* DDCB prefixed */
    int8_t displacement = mc[2];
    uint8_t x =  mc[3] >> 6;
    uint8_t y = (mc[3] >> 3) & 0b111;
    uint8_t z =  mc[3] & 0b111;

    z80->r = (z80->r + 2) & 0x7F;

    if (0 == x && 6 == z) {
      dt_t dt = dt_rot[y];
      z80_trace(z80, mem, 4, "%s (IX)", dt_text(dt));
      switch (dt) {
      case DT_ROT_RLC: 
        mem_write(mem, z80->ix + displacement,
          z80_rlc(z80, mem_read(mem, z80->ix + displacement)));
        break;
      case DT_ROT_RRC: 
        mem_write(mem, z80->ix + displacement,
          z80_rrc(z80, mem_read(mem, z80->ix + displacement)));
        break;
      case DT_ROT_RL:  
        mem_write(mem, z80->ix + displacement,
          z80_rl(z80, mem_read(mem, z80->ix + displacement)));
        break;
      case DT_ROT_RR:  
        mem_write(mem, z80->ix + displacement,
          z80_rr(z80, mem_read(mem, z80->ix + displacement)));
        break;
      case DT_ROT_SLA: 
        mem_write(mem, z80->ix + displacement,
          z80_sla(z80, mem_read(mem, z80->ix + displacement)));
        break;
      case DT_ROT_SRA: 
        mem_write(mem, z80->ix + displacement,
          z80_sra(z80, mem_read(mem, z80->ix + displacement)));
        break;
      case DT_ROT_SLL: 
        mem_write(mem, z80->ix + displacement,
          z80_sll(z80, mem_read(mem, z80->ix + displacement)));
        break;
      case DT_ROT_SRL: 
        mem_write(mem, z80->ix + displacement,
          z80_srl(z80, mem_read(mem, z80->ix + displacement)));
        break;
      default: break;
      }
      z80->pc += 4;
      z80->cycles += 23;

    } else if (1 == x) {
      z80_trace(z80, mem, 4, "BIT %d,(IX)", y);
      z80_bit(z80, mem_read(mem, z80->ix + displacement), y);
      z80->pc += 4;
      z80->cycles += 20;

    } else if (2 == x && 6 == z) {
      z80_trace(z80, mem, 4, "RES %d,(IX)", y);
      mem_write(mem, z80->ix + displacement,
        reset_bit(mem_read(mem, z80->ix + displacement), y));
      z80->pc += 4;
      z80->cycles += 23;

    } else if (3 == x && 6 == z) {
      z80_trace(z80, mem, 4, "SET %d,(IX)", y);
      mem_write(mem, z80->ix + displacement,
        set_bit(mem_read(mem, z80->ix + displacement), y));
      z80->pc += 4;
      z80->cycles += 23;

    } else {
      panic("Unhandled DDCB opcode: %02x [x=%d z=%d y=%d]\n", mc[3], x, z, y);
      z80->pc += 4;
    }

  } else if (mc[0] == 0xFD && mc[1] == 0xCB) {
    /* FDCB prefixed */
    int8_t displacement = mc[2];
    uint8_t x =  mc[3] >> 6;
    uint8_t y = (mc[3] >> 3) & 0b111;
    uint8_t z =  mc[3] & 0b111;

    z80->r = (z80->r + 2) & 0x7F;

    if (0 == x && 6 == z) {
      dt_t dt = dt_rot[y];
      z80_trace(z80, mem, 4, "%s (IY)", dt_text(dt));
      switch (dt) {
      case DT_ROT_RLC: 
        mem_write(mem, z80->iy + displacement,
          z80_rlc(z80, mem_read(mem, z80->iy + displacement)));
        break;
      case DT_ROT_RRC: 
        mem_write(mem, z80->iy + displacement,
          z80_rrc(z80, mem_read(mem, z80->iy + displacement)));
        break;
      case DT_ROT_RL:  
        mem_write(mem, z80->iy + displacement,
          z80_rl(z80, mem_read(mem, z80->iy + displacement)));
        break;
      case DT_ROT_RR:  
        mem_write(mem, z80->iy + displacement,
          z80_rr(z80, mem_read(mem, z80->iy + displacement)));
        break;
      case DT_ROT_SLA: 
        mem_write(mem, z80->iy + displacement,
          z80_sla(z80, mem_read(mem, z80->iy + displacement)));
        break;
      case DT_ROT_SRA: 
        mem_write(mem, z80->iy + displacement,
          z80_sra(z80, mem_read(mem, z80->iy + displacement)));
        break;
      case DT_ROT_SLL: 
        mem_write(mem, z80->iy + displacement,
          z80_sll(z80, mem_read(mem, z80->iy + displacement)));
        break;
      case DT_ROT_SRL: 
        mem_write(mem, z80->iy + displacement,
          z80_srl(z80, mem_read(mem, z80->iy + displacement)));
        break;
      default: break;
      }
      z80->pc += 4;
      z80->cycles += 23;

    } else if (1 == x) {
      z80_trace(z80, mem, 4, "BIT %d,(IY)", y);
      z80_bit(z80, mem_read(mem, z80->iy + displacement), y);
      z80->pc += 4;
      z80->cycles += 20;

    } else if (2 == x && 6 == z) {
      z80_trace(z80, mem, 4, "RES %d,(IY)", y);
      mem_write(mem, z80->iy + displacement,
        reset_bit(mem_read(mem, z80->iy + displacement), y));
      z80->pc += 4;
      z80->cycles += 23;

    } else if (3 == x && 6 == z) {
      z80_trace(z80, mem, 4, "SET %d,(IY)", y);
      mem_write(mem, z80->iy + displacement,
        set_bit(mem_read(mem, z80->iy + displacement), y));
      z80->pc += 4;
      z80->cycles += 23;

    } else {
      panic("Unhandled FDCB opcode: %02x [x=%d z=%d y=%d]\n", mc[3], x, z, y);
      z80->pc += 4;
    }

  } else if (mc[0] == 0xCB) {
    /* CB prefixed */
    uint8_t x =  mc[1] >> 6;
    uint8_t y = (mc[1] >> 3) & 0b111;
    uint8_t z =  mc[1] & 0b111;

    z80->r = (z80->r + 2) & 0x7F;

    if (0 == x) {
      dt_t dt_op = dt_rot[y];
      dt_t dt_reg = dt_r[z];
      z80_trace(z80, mem, 2, "%s %s", dt_text(dt_op), dt_text(dt_reg));
      uint8_t value = 0;
      switch (dt_reg) {
      case DT_R_B:   value = z80->b; break;
      case DT_R_C:   value = z80->c; break;
      case DT_R_D:   value = z80->d; break;
      case DT_R_E:   value = z80->e; break;
      case DT_R_H:   value = z80->h; break;
      case DT_R_L:   value = z80->l; break;
      case DT_R_HLI: value = mem_read(mem, z80->hl); break;
      case DT_R_A:   value = z80->a; break;
      default: break;
      }
      switch (dt_op) {
      case DT_ROT_RLC: value = z80_rlc(z80, value); break;
      case DT_ROT_RRC: value = z80_rrc(z80, value); break;
      case DT_ROT_RL:  value = z80_rl(z80, value); break;
      case DT_ROT_RR:  value = z80_rr(z80, value); break;
      case DT_ROT_SLA: value = z80_sla(z80, value); break;
      case DT_ROT_SRA: value = z80_sra(z80, value); break;
      case DT_ROT_SLL: value = z80_sll(z80, value); break;
      case DT_ROT_SRL: value = z80_srl(z80, value); break;
      default: break;
      }
      switch (dt_reg) {
      case DT_R_B:   z80->b = value; break;
      case DT_R_C:   z80->c = value; break;
      case DT_R_D:   z80->d = value; break;
      case DT_R_E:   z80->e = value; break;
      case DT_R_H:   z80->h = value; break;
      case DT_R_L:   z80->l = value; break;
      case DT_R_HLI:
        mem_write(mem, z80->hl, value);
        z80->cycles += 7;
        break;
      case DT_R_A:   z80->a = value; break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 8;

    } else if (1 == x) {
      dt_t dt = dt_r[z];
      z80_trace(z80, mem, 2, "BIT %d,%s", y, dt_text(dt));
      switch (dt) {
      case DT_R_B:   z80_bit(z80, z80->b, y); break;
      case DT_R_C:   z80_bit(z80, z80->c, y); break;
      case DT_R_D:   z80_bit(z80, z80->d, y); break;
      case DT_R_E:   z80_bit(z80, z80->e, y); break;
      case DT_R_H:   z80_bit(z80, z80->h, y); break;
      case DT_R_L:   z80_bit(z80, z80->l, y); break;
      case DT_R_HLI:
        z80_bit(z80, mem_read(mem, z80->hl), y);
        z80->cycles += 4;
        break;
      case DT_R_A:   z80_bit(z80, z80->a, y); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 8;

    } else if (2 == x) {
      dt_t dt = dt_r[z];
      z80_trace(z80, mem, 2, "RES %d,%s", y, dt_text(dt));
      switch (dt) {
      case DT_R_B:   z80->b = reset_bit(z80->b, y); break;
      case DT_R_C:   z80->c = reset_bit(z80->c, y); break;
      case DT_R_D:   z80->d = reset_bit(z80->d, y); break;
      case DT_R_E:   z80->e = reset_bit(z80->e, y); break;
      case DT_R_H:   z80->h = reset_bit(z80->h, y); break;
      case DT_R_L:   z80->l = reset_bit(z80->l, y); break;
      case DT_R_HLI:
        mem_write(mem, z80->hl, reset_bit(mem_read(mem, z80->hl), y));
        z80->cycles += 7;
        break;
      case DT_R_A:   z80->a = reset_bit(z80->a, y); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 8;

    } else if (3 == x) {
      dt_t dt = dt_r[z];
      z80_trace(z80, mem, 2, "SET %d,%s", y, dt_text(dt));
      switch (dt) {
      case DT_R_B:   z80->b = set_bit(z80->b, y); break;
      case DT_R_C:   z80->c = set_bit(z80->c, y); break;
      case DT_R_D:   z80->d = set_bit(z80->d, y); break;
      case DT_R_E:   z80->e = set_bit(z80->e, y); break;
      case DT_R_H:   z80->h = set_bit(z80->h, y); break;
      case DT_R_L:   z80->l = set_bit(z80->l, y); break;
      case DT_R_HLI:
        mem_write(mem, z80->hl, set_bit(mem_read(mem, z80->hl), y));
        z80->cycles += 7;
        break;
      case DT_R_A:   z80->a = set_bit(z80->a, y); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 8;

    } else {
      panic("Unhandled CB opcode: %02x [x=%d z=%d y=%d]\n", mc[1], x, z, y);
      z80->pc += 2;
    }

  } else if (mc[0] == 0xED) {
    /* ED prefixed */
    uint8_t x =  mc[1] >> 6;
    uint8_t y = (mc[1] >> 3) & 0b111;
    uint8_t z =  mc[1] & 0b111;
    uint8_t p = y >> 1;
    uint8_t q = y % 2;

    z80->r = (z80->r + 2) & 0x7F;

    if (1 == x && 0 == z && 6 != y) {
      dt_t dt = dt_r[y];
      z80_trace(z80, mem, 2, "IN %s,(C)", dt_text(dt));
      switch (dt) {
      case DT_R_B: z80->b = z80_io_read_with_flag(z80, z80->c, z80->b); break;
      case DT_R_C: z80->c = z80_io_read_with_flag(z80, z80->c, z80->b); break;
      case DT_R_D: z80->d = z80_io_read_with_flag(z80, z80->c, z80->b); break;
      case DT_R_E: z80->e = z80_io_read_with_flag(z80, z80->c, z80->b); break;
      case DT_R_H: z80->h = z80_io_read_with_flag(z80, z80->c, z80->b); break;
      case DT_R_L: z80->l = z80_io_read_with_flag(z80, z80->c, z80->b); break;
      case DT_R_A: z80->a = z80_io_read_with_flag(z80, z80->c, z80->b); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 12;

    } else if (1 == x && 1 == z && 6 != y) {
      dt_t dt = dt_r[y];
      z80_trace(z80, mem, 2, "OUT (C),%s", dt_text(dt));
      switch (dt) {
      case DT_R_B: z80_io_write(z80, z80->c, z80->b, z80->b); break;
      case DT_R_C: z80_io_write(z80, z80->c, z80->b, z80->c); break;
      case DT_R_D: z80_io_write(z80, z80->c, z80->b, z80->d); break;
      case DT_R_E: z80_io_write(z80, z80->c, z80->b, z80->e); break;
      case DT_R_H: z80_io_write(z80, z80->c, z80->b, z80->h); break;
      case DT_R_L: z80_io_write(z80, z80->c, z80->b, z80->l); break;
      case DT_R_A: z80_io_write(z80, z80->c, z80->b, z80->a); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 12;

    } else if (1 == x && 2 == z && 0 == q) {
      dt_t dt = dt_rp[p];
      z80_trace(z80, mem, 2, "SBC HL,%s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC: z80_sbc_16(z80, &z80->hl, z80->bc); break;
      case DT_RP_DE: z80_sbc_16(z80, &z80->hl, z80->de); break;
      case DT_RP_HL: z80_sbc_16(z80, &z80->hl, z80->hl); break;
      case DT_RP_SP: z80_sbc_16(z80, &z80->hl, z80->sp); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 15;

    } else if (1 == x && 2 == z && 1 == q) {
      dt_t dt = dt_rp[p];
      z80_trace(z80, mem, 2, "ADC HL,%s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC: z80_adc_16(z80, &z80->hl, z80->bc); break;
      case DT_RP_DE: z80_adc_16(z80, &z80->hl, z80->de); break;
      case DT_RP_HL: z80_adc_16(z80, &z80->hl, z80->hl); break;
      case DT_RP_SP: z80_adc_16(z80, &z80->hl, z80->sp); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 15;

    } else if (1 == x && 3 == z && 0 == q) {
      uint16_t address = (mc[3] << 8) + mc[2];
      dt_t dt = dt_rp[p];
      z80_trace(z80, mem, 4, "LD (%04x),%s", address, dt_text(dt));
      switch (dt) {
      case DT_RP_BC: 
        mem_write(mem, address,     z80->c);
        mem_write(mem, address + 1, z80->b);
        break;
      case DT_RP_DE:
        mem_write(mem, address,     z80->e); 
        mem_write(mem, address + 1, z80->d); 
        break;
      case DT_RP_HL:
        mem_write(mem, address,     z80->l);
        mem_write(mem, address + 1, z80->h);
        break;
      case DT_RP_SP:
        mem_write(mem, address,     z80->sp % 256);
        mem_write(mem, address + 1, z80->sp / 256);
        break;
      default:
        break;
      }
      z80->pc += 4;
      z80->cycles += 20;

    } else if (1 == x && 3 == z && 1 == q) {
      uint16_t address = (mc[3] << 8) + mc[2];
      dt_t dt = dt_rp[p];
      z80_trace(z80, mem, 4, "LD %s,(%04x)", dt_text(dt), address);
      switch (dt) {
      case DT_RP_BC:
        z80->c = mem_read(mem, address);
        z80->b = mem_read(mem, address + 1);
        break;
      case DT_RP_DE:
        z80->e = mem_read(mem, address);
        z80->d = mem_read(mem, address + 1);
        break;
      case DT_RP_HL:
        z80->l = mem_read(mem, address);
        z80->h = mem_read(mem, address + 1);
        break;
      case DT_RP_SP:
        z80->sp =  mem_read(mem, address);
        z80->sp += mem_read(mem, address + 1) << 8;
        break;
      default:
        break;
      }
      z80->pc += 4;
      z80->cycles += 20;

    } else if (1 == x && 4 == z) {
      z80_trace(z80, mem, 2, "NEG");
      uint8_t value = z80->a;
      z80->a = 0;
      z80_sub(z80, value);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (1 == x && 5 == z && 0 == y) {
      z80_trace(z80, mem, 2, "RETN");
      z80->pc  = mem_read(mem, z80->sp);
      z80->sp++;
      z80->pc += mem_read(mem, z80->sp) << 8;
      z80->sp++;
      z80->iff1 = z80->iff2;
      z80->cycles += 14;

    } else if (1 == x && 5 == z && 1 == y) {
      z80_trace(z80, mem, 2, "RETI");
      z80->pc  = mem_read(mem, z80->sp);
      z80->sp++;
      z80->pc += mem_read(mem, z80->sp) << 8;
      z80->sp++;
      z80->iff1 = z80->iff2;
      z80->cycles += 14;

    } else if (1 == x && 6 == z) {
      dt_t dt = dt_im[y];
      z80_trace(z80, mem, 2, "IM %s", dt_text(dt));
      switch (dt) {
      case DT_IM_0:
        z80->im = 0;
        break;
      case DT_IM_01:
      case DT_IM_1:
        z80->im = 1;
        break;
      case DT_IM_2:
        z80->im = 2;
        break;
      default:
        panic("Unhandled IM (%02x)\n", dt);
        break;
      }
      z80->pc += 2;
      z80->cycles += 8;

    } else if (1 == x && 7 == z && 0 == y) {
      z80_trace(z80, mem, 2, "LD I,A");
      z80->i = z80->a;
      z80->pc += 2;
      z80->cycles += 9;

    } else if (1 == x && 7 == z && 1 == y) {
      z80_trace(z80, mem, 2, "LD R,A");
      z80->r = z80->a;
      z80->pc += 2;
      z80->cycles += 9;

    } else if (1 == x && 7 == z && 2 == y) {
      z80_trace(z80, mem, 2, "LD A,I");
      z80->a = z80->i;
      z80->flag.pv = z80->iff2;
      z80->flag.s = (z80->a & 0x80) ? 1 : 0;
      z80->flag.z = (z80->a == 0) ? 1 : 0;
      z80->flag.h = 0;
      z80->flag.n = 0;
      z80->pc += 2;
      z80->cycles += 9;

    } else if (1 == x && 7 == z && 3 == y) {
      z80_trace(z80, mem, 2, "LD A,R");
      z80->a = z80->r;
      z80->flag.pv = z80->iff2;
      z80->flag.s = (z80->a & 0x80) ? 1 : 0;
      z80->flag.z = (z80->a == 0) ? 1 : 0;
      z80->flag.h = 0;
      z80->flag.n = 0;
      z80->pc += 2;
      z80->cycles += 9;

    } else if (1 == x && 7 == z && 4 == y) {
      z80_trace(z80, mem, 2, "RRD");
      uint8_t value_a = z80->a;
      uint8_t value_hl = mem_read(mem, z80->hl);
      z80->a = (z80->a & 0xF0) | (value_hl & 0x0F);
      mem_write(mem, z80->hl,
        ((value_a << 4) & 0xF0) | ((value_hl >> 4) & 0x0F));
      z80->flag.s = (z80->a & 0x80) ? 1 : 0;
      z80->flag.z = (z80->a == 0) ? 1 : 0;
      z80->flag.h = 0;
      z80->flag.pv = parity_even(z80->a);
      z80->flag.n = 0;
      z80->pc += 2;
      z80->cycles += 18;

    } else if (1 == x && 7 == z && 5 == y) {
      z80_trace(z80, mem, 2, "RLD");
      uint8_t value_a = z80->a;
      uint8_t value_hl = mem_read(mem, z80->hl);
      z80->a = (z80->a & 0xF0) | ((value_hl >> 4) & 0x0F);
      mem_write(mem, z80->hl,
        ((value_hl << 4) & 0xF0) | (value_a & 0x0F));
      z80->flag.s = (z80->a & 0x80) ? 1 : 0;
      z80->flag.z = (z80->a == 0) ? 1 : 0;
      z80->flag.h = 0;
      z80->flag.pv = parity_even(z80->a);
      z80->flag.n = 0;
      z80->pc += 2;
      z80->cycles += 18;

    } else if (2 == x && 3 >= z && 4 <= y) {
      dt_t dt = dt_bli[y-4][z];
      z80_trace(z80, mem, 2, "%s", dt_text(dt));
      switch (dt) {
      case DT_BLI_LDI: z80_ldi(z80, mem); break;
      case DT_BLI_LDD: z80_ldd(z80, mem); break;
      case DT_BLI_LDIR:
        z80_ldi(z80, mem);
        if (z80->flag.pv) {
          z80->pc -= 2; /* Repeat */
          z80->cycles += 5;
        }
        break;
      case DT_BLI_LDDR:
        z80_ldd(z80, mem);
        if (z80->flag.pv) {
          z80->pc -= 2; /* Repeat */
          z80->cycles += 5;
        }
        break;
      case DT_BLI_CPI: z80_cpi(z80, mem); break;
      case DT_BLI_CPD: z80_cpd(z80, mem); break;
      case DT_BLI_CPIR:
        z80_cpi(z80, mem);
        if (z80->flag.pv && z80->flag.z == 0) {
          z80->pc -= 2; /* Repeat */
          z80->cycles += 5;
        }
        break;
      case DT_BLI_CPDR:
        z80_cpd(z80, mem);
        if (z80->flag.pv && z80->flag.z == 0) {
          z80->pc -= 2; /* Repeat */
          z80->cycles += 5;
        }
        break;
      case DT_BLI_INI:  z80_ini(z80, mem); break;
      case DT_BLI_IND:  z80_ind(z80, mem); break;
      case DT_BLI_INIR:
        z80_ini(z80, mem);
        if (z80->b != 0) {
          z80->pc -= 2; /* Repeat */
          z80->cycles += 5;
        }
        break;
      case DT_BLI_INDR:
        z80_ind(z80, mem);
        if (z80->b != 0) {
          z80->pc -= 2; /* Repeat */
          z80->cycles += 5;
        }
        break;
      case DT_BLI_OUTI: z80_outi(z80, mem); break;
      case DT_BLI_OUTD: z80_outd(z80, mem); break;
      case DT_BLI_OTIR:
        z80_outi(z80, mem);
        if (z80->b != 0) {
          z80->pc -= 2; /* Repeat */
          z80->cycles += 5;
        }
        break;
      case DT_BLI_OTDR:
        z80_outd(z80, mem);
        if (z80->b != 0) {
          z80->pc -= 2; /* Repeat */
          z80->cycles += 5;
        }
        break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 16;

    } else {
      panic("Unhandled ED opcode: %02x [x=%d z=%d (y=%d | q=%d p=%d)]\n",
        mc[1], x, z, y, q, p);
      z80->pc += 2;
    }

  } else if (mc[0] == 0xDD) {
    /* DD prefixed */
    uint8_t x =  mc[1] >> 6;
    uint8_t y = (mc[1] >> 3) & 0b111;
    uint8_t z =  mc[1] & 0b111;
    uint8_t p = y >> 1;
    uint8_t q = y % 2;

    z80->r = (z80->r + 2) & 0x7F;

    if (0 == x && 1 == z && 0 == q && 2 == p) {
      uint16_t value = (mc[3] << 8) + mc[2];
      z80_trace(z80, mem, 4, "LD IX,%04x", value);
      z80->ix = value;
      z80->pc += 4;
      z80->cycles += 14;

    } else if (0 == x && 1 == z && 1 == q) {
      dt_t dt = dt_rpix[p];
      z80_trace(z80, mem, 2, "ADD IX,%s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC: z80_add_16(z80, &z80->ix, z80->bc); break;
      case DT_RP_DE: z80_add_16(z80, &z80->ix, z80->de); break;
      case DT_RP_IX: z80_add_16(z80, &z80->ix, z80->ix); break;
      case DT_RP_SP: z80_add_16(z80, &z80->ix, z80->sp); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 15;

    } else if (0 == x && 2 == z && 4 == y) {
      uint16_t address = (mc[3] << 8) + mc[2];
      z80_trace(z80, mem, 4, "LD (%04x),IX", address);
      mem_write(mem, address,     z80->ixl);
      mem_write(mem, address + 1, z80->ixh);
      z80->pc += 4;
      z80->cycles += 20;

    } else if (0 == x && 2 == z && 5 == y) {
      uint16_t address = (mc[3] << 8) + mc[2];
      z80_trace(z80, mem, 4, "LD IX,(%04x)", address);
      z80->ixl = mem_read(mem, address);
      z80->ixh = mem_read(mem, address + 1);
      z80->pc += 4;
      z80->cycles += 20;

    } else if (0 == x && 3 == z && 4 == y) {
      z80_trace(z80, mem, 2, "INC IX");
      z80->ix++;
      z80->pc += 2;
      z80->cycles += 10;

    } else if (0 == x && 3 == z && 5 == y) {
      z80_trace(z80, mem, 2, "DEC IX");
      z80->ix--;
      z80->pc += 2;
      z80->cycles += 10;

    } else if (0 == x && 4 == z && 4 == y) {
      z80_trace(z80, mem, 2, "INC IXH");
      z80->ixh = z80_inc(z80, z80->ixh);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (0 == x && 4 == z && 5 == y) {
      z80_trace(z80, mem, 2, "INC IXL");
      z80->ixl = z80_inc(z80, z80->ixl);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (0 == x && 4 == z && 6 == y) {
      int8_t displacement = mc[2];
      z80_trace(z80, mem, 3, "INC (IX)");
      mem_write(mem, z80->ix + displacement, 
        z80_inc(z80, mem_read(mem, z80->ix + displacement)));
      z80->pc += 3;
      z80->cycles += 23;

    } else if (0 == x && 5 == z && 4 == y) {
      z80_trace(z80, mem, 2, "DEC IXH");
      z80->ixh = z80_dec(z80, z80->ixh);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (0 == x && 5 == z && 5 == y) {
      z80_trace(z80, mem, 2, "DEC IXL");
      z80->ixl = z80_dec(z80, z80->ixl);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (0 == x && 5 == z && 6 == y) {
      int8_t displacement = mc[2];
      z80_trace(z80, mem, 3, "DEC (IX)");
      mem_write(mem, z80->ix + displacement, 
        z80_dec(z80, mem_read(mem, z80->ix + displacement)));
      z80->pc += 3;
      z80->cycles += 23;

    } else if (0 == x && 6 == z && 4 == y) {
      uint8_t value = mc[2];
      z80_trace(z80, mem, 3, "LD IXH,%02x", value);
      z80->ixh = value;
      z80->pc += 3;
      z80->cycles += 11;

    } else if (0 == x && 6 == z && 5 == y) {
      uint8_t value = mc[2];
      z80_trace(z80, mem, 3, "LD IXL,%02x", value);
      z80->ixl = value;
      z80->pc += 3;
      z80->cycles += 11;

    } else if (0 == x && 6 == z && 6 == y) {
      int8_t displacement = mc[2];
      uint8_t value = mc[3];
      z80_trace(z80, mem, 4, "LD (IX),%02x", value);
      mem_write(mem, z80->ix + displacement, value);
      z80->pc += 4;
      z80->cycles += 19;

    } else if (1 == x && 6 != z) {
      int8_t displacement = mc[2];
      dt_t dt_dst = dt_rix[y];
      dt_t dt_src = dt_rix[z];
      z80_trace(z80, mem, 3, "LD %s,%s", dt_text(dt_dst), dt_text(dt_src));
      uint8_t value = 0;
      switch (dt_src) {
      case DT_R_B:   value = z80->b; break;
      case DT_R_C:   value = z80->c; break;
      case DT_R_D:   value = z80->d; break;
      case DT_R_E:   value = z80->e; break;
      case DT_R_IXH: value = z80->ixh; break;
      case DT_R_IXL: value = z80->ixl; break;
      case DT_R_A:   value = z80->a; break;
      default: break;
      }
      switch (dt_dst) {
      case DT_R_B:   z80->b = value; break;
      case DT_R_C:   z80->c = value; break;
      case DT_R_D:   z80->d = value; break;
      case DT_R_E:   z80->e = value; break;
      case DT_R_IXH: z80->ixh = value; break;
      case DT_R_IXL: z80->ixl = value; break;
      case DT_R_IXI:
        if (dt_src == DT_R_IXH) value = z80->h;
        if (dt_src == DT_R_IXL) value = z80->l;
        mem_write(mem, z80->ix + displacement, value);
        break;
      case DT_R_A:   z80->a = value; break;
      default: break;
      }
      if (dt_dst == DT_R_IXI) {
        z80->pc += 3;
      } else {
        z80->pc += 2;
      }
      z80->cycles += 19;

    } else if (1 == x && 6 == z) {
      int8_t displacement = mc[2];
      dt_t dt = dt_r[y];
      z80_trace(z80, mem, 3, "LD %s,(IX)", dt_text(dt));
      switch (dt) {
      case DT_R_B: z80->b = mem_read(mem, z80->ix + displacement); break;
      case DT_R_C: z80->c = mem_read(mem, z80->ix + displacement); break;
      case DT_R_D: z80->d = mem_read(mem, z80->ix + displacement); break;
      case DT_R_E: z80->e = mem_read(mem, z80->ix + displacement); break;
      case DT_R_H: z80->h = mem_read(mem, z80->ix + displacement); break;
      case DT_R_L: z80->l = mem_read(mem, z80->ix + displacement); break;
      case DT_R_A: z80->a = mem_read(mem, z80->ix + displacement); break;
      default: break;
      }
      z80->pc += 3;
      z80->cycles += 19;

    } else if (2 == x) {
      int8_t displacement = mc[2];
      dt_t dt_op  = dt_alu[y];
      dt_t dt_reg = dt_rix[z];
      z80_trace(z80, mem, 3, "%s%s", dt_text(dt_op), dt_text(dt_reg));
      uint8_t value = 0;
      switch (dt_reg) {
      case DT_R_IXH: value = z80->ixh; break;
      case DT_R_IXL: value = z80->ixl; break;
      case DT_R_IXI: value = mem_read(mem, z80->ix + displacement); break;
      default: break;
      }
      switch (dt_op) {
      case DT_ALU_ADD: z80_add(z80, value); break;
      case DT_ALU_ADC: z80_adc(z80, value); break;
      case DT_ALU_SUB: z80_sub(z80, value); break;
      case DT_ALU_SBC: z80_sbc(z80, value); break;
      case DT_ALU_AND: z80_and(z80, value); break;
      case DT_ALU_XOR: z80_xor(z80, value); break;
      case DT_ALU_OR:  z80_or(z80, value); break;
      case DT_ALU_CP:  z80_compare(z80, value); break;
      default: break;
      }
      if (dt_reg == DT_R_IXI) {
        z80->pc += 3;
        z80->cycles += 19;
      } else {
        z80->pc += 2;
        z80->cycles += 8;
      }

    } else if (3 == x && 1 == z && 4 == y) {
      z80_trace(z80, mem, 2, "POP IX");
      z80->ix  = mem_read(mem, z80->sp);
      z80->sp++;
      z80->ix += mem_read(mem, z80->sp) << 8;
      z80->sp++;
      z80->pc += 2;
      z80->cycles += 14;

    } else if (3 == x && 1 == z && 5 == y) {
      z80_trace(z80, mem, 2, "JP IX");
      z80->pc = z80->ix;
      z80->cycles += 8;

    } else if (3 == x && 1 == z && 7 == y) {
      z80_trace(z80, mem, 2, "LD SP,IX");
      z80->sp = z80->ix;
      z80->pc += 2;
      z80->cycles += 10;

    } else if (3 == x && 3 == z && 4 == y) {
      z80_trace(z80, mem, 2, "EX (SP),IX");
      uint8_t value;
      value = mem_read(mem, z80->sp);
      mem_write(mem, z80->sp, z80->ixl);
      z80->ixl = value;
      value = mem_read(mem, z80->sp + 1);
      mem_write(mem, z80->sp, z80->ixh);
      z80->ixh = value;
      z80->pc += 2;
      z80->cycles += 23;

    } else if (3 == x && 5 == z && 4 == y) {
      z80_trace(z80, mem, 2, "PUSH IX");
      z80->sp--;
      mem_write(mem, z80->sp, z80->ix / 256);
      z80->sp--;
      mem_write(mem, z80->sp, z80->ix % 256);
      z80->pc += 2;
      z80->cycles += 15;

    } else {
      panic("Unhandled DD opcode: %02x [x=%d z=%d (y=%d | q=%d p=%d)]\n",
        mc[1], x, z, y, q, p);
      z80->pc += 2;
    }

  } else if (mc[0] == 0xFD) {
    /* FD prefixed */
    uint8_t x =  mc[1] >> 6;
    uint8_t y = (mc[1] >> 3) & 0b111;
    uint8_t z =  mc[1] & 0b111;
    uint8_t p = y >> 1;
    uint8_t q = y % 2;

    z80->r = (z80->r + 2) & 0x7F;

    if (0 == x && 1 == z && 0 == q && 2 == p) {
      uint16_t value = (mc[3] << 8) + mc[2];
      z80_trace(z80, mem, 4, "LD IY,%04x", value);
      z80->iy = value;
      z80->pc += 4;
      z80->cycles += 14;

    } else if (0 == x && 1 == z && 1 == q) {
      dt_t dt = dt_rpiy[p];
      z80_trace(z80, mem, 2, "ADD IY,%s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC: z80_add_16(z80, &z80->iy, z80->bc); break;
      case DT_RP_DE: z80_add_16(z80, &z80->iy, z80->de); break;
      case DT_RP_IY: z80_add_16(z80, &z80->iy, z80->iy); break;
      case DT_RP_SP: z80_add_16(z80, &z80->iy, z80->sp); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 15;

    } else if (0 == x && 2 == z && 4 == y) {
      uint16_t address = (mc[3] << 8) + mc[2];
      z80_trace(z80, mem, 4, "LD (%04x),IY", address);
      mem_write(mem, address,     z80->iyl);
      mem_write(mem, address + 1, z80->iyh);
      z80->pc += 4;
      z80->cycles += 20;

    } else if (0 == x && 2 == z && 5 == y) {
      uint16_t address = (mc[3] << 8) + mc[2];
      z80_trace(z80, mem, 4, "LD IY,(%04x)", address);
      z80->iyl = mem_read(mem, address);
      z80->iyh = mem_read(mem, address + 1);
      z80->pc += 4;
      z80->cycles += 20;

    } else if (0 == x && 3 == z && 4 == y) {
      z80_trace(z80, mem, 2, "INC IY");
      z80->iy++;
      z80->pc += 2;
      z80->cycles += 10;

    } else if (0 == x && 3 == z && 5 == y) {
      z80_trace(z80, mem, 2, "DEC IY");
      z80->iy--;
      z80->pc += 2;
      z80->cycles += 10;

    } else if (0 == x && 4 == z && 4 == y) {
      z80_trace(z80, mem, 2, "INC IYH");
      z80->iyh = z80_inc(z80, z80->iyh);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (0 == x && 4 == z && 5 == y) {
      z80_trace(z80, mem, 2, "INC IYL");
      z80->iyl = z80_inc(z80, z80->iyl);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (0 == x && 4 == z && 6 == y) {
      int8_t displacement = mc[2];
      z80_trace(z80, mem, 3, "INC (IY)");
      mem_write(mem, z80->iy + displacement, 
        z80_inc(z80, mem_read(mem, z80->iy + displacement)));
      z80->pc += 3;
      z80->cycles += 23;

    } else if (0 == x && 5 == z && 4 == y) {
      z80_trace(z80, mem, 2, "DEC IYH");
      z80->iyh = z80_dec(z80, z80->iyh);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (0 == x && 5 == z && 5 == y) {
      z80_trace(z80, mem, 2, "DEC IYL");
      z80->iyl = z80_dec(z80, z80->iyl);
      z80->pc += 2;
      z80->cycles += 8;

    } else if (0 == x && 5 == z && 6 == y) {
      int8_t displacement = mc[2];
      z80_trace(z80, mem, 3, "DEC (IY)");
      mem_write(mem, z80->iy + displacement, 
        z80_dec(z80, mem_read(mem, z80->iy + displacement)));
      z80->pc += 3;
      z80->cycles += 23;

    } else if (0 == x && 6 == z && 4 == y) {
      uint8_t value = mc[2];
      z80_trace(z80, mem, 3, "LD IYH,%02x", value);
      z80->iyh = value;
      z80->pc += 3;
      z80->cycles += 11;

    } else if (0 == x && 6 == z && 5 == y) {
      uint8_t value = mc[2];
      z80_trace(z80, mem, 3, "LD IYL,%02x", value);
      z80->iyl = value;
      z80->pc += 3;
      z80->cycles += 11;

    } else if (0 == x && 6 == z && 6 == y) {
      int8_t displacement = mc[2];
      uint8_t value = mc[3];
      z80_trace(z80, mem, 4, "LD (IY),%02x", value);
      mem_write(mem, z80->iy + displacement, value);
      z80->pc += 4;
      z80->cycles += 19;

    } else if (1 == x && 6 != z) {
      int8_t displacement = mc[2];
      dt_t dt_dst = dt_riy[y];
      dt_t dt_src = dt_riy[z];
      z80_trace(z80, mem, 3, "LD %s,%s", dt_text(dt_dst), dt_text(dt_src));
      uint8_t value = 0;
      switch (dt_src) {
      case DT_R_B:   value = z80->b; break;
      case DT_R_C:   value = z80->c; break;
      case DT_R_D:   value = z80->d; break;
      case DT_R_E:   value = z80->e; break;
      case DT_R_IYH: value = z80->iyh; break;
      case DT_R_IYL: value = z80->iyl; break;
      case DT_R_A:   value = z80->a; break;
      default: break;
      }
      switch (dt_dst) {
      case DT_R_B:   z80->b = value; break;
      case DT_R_C:   z80->c = value; break;
      case DT_R_D:   z80->d = value; break;
      case DT_R_E:   z80->e = value; break;
      case DT_R_IYH: z80->iyh = value; break;
      case DT_R_IYL: z80->iyl = value; break;
      case DT_R_IYI:
        if (dt_src == DT_R_IYH) value = z80->h;
        if (dt_src == DT_R_IYL) value = z80->l;
        mem_write(mem, z80->iy + displacement, value);
        break;
      case DT_R_A:   z80->a = value; break;
      default: break;
      }
      if (dt_dst == DT_R_IYI) {
        z80->pc += 3;
        z80->cycles += 19;
      } else {
        z80->pc += 2;
        z80->cycles += 8;
      }

    } else if (1 == x && 6 == z) {
      int8_t displacement = mc[2];
      dt_t dt = dt_r[y];
      z80_trace(z80, mem, 3, "LD %s,(IY)", dt_text(dt));
      switch (dt) {
      case DT_R_B: z80->b = mem_read(mem, z80->iy + displacement); break;
      case DT_R_C: z80->c = mem_read(mem, z80->iy + displacement); break;
      case DT_R_D: z80->d = mem_read(mem, z80->iy + displacement); break;
      case DT_R_E: z80->e = mem_read(mem, z80->iy + displacement); break;
      case DT_R_H: z80->h = mem_read(mem, z80->iy + displacement); break;
      case DT_R_L: z80->l = mem_read(mem, z80->iy + displacement); break;
      case DT_R_A: z80->a = mem_read(mem, z80->iy + displacement); break;
      default: break;
      }
      z80->pc += 3;
      z80->cycles += 19;

    } else if (2 == x) {
      int8_t displacement = mc[2];
      dt_t dt_op  = dt_alu[y];
      dt_t dt_reg = dt_riy[z];
      z80_trace(z80, mem, 3, "%s%s", dt_text(dt_op), dt_text(dt_reg));
      uint8_t value = 0;
      switch (dt_reg) {
      case DT_R_IYH: value = z80->iyh; break;
      case DT_R_IYL: value = z80->iyl; break;
      case DT_R_IYI: value = mem_read(mem, z80->iy + displacement); break;
      default: break;
      }
      switch (dt_op) {
      case DT_ALU_ADD: z80_add(z80, value); break;
      case DT_ALU_ADC: z80_adc(z80, value); break;
      case DT_ALU_SUB: z80_sub(z80, value); break;
      case DT_ALU_SBC: z80_sbc(z80, value); break;
      case DT_ALU_AND: z80_and(z80, value); break;
      case DT_ALU_XOR: z80_xor(z80, value); break;
      case DT_ALU_OR:  z80_or(z80, value); break;
      case DT_ALU_CP:  z80_compare(z80, value); break;
      default: break;
      }
      if (dt_reg == DT_R_IYI) {
        z80->pc += 3;
        z80->cycles += 19;
      } else {
        z80->pc += 2;
        z80->cycles += 8;
      }

    } else if (3 == x && 1 == z && 4 == y) {
      z80_trace(z80, mem, 2, "POP IY");
      z80->iy  = mem_read(mem, z80->sp);
      z80->sp++;
      z80->iy += mem_read(mem, z80->sp) << 8;
      z80->sp++;
      z80->pc += 2;
      z80->cycles += 14;

    } else if (3 == x && 1 == z && 5 == y) {
      z80_trace(z80, mem, 2, "JP IY");
      z80->pc = z80->iy;
      z80->cycles += 8;

    } else if (3 == x && 1 == z && 7 == y) {
      z80_trace(z80, mem, 2, "LD SP,IY");
      z80->sp = z80->iy;
      z80->pc += 2;
      z80->cycles += 10;

    } else if (3 == x && 3 == z && 4 == y) {
      z80_trace(z80, mem, 2, "EX (SP),IY");
      uint8_t value;
      value = mem_read(mem, z80->sp);
      mem_write(mem, z80->sp, z80->iyl);
      z80->iyl = value;
      value = mem_read(mem, z80->sp + 1);
      mem_write(mem, z80->sp, z80->iyh);
      z80->iyh = value;
      z80->pc += 2;
      z80->cycles += 23;

    } else if (3 == x && 5 == z && 4 == y) {
      z80_trace(z80, mem, 2, "PUSH IY");
      z80->sp--;
      mem_write(mem, z80->sp, z80->iy / 256);
      z80->sp--;
      mem_write(mem, z80->sp, z80->iy % 256);
      z80->pc += 2;
      z80->cycles += 15;

    } else {
      panic("Unhandled FD opcode: %02x [x=%d z=%d (y=%d | q=%d p=%d)]\n",
        mc[1], x, z, y, q, p);
      z80->pc += 2;
    }

  } else {
    /* Unprefixed */
    uint8_t x =  mc[0] >> 6;
    uint8_t y = (mc[0] >> 3) & 0b111;
    uint8_t z =  mc[0] & 0b111;
    uint8_t p = y >> 1;
    uint8_t q = y % 2;

    z80->r = (z80->r + 1) & 0x7F;

    if (0 == x && 0 == z && 0 == y) {
      z80_trace(z80, mem, 1, "NOP");
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 0 == z && 1 == y) {
      z80_trace(z80, mem, 1, "EX AF,AF'");
      uint16_t value = z80->af;
      z80->af  = z80->af_;
      z80->af_ = value;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 0 == z && 2 == y) {
      int8_t displacement = mc[1];
      z80_trace(z80, mem, 2, "DJNZ %04x", z80->pc + displacement + 2);
      z80->b--;
      if (z80->b == 0) {
        z80->pc += 2;
        z80->cycles += 8;
      } else {
        z80->pc += displacement + 2;
        z80->cycles += 13;
      }

    } else if (0 == x && 0 == z && 3 == y) {
      int8_t displacement = mc[1];
      z80_trace(z80, mem, 2, "JR %04x", z80->pc + displacement + 2);
      z80->pc += displacement + 2;
      z80->cycles += 12;

    } else if (0 == x && 0 == z && 4 <= y) {
      int8_t displacement = mc[1];
      dt_t dt = dt_cc[y-4];
      z80_trace(z80, mem, 2, "JR %s,%04x", dt_text(dt),
        z80->pc + displacement + 2);
      bool go = false;
      switch (dt) {
      case DT_CC_NZ: go = z80->flag.z  == 0; break;
      case DT_CC_Z:  go = z80->flag.z  == 1; break;
      case DT_CC_NC: go = z80->flag.c  == 0; break;
      case DT_CC_C:  go = z80->flag.c  == 1; break;
      case DT_CC_PO: go = z80->flag.pv == 0; break;
      case DT_CC_PE: go = z80->flag.pv == 1; break;
      case DT_CC_P:  go = z80->flag.s  == 0; break;
      case DT_CC_M:  go = z80->flag.s  == 1; break;
      default: break;
      }
      if (go) {
        z80->pc += displacement + 2;
        z80->cycles += 12;
      } else {
        z80->pc += 2;
        z80->cycles += 7;
      }

    } else if (0 == x && 1 == z && 0 == q) {
      uint16_t value = (mc[2] << 8) + mc[1];
      dt_t dt = dt_rp[p];
      z80_trace(z80, mem, 3, "LD %s,%04x", dt_text(dt), value);
      switch (dt) {
      case DT_RP_BC: z80->bc = value; break;
      case DT_RP_DE: z80->de = value; break;
      case DT_RP_HL: z80->hl = value; break;
      case DT_RP_SP: z80->sp = value; break;
      default: break;
      }
      z80->pc += 3;
      z80->cycles += 10;

    } else if (0 == x && 1 == z && 1 == q) {
      dt_t dt = dt_rp[p];
      z80_trace(z80, mem, 1, "ADD HL,%s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC: z80_add_16(z80, &z80->hl, z80->bc); break;
      case DT_RP_DE: z80_add_16(z80, &z80->hl, z80->de); break;
      case DT_RP_HL: z80_add_16(z80, &z80->hl, z80->hl); break;
      case DT_RP_SP: z80_add_16(z80, &z80->hl, z80->sp); break;
      default: break;
      }
      z80->pc += 1;
      z80->cycles += 11;

    } else if (0 == x && 2 == z && 0 == q && 0 == p) {
      z80_trace(z80, mem, 1, "LD (BC),A");
      mem_write(mem, z80->bc, z80->a);
      z80->pc += 1;
      z80->cycles += 7;

    } else if (0 == x && 2 == z && 0 == q && 1 == p) {
      z80_trace(z80, mem, 1, "LD (DE),A");
      mem_write(mem, z80->de, z80->a);
      z80->pc += 1;
      z80->cycles += 7;

    } else if (0 == x && 2 == z && 0 == q && 2 == p) {
      uint16_t address = (mc[2] << 8) + mc[1];
      z80_trace(z80, mem, 3, "LD (%04x),HL", address);
      mem_write(mem, address, z80->l);
      mem_write(mem, address + 1, z80->h);
      z80->pc += 3;
      z80->cycles += 16;

    } else if (0 == x && 2 == z && 0 == q && 3 == p) {
      uint16_t address = (mc[2] << 8) + mc[1];
      z80_trace(z80, mem, 3, "LD (%04x),A", address);
      mem_write(mem, address, z80->a);
      z80->pc += 3;
      z80->cycles += 13;

    } else if (0 == x && 2 == z && 1 == q && 0 == p) {
      z80_trace(z80, mem, 1, "LD A,(BC)");
      z80->a = mem_read(mem, z80->bc);
      z80->pc += 1;
      z80->cycles += 7;

    } else if (0 == x && 2 == z && 1 == q && 1 == p) {
      z80_trace(z80, mem, 1, "LD A,(DE)");
      z80->a = mem_read(mem, z80->de);
      z80->pc += 1;
      z80->cycles += 7;

    } else if (0 == x && 2 == z && 1 == q && 2 == p) {
      uint16_t address = (mc[2] << 8) + mc[1];
      z80_trace(z80, mem, 3, "LD HL,(%04x)", address);
      z80->l = mem_read(mem, address);
      z80->h = mem_read(mem, address + 1);
      z80->pc += 3;
      z80->cycles += 16;

    } else if (0 == x && 2 == z && 1 == q && 3 == p) {
      uint16_t address = (mc[2] << 8) + mc[1];
      z80_trace(z80, mem, 3, "LD A,(%04x)", address);
      z80->a = mem_read(mem, address);
      z80->pc += 3;
      z80->cycles += 13;

    } else if (0 == x && 3 == z && 0 == q) {
      dt_t dt = dt_rp[p];
      z80_trace(z80, mem, 1, "INC %s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC: z80->bc++; break;
      case DT_RP_DE: z80->de++; break;
      case DT_RP_HL: z80->hl++; break;
      case DT_RP_SP: z80->sp++; break;
      default: break;
      }
      z80->pc += 1;
      z80->cycles += 6;

    } else if (0 == x && 3 == z && 1 == q) {
      dt_t dt = dt_rp[p];
      z80_trace(z80, mem, 1, "DEC %s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC: z80->bc--; break;
      case DT_RP_DE: z80->de--; break;
      case DT_RP_HL: z80->hl--; break;
      case DT_RP_SP: z80->sp--; break;
      default: break;
      }
      z80->pc += 1;
      z80->cycles += 6;

    } else if (0 == x && 4 == z) {
      dt_t dt = dt_r[y];
      z80_trace(z80, mem, 1, "INC %s", dt_text(dt));
      switch (dt) {
      case DT_R_B: z80->b = z80_inc(z80, z80->b); break;
      case DT_R_C: z80->c = z80_inc(z80, z80->c); break;
      case DT_R_D: z80->d = z80_inc(z80, z80->d); break;
      case DT_R_E: z80->e = z80_inc(z80, z80->e); break;
      case DT_R_H: z80->h = z80_inc(z80, z80->h); break;
      case DT_R_L: z80->l = z80_inc(z80, z80->l); break;
      case DT_R_HLI:
        mem_write(mem, z80->hl, z80_inc(z80, mem_read(mem, z80->hl)));
        z80->cycles += 7;
        break;
      case DT_R_A: z80->a = z80_inc(z80, z80->a); break;
      default: break;
      }
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 5 == z) {
      dt_t dt = dt_r[y];
      z80_trace(z80, mem, 1, "DEC %s", dt_text(dt));
      switch (dt) {
      case DT_R_B: z80->b = z80_dec(z80, z80->b); break;
      case DT_R_C: z80->c = z80_dec(z80, z80->c); break;
      case DT_R_D: z80->d = z80_dec(z80, z80->d); break;
      case DT_R_E: z80->e = z80_dec(z80, z80->e); break;
      case DT_R_H: z80->h = z80_dec(z80, z80->h); break;
      case DT_R_L: z80->l = z80_dec(z80, z80->l); break;
      case DT_R_HLI:
        mem_write(mem, z80->hl, z80_dec(z80, mem_read(mem, z80->hl)));
        z80->cycles += 7;
        break;
      case DT_R_A: z80->a = z80_dec(z80, z80->a); break;
      default: break;
      }
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 6 == z) {
      uint8_t value = mc[1];
      dt_t dt = dt_r[y];
      z80_trace(z80, mem, 2, "LD %s,%02x", dt_text(dt), value);
      switch (dt) {
      case DT_R_B:   z80->b = value; break;
      case DT_R_C:   z80->c = value; break;
      case DT_R_D:   z80->d = value; break;
      case DT_R_E:   z80->e = value; break;
      case DT_R_H:   z80->h = value; break;
      case DT_R_L:   z80->l = value; break;
      case DT_R_HLI:
        mem_write(mem, z80->hl, value);
        z80->cycles += 3;
        break;
      case DT_R_A:   z80->a = value; break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 7;

    } else if (0 == x && 7 == z && 0 == y) {
      z80_trace(z80, mem, 1, "RLCA");
      bool bit = z80->a & 0x80;
      z80->a <<= 1;
      if (bit) {
        z80->a |= 0x01;
      }
      z80->flag.c = bit;
      z80->flag.h = 0;
      z80->flag.n = 0;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 7 == z && 1 == y) {
      z80_trace(z80, mem, 1, "RRCA");
      bool bit = z80->a & 0x01;
      z80->a >>= 1;
      if (bit) {
        z80->a |= 0x80;
      }
      z80->flag.c = bit;
      z80->flag.h = 0;
      z80->flag.n = 0;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 7 == z && 2 == y) {
      z80_trace(z80, mem, 1, "RLA");
      bool bit = z80->a & 0x80;
      z80->a <<= 1;
      if (z80->flag.c) {
        z80->a |= 0x01;
      }
      z80->flag.c = bit;
      z80->flag.h = 0;
      z80->flag.n = 0;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 7 == z && 3 == y) {
      z80_trace(z80, mem, 1, "RRA");
      bool bit = z80->a & 0x01;
      z80->a >>= 1;
      if (z80->flag.c) {
        z80->a |= 0x80;
      }
      z80->flag.c = bit;
      z80->flag.h = 0;
      z80->flag.n = 0;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 7 == z && 4 == y) {
      z80_trace(z80, mem, 1, "DAA");
      uint8_t diff;
      bool out_c;
      bool out_h;
      if (z80->flag.c) {
        if ((z80->a & 0x0F) < 0x0A) {
          diff = z80->flag.h ? 0x66 : 0x60;
        } else {
          diff = 0x66;
        }
        out_c = 1;
      } else {
        if ((z80->a & 0x0F) < 0x0A) {
          if (((z80->a >> 4) & 0x0F) < 0xA) {
            diff = z80->flag.h ? 0x06 : 0x00;
          } else {
            diff = z80->flag.h ? 0x66 : 0x60;
          }
        } else {
          diff = (((z80->a >> 4) & 0x0F) < 0x9) ? 0x06 : 0x66;
        }
        if ((z80->a & 0x0F) < 0xA) {
          out_c = (((z80->a >> 4) & 0x0F) < 0x0A) ? 0 : 1;
        } else {
          out_c = (((z80->a >> 4) & 0x0F) < 0x09) ? 0 : 1;
        }
      }
      if (z80->flag.n) {
        if (z80->flag.h) {
          out_h = ((z80->a & 0x0F) < 0x06) ? 1 : 0;
        } else {
          out_h = 0;
        }
      } else {
        out_h = ((z80->a & 0x0F) < 0x0A) ? 0 : 1;
      }
      z80->flag.c = 0;
      (z80->flag.n) ? z80_sub(z80, diff) : z80_add(z80, diff);
      z80->flag.pv = parity_even(z80->a);
      z80->flag.c = out_c;
      z80->flag.h = out_h;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 7 == z && 5 == y) {
      z80_trace(z80, mem, 1, "CPL");
      z80->a = ~z80->a;
      z80->flag.h = 1;
      z80->flag.n = 1;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 7 == z && 6 == y) {
      z80_trace(z80, mem, 1, "SCF");
      z80->flag.h = 0;
      z80->flag.n = 0;
      z80->flag.c = 1;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (0 == x && 7 == z && 7 == y) {
      z80_trace(z80, mem, 1, "CCF");
      z80->flag.h = z80->flag.c;
      z80->flag.n = 0;
      z80->flag.c = ~z80->flag.c;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (1 == x && !(6 == z && 6 == y)) {
      dt_t dt_dst = dt_r[y];
      dt_t dt_src = dt_r[z];
      z80_trace(z80, mem, 1, "LD %s,%s", dt_text(dt_dst), dt_text(dt_src));
      uint8_t value = 0;
      switch (dt_src) {
      case DT_R_B:   value = z80->b; break;
      case DT_R_C:   value = z80->c; break;
      case DT_R_D:   value = z80->d; break;
      case DT_R_E:   value = z80->e; break;
      case DT_R_H:   value = z80->h; break;
      case DT_R_L:   value = z80->l; break;
      case DT_R_HLI:
        value = mem_read(mem, z80->hl);
        z80->cycles += 3;
        break;
      case DT_R_A:   value = z80->a; break;
      default: break;
      }
      switch (dt_dst) {
      case DT_R_B:   z80->b = value; break;
      case DT_R_C:   z80->c = value; break;
      case DT_R_D:   z80->d = value; break;
      case DT_R_E:   z80->e = value; break;
      case DT_R_H:   z80->h = value; break;
      case DT_R_L:   z80->l = value; break;
      case DT_R_HLI:
        mem_write(mem, z80->hl, value);
        z80->cycles += 3;
        break;
      case DT_R_A:   z80->a = value; break;
      default: break;
      }
      z80->pc += 1;
      z80->cycles += 4;

    } else if (1 == x && (6 == z && 6 == y)) {
      z80_trace(z80, mem, 1, "HALT");
      z80->halted = true;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (2 == x) {
      dt_t dt_op  = dt_alu[y];
      dt_t dt_reg = dt_r[z];
      z80_trace(z80, mem, 1, "%s%s", dt_text(dt_op), dt_text(dt_reg));
      uint8_t value = 0;
      switch (dt_reg) {
      case DT_R_B:   value = z80->b; break;
      case DT_R_C:   value = z80->c; break;
      case DT_R_D:   value = z80->d; break;
      case DT_R_E:   value = z80->e; break;
      case DT_R_H:   value = z80->h; break;
      case DT_R_L:   value = z80->l; break;
      case DT_R_HLI:
        value = mem_read(mem, z80->hl);
        z80->cycles += 3;
        break;
      case DT_R_A:   value = z80->a; break;
      default: break;
      }
      switch (dt_op) {
      case DT_ALU_ADD: z80_add(z80, value); break;
      case DT_ALU_ADC: z80_adc(z80, value); break;
      case DT_ALU_SUB: z80_sub(z80, value); break;
      case DT_ALU_SBC: z80_sbc(z80, value); break;
      case DT_ALU_AND: z80_and(z80, value); break;
      case DT_ALU_XOR: z80_xor(z80, value); break;
      case DT_ALU_OR:  z80_or(z80, value); break;
      case DT_ALU_CP:  z80_compare(z80, value); break;
      default: break;
      }
      z80->pc += 1;
      z80->cycles += 4;

    } else if (3 == x && 1 == z && 0 == q) {
      dt_t dt = dt_rpaf[p];
      z80_trace(z80, mem, 1, "POP %s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC:
        z80->c = mem_read(mem, z80->sp);
        z80->sp++;
        z80->b = mem_read(mem, z80->sp);
        z80->sp++;
        break;
      case DT_RP_DE:
        z80->e = mem_read(mem, z80->sp);
        z80->sp++;
        z80->d = mem_read(mem, z80->sp);
        z80->sp++;
        break;
      case DT_RP_HL:
        z80->l = mem_read(mem, z80->sp);
        z80->sp++;
        z80->h = mem_read(mem, z80->sp);
        z80->sp++;
        break;
      case DT_RP_AF:
        z80->f = mem_read(mem, z80->sp);
        z80->sp++;
        z80->a = mem_read(mem, z80->sp);
        z80->sp++;
        break;
      default:
        break;
      }
      z80->pc += 1;
      z80->cycles += 10;

    } else if (3 == x && 0 == z) {
      dt_t dt = dt_cc[y];
      z80_trace(z80, mem, 1, "RET %s", dt_text(dt));
      bool go = false;
      switch (dt) {
      case DT_CC_NZ: go = z80->flag.z  == 0; break;
      case DT_CC_Z:  go = z80->flag.z  == 1; break;
      case DT_CC_NC: go = z80->flag.c  == 0; break;
      case DT_CC_C:  go = z80->flag.c  == 1; break;
      case DT_CC_PO: go = z80->flag.pv == 0; break;
      case DT_CC_PE: go = z80->flag.pv == 1; break;
      case DT_CC_P:  go = z80->flag.s  == 0; break;
      case DT_CC_M:  go = z80->flag.s  == 1; break;
      default: break;
      }
      if (go) {
        z80->pc  = mem_read(mem, z80->sp);
        z80->sp++;
        z80->pc += mem_read(mem, z80->sp) << 8;
        z80->cycles += 11;
        z80->sp++;
      } else {
        z80->pc += 1;
        z80->cycles += 5;
      }

    } else if (3 == x && 1 == z && 1 == q && p == 0) {
      z80_trace(z80, mem, 1, "RET");
      z80->pc  = mem_read(mem, z80->sp);
      z80->sp++;
      z80->pc += mem_read(mem, z80->sp) << 8;
      z80->sp++;
      z80->cycles += 10;

    } else if (3 == x && 1 == z && 1 == q && 1 == p) {
      z80_trace(z80, mem, 1, "EXX");
      uint16_t value;
      value    = z80->bc;
      z80->bc  = z80->bc_;
      z80->bc_ = value;
      value    = z80->de;
      z80->de  = z80->de_;
      z80->de_ = value;
      value    = z80->hl;
      z80->hl  = z80->hl_;
      z80->hl_ = value;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (3 == x && 1 == z && 1 == q && 2 == p) {
      z80_trace(z80, mem, 1, "JP HL");
      z80->pc  = z80->l;
      z80->pc += z80->h << 8;
      z80->cycles += 4;

    } else if (3 == x && 1 == z && 1 == q && 3 == p) {
      z80_trace(z80, mem, 1, "LD SP,HL");
      z80->sp = z80->hl;
      z80->pc += 1;
      z80->cycles += 6;

    } else if (3 == x && 2 == z) {
      uint16_t address = (mc[2] << 8) + mc[1];
      dt_t dt = dt_cc[y];
      z80_trace(z80, mem, 3, "JP %s,%04x", dt_text(dt), address);
      bool go = false;
      switch (dt) {
      case DT_CC_NZ: go = z80->flag.z  == 0; break;
      case DT_CC_Z:  go = z80->flag.z  == 1; break;
      case DT_CC_NC: go = z80->flag.c  == 0; break;
      case DT_CC_C:  go = z80->flag.c  == 1; break;
      case DT_CC_PO: go = z80->flag.pv == 0; break;
      case DT_CC_PE: go = z80->flag.pv == 1; break;
      case DT_CC_P:  go = z80->flag.s  == 0; break;
      case DT_CC_M:  go = z80->flag.s  == 1; break;
      default: break;
      }
      if (go) {
        z80->pc = address;
      } else {
        z80->pc += 3;
      }
      z80->cycles += 10;

    } else if (3 == x && 3 == z && 0 == y) {
      uint16_t address = (mc[2] << 8) + mc[1];
      z80_trace(z80, mem, 3, "JP %04x", address);
      z80->pc = address;
      z80->cycles += 10;

    } else if (3 == x && 3 == z && 2 == y) {
      uint8_t value = mc[1];
      z80_trace(z80, mem, 2, "OUT (%02x),A", value);
      z80_io_write(z80, value, z80->a, z80->a);
      z80->pc += 2;
      z80->cycles += 11;

    } else if (3 == x && 3 == z && 3 == y) {
      uint8_t value = mc[1];
      z80_trace(z80, mem, 2, "IN A,(%02x)", value);
      z80->a = z80_io_read(z80, value, z80->a);
      z80->pc += 2;
      z80->cycles += 11;

    } else if (3 == x && 3 == z && 4 == y) {
      z80_trace(z80, mem, 1, "EX (SP),HL");
      uint16_t value;
      value  = mem_read(mem, z80->sp);
      value += mem_read(mem, z80->sp + 1) << 8;
      mem_write(mem, z80->sp, z80->l);
      mem_write(mem, z80->sp + 1, z80->h);
      z80->hl = value;
      z80->pc += 1;
      z80->cycles += 19;

    } else if (3 == x && 3 == z && 5 == y) {
      z80_trace(z80, mem, 1, "EX DE,HL");
      uint16_t value = z80->hl;
      z80->hl = z80->de;
      z80->de = value;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (3 == x && 3 == z && 6 == y) {
      z80_trace(z80, mem, 1, "DI");
      z80->iff1 = 0;
      z80->iff2 = 0;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (3 == x && 3 == z && 7 == y) {
      z80_trace(z80, mem, 1, "EI");
      z80->iff1 = 1;
      z80->iff2 = 1;
      z80->ei_executed = true;
      z80->pc += 1;
      z80->cycles += 4;

    } else if (3 == x && 4 == z) {
      uint16_t address = (mc[2] << 8) + mc[1];
      dt_t dt = dt_cc[y];
      z80_trace(z80, mem, 3, "CALL %s,%04x", dt_text(dt), address);
      bool go = false;
      switch (dt) {
      case DT_CC_NZ: go = z80->flag.z  == 0; break;
      case DT_CC_Z:  go = z80->flag.z  == 1; break;
      case DT_CC_NC: go = z80->flag.c  == 0; break;
      case DT_CC_C:  go = z80->flag.c  == 1; break;
      case DT_CC_PO: go = z80->flag.pv == 0; break;
      case DT_CC_PE: go = z80->flag.pv == 1; break;
      case DT_CC_P:  go = z80->flag.s  == 0; break;
      case DT_CC_M:  go = z80->flag.s  == 1; break;
      default: break;
      }
      if (go) {
        z80->sp--;
        mem_write(mem, z80->sp, (z80->pc + 3) / 256);
        z80->sp--;
        mem_write(mem, z80->sp, (z80->pc + 3) % 256);
        z80->pc = address;
        z80->cycles += 17;
      } else {
        z80->pc += 3;
        z80->cycles += 10;
      }

    } else if (3 == x && 5 == z && 1 == q && 0 == p) {
      uint16_t address = (mc[2] << 8) + mc[1];
      z80_trace(z80, mem, 3, "CALL %04x", address);
      z80->sp--;
      mem_write(mem, z80->sp, (z80->pc + 3) / 256);
      z80->sp--;
      mem_write(mem, z80->sp, (z80->pc + 3) % 256);
      z80->pc = address;
      z80->cycles = 17;

    } else if (3 == x && 5 == z && 0 == q) {
      dt_t dt = dt_rpaf[p];
      z80_trace(z80, mem, 1, "PUSH %s", dt_text(dt));
      switch (dt) {
      case DT_RP_BC:
        z80->sp--;
        mem_write(mem, z80->sp, z80->b);
        z80->sp--;
        mem_write(mem, z80->sp, z80->c);
        break;
      case DT_RP_DE:
        z80->sp--;
        mem_write(mem, z80->sp, z80->d);
        z80->sp--;
        mem_write(mem, z80->sp, z80->e);
        break;
      case DT_RP_HL:
        z80->sp--;
        mem_write(mem, z80->sp, z80->h);
        z80->sp--;
        mem_write(mem, z80->sp, z80->l);
        break;
      case DT_RP_AF:
        z80->sp--;
        mem_write(mem, z80->sp, z80->a);
        z80->sp--;
        mem_write(mem, z80->sp, z80->f);
        break;
      default:
        break;
      }
      z80->pc += 1;
      z80->cycles += 11;

    } else if (3 == x && 6 == z) {
      dt_t dt = dt_alu[y];
      uint8_t value = mc[1];
      z80_trace(z80, mem, 2, "%s %02x", dt_text(dt), value);
      switch (dt) {
      case DT_ALU_ADD: z80_add(z80, value); break;
      case DT_ALU_ADC: z80_adc(z80, value); break;
      case DT_ALU_SUB: z80_sub(z80, value); break;
      case DT_ALU_SBC: z80_sbc(z80, value); break;
      case DT_ALU_AND: z80_and(z80, value); break;
      case DT_ALU_XOR: z80_xor(z80, value); break;
      case DT_ALU_OR:  z80_or(z80, value); break;
      case DT_ALU_CP:  z80_compare(z80, value); break;
      default: break;
      }
      z80->pc += 2;
      z80->cycles += 7;

    } else if (3 == x && 7 == z) {
      uint8_t zero_address = y * 8;
      z80_trace(z80, mem, 1, "RST %02x", zero_address);
      z80->sp--;
      mem_write(mem, z80->sp, (z80->pc + 1) / 256);
      z80->sp--;
      mem_write(mem, z80->sp, (z80->pc + 1) % 256);
      z80->pc = zero_address;
      z80->cycles += 11;

    } else {
      panic("Unhandled opcode: %02x [x=%d z=%d (y=%d | q=%d p=%d)]\n",
        mc[0], x, z, y, q, p);
      z80->pc += 1;
    }
  }
}



void z80_irq(z80_t *z80, mem_t *mem)
{
  if (z80->ei_executed) {
    /* Delay IRQ and execute one more instruction if EI was previous. */
    z80->ei_executed = false;
    return;
  }

  if (z80->iff1 == 0) {
    return;
  }

  z80->iff1 = 0;
  z80->iff2 = 0;
  z80->halted = false;

  if (z80->im == 1) {
    z80_trace(z80, mem, 0, "IRQ IM1");
    z80->sp--;
    mem_write(mem, z80->sp, z80->pc / 256);
    z80->sp--;
    mem_write(mem, z80->sp, z80->pc % 256);
    z80->pc = 0x38;
  } else {
    panic("Unhandled IM on IRQ (%d)\n", z80->im);
  }
}



