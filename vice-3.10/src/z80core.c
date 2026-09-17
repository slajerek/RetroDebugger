/*
 * z80core.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifdef Z80_4MHZ
#define CLK_ADD(clock, amount) clock = z80cpu_clock_add(clock, amount)
#else
#define CLK_ADD(clock, amount) clock += amount
#endif

static uint8_t reg_a = 0;
static uint8_t reg_b = 0;
static uint8_t reg_c = 0;
static uint8_t reg_d = 0;
static uint8_t reg_e = 0;
static uint8_t reg_f = 0;
static uint8_t reg_h = 0;
static uint8_t reg_l = 0;
static uint8_t reg_ixh = 0;
static uint8_t reg_ixl = 0;
static uint8_t reg_iyh = 0;
static uint8_t reg_iyl = 0;
static uint16_t reg_sp = 0;
static uint32_t z80_reg_pc = 0;
static uint8_t reg_i = 0;
static uint8_t reg_r = 0;

static uint8_t iff1 = 0;
static uint8_t iff2 = 0;
static uint8_t im_mode = 0;

static uint8_t reg_a2 = 0;
static uint8_t reg_b2 = 0;
static uint8_t reg_c2 = 0;
static uint8_t reg_d2 = 0;
static uint8_t reg_e2 = 0;
static uint8_t reg_f2 = 0;
static uint8_t reg_h2 = 0;
static uint8_t reg_l2 = 0;

static uint8_t iff1_1 = 0;
static uint8_t iff2_1 = 0;
static uint8_t iff1_2 = 0;
static uint8_t iff2_2 = 0;

static uint8_t halt = 0;

/* See: https://gist.github.com/drhelius/8497817
   for proper implementation of the BIT flags.
   Called either memptr or WZ (reg_wz) */
static uint16_t reg_wz = 0;

enum INST_MODE_s {
    INST_NONE,
    INST_IX,
    INST_IY
};
typedef enum INST_MODE_s INST_MODE_t;
static INST_MODE_t inst_mode = INST_NONE;

static void z80core_reset(void)
{
    z80_reg_pc = 0;
    z80_regs.reg_pc = 0;
    iff1 = 0;
    iff2 = 0;
    im_mode = 0;
    iff1_1 = 0;
    iff2_1 = 0;
    iff1_2 = 0;
    iff2_2 = 0;
    halt = 0;
}

#define opcode_t uint32_t

#define FETCH_OPCODE(o) ((o) = (LOAD(z80_reg_pc)               \
                                | (LOAD(z80_reg_pc + 1) << 8)  \
                                | (LOAD(z80_reg_pc + 2) << 16) \
                                | (LOAD(z80_reg_pc + 3) << 24)))

#define p0 (opcode & 0xff)
#define p1 ((opcode >> 8) & 0xff)
#define p2 ((opcode >> 16) & 0xff)
#define p3 (opcode >> 24)

#define p12 ((opcode >> 8) & 0xffff)
#define p23 ((opcode >> 16) & 0xffff)

#define INC_PC(value) (z80_reg_pc += (value))

/* ------------------------------------------------------------------------- */

static unsigned int z80_last_opcode_info;
static unsigned int z80_last_opcode_addr;

#define LAST_OPCODE_INFO z80_last_opcode_info
#define LAST_OPCODE_ADDR z80_last_opcode_addr

/* Remember the number of the last opcode.  By default, the opcode does not
   delay interrupt and does not change the I flag.  */
#define SET_LAST_OPCODE(x) OPINFO_SET(LAST_OPCODE_INFO, (x), 0, 0, 0)

/* Remember that the last opcode delayed a pending IRQ or NMI by one cycle.  */
#define OPCODE_DELAYS_INTERRUPT() OPINFO_SET_DELAYS_INTERRUPT(LAST_OPCODE_INFO, 1)

/* Remember that the last opcode changed the I flag from 0 to 1, so we have
   to dispatch an IRQ even if the I flag is 0 when we check it.  */
#define OPCODE_DISABLES_IRQ() OPINFO_SET_DISABLES_IRQ(LAST_OPCODE_INFO, 1)

/* Remember that the last opcode changed the I flag from 1 to 0, so we must
   not dispatch an IRQ even if the I flag is 1 when we check it.  */
#define OPCODE_ENABLES_IRQ() OPINFO_SET_ENABLES_IRQ(LAST_OPCODE_INFO, 1)

#ifdef LAST_OPCODE_ADDR
#define SET_LAST_ADDR(x) LAST_OPCODE_ADDR = (x)
#else
#error "please define LAST_OPCODE_ADDR"
#endif

/* ------------------------------------------------------------------------- */

/* Word register manipulation.  */

#define BC_WORD() ((reg_b << 8) | reg_c)
#define DE_WORD() ((reg_d << 8) | reg_e)
#define HL_WORD() ((reg_h << 8) | reg_l)
#define IX_WORD() ((reg_ixh << 8) | reg_ixl)
#define IY_WORD() ((reg_iyh << 8) | reg_iyl)

/* reg_wz needs to be set whenever IX+d or XY+d is used; can't use a macro */
static inline uint16_t IX_WORD_OFF(int8_t offset)
{
    reg_wz = IX_WORD() + (signed char)(offset);
    return reg_wz;
}

static inline uint16_t IY_WORD_OFF(int8_t offset)
{
    reg_wz = IY_WORD() + (signed char)(offset);
    return reg_wz;
}
#if 0
#define IX_WORD_OFF(offset) (IX_WORD() + (signed char)(offset))
#define IY_WORD_OFF(offset) (IY_WORD() + (signed char)(offset))
#endif

#define DEC_BC_WORD() \
    do {              \
        if (!reg_c) { \
            reg_b--;  \
        }             \
        reg_c--;      \
    } while (0)

#define DEC_DE_WORD() \
    do {              \
        if (!reg_e) { \
            reg_d--;  \
        }             \
        reg_e--;      \
    } while (0)

#define DEC_HL_WORD() \
    do {              \
        if (!reg_l) { \
            reg_h--;  \
        }             \
        reg_l--;      \
    } while (0)

#define DEC_IX_WORD()   \
    do {                \
        if (!reg_ixl) { \
            reg_ixh--;  \
        }               \
        reg_ixl--;      \
    } while (0)

#define DEC_IY_WORD()   \
    do {                \
        if (!reg_iyl) { \
            reg_iyh--;  \
        }               \
        reg_iyl--;      \
    } while (0)

#define INC_BC_WORD() \
    do {              \
        reg_c++;      \
        if (!reg_c) { \
            reg_b++;  \
        }             \
    } while (0)

#define INC_DE_WORD() \
    do {              \
        reg_e++;      \
        if (!reg_e) { \
            reg_d++;  \
        }             \
    } while (0)

#define INC_HL_WORD() \
    do {              \
        reg_l++;      \
        if (!reg_l) { \
            reg_h++;  \
        }             \
    } while (0)

#define INC_IX_WORD()   \
    do {                \
        reg_ixl++;      \
        if (!reg_ixl) { \
            reg_ixh++;  \
        }               \
    } while (0)

#define INC_IY_WORD()   \
    do {                \
        reg_iyl++;      \
        if (!reg_iyl) { \
            reg_iyh++;  \
        }               \
    } while (0)

/* ------------------------------------------------------------------------- */

/* Flags.  */

#define C_FLAG  0x01
#define N_FLAG  0x02
#define P_FLAG  0x04
#define U3_FLAG 0x08
#define H_FLAG  0x10
#define U5_FLAG 0x20
#define Z_FLAG  0x40
#define S_FLAG  0x80

#define U35_FLAG (U3_FLAG | U5_FLAG)

#define LOCAL_SET_CARRY(val)  \
    do {                      \
        if (val) {            \
            reg_f |= C_FLAG;  \
        } else {              \
            reg_f &= ~C_FLAG; \
        }                     \
    } while (0)

#define LOCAL_SET_NADDSUB(val) \
    do {                       \
        if (val) {             \
            reg_f |= N_FLAG;   \
        } else {               \
            reg_f &= ~N_FLAG;  \
        }                      \
    } while (0)

#define LOCAL_SET_PARITY(val) \
    do {                      \
        if (val) {            \
            reg_f |= P_FLAG;  \
        } else {              \
            reg_f &= ~P_FLAG; \
        }                     \
    } while (0)

#define LOCAL_SET_HALFCARRY(val) \
    do {                         \
        if (val) {               \
            reg_f |= H_FLAG;     \
        } else {                 \
            reg_f &= ~H_FLAG;    \
        }                        \
    } while (0)

#define LOCAL_SET_ZERO(val)   \
    do {                      \
        if (val) {            \
            reg_f |= Z_FLAG;  \
        } else {              \
            reg_f &= ~Z_FLAG; \
        }                     \
    } while (0)

#define LOCAL_SET_SIGN(val)   \
    do {                      \
        if (val) {            \
            reg_f |= S_FLAG;  \
        } else {              \
            reg_f &= ~S_FLAG; \
        }                     \
    } while (0)

#define LOCAL_CARRY()     (reg_f & C_FLAG)
#define LOCAL_NADDSUB()   (reg_f & N_FLAG)
#define LOCAL_PARITY()    (reg_f & P_FLAG)
#define LOCAL_HALFCARRY() (reg_f & H_FLAG)
#define LOCAL_ZERO()      (reg_f & Z_FLAG)
#define LOCAL_SIGN()      (reg_f & S_FLAG)

#if 0
static const uint8_t SZP[256] = {
    P_FLAG | Z_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             P_FLAG,               0,               0,          P_FLAG,
             P_FLAG,               0,               0,          P_FLAG,
                  0,          P_FLAG,          P_FLAG,               0,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
             S_FLAG, S_FLAG | P_FLAG, S_FLAG | P_FLAG,          S_FLAG,
    S_FLAG | P_FLAG,          S_FLAG,          S_FLAG, S_FLAG | P_FLAG
};
#endif
static const uint8_t SZP[256] = {
    P_FLAG | Z_FLAG,
    0,
    0,
    P_FLAG,
    0,
    P_FLAG,
    P_FLAG,
    0,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    0,
    P_FLAG,
    P_FLAG,
    0,
    P_FLAG,
    0,
    0,
    P_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    0,
    P_FLAG,
    P_FLAG,
    0,
    P_FLAG,
    0,
    0,
    P_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    P_FLAG,
    0,
    0,
    P_FLAG,
    0,
    P_FLAG,
    P_FLAG,
    0,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U3_FLAG,
    U3_FLAG,
    U3_FLAG | P_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG,
    U5_FLAG,
    U5_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    S_FLAG,
    S_FLAG,
    S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG | P_FLAG,
    U3_FLAG | S_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG,
    U5_FLAG | U3_FLAG | S_FLAG | P_FLAG,
};

/* ------------------------------------------------------------------------- */

static void import_registers(void)
{
    reg_a = z80_regs.reg_af >> 8;
    reg_f = z80_regs.reg_af & 0xff;
    reg_b = z80_regs.reg_bc >> 8;
    reg_c = z80_regs.reg_bc & 0xff;
    reg_d = z80_regs.reg_de >> 8;
    reg_e = z80_regs.reg_de & 0xff;
    reg_h = z80_regs.reg_hl >> 8;
    reg_l = z80_regs.reg_hl & 0xff;
    reg_ixh = z80_regs.reg_ix >> 8;
    reg_ixl = z80_regs.reg_ix & 0xff;
    reg_iyh = z80_regs.reg_iy >> 8;
    reg_iyl = z80_regs.reg_iy & 0xff;
    reg_sp = z80_regs.reg_sp;
    z80_reg_pc = (uint32_t)z80_regs.reg_pc;
    reg_i = z80_regs.reg_i;
    reg_r = z80_regs.reg_r;
    reg_a2 = z80_regs.reg_af2 >> 8;
    reg_f2 = z80_regs.reg_af2 & 0xff;
    reg_b2 = z80_regs.reg_bc2 >> 8;
    reg_c2 = z80_regs.reg_bc2 & 0xff;
    reg_d2 = z80_regs.reg_de2 >> 8;
    reg_e2 = z80_regs.reg_de2 & 0xff;
    reg_h2 = z80_regs.reg_hl2 >> 8;
    reg_l2 = z80_regs.reg_hl2 & 0xff;
}

static void export_registers(void)
{
    z80_regs.reg_af = (reg_a << 8) | reg_f;
    z80_regs.reg_bc = (reg_b << 8) | reg_c;
    z80_regs.reg_de = (reg_d << 8) | reg_e;
    z80_regs.reg_hl = (reg_h << 8) | reg_l;
    z80_regs.reg_ix = (reg_ixh << 8) | reg_ixl;
    z80_regs.reg_iy = (reg_iyh << 8) | reg_iyl;
    z80_regs.reg_sp = reg_sp;
    z80_regs.reg_pc = (uint16_t)z80_reg_pc;
    z80_regs.reg_i = reg_i;
    z80_regs.reg_r = reg_r;
    z80_regs.reg_af2 = (reg_a2 << 8) | reg_f2;
    z80_regs.reg_bc2 = (reg_b2 << 8) | reg_c2;
    z80_regs.reg_de2 = (reg_d2 << 8) | reg_e2;
    z80_regs.reg_hl2 = (reg_h2 << 8) | reg_l2;
}

/* ------------------------------------------------------------------------- */

/* Interrupt handling.  */

/* FIXME: Only IM 1 is really supported; don't trust others or their timing. */

#define DO_INTERRUPT(int_kind)                                                            \
    do {                                                                                  \
        uint8_t ik = (int_kind);                                                             \
                                                                                          \
        if (ik & (IK_IRQ | IK_NMI)) {                                                     \
            if ((ik & IK_NMI) && 0) {                                                     \
            } else if ((ik & IK_IRQ) && iff1) {                                           \
                uint16_t jumpdst;                                                             \
                if (monitor_mask[e_comp_space] & (MI_STEP)) {                             \
                    monitor_check_icount_interrupt();                                     \
                }                                                                         \
                halt = 0;                                                                 \
                CLK_ADD(CLK, 7);                                                          \
                --reg_sp;                                                                 \
                STORE((reg_sp), ((uint8_t)(z80_reg_pc >> 8)));                               \
                CLK_ADD(CLK, 3);                                                          \
                --reg_sp;                                                                 \
                STORE((reg_sp), ((uint8_t)(z80_reg_pc & 0xff)));                             \
                iff1 = 0;                                                                 \
                iff2 = 0;                                                                 \
                iff1_1 = 0;                                                               \
                iff2_1 = 0;                                                               \
                iff1_2 = 0;                                                               \
                iff2_2 = 0;                                                               \
                if (im_mode == 1) {                                                       \
                    jumpdst = 0x38;                                                       \
                    CLK_ADD(CLK, 3);                                                      \
                    JUMP(jumpdst);                                                        \
                } else {                                                                  \
                    jumpdst = (LOAD(reg_i << 8) << 8);                                    \
                    CLK_ADD(CLK, 3);                                                      \
                    jumpdst |= (LOAD((reg_i << 8) + 1));                                  \
                    JUMP(jumpdst);                                                        \
                    CLK_ADD(CLK, 6);                                                      \
                }                                                                         \
                interrupt_ack_irq(cpu_int_status);                                        \
            }                                                                             \
        }                                                                                 \
        if (ik & (IK_TRAP | IK_RESET)) {                                                  \
            if (ik & IK_TRAP) {                                                           \
                export_registers();                                                       \
                interrupt_do_trap(cpu_int_status, (uint16_t)z80_reg_pc);                      \
                import_registers();                                                       \
                if (cpu_int_status->global_pending_int & IK_RESET) {                      \
                    ik |= IK_RESET;                                                       \
                }                                                                         \
            }                                                                             \
            if (ik & IK_RESET) {                                                          \
                maincpu_reset();                                                          \
                interrupt_ack_reset(cpu_int_status);                                      \
            }                                                                             \
        }                                                                                 \
        if (ik & (IK_MONITOR)) {                                                          \
            if (monitor_mask[e_comp_space] & (MI_STEP)) {                                 \
                export_registers();                                                       \
                monitor_check_icount((uint16_t)z80_reg_pc);                               \
                import_registers();                                                       \
            }                                                                             \
            if (monitor_mask[e_comp_space] & (MI_BREAK)) {                                \
                export_registers();                                                       \
                if (monitor_check_breakpoints(e_comp_space, (uint16_t)z80_reg_pc)) {      \
                    monitor_startup(e_comp_space);                                        \
                }                                                                         \
                import_registers();                                                       \
            }                                                                             \
            if (monitor_mask[e_comp_space] & (MI_WATCH)) {                                \
                export_registers();                                                       \
                monitor_check_watchpoints(LAST_OPCODE_ADDR, (uint16_t)z80_reg_pc);        \
                import_registers();                                                       \
            }                                                                             \
        }                                                                                 \
    } while (0)

/* ------------------------------------------------------------------------- */

/* Opcodes.  */

#define ADC(loadval, clk_inc1, clk_inc2, pc_inc)                                    \
    do {                                                                            \
        uint8_t tmp, carry, value;                                                     \
                                                                                    \
        CLK_ADD(CLK, clk_inc1);                                                     \
        value = (uint8_t)(loadval);                                                    \
        carry = LOCAL_CARRY();                                                      \
        tmp = reg_a + value + carry;                                                \
        reg_f = SZP[tmp];                                                           \
        LOCAL_SET_CARRY((uint16_t)((uint16_t)reg_a + (uint16_t)value + (uint16_t)(carry)) & 0x100); \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);                        \
        LOCAL_SET_PARITY((~(reg_a ^ value)) & (reg_a ^ tmp) & 0x80);                \
        reg_a = tmp;                                                                \
        CLK_ADD(CLK, clk_inc2);                                                     \
        INC_PC(pc_inc);                                                             \
    } while (0)

#define ADCHLREG(reg_valh, reg_vall)                                                       \
    do {                                                                                   \
        uint32_t tmp, carry;                                                                  \
                                                                                           \
        carry = LOCAL_CARRY();                                                             \
        tmp = (uint32_t)((reg_h << 8) + reg_l) + (uint32_t)((reg_valh << 8) + reg_vall) + carry; \
        LOCAL_SET_ZERO(!(tmp & 0xffff));                                                   \
        LOCAL_SET_NADDSUB(0);                                                              \
        LOCAL_SET_SIGN(tmp & 0x8000);                                                      \
        LOCAL_SET_CARRY(tmp & 0x10000);                                                    \
        LOCAL_SET_HALFCARRY(((tmp >> 8) ^ reg_valh ^ reg_h) & H_FLAG);                     \
        LOCAL_SET_PARITY((~(reg_h ^ reg_valh)) & (reg_valh ^ (tmp >> 8)) & 0x80);          \
        reg_h = (uint8_t)(tmp >> 8);                                                          \
        reg_l = (uint8_t)(tmp & 0xff);                                                        \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_h & U35_FLAG);                                \
        CLK_ADD(CLK, 15);                                                                  \
        INC_PC(2);                                                                         \
    } while (0)

#define ADCHLSP()                                                                           \
    do {                                                                                    \
        uint32_t tmp, carry;                                                                   \
                                                                                            \
        carry = LOCAL_CARRY();                                                              \
        tmp = (uint32_t)((reg_h << 8) + reg_l) + (uint32_t)(reg_sp) + carry;                      \
        reg_wz = HL_WORD();                                                                 \
        reg_wz++;                                                                           \
        LOCAL_SET_ZERO(!(tmp & 0xffff));                                                    \
        LOCAL_SET_NADDSUB(0);                                                               \
        LOCAL_SET_SIGN(tmp & 0x8000);                                                       \
        LOCAL_SET_CARRY(tmp & 0x10000);                                                     \
        LOCAL_SET_HALFCARRY(((tmp >> 8) ^ (reg_sp >> 8) ^ reg_h) & H_FLAG);                 \
        LOCAL_SET_PARITY((~(reg_h ^ (reg_sp >> 8))) & ((reg_sp >> 8) ^ (tmp >> 8)) & 0x80); \
        reg_h = (uint8_t)(tmp >> 8);                                                           \
        reg_l = (uint8_t)(tmp & 0xff);                                                         \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_h & U35_FLAG);                                 \
        CLK_ADD(CLK, 15);                                                                   \
        INC_PC(2);                                                                          \
    } while (0)

#define ADD(loadval, clk_inc1, clk_inc2, pc_inc)                     \
    do {                                                             \
        uint8_t tmp, value;                                             \
                                                                     \
        CLK_ADD(CLK, clk_inc1);                                      \
        value = (uint8_t)(loadval);                                     \
        tmp = reg_a + value;                                         \
        reg_f = SZP[tmp];                                            \
        LOCAL_SET_CARRY((uint16_t)((uint16_t)reg_a + (uint16_t)value) & 0x100);  \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);         \
        LOCAL_SET_PARITY((~(reg_a ^ value)) & (reg_a ^ tmp) & 0x80); \
        reg_a = tmp;                                                 \
        CLK_ADD(CLK, clk_inc2);                                      \
        INC_PC(pc_inc);                                              \
    } while (0)

#define ADDXXREG(reg_dsth, reg_dstl, reg_valh, reg_vall, clk_inc, pc_inc)                \
    do {                                                                                 \
        uint32_t tmp;                                                                       \
                                                                                         \
        tmp = (uint32_t)((reg_dsth << 8) + reg_dstl) + (uint32_t)((reg_valh << 8) + reg_vall); \
        LOCAL_SET_NADDSUB(0);                                                            \
        LOCAL_SET_CARRY(tmp & 0x10000);                                                  \
        LOCAL_SET_HALFCARRY(((tmp >> 8) ^ reg_valh ^ reg_dsth) & H_FLAG);                \
        reg_wz = (reg_dsth << 8) | reg_dstl;                                             \
        reg_wz++;                                                                        \
        reg_dsth = (uint8_t)(tmp >> 8);                                                     \
        reg_dstl = (uint8_t)(tmp & 0xff);                                                   \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_dsth & U35_FLAG);                           \
        CLK_ADD(CLK, clk_inc);                                                           \
        INC_PC(pc_inc);                                                                  \
    } while (0)

#define ADDXXSP(reg_dsth, reg_dstl, clk_inc, pc_inc)                           \
    do {                                                                       \
        uint32_t tmp;                                                             \
                                                                               \
        tmp = (uint32_t)((reg_dsth << 8) + reg_dstl) + (uint32_t)(reg_sp);           \
        LOCAL_SET_NADDSUB(0);                                                  \
        LOCAL_SET_CARRY(tmp & 0x10000);                                        \
        LOCAL_SET_HALFCARRY(((tmp >> 8) ^ (reg_sp >> 8) ^ reg_dsth) & H_FLAG); \
        reg_wz = (reg_dsth << 8) | reg_dstl;                                   \
        reg_wz++;                                                              \
        reg_dsth = (uint8_t)(tmp >> 8);                                           \
        reg_dstl = (uint8_t)(tmp & 0xff);                                         \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_dsth & U35_FLAG);                    \
        CLK_ADD(CLK, clk_inc);                                                 \
        INC_PC(pc_inc);                                                        \
    } while (0)

#define AND(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                       \
        CLK_ADD(CLK, clk_inc1);                \
        reg_a &= (value);                      \
        reg_f = SZP[reg_a];                    \
        LOCAL_SET_HALFCARRY(1);                \
        CLK_ADD(CLK, clk_inc2);                \
        INC_PC(pc_inc);                        \
    } while (0)

/* reg_wz is used here to determine F5 and F3 */
#define BIT(reg_val, value, clk_inc1, clk_inc2, pc_inc) \
    do {                                                \
        uint8_t tmp;                                    \
        CLK_ADD(CLK, clk_inc1);                         \
        tmp = (reg_val) & (1 << value);                 \
        LOCAL_SET_NADDSUB(0);                           \
        LOCAL_SET_HALFCARRY(1);                         \
        LOCAL_SET_ZERO(!tmp);                           \
        LOCAL_SET_PARITY(!tmp);                         \
        reg_f = (reg_f & ~(S_FLAG | U35_FLAG)) | (tmp & S_FLAG) | (reg_val & U35_FLAG); \
        CLK_ADD(CLK, clk_inc2);                         \
        INC_PC(pc_inc);                                 \
    } while (0)

/* reg_wz is used here to determine F5 and F3 */
#define BIT16(reg_val, value, clk_inc1, clk_inc2, pc_inc) \
    do {                                                \
        uint8_t tmp;                                    \
        CLK_ADD(CLK, clk_inc1);                         \
        tmp = (reg_val) & (1 << value);                 \
        LOCAL_SET_NADDSUB(0);                           \
        LOCAL_SET_HALFCARRY(1);                         \
        LOCAL_SET_ZERO(!tmp);                           \
        LOCAL_SET_PARITY(!tmp);                         \
        reg_f = (reg_f & ~(S_FLAG | U35_FLAG)) | (tmp & S_FLAG) | ((reg_wz >> 8) & U35_FLAG); \
        CLK_ADD(CLK, clk_inc2);                         \
        INC_PC(pc_inc);                                 \
    } while (0)

#define BRANCH(cond, value, pc_inc)                                 \
    do {                                                            \
        if (cond) {                                                 \
            unsigned int dest_addr;                                 \
                                                                    \
            dest_addr = z80_reg_pc + pc_inc + (signed char)(value); \
            z80_reg_pc = dest_addr & 0xffff;                        \
            CLK_ADD(CLK, 12);                                        \
        } else {                                                    \
            CLK_ADD(CLK, 7);                                        \
            INC_PC(pc_inc);                                         \
        }                                                           \
    } while (0)

#define CALL(reg_val, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                    \
        INC_PC(pc_inc);                                     \
        CLK_ADD(CLK, clk_inc1);                             \
        --reg_sp;                                           \
        STORE((reg_sp), ((uint8_t)(z80_reg_pc >> 8)));         \
        CLK_ADD(CLK, clk_inc2);                             \
        --reg_sp;                                           \
        STORE((reg_sp), ((uint8_t)(z80_reg_pc & 0xff)));       \
        JUMP(reg_val);                                      \
        CLK_ADD(CLK, clk_inc3);                             \
    } while (0)

#define CALL_COND(reg_value, cond, clk_inc1, clk_inc2, clk_inc3, clk_inc4, pc_inc) \
    do {                                                                           \
        if (cond) {                                                                \
            CALL(reg_value, clk_inc1, clk_inc2, clk_inc3, pc_inc);                 \
        } else {                                                                   \
            CLK_ADD(CLK, clk_inc4);                                                \
            INC_PC(3);                                                             \
        }                                                                          \
    } while (0)

/* note that bits 5 and 3 are set based on previous flag and A */
/* this passes zexall, but see:
   https://github.com/hoglet67/Z80Decoder/wiki/Undocumented-Flags#scfccf
*/
#define CCF(clk_inc, pc_inc)                  \
    do {                                      \
        LOCAL_SET_HALFCARRY((LOCAL_CARRY())); \
        LOCAL_SET_CARRY(!(LOCAL_CARRY()));    \
        LOCAL_SET_NADDSUB(0);                 \
        reg_f = reg_f | (reg_a & U35_FLAG);   \
        CLK_ADD(CLK, clk_inc);                \
        INC_PC(pc_inc);                       \
    } while (0)

/* Bits 3 and 5 of flag based on passed value */
#define CP(loadval, clk_inc1, clk_inc2, pc_inc)                   \
    do {                                                          \
        uint8_t tmp, value;                                          \
                                                                  \
        CLK_ADD(CLK, clk_inc1);                                   \
        value = (uint8_t)(loadval);                                  \
        tmp = reg_a - value;                                      \
        reg_f = N_FLAG | SZP[tmp];                                \
        LOCAL_SET_CARRY(value > reg_a);                           \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);      \
        LOCAL_SET_PARITY((reg_a ^ value) & (reg_a ^ tmp) & 0x80); \
        reg_f = (reg_f & ~(U35_FLAG)) | (value & U35_FLAG);       \
        CLK_ADD(CLK, clk_inc2);                                   \
        INC_PC(pc_inc);                                           \
    } while (0)

#define CPDI(HL_FUNC,WZ_FUNC)                              \
    do {                                                   \
        uint8_t val, tmp;                                     \
                                                           \
        WZ_FUNC;                                           \
        CLK_ADD(CLK, 4);                                   \
        val = LOAD(HL_WORD());                             \
        tmp = reg_a - val;                                 \
        HL_FUNC;                                           \
        DEC_BC_WORD();                                     \
        reg_f = N_FLAG | SZP[tmp] | LOCAL_CARRY();         \
        LOCAL_SET_HALFCARRY((reg_a ^ val ^ tmp) & H_FLAG); \
        LOCAL_SET_PARITY(reg_b | reg_c);                   \
        tmp = tmp - (LOCAL_HALFCARRY() ? 1 : 0);           \
        reg_f = (reg_f & ~(U35_FLAG)) | (tmp & U3_FLAG) | ((tmp << 4) & U5_FLAG); \
        CLK_ADD(CLK, 12);                                  \
        INC_PC(2);                                         \
    } while (0)

#define CPDIR(HL_FUNC)                                         \
    do {                                                       \
        uint8_t val, tmp;                                         \
                                                               \
        CLK_ADD(CLK, 4);                                       \
        val = LOAD(HL_WORD());                                 \
        tmp = reg_a - val;                                     \
        HL_FUNC;                                               \
        DEC_BC_WORD();                                         \
        CLK_ADD(CLK, 12);                                      \
        reg_wz = z80_reg_pc + 1;                               \
        if (!(BC_WORD() && tmp)) {                             \
            reg_f = N_FLAG | SZP[tmp] | LOCAL_CARRY();         \
            LOCAL_SET_HALFCARRY((reg_a ^ val ^ tmp) & H_FLAG); \
            LOCAL_SET_PARITY(reg_b | reg_c);                   \
            tmp = tmp - (LOCAL_HALFCARRY() ? 1 : 0);           \
            reg_f = (reg_f & ~(U35_FLAG)) | (tmp & U3_FLAG) | ((tmp << 4) & U5_FLAG); \
            INC_PC(2);                                         \
        } else {                                               \
            CLK_ADD(CLK, 5);                                   \
        }                                                      \
    } while (0)

#define CPL(clk_inc, pc_inc)    \
    do {                        \
        reg_a ^= 0xff;          \
        LOCAL_SET_NADDSUB(1);   \
        LOCAL_SET_HALFCARRY(1); \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_a & U35_FLAG); \
        CLK_ADD(CLK, clk_inc);  \
        INC_PC(pc_inc);         \
    } while (0)

#define DAA(clk_inc, pc_inc)                                                                                         \
    do {                                                                                                             \
        uint16_t tmp;                                                                                                    \
                                                                                                                     \
        tmp = reg_a | (LOCAL_CARRY() ? 0x100 : 0) | (LOCAL_HALFCARRY() ? 0x200 : 0) | (LOCAL_NADDSUB() ? 0x400 : 0); \
        reg_a = daa_reg_a[tmp];                                                                                      \
        reg_f = daa_reg_f[tmp];                                                                                      \
        CLK_ADD(CLK, clk_inc);                                                                                       \
        INC_PC(pc_inc);                                                                                              \
    } while (0)

#define DECXXIND(reg_val, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                        \
        uint8_t tmp;                                               \
                                                                \
        CLK_ADD(CLK, clk_inc1);                                 \
        tmp = LOAD((reg_val));                                  \
        tmp--;                                                  \
        CLK_ADD(CLK, clk_inc2);                                 \
        STORE((reg_val), tmp);                                  \
        reg_f = N_FLAG | SZP[tmp] | LOCAL_CARRY();              \
        LOCAL_SET_PARITY((tmp == 0x7f));                        \
        LOCAL_SET_HALFCARRY(((tmp & 0x0f) == 0x0f));            \
        CLK_ADD(CLK, clk_inc3);                                 \
        INC_PC(pc_inc);                                         \
    } while (0)

#define DECINC(FUNC, clk_inc, pc_inc) \
    do {                              \
        CLK_ADD(CLK, clk_inc);        \
        FUNC;                         \
        INC_PC(pc_inc);               \
    } while (0)

#define DECREG(reg_val, clk_inc, pc_inc)                 \
    do {                                                 \
        reg_val--;                                       \
        reg_f = N_FLAG | SZP[reg_val] | LOCAL_CARRY();   \
        LOCAL_SET_PARITY((reg_val == 0x7f));             \
        LOCAL_SET_HALFCARRY(((reg_val & 0x0f) == 0x0f)); \
        CLK_ADD(CLK, clk_inc);                           \
        INC_PC(pc_inc);                                  \
    } while (0)

#define DJNZ(value, pc_inc)           \
    do {                              \
        reg_b--;                      \
        CLK_ADD(CLK, 1);              \
        BRANCH(reg_b, value, pc_inc); \
    } while (0)

#define DI(clk_inc, pc_inc)    \
    do {                       \
        iff1_2 = 0;            \
        iff2_2 = 0;            \
        iff1_1 = 0;            \
        iff2_1 = 0;            \
        iff1 = 0;              \
        iff2 = 0;              \
        OPCODE_DISABLES_IRQ(); \
        CLK_ADD(CLK, clk_inc); \
        INC_PC(pc_inc);        \
    } while (0)

/* Back to back EIs results in interrupts being delayed until one instruction after the last EI.
   So we set iff1_1 and iff2_1 to 0.
   See:
   http://www.visual6502.org/JSSim/expert-z80.html?a=0&d=ed56fbfbfbfbfbfbfbfbfbfbfbfbfbfbfbfbfb0000000000000000&a=38&d=c9&int0=48&steps=200&graphics=false
*/

#define EI(clk_inc, pc_inc)    \
    do {                       \
        iff1_2 = 1;            \
        iff2_2 = 1;            \
        iff1_1 = 0;            \
        iff2_1 = 0;            \
        OPCODE_ENABLES_IRQ();  \
        CLK_ADD(CLK, clk_inc); \
        INC_PC(pc_inc);        \
        OPCODE_DELAYS_INTERRUPT();\
    } while (0)

#define EXAFAF(clk_inc, pc_inc) \
    do {                        \
        uint8_t tmpl, tmph;     \
                                \
        tmph = reg_a;           \
        tmpl = reg_f;           \
        reg_a = reg_a2;         \
        reg_f = reg_f2;         \
        reg_a2 = tmph;          \
        reg_f2 = tmpl;          \
        CLK_ADD(CLK, clk_inc);  \
        INC_PC(pc_inc);         \
    } while (0)

#define EXX(clk_inc, pc_inc)   \
    do {                       \
        uint8_t tmpl, tmph;       \
                               \
        tmph = reg_b;          \
        tmpl = reg_c;          \
        reg_b = reg_b2;        \
        reg_c = reg_c2;        \
        reg_b2 = tmph;         \
        reg_c2 = tmpl;         \
        tmph = reg_d;          \
        tmpl = reg_e;          \
        reg_d = reg_d2;        \
        reg_e = reg_e2;        \
        reg_d2 = tmph;         \
        reg_e2 = tmpl;         \
        tmph = reg_h;          \
        tmpl = reg_l;          \
        reg_h = reg_h2;        \
        reg_l = reg_l2;        \
        reg_h2 = tmph;         \
        reg_l2 = tmpl;         \
        CLK_ADD(CLK, clk_inc); \
        INC_PC(pc_inc);        \
    } while (0)

#define EXDEHL(clk_inc, pc_inc) \
    do {                        \
        uint8_t tmpl, tmph;        \
                                \
        tmph = reg_d;           \
        tmpl = reg_e;           \
        reg_d = reg_h;          \
        reg_e = reg_l;          \
        reg_h = tmph;           \
        reg_l = tmpl;           \
        CLK_ADD(CLK, clk_inc);  \
        INC_PC(pc_inc);         \
    } while (0)

#define EXXXSP(reg_valh, reg_vall, clk_inc1, clk_inc2, clk_inc3, clk_inc4, clk_inc5, pc_inc) \
    do {                                                                                     \
        uint8_t tmpl, tmph;                                                                     \
                                                                                             \
        tmph = reg_valh;                                                                     \
        tmpl = reg_vall;                                                                     \
        CLK_ADD(CLK, clk_inc1);                                                              \
        reg_valh = LOAD(reg_sp + 1);                                                         \
        CLK_ADD(CLK, clk_inc2);                                                              \
        reg_vall = LOAD(reg_sp);                                                             \
        reg_wz = (reg_valh << 8) | reg_vall;                                                 \
        CLK_ADD(CLK, clk_inc3);                                                              \
        STORE((reg_sp + 1), tmph);                                                           \
        CLK_ADD(CLK, clk_inc4);                                                              \
        STORE(reg_sp, tmpl);                                                                 \
        CLK_ADD(CLK, clk_inc5);                                                              \
        INC_PC(pc_inc);                                                                      \
    } while (0)

/* FIXME: Continue if INT occurs.  */
#define HALT()           \
    do {                 \
        CLK_ADD(CLK, 4); \
        INC_PC(1);       \
        halt = 1;        \
    } while (0)

#define IM(value)        \
    do {                 \
        im_mode = value; \
        CLK_ADD(CLK, 8); \
        INC_PC(2);       \
    } while (0)

#define INA(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                       \
        uint16_t tmp;                          \
        CLK_ADD(CLK, clk_inc1);                \
        tmp = (reg_a << 8) | value;            \
        reg_a = IN(tmp);                       \
        CLK_ADD(CLK, clk_inc2);                \
        reg_wz = tmp + 1;                      \
        INC_PC(pc_inc);                        \
    } while (0)

#ifdef Z80_4MHZ
#define INBC(reg_val, clk_inc1, clk_inc2, pc_inc)    \
    do {                                             \
        uint8_t tmp = z80_half_cycle;                \
        CLK_ADD(CLK, (clk_inc1 + tmp) );             \
        reg_val = IN(BC_WORD());                     \
        reg_f = SZP[reg_val & 0xffu] | LOCAL_CARRY(); \
        CLK_ADD(CLK, (clk_inc2 - tmp) );             \
        reg_wz = BC_WORD() + 1;                      \
        INC_PC(pc_inc);                              \
    } while (0)
#else
#define INBC(reg_val, clk_inc1, clk_inc2, pc_inc)    \
    do {                                             \
        CLK_ADD(CLK, clk_inc1);                      \
        reg_val = IN(BC_WORD());                     \
        reg_f = SZP[reg_val & 0xffu] | LOCAL_CARRY(); \
        CLK_ADD(CLK, clk_inc2);                      \
        reg_wz = BC_WORD() + 1;                      \
        INC_PC(pc_inc);                              \
    } while (0)
#endif

#define INBC0(clk_inc1, clk_inc2, pc_inc) \
    do {                                  \
        uint8_t tmp;                         \
        CLK_ADD(CLK, clk_inc1);           \
        tmp = IN(BC_WORD());              \
        reg_f = SZP[tmp] | LOCAL_CARRY(); \
        CLK_ADD(CLK, clk_inc2);           \
        reg_wz = BC_WORD() + 1;           \
        INC_PC(pc_inc);                   \
    } while (0)

#define INCXXIND(reg_val, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                        \
        uint8_t tmp;                                               \
                                                                \
        CLK_ADD(CLK, clk_inc1);                                 \
        tmp = LOAD((reg_val));                                  \
        tmp++;                                                  \
        CLK_ADD(CLK, clk_inc2);                                 \
        STORE((reg_val), tmp);                                  \
        reg_f = SZP[tmp] | LOCAL_CARRY();                       \
        LOCAL_SET_PARITY((tmp == 0x80));                        \
        LOCAL_SET_HALFCARRY(!(tmp & 0x0f));                     \
        CLK_ADD(CLK, clk_inc3);                                 \
        INC_PC(pc_inc);                                         \
    } while (0)

#define INCREG(reg_val, clk_inc, pc_inc)        \
    do {                                        \
        reg_val++;                              \
        reg_f = SZP[reg_val] | LOCAL_CARRY();   \
        LOCAL_SET_PARITY((reg_val == 0x80));    \
        LOCAL_SET_HALFCARRY(!(reg_val & 0x0f)); \
        CLK_ADD(CLK, clk_inc);                  \
        INC_PC(pc_inc);                         \
    } while (0)

#define INDI(HL_FUNC)           \
    do {                        \
        uint8_t tmp;               \
                                \
        CLK_ADD(CLK, 4);        \
        tmp = IN(BC_WORD());    \
        CLK_ADD(CLK, 4);        \
        STORE(HL_WORD(), tmp);  \
        HL_FUNC;                \
        reg_wz = BC_WORD() + 1; \
        reg_b--;                \
        reg_f = N_FLAG;         \
        LOCAL_SET_ZERO(!reg_b); \
        CLK_ADD(CLK, 4);        \
        INC_PC(2);              \
    } while (0)

#define INDIR(HL_FUNC)               \
    do {                             \
        uint8_t tmp;                    \
                                     \
        CLK_ADD(CLK, 4);             \
        tmp = IN(BC_WORD());         \
        CLK_ADD(CLK, 4);             \
        STORE(HL_WORD(), tmp);       \
        HL_FUNC;                     \
        reg_b--;                     \
        reg_wz = z80_reg_pc + 1;     \
        if (!reg_b) {                \
            CLK_ADD(CLK, 4);         \
            reg_f = N_FLAG | Z_FLAG; \
            INC_PC(2);               \
        } else {                     \
            CLK_ADD(CLK, 9);         \
            reg_f = N_FLAG;          \
        }                            \
        CLK_ADD(CLK, 4);             \
    } while (0)

#define JMP(addr, clk_inc)     \
    do {                       \
        CLK_ADD(CLK, clk_inc); \
        JUMP(addr);            \
    } while (0)

#define JMP_COND(addr, cond, clk_inc1, clk_inc2) \
    do {                                         \
        if (cond) {                              \
            JMP(addr, clk_inc1);                 \
        } else {                                 \
            CLK_ADD(CLK, clk_inc2);              \
            INC_PC(3);                           \
        }                                        \
    } while (0)

#define LDAIR(reg_val)                      \
    do {                                    \
        CLK_ADD(CLK, 6);                    \
        reg_a = reg_val;                    \
        reg_f = SZP[reg_a] | LOCAL_CARRY(); \
        LOCAL_SET_PARITY(iff2);             \
        CLK_ADD(CLK, 3);                    \
        INC_PC(2);                          \
    } while (0)

#define LDDI(DE_FUNC, HL_FUNC)           \
    do {                                 \
        uint8_t tmp;                        \
                                         \
        CLK_ADD(CLK, 4);                 \
        tmp = LOAD(HL_WORD());           \
        CLK_ADD(CLK, 4);                 \
        STORE(DE_WORD(), tmp);           \
        DEC_BC_WORD();                   \
        DE_FUNC;                         \
        HL_FUNC;                         \
        LOCAL_SET_NADDSUB(0);            \
        LOCAL_SET_PARITY(reg_b | reg_c); \
        LOCAL_SET_HALFCARRY(0);          \
        tmp = tmp + reg_a;               \
        reg_f = (reg_f & ~(U35_FLAG)) | (tmp & U3_FLAG) | ((tmp << 4) & U5_FLAG); \
        CLK_ADD(CLK, 8);                 \
        INC_PC(2);                       \
    } while (0)

#define LDDIR(DE_FUNC, HL_FUNC)     \
    do {                            \
        uint8_t tmp;                   \
                                    \
        CLK_ADD(CLK, 4);            \
        tmp = LOAD(HL_WORD());      \
        CLK_ADD(CLK, 4);            \
        STORE(DE_WORD(), tmp);      \
        DEC_BC_WORD();              \
        DE_FUNC;                    \
        HL_FUNC;                    \
        CLK_ADD(CLK, 8);            \
        reg_wz = z80_reg_pc + 1;    \
        if (!(BC_WORD())) {         \
            LOCAL_SET_NADDSUB(0);   \
            LOCAL_SET_PARITY(0);    \
            LOCAL_SET_HALFCARRY(0); \
            tmp = tmp + reg_a;      \
            reg_f = (reg_f & ~(U35_FLAG)) | (tmp & U3_FLAG) | ((tmp << 4) & U5_FLAG); \
            INC_PC(2);              \
        } else {                    \
            CLK_ADD(CLK, 5);        \
        }                           \
    } while (0)

#define LDIND(val, reg_valh, reg_vall, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                                     \
        CLK_ADD(CLK, clk_inc1);                                              \
        reg_vall = LOAD((val));                                              \
        CLK_ADD(CLK, clk_inc2);                                              \
        reg_valh = LOAD((val) + 1);                                          \
        CLK_ADD(CLK, clk_inc3);                                              \
        reg_wz = (val) + 1;                                                  \
        INC_PC(pc_inc);                                                      \
    } while (0)

#define LDSP(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                        \
        CLK_ADD(CLK, clk_inc1);                 \
        reg_sp = (uint16_t)(value);                 \
        CLK_ADD(CLK, clk_inc2);                 \
        INC_PC(pc_inc);                         \
    } while (0)

#define LDSPIND(value, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                     \
        CLK_ADD(CLK, clk_inc1);                              \
        reg_sp = LOAD(value);                                \
        CLK_ADD(CLK, clk_inc2);                              \
        reg_sp |= LOAD(value + 1) << 8;                      \
        reg_wz = value + 1;                                  \
        CLK_ADD(CLK, clk_inc3);                              \
        INC_PC(pc_inc);                                      \
    } while (0)

#define LDREG(reg_dest, value, clk_inc1, clk_inc2, pc_inc) \
    do {                                                   \
        uint8_t tmp;                                          \
                                                           \
        CLK_ADD(CLK, clk_inc1);                            \
        tmp = (uint8_t)(value);                               \
        reg_dest = tmp;                                    \
        CLK_ADD(CLK, clk_inc2);                            \
        INC_PC(pc_inc);                                    \
    } while (0)

#define LDREGWZ(reg_dest, addr, clk_inc1, clk_inc2, pc_inc) \
    do {                                                   \
        uint8_t tmp;                                          \
                                                           \
        CLK_ADD(CLK, clk_inc1);                            \
        tmp = (uint8_t)(LOAD(addr));                       \
        reg_dest = tmp;                                    \
        CLK_ADD(CLK, clk_inc2);                            \
        reg_wz = addr + 1;                                 \
        INC_PC(pc_inc);                                    \
    } while (0)

#define LDW(value, reg_valh, reg_vall, clk_inc1, clk_inc2, pc_inc) \
    do {                                                           \
        CLK_ADD(CLK, clk_inc1);                                    \
        reg_vall = (uint8_t)((value) & 0xff);                         \
        reg_valh = (uint8_t)((value) >> 8);                           \
        CLK_ADD(CLK, clk_inc2);                                    \
        INC_PC(pc_inc);                                            \
    } while (0)

#define NEG()                                        \
    do {                                             \
        uint8_t tmp;                                    \
                                                     \
        tmp = 0 - reg_a;                             \
        reg_f = N_FLAG | SZP[tmp];                   \
        LOCAL_SET_HALFCARRY((reg_a ^ tmp) & H_FLAG); \
        LOCAL_SET_PARITY(reg_a & tmp & 0x80);        \
        LOCAL_SET_CARRY(reg_a > 0);                  \
        reg_a = tmp;                                 \
        CLK_ADD(CLK, 8);                             \
        INC_PC(2);                                   \
    } while (0)

#define NOP(clk_inc, pc_inc)   \
    do {                       \
        CLK_ADD(CLK, clk_inc); \
        INC_PC(pc_inc);        \
    } while (0)

#define OR(reg_val, clk_inc1, clk_inc2, pc_inc) \
    do {                                        \
        CLK_ADD(CLK, clk_inc1);                 \
        reg_a |= reg_val;                       \
        reg_f = SZP[reg_a];                     \
        CLK_ADD(CLK, clk_inc2);                 \
        INC_PC(pc_inc);                         \
    } while (0)

#define OUTA(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                        \
        CLK_ADD(CLK, clk_inc1);                 \
        OUT((reg_a << 8) | value, reg_a);       \
        CLK_ADD(CLK, clk_inc2);                 \
        INC_PC(pc_inc);                         \
    } while (0)

#ifdef Z80_4MHZ
#define OUTBC(value, clk_inc1, clk_inc2, pc_inc)    \
    do {                                            \
        uint8_t tmp = z80_half_cycle;               \
        CLK_ADD(CLK, (clk_inc1 + tmp) );            \
        OUT(BC_WORD(), value);                      \
        CLK_ADD(CLK, (clk_inc2 - tmp) );            \
        reg_wz = BC_WORD() + 1;                     \
        INC_PC(pc_inc);                             \
    } while (0)
#else
#define OUTBC(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                         \
        CLK_ADD(CLK, clk_inc1);                  \
        OUT(BC_WORD(), value);                   \
        CLK_ADD(CLK, clk_inc2);                  \
        reg_wz = BC_WORD() + 1;                  \
        INC_PC(pc_inc);                          \
    } while (0)
#endif

#define OUTDI(HL_FUNC)          \
    do {                        \
        uint8_t tmp;               \
                                \
        CLK_ADD(CLK, 4);        \
        reg_b--;                \
        reg_wz = BC_WORD() + 1; \
        tmp = LOAD(HL_WORD());  \
        CLK_ADD(CLK, 4);        \
        OUT(BC_WORD(), tmp);    \
        HL_FUNC;                \
        reg_f = N_FLAG;         \
        LOCAL_SET_ZERO(!reg_b); \
        CLK_ADD(CLK, 4);        \
        CLK_ADD(CLK, 4);        \
        INC_PC(2);              \
    } while (0)

#define OTDIR(HL_FUNC)               \
    do {                             \
        uint8_t tmp;                    \
                                     \
        CLK_ADD(CLK, 4);             \
        reg_b--;                     \
        tmp = LOAD(HL_WORD());       \
        CLK_ADD(CLK, 4);             \
        OUT(BC_WORD(), tmp);         \
        HL_FUNC;                     \
        reg_wz = z80_reg_pc + 1;     \
        if (!reg_b) {                \
            reg_f = N_FLAG | Z_FLAG; \
            INC_PC(2);               \
        } else {                     \
            reg_f = N_FLAG;          \
            CLK_ADD(CLK, 5);         \
        }                            \
        CLK_ADD(CLK, 8);             \
    } while (0)

#define POP(reg_valh, reg_vall, pc_inc) \
    do {                                \
        CLK_ADD(CLK, 4);                \
        reg_vall = LOAD(reg_sp);        \
        ++reg_sp;                       \
        CLK_ADD(CLK, 4);                \
        reg_valh = LOAD(reg_sp);        \
        ++reg_sp;                       \
        CLK_ADD(CLK, 2);                \
        INC_PC(pc_inc);                 \
    } while (0)

#define PUSH(reg_valh, reg_vall, pc_inc) \
    do {                                 \
        CLK_ADD(CLK, 4);                 \
        --reg_sp;                        \
        STORE((reg_sp), (reg_valh));     \
        CLK_ADD(CLK, 4);                 \
        --reg_sp;                        \
        STORE((reg_sp), (reg_vall));     \
        CLK_ADD(CLK, 3);                 \
        INC_PC(pc_inc);                  \
    } while (0)

#define RES(reg_val, value)         \
    do {                            \
        reg_val &= (~(1 << value)); \
        CLK_ADD(CLK, 8);            \
        INC_PC(2);                  \
    } while (0)

#define RESXX(value, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                         \
        uint8_t tmp;                                                \
                                                                 \
        CLK_ADD(CLK, clk_inc1);                                  \
        tmp = LOAD((addr));                                      \
        tmp &= (~(1 << value));                                  \
        CLK_ADD(CLK, clk_inc2);                                  \
        STORE((addr), tmp);                                      \
        CLK_ADD(CLK, clk_inc3);                                  \
        INC_PC(pc_inc);                                          \
    } while (0)

#define RESXXREG(value, reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                                     \
        uint8_t tmp;                                                            \
                                                                             \
        CLK_ADD(CLK, clk_inc1);                                              \
        tmp = LOAD((addr));                                                  \
        tmp &= (~(1 << value));                                              \
        CLK_ADD(CLK, clk_inc2);                                              \
        STORE((addr), tmp);                                                  \
        reg_val = tmp;                                                       \
        CLK_ADD(CLK, clk_inc3);                                              \
        INC_PC(pc_inc);                                                      \
    } while (0)

#define RET(clk_inc1, clk_inc2, clk_inc3) \
    do {                                  \
        uint16_t tmp;                         \
                                          \
        CLK_ADD(CLK, clk_inc1);           \
        tmp = LOAD(reg_sp);               \
        CLK_ADD(CLK, clk_inc2);           \
        tmp |= LOAD((reg_sp + 1)) << 8;   \
        reg_sp += 2;                      \
        JUMP(tmp);                        \
        CLK_ADD(CLK, clk_inc3);           \
    } while (0)

#define RET_COND(cond, clk_inc1, clk_inc2, clk_inc3, clk_inc4, pc_inc) \
    do {                                                               \
        if (cond) {                                                    \
            RET(clk_inc1, clk_inc2, clk_inc3);                         \
        } else {                                                       \
            CLK_ADD(CLK, clk_inc4);                                    \
            INC_PC(pc_inc);                                            \
        }                                                              \
    } while (0)

#define RETNI()                         \
    do {                                \
        uint16_t tmp;                       \
                                        \
        CLK_ADD(CLK, 4);                \
        tmp = LOAD(reg_sp);             \
        CLK_ADD(CLK, 4);                \
        tmp |= LOAD((reg_sp + 1)) << 8; \
        reg_sp += 2;                    \
        iff1 = iff2;                    \
        JUMP(tmp);                      \
        CLK_ADD(CLK, 6);                \
    } while (0)

#define RL(reg_val)                               \
    do {                                          \
        uint8_t rot;                                 \
                                                  \
        rot = (reg_val & 0x80) ? C_FLAG : 0;      \
        reg_val = (reg_val << 1) | LOCAL_CARRY(); \
        reg_f = rot | SZP[reg_val];               \
        CLK_ADD(CLK, 8);                          \
        INC_PC(2);                                \
    } while (0)

#define RLA(clk_inc, pc_inc)                  \
    do {                                      \
        uint8_t rot;                             \
                                              \
        rot = (reg_a & 0x80) ? C_FLAG : 0;    \
        reg_a = (reg_a << 1) | LOCAL_CARRY(); \
        LOCAL_SET_CARRY(rot);                 \
        LOCAL_SET_NADDSUB(0);                 \
        LOCAL_SET_HALFCARRY(0);               \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_a & U35_FLAG); \
        CLK_ADD(CLK, clk_inc);                \
        INC_PC(pc_inc);                       \
    } while (0)

#define RLC(reg_val)                         \
    do {                                     \
        uint8_t rot;                            \
                                             \
        rot = (reg_val & 0x80) ? C_FLAG : 0; \
        reg_val = (reg_val << 1) | rot;      \
        reg_f = rot | SZP[reg_val];          \
        CLK_ADD(CLK, 8);                     \
        INC_PC(2);                           \
    } while (0)

#define RLCA(clk_inc, pc_inc)              \
    do {                                   \
        uint8_t rot;                          \
                                           \
        rot = (reg_a & 0x80) ? C_FLAG : 0; \
        reg_a = (reg_a << 1) | rot;        \
        LOCAL_SET_CARRY(rot);              \
        LOCAL_SET_NADDSUB(0);              \
        LOCAL_SET_HALFCARRY(0);            \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_a & U35_FLAG); \
        CLK_ADD(CLK, clk_inc);             \
        INC_PC(pc_inc);                    \
    } while (0)

#define RLCXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        uint8_t rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = (tmp & 0x80) ? C_FLAG : 0;                  \
        tmp = (tmp << 1) | rot;                           \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define RLCXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        uint8_t rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = (tmp & 0x80) ? C_FLAG : 0;                              \
        tmp = (tmp << 1) | rot;                                       \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define RLD()                                          \
    do {                                               \
        uint8_t tmp;                                      \
                                                       \
        reg_wz = HL_WORD();                            \
        tmp = LOAD(reg_wz);                            \
        reg_wz++;                                      \
        CLK_ADD(CLK, 8);                               \
        STORE(HL_WORD(), (tmp << 4) | (reg_a & 0x0f)); \
        reg_a = (tmp >> 4) | (reg_a & 0xf0);           \
        reg_f = SZP[reg_a] | LOCAL_CARRY();            \
        CLK_ADD(CLK, 10);                              \
        INC_PC(2);                                     \
    } while (0)

#define RLXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                 \
        uint8_t rot, tmp;                                   \
                                                         \
        CLK_ADD(CLK, clk_inc1);                          \
        tmp = LOAD((addr));                              \
        rot = (tmp & 0x80) ? C_FLAG : 0;                 \
        tmp = (tmp << 1) | LOCAL_CARRY();                \
        CLK_ADD(CLK, clk_inc2);                          \
        STORE((addr), tmp);                              \
        reg_f = rot | SZP[tmp];                          \
        CLK_ADD(CLK, clk_inc3);                          \
        INC_PC(pc_inc);                                  \
    } while (0)

#define RLXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                             \
        uint8_t rot, tmp;                                               \
                                                                     \
        CLK_ADD(CLK, clk_inc1);                                      \
        tmp = LOAD((addr));                                          \
        rot = (tmp & 0x80) ? C_FLAG : 0;                             \
        tmp = (tmp << 1) | LOCAL_CARRY();                            \
        CLK_ADD(CLK, clk_inc2);                                      \
        STORE((addr), tmp);                                          \
        reg_val = tmp;                                               \
        reg_f = rot | SZP[tmp];                                      \
        CLK_ADD(CLK, clk_inc3);                                      \
        INC_PC(pc_inc);                                              \
    } while (0)

#define RR(reg_val)                                            \
    do {                                                       \
        uint8_t rot;                                              \
                                                               \
        rot = reg_val & C_FLAG;                                \
        reg_val = (reg_val >> 1) | (LOCAL_CARRY() ? 0x80 : 0); \
        reg_f = rot | SZP[reg_val];                            \
        CLK_ADD(CLK, 8);                                       \
        INC_PC(2);                                             \
    } while (0)

#define RRA(clk_inc, pc_inc)                               \
    do {                                                   \
        uint8_t rot;                                          \
                                                           \
        rot = reg_a & C_FLAG;                              \
        reg_a = (reg_a >> 1) | (LOCAL_CARRY() ? 0x80 : 0); \
        LOCAL_SET_CARRY(rot);                              \
        LOCAL_SET_NADDSUB(0);                              \
        LOCAL_SET_HALFCARRY(0);                            \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_a & U35_FLAG); \
        CLK_ADD(CLK, clk_inc);                             \
        INC_PC(pc_inc);                                    \
    } while (0)

#define RRC(reg_val)                                   \
    do {                                               \
        uint8_t rot;                                      \
                                                       \
        rot = reg_val & C_FLAG;                        \
        reg_val = (reg_val >> 1) | ((rot) ? 0x80 : 0); \
        reg_f = rot | SZP[reg_val];                    \
        CLK_ADD(CLK, 8);                               \
        INC_PC(2);                                     \
    } while (0)

#define RRCA(clk_inc, pc_inc)                      \
    do {                                           \
        uint8_t rot;                                  \
                                                   \
        rot = reg_a & C_FLAG;                      \
        reg_a = (reg_a >> 1) | ((rot) ? 0x80 : 0); \
        LOCAL_SET_CARRY(rot);                      \
        LOCAL_SET_NADDSUB(0);                      \
        LOCAL_SET_HALFCARRY(0);                    \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_a & U35_FLAG); \
        CLK_ADD(CLK, clk_inc);                     \
        INC_PC(pc_inc);                            \
    } while (0)

#define RRCXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        uint8_t rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = tmp & C_FLAG;                               \
        tmp = (tmp >> 1) | ((rot) ? 0x80 : 0);            \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define RRCXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        uint8_t rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = tmp & C_FLAG;                                           \
        tmp = (tmp >> 1) | ((rot) ? 0x80 : 0);                        \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define RRD()                                        \
    do {                                             \
        uint8_t tmp;                                    \
                                                     \
        reg_wz = HL_WORD();                          \
        tmp = LOAD(reg_wz);                          \
        reg_wz++;                                    \
        CLK_ADD(CLK, 8);                             \
        STORE(HL_WORD(), (tmp >> 4) | (reg_a << 4)); \
        reg_a = (tmp & 0x0f) | (reg_a & 0xf0);       \
        reg_f = SZP[reg_a] | LOCAL_CARRY();          \
        CLK_ADD(CLK, 10);                            \
        INC_PC(2);                                   \
    } while (0)

#define RRXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                 \
        uint8_t rot, tmp;                                   \
                                                         \
        CLK_ADD(CLK, clk_inc1);                          \
        tmp = LOAD((addr));                              \
        rot = tmp & C_FLAG;                              \
        tmp = (tmp >> 1) | (LOCAL_CARRY() ? 0x80 : 0);   \
        CLK_ADD(CLK, clk_inc2);                          \
        STORE((addr), tmp);                              \
        reg_f = rot | SZP[tmp];                          \
        CLK_ADD(CLK, clk_inc3);                          \
        INC_PC(pc_inc);                                  \
    } while (0)

#define RRXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                             \
        uint8_t rot, tmp;                                               \
                                                                     \
        CLK_ADD(CLK, clk_inc1);                                      \
        tmp = LOAD((addr));                                          \
        rot = tmp & C_FLAG;                                          \
        tmp = (tmp >> 1) | (LOCAL_CARRY() ? 0x80 : 0);               \
        CLK_ADD(CLK, clk_inc2);                                      \
        STORE((addr), tmp);                                          \
        reg_val = tmp;                                               \
        reg_f = rot | SZP[tmp];                                      \
        CLK_ADD(CLK, clk_inc3);                                      \
        INC_PC(pc_inc);                                              \
    } while (0)

#define SBCHLREG(reg_valh, reg_vall)                                                     \
    do {                                                                                 \
        uint32_t tmp;                                                                       \
        uint8_t carry;                                                                      \
                                                                                         \
        carry = LOCAL_CARRY();                                                           \
        tmp = (uint32_t)(HL_WORD()) - (uint32_t)((reg_valh << 8) + reg_vall) - (uint32_t)(carry); \
        reg_wz = HL_WORD();                                                              \
        reg_wz++;                                                                        \
        reg_f = N_FLAG;                                                                  \
        LOCAL_SET_CARRY(tmp & 0x10000);                                                  \
        LOCAL_SET_HALFCARRY((reg_h ^ reg_valh ^ (tmp >> 8)) & H_FLAG);                   \
        LOCAL_SET_PARITY(((reg_h ^ (tmp >> 8)) & (reg_h ^ reg_valh)) & 0x80);            \
        LOCAL_SET_ZERO(!(tmp & 0xffff));                                                 \
        LOCAL_SET_SIGN(tmp & 0x8000);                                                    \
        reg_h = (uint8_t)(tmp >> 8);                                                        \
        reg_l = (uint8_t)(tmp & 0xff);                                                      \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_h & U35_FLAG);                              \
        CLK_ADD(CLK, 15);                                                                \
        INC_PC(2);                                                                       \
    } while (0)

#define SBCHLSP()                                                                  \
    do {                                                                           \
        uint32_t tmp;                                                                 \
        uint8_t carry;                                                                \
                                                                                   \
        carry = LOCAL_CARRY();                                                     \
        tmp = (uint32_t)(HL_WORD()) - (uint32_t)reg_sp - (uint32_t)(carry);                 \
        reg_f = N_FLAG;                                                            \
        LOCAL_SET_CARRY(tmp & 0x10000);                                            \
        LOCAL_SET_HALFCARRY((reg_h ^ (reg_sp >> 8) ^ (tmp >> 8)) & H_FLAG);        \
        LOCAL_SET_PARITY(((reg_h ^ (tmp >> 8)) & (reg_h ^ (reg_sp >> 8))) & 0x80); \
        LOCAL_SET_ZERO(!(tmp & 0xffff));                                           \
        LOCAL_SET_SIGN(tmp & 0x8000);                                              \
        reg_h = (uint8_t)(tmp >> 8);                                                  \
        reg_l = (uint8_t)(tmp & 0xff);                                                \
        reg_f = (reg_f & ~(U35_FLAG)) | (reg_h & U35_FLAG);                        \
        CLK_ADD(CLK, 15);                                                          \
        INC_PC(2);                                                                 \
    } while (0)

#define SBC(loadval, clk_inc1, clk_inc2, pc_inc)                      \
    do {                                                              \
        uint8_t tmp, carry, value;                                       \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        value = (uint8_t)(loadval);                                      \
        carry = LOCAL_CARRY();                                        \
        tmp = reg_a - value - carry;                                  \
        reg_f = N_FLAG | SZP[tmp];                                    \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);          \
        LOCAL_SET_PARITY((reg_a ^ value) & (reg_a ^ tmp) & 0x80);     \
        LOCAL_SET_CARRY((uint16_t)((uint16_t)value + (uint16_t)(carry)) > reg_a); \
        reg_a = tmp;                                                  \
        CLK_ADD(CLK, clk_inc2);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

/* note that bits 5 and 3 are set based on previous flag and A */
/* this passes zexall, but see:
   https://github.com/hoglet67/Z80Decoder/wiki/Undocumented-Flags#scfccf
*/
#define SCF(clk_inc, pc_inc)    \
    do {                        \
        LOCAL_SET_CARRY(1);     \
        LOCAL_SET_HALFCARRY(0); \
        LOCAL_SET_NADDSUB(0);   \
        reg_f = reg_f | (reg_a & U35_FLAG); \
        CLK_ADD(CLK, clk_inc);  \
        INC_PC(pc_inc);         \
    } while (0)

#define SET(reg_val, value)      \
    do {                         \
        reg_val |= (1 << value); \
        CLK_ADD(CLK, 8);         \
        INC_PC(2);               \
    } while (0)

#define SETXX(value, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                         \
        uint8_t tmp;                                                \
                                                                 \
        CLK_ADD(CLK, clk_inc1);                                  \
        tmp = LOAD((addr));                                      \
        tmp |= (1 << value);                                     \
        CLK_ADD(CLK, clk_inc2);                                  \
        STORE((addr), tmp);                                      \
        CLK_ADD(CLK, clk_inc3);                                  \
        INC_PC(pc_inc);                                          \
    } while (0)

#define SETXXREG(value, reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                                     \
        uint8_t tmp;                                                            \
                                                                             \
        CLK_ADD(CLK, clk_inc1);                                              \
        tmp = LOAD((addr));                                                  \
        tmp |= (1 << value);                                                 \
        CLK_ADD(CLK, clk_inc2);                                              \
        STORE((addr), tmp);                                                  \
        reg_val = tmp;                                                       \
        CLK_ADD(CLK, clk_inc3);                                              \
        INC_PC(pc_inc);                                                      \
    } while (0)

#define SLA(reg_val)                         \
    do {                                     \
        uint8_t rot;                            \
                                             \
        rot = (reg_val & 0x80) ? C_FLAG : 0; \
        reg_val <<= 1;                       \
        reg_f = rot | SZP[reg_val];          \
        CLK_ADD(CLK, 8);                     \
        INC_PC(2);                           \
    } while (0)

#define SLAXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        uint8_t rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = (tmp & 0x80) ? C_FLAG : 0;                  \
        tmp <<= 1;                                        \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define SLAXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        uint8_t rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = (tmp & 0x80) ? C_FLAG : 0;                              \
        tmp <<= 1;                                                    \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define SLL(reg_val)                         \
    do {                                     \
        uint8_t rot;                            \
                                             \
        rot = (reg_val & 0x80) ? C_FLAG : 0; \
        reg_val = (reg_val << 1) | 1;        \
        reg_f = rot | SZP[reg_val];          \
        CLK_ADD(CLK, 8);                     \
        INC_PC(2);                           \
    } while (0)

#define SLLXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        uint8_t rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = (tmp & 0x80) ? C_FLAG : 0;                  \
        tmp = (tmp << 1) | 1;                             \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define SLLXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        uint8_t rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = (tmp & 0x80) ? C_FLAG : 0;                              \
        tmp = (tmp << 1) | 1;                                         \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define SRA(reg_val)                                 \
    do {                                             \
        uint8_t rot;                                    \
                                                     \
        rot = reg_val & C_FLAG;                      \
        reg_val = (reg_val >> 1) | (reg_val & 0x80); \
        reg_f = rot | SZP[reg_val];                  \
        CLK_ADD(CLK, 8);                             \
        INC_PC(2);                                   \
    } while (0)

#define SRAXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        uint8_t rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = tmp & C_FLAG;                               \
        tmp = (tmp >> 1) | (tmp & 0x80);                  \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define SRAXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        uint8_t rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = tmp & C_FLAG;                                           \
        tmp = (tmp >> 1) | (tmp & 0x80);                              \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define SRL(reg_val)                \
    do {                            \
        uint8_t rot;                   \
                                    \
        rot = reg_val & C_FLAG;     \
        reg_val >>= 1;              \
        reg_f = rot | SZP[reg_val]; \
        CLK_ADD(CLK, 8);            \
        INC_PC(2);                  \
    } while (0)

#define SRLXX(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        uint8_t rot, tmp;                                    \
                                                          \
        CLK_ADD(CLK, clk_inc1);                           \
        tmp = LOAD((addr));                               \
        rot = tmp & C_FLAG;                               \
        tmp >>= 1;                                        \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((addr), tmp);                               \
        reg_f = rot | SZP[tmp];                           \
        CLK_ADD(CLK, clk_inc3);                           \
        INC_PC(pc_inc);                                   \
    } while (0)

#define SRLXXREG(reg_val, addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                              \
        uint8_t rot, tmp;                                                \
                                                                      \
        CLK_ADD(CLK, clk_inc1);                                       \
        tmp = LOAD((addr));                                           \
        rot = tmp & C_FLAG;                                           \
        tmp >>= 1;                                                    \
        CLK_ADD(CLK, clk_inc2);                                       \
        STORE((addr), tmp);                                           \
        reg_val = tmp;                                                \
        reg_f = rot | SZP[tmp];                                       \
        CLK_ADD(CLK, clk_inc3);                                       \
        INC_PC(pc_inc);                                               \
    } while (0)

#define STW(addr, reg_valh, reg_vall, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                                    \
        CLK_ADD(CLK, clk_inc1);                                             \
        STORE((uint16_t)(addr), reg_vall);                                      \
        CLK_ADD(CLK, clk_inc2);                                             \
        STORE((uint16_t)(addr + 1), reg_valh);                                  \
        CLK_ADD(CLK, clk_inc3);                                             \
        reg_wz = addr + 1;                                                  \
        INC_PC(pc_inc);                                                     \
    } while (0)

#define STSPW(addr, clk_inc1, clk_inc2, clk_inc3, pc_inc) \
    do {                                                  \
        CLK_ADD(CLK, clk_inc1);                           \
        STORE((uint16_t)(addr), (reg_sp & 0xff));             \
        CLK_ADD(CLK, clk_inc2);                           \
        STORE((uint16_t)(addr + 1), (reg_sp >> 8));           \
        CLK_ADD(CLK, clk_inc3);                           \
        reg_wz = addr + 1;                                \
        INC_PC(pc_inc);                                   \
    } while (0)

#define STREG(addr, reg_val, clk_inc1, clk_inc2, pc_inc) \
    do {                                                 \
        CLK_ADD(CLK, clk_inc1);                          \
        STORE(addr, reg_val);                            \
        CLK_ADD(CLK, clk_inc2);                          \
        INC_PC(pc_inc);                                  \
    } while (0)

#define STREGWZ(addr, reg_val, clk_inc1, clk_inc2, pc_inc) \
    do {                                                 \
        CLK_ADD(CLK, clk_inc1);                          \
        STORE(addr, reg_val);                            \
        CLK_ADD(CLK, clk_inc2);                          \
        reg_wz = ((addr + 1) & 0xff) | (reg_val << 8);   \
        INC_PC(pc_inc);                                  \
    } while (0)

#define SUB(loadval, clk_inc1, clk_inc2, pc_inc)                  \
    do {                                                          \
        uint8_t tmp, value;                                          \
                                                                  \
        CLK_ADD(CLK, clk_inc1);                                   \
        value = (uint8_t)(loadval);                                  \
        tmp = reg_a - value;                                      \
        reg_f = N_FLAG | SZP[tmp];                                \
        LOCAL_SET_HALFCARRY((reg_a ^ value ^ tmp) & H_FLAG);      \
        LOCAL_SET_PARITY((reg_a ^ value) & (reg_a ^ tmp) & 0x80); \
        LOCAL_SET_CARRY(value > reg_a);                           \
        reg_a = tmp;                                              \
        CLK_ADD(CLK, clk_inc2);                                   \
        INC_PC(pc_inc);                                           \
    } while (0)

#define XOR(value, clk_inc1, clk_inc2, pc_inc) \
    do {                                       \
        CLK_ADD(CLK, clk_inc1);                \
        reg_a ^= value;                        \
        reg_f = SZP[reg_a];                    \
        CLK_ADD(CLK, clk_inc2);                \
        INC_PC(pc_inc);                        \
    } while (0)


/* ------------------------------------------------------------------------- */

/* Extended opcodes.  */

static void opcode_cb(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint16_t ip12, uint16_t ip23)
{
    switch (ip1) {
        case 0x00: /* RLC B */
            RLC(reg_b);
            break;
        case 0x01: /* RLC C */
            RLC(reg_c);
            break;
        case 0x02: /* RLC D */
            RLC(reg_d);
            break;
        case 0x03: /* RLC E */
            RLC(reg_e);
            break;
        case 0x04: /* RLC H */
            RLC(reg_h);
            break;
        case 0x05: /* RLC L */
            RLC(reg_l);
            break;
        case 0x06: /* RLC (HL) */
            RLCXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x07: /* RLC A */
            RLC(reg_a);
            break;
        case 0x08: /* RRC B */
            RRC(reg_b);
            break;
        case 0x09: /* RRC C */
            RRC(reg_c);
            break;
        case 0x0a: /* RRC D */
            RRC(reg_d);
            break;
        case 0x0b: /* RRC E */
            RRC(reg_e);
            break;
        case 0x0c: /* RRC H */
            RRC(reg_h);
            break;
        case 0x0d: /* RRC L */
            RRC(reg_l);
            break;
        case 0x0e: /* RRC (HL) */
            RRCXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x0f: /* RRC A */
            RRC(reg_a);
            break;
        case 0x10: /* RL B */
            RL(reg_b);
            break;
        case 0x11: /* RL C */
            RL(reg_c);
            break;
        case 0x12: /* RL D */
            RL(reg_d);
            break;
        case 0x13: /* RL E */
            RL(reg_e);
            break;
        case 0x14: /* RL H */
            RL(reg_h);
            break;
        case 0x15: /* RL L */
            RL(reg_l);
            break;
        case 0x16: /* RL (HL) */
            RLXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x17: /* RL A */
            RL(reg_a);
            break;
        case 0x18: /* RR B */
            RR(reg_b);
            break;
        case 0x19: /* RR C */
            RR(reg_c);
            break;
        case 0x1a: /* RR D */
            RR(reg_d);
            break;
        case 0x1b: /* RR E */
            RR(reg_e);
            break;
        case 0x1c: /* RR H */
            RR(reg_h);
            break;
        case 0x1d: /* RR L */
            RR(reg_l);
            break;
        case 0x1e: /* RR (HL) */
            RRXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x1f: /* RR A */
            RR(reg_a);
            break;
        case 0x20: /* SLA B */
            SLA(reg_b);
            break;
        case 0x21: /* SLA C */
            SLA(reg_c);
            break;
        case 0x22: /* SLA D */
            SLA(reg_d);
            break;
        case 0x23: /* SLA E */
            SLA(reg_e);
            break;
        case 0x24: /* SLA H */
            SLA(reg_h);
            break;
        case 0x25: /* SLA L */
            SLA(reg_l);
            break;
        case 0x26: /* SLA (HL) */
            SLAXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x27: /* SLA A */
            SLA(reg_a);
            break;
        case 0x28: /* SRA B */
            SRA(reg_b);
            break;
        case 0x29: /* SRA C */
            SRA(reg_c);
            break;
        case 0x2a: /* SRA D */
            SRA(reg_d);
            break;
        case 0x2b: /* SRA E */
            SRA(reg_e);
            break;
        case 0x2c: /* SRA H */
            SRA(reg_h);
            break;
        case 0x2d: /* SRA L */
            SRA(reg_l);
            break;
        case 0x2e: /* SRA (HL) */
            SRAXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x2f: /* SRA A */
            SRA(reg_a);
            break;
        case 0x30: /* SLL B */
            SLL(reg_b);
            break;
        case 0x31: /* SLL C */
            SLL(reg_c);
            break;
        case 0x32: /* SLL D */
            SLL(reg_d);
            break;
        case 0x33: /* SLL E */
            SLL(reg_e);
            break;
        case 0x34: /* SLL H */
            SLL(reg_h);
            break;
        case 0x35: /* SLL L */
            SLL(reg_l);
            break;
        case 0x36: /* SLL (HL) */
            SLLXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x37: /* SLL A */
            SLL(reg_a);
            break;
        case 0x38: /* SRL B */
            SRL(reg_b);
            break;
        case 0x39: /* SRL C */
            SRL(reg_c);
            break;
        case 0x3a: /* SRL D */
            SRL(reg_d);
            break;
        case 0x3b: /* SRL E */
            SRL(reg_e);
            break;
        case 0x3c: /* SRL H */
            SRL(reg_h);
            break;
        case 0x3d: /* SRL L */
            SRL(reg_l);
            break;
        case 0x3e: /* SRL (HL) */
            SRLXX(HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x3f: /* SRL A */
            SRL(reg_a);
            break;
        case 0x40: /* BIT B 0 */
            BIT(reg_b, 0, 0, 8, 2);
            break;
        case 0x41: /* BIT C 0 */
            BIT(reg_c, 0, 0, 8, 2);
            break;
        case 0x42: /* BIT D 0 */
            BIT(reg_d, 0, 0, 8, 2);
            break;
        case 0x43: /* BIT E 0 */
            BIT(reg_e, 0, 0, 8, 2);
            break;
        case 0x44: /* BIT H 0 */
            BIT(reg_h, 0, 0, 8, 2);
            break;
        case 0x45: /* BIT L 0 */
            BIT(reg_l, 0, 0, 8, 2);
            break;
        case 0x46: /* BIT (HL) 0 */
            BIT16(LOAD(HL_WORD()), 0, 4, 8, 2);
            break;
        case 0x47: /* BIT A 0 */
            BIT(reg_a, 0, 0, 8, 2);
            break;
        case 0x48: /* BIT B 1 */
            BIT(reg_b, 1, 0, 8, 2);
            break;
        case 0x49: /* BIT C 1 */
            BIT(reg_c, 1, 0, 8, 2);
            break;
        case 0x4a: /* BIT D 1 */
            BIT(reg_d, 1, 0, 8, 2);
            break;
        case 0x4b: /* BIT E 1 */
            BIT(reg_e, 1, 0, 8, 2);
            break;
        case 0x4c: /* BIT H 1 */
            BIT(reg_h, 1, 0, 8, 2);
            break;
        case 0x4d: /* BIT L 1 */
            BIT(reg_l, 1, 0, 8, 2);
            break;
        case 0x4e: /* BIT (HL) 1 */
            BIT16(LOAD(HL_WORD()), 1, 4, 8, 2);
            break;
        case 0x4f: /* BIT A 1 */
            BIT(reg_a, 1, 0, 8, 2);
            break;
        case 0x50: /* BIT B 2 */
            BIT(reg_b, 2, 0, 8, 2);
            break;
        case 0x51: /* BIT C 2 */
            BIT(reg_c, 2, 0, 8, 2);
            break;
        case 0x52: /* BIT D 2 */
            BIT(reg_d, 2, 0, 8, 2);
            break;
        case 0x53: /* BIT E 2 */
            BIT(reg_e, 2, 0, 8, 2);
            break;
        case 0x54: /* BIT H 2 */
            BIT(reg_h, 2, 0, 8, 2);
            break;
        case 0x55: /* BIT L 2 */
            BIT(reg_l, 2, 0, 8, 2);
            break;
        case 0x56: /* BIT (HL) 2 */
            BIT16(LOAD(HL_WORD()), 2, 4, 8, 2);
            break;
        case 0x57: /* BIT A 2 */
            BIT(reg_a, 2, 0, 8, 2);
            break;
        case 0x58: /* BIT B 3 */
            BIT(reg_b, 3, 0, 8, 2);
            break;
        case 0x59: /* BIT C 3 */
            BIT(reg_c, 3, 0, 8, 2);
            break;
        case 0x5a: /* BIT D 3 */
            BIT(reg_d, 3, 0, 8, 2);
            break;
        case 0x5b: /* BIT E 3 */
            BIT(reg_e, 3, 0, 8, 2);
            break;
        case 0x5c: /* BIT H 3 */
            BIT(reg_h, 3, 0, 8, 2);
            break;
        case 0x5d: /* BIT L 3 */
            BIT(reg_l, 3, 0, 8, 2);
            break;
        case 0x5e: /* BIT (HL) 3 */
            BIT16(LOAD(HL_WORD()), 3, 4, 8, 2);
            break;
        case 0x5f: /* BIT A 3 */
            BIT(reg_a, 3, 0, 8, 2);
            break;
        case 0x60: /* BIT B 4 */
            BIT(reg_b, 4, 0, 8, 2);
            break;
        case 0x61: /* BIT C 4 */
            BIT(reg_c, 4, 0, 8, 2);
            break;
        case 0x62: /* BIT D 4 */
            BIT(reg_d, 4, 0, 8, 2);
            break;
        case 0x63: /* BIT E 4 */
            BIT(reg_e, 4, 0, 8, 2);
            break;
        case 0x64: /* BIT H 4 */
            BIT(reg_h, 4, 0, 8, 2);
            break;
        case 0x65: /* BIT L 4 */
            BIT(reg_l, 4, 0, 8, 2);
            break;
        case 0x66: /* BIT (HL) 4 */
            BIT16(LOAD(HL_WORD()), 4, 4, 8, 2);
            break;
        case 0x67: /* BIT A 4 */
            BIT(reg_a, 4, 0, 8, 2);
            break;
        case 0x68: /* BIT B 5 */
            BIT(reg_b, 5, 0, 8, 2);
            break;
        case 0x69: /* BIT C 5 */
            BIT(reg_c, 5, 0, 8, 2);
            break;
        case 0x6a: /* BIT D 5 */
            BIT(reg_d, 5, 0, 8, 2);
            break;
        case 0x6b: /* BIT E 5 */
            BIT(reg_e, 5, 0, 8, 2);
            break;
        case 0x6c: /* BIT H 5 */
            BIT(reg_h, 5, 0, 8, 2);
            break;
        case 0x6d: /* BIT L 5 */
            BIT(reg_l, 5, 0, 8, 2);
            break;
        case 0x6e: /* BIT (HL) 5 */
            BIT16(LOAD(HL_WORD()), 5, 4, 8, 2);
            break;
        case 0x6f: /* BIT A 5 */
            BIT(reg_a, 5, 0, 8, 2);
            break;
        case 0x70: /* BIT B 6 */
            BIT(reg_b, 6, 0, 8, 2);
            break;
        case 0x71: /* BIT C 6 */
            BIT(reg_c, 6, 0, 8, 2);
            break;
        case 0x72: /* BIT D 6 */
            BIT(reg_d, 6, 0, 8, 2);
            break;
        case 0x73: /* BIT E 6 */
            BIT(reg_e, 6, 0, 8, 2);
            break;
        case 0x74: /* BIT H 6 */
            BIT(reg_h, 6, 0, 8, 2);
            break;
        case 0x75: /* BIT L 6 */
            BIT(reg_l, 6, 0, 8, 2);
            break;
        case 0x76: /* BIT (HL) 6 */
            BIT16(LOAD(HL_WORD()), 6, 4, 8, 2);
            break;
        case 0x77: /* BIT A 6 */
            BIT(reg_a, 6, 0, 8, 2);
            break;
        case 0x78: /* BIT B 7 */
            BIT(reg_b, 7, 0, 8, 2);
            break;
        case 0x79: /* BIT C 7 */
            BIT(reg_c, 7, 0, 8, 2);
            break;
        case 0x7a: /* BIT D 7 */
            BIT(reg_d, 7, 0, 8, 2);
            break;
        case 0x7b: /* BIT E 7 */
            BIT(reg_e, 7, 0, 8, 2);
            break;
        case 0x7c: /* BIT H 7 */
            BIT(reg_h, 7, 0, 8, 2);
            break;
        case 0x7d: /* BIT L 7 */
            BIT(reg_l, 7, 0, 8, 2);
            break;
        case 0x7e: /* BIT (HL) 7 */
            BIT16(LOAD(HL_WORD()), 7, 4, 8, 2);
            break;
        case 0x7f: /* BIT A 7 */
            BIT(reg_a, 7, 0, 8, 2);
            break;
        case 0x80: /* RES B 0 */
            RES(reg_b, 0);
            break;
        case 0x81: /* RES C 0 */
            RES(reg_c, 0);
            break;
        case 0x82: /* RES D 0 */
            RES(reg_d, 0);
            break;
        case 0x83: /* RES E 0 */
            RES(reg_e, 0);
            break;
        case 0x84: /* RES H 0 */
            RES(reg_h, 0);
            break;
        case 0x85: /* RES L 0 */
            RES(reg_l, 0);
            break;
        case 0x86: /* RES (HL) 0 */
            RESXX(0, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x87: /* RES A 0 */
            RES(reg_a, 0);
            break;
        case 0x88: /* RES B 1 */
            RES(reg_b, 1);
            break;
        case 0x89: /* RES C 1 */
            RES(reg_c, 1);
            break;
        case 0x8a: /* RES D 1 */
            RES(reg_d, 1);
            break;
        case 0x8b: /* RES E 1 */
            RES(reg_e, 1);
            break;
        case 0x8c: /* RES H 1 */
            RES(reg_h, 1);
            break;
        case 0x8d: /* RES L 1 */
            RES(reg_l, 1);
            break;
        case 0x8e: /* RES (HL) 1 */
            RESXX(1, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x8f: /* RES A 1 */
            RES(reg_a, 1);
            break;
        case 0x90: /* RES B 2 */
            RES(reg_b, 2);
            break;
        case 0x91: /* RES C 2 */
            RES(reg_c, 2);
            break;
        case 0x92: /* RES D 2 */
            RES(reg_d, 2);
            break;
        case 0x93: /* RES E 2 */
            RES(reg_e, 2);
            break;
        case 0x94: /* RES H 2 */
            RES(reg_h, 2);
            break;
        case 0x95: /* RES L 2 */
            RES(reg_l, 2);
            break;
        case 0x96: /* RES (HL) 2 */
            RESXX(2, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x97: /* RES A 2 */
            RES(reg_a, 2);
            break;
        case 0x98: /* RES B 3 */
            RES(reg_b, 3);
            break;
        case 0x99: /* RES C 3 */
            RES(reg_c, 3);
            break;
        case 0x9a: /* RES D 3 */
            RES(reg_d, 3);
            break;
        case 0x9b: /* RES E 3 */
            RES(reg_e, 3);
            break;
        case 0x9c: /* RES H 3 */
            RES(reg_h, 3);
            break;
        case 0x9d: /* RES L 3 */
            RES(reg_l, 3);
            break;
        case 0x9e: /* RES (HL) 3 */
            RESXX(3, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0x9f: /* RES A 3 */
            RES(reg_a, 3);
            break;
        case 0xa0: /* RES B 4 */
            RES(reg_b, 4);
            break;
        case 0xa1: /* RES C 4 */
            RES(reg_c, 4);
            break;
        case 0xa2: /* RES D 4 */
            RES(reg_d, 4);
            break;
        case 0xa3: /* RES E 4 */
            RES(reg_e, 4);
            break;
        case 0xa4: /* RES H 4 */
            RES(reg_h, 4);
            break;
        case 0xa5: /* RES L 4 */
            RES(reg_l, 4);
            break;
        case 0xa6: /* RES (HL) 4 */
            RESXX(4, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xa7: /* RES A 4 */
            RES(reg_a, 4);
            break;
        case 0xa8: /* RES B 5 */
            RES(reg_b, 5);
            break;
        case 0xa9: /* RES C 5 */
            RES(reg_c, 5);
            break;
        case 0xaa: /* RES D 5 */
            RES(reg_d, 5);
            break;
        case 0xab: /* RES E 5 */
            RES(reg_e, 5);
            break;
        case 0xac: /* RES H 5 */
            RES(reg_h, 5);
            break;
        case 0xad: /* RES L 5 */
            RES(reg_l, 5);
            break;
        case 0xae: /* RES (HL) 5 */
            RESXX(5, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xaf: /* RES A 5 */
            RES(reg_a, 5);
            break;
        case 0xb0: /* RES B 6 */
            RES(reg_b, 6);
            break;
        case 0xb1: /* RES C 6 */
            RES(reg_c, 6);
            break;
        case 0xb2: /* RES D 6 */
            RES(reg_d, 6);
            break;
        case 0xb3: /* RES E 6 */
            RES(reg_e, 6);
            break;
        case 0xb4: /* RES H 6 */
            RES(reg_h, 6);
            break;
        case 0xb5: /* RES L 6 */
            RES(reg_l, 6);
            break;
        case 0xb6: /* RES (HL) 6 */
            RESXX(6, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xb7: /* RES A 6 */
            RES(reg_a, 6);
            break;
        case 0xb8: /* RES B 7 */
            RES(reg_b, 7);
            break;
        case 0xb9: /* RES C 7 */
            RES(reg_c, 7);
            break;
        case 0xba: /* RES D 7 */
            RES(reg_d, 7);
            break;
        case 0xbb: /* RES E 7 */
            RES(reg_e, 7);
            break;
        case 0xbc: /* RES H 7 */
            RES(reg_h, 7);
            break;
        case 0xbd: /* RES L 7 */
            RES(reg_l, 7);
            break;
        case 0xbe: /* RES (HL) 7 */
            RESXX(7, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xbf: /* RES A 7 */
            RES(reg_a, 7);
            break;
        case 0xc0: /* SET B 0 */
            SET(reg_b, 0);
            break;
        case 0xc1: /* SET C 0 */
            SET(reg_c, 0);
            break;
        case 0xc2: /* SET D 0 */
            SET(reg_d, 0);
            break;
        case 0xc3: /* SET E 0 */
            SET(reg_e, 0);
            break;
        case 0xc4: /* SET H 0 */
            SET(reg_h, 0);
            break;
        case 0xc5: /* SET L 0 */
            SET(reg_l, 0);
            break;
        case 0xc6: /* SET (HL) 0 */
            SETXX(0, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xc7: /* SET A 0 */
            SET(reg_a, 0);
            break;
        case 0xc8: /* SET B 1 */
            SET(reg_b, 1);
            break;
        case 0xc9: /* SET C 1 */
            SET(reg_c, 1);
            break;
        case 0xca: /* SET D 1 */
            SET(reg_d, 1);
            break;
        case 0xcb: /* SET E 1 */
            SET(reg_e, 1);
            break;
        case 0xcc: /* SET H 1 */
            SET(reg_h, 1);
            break;
        case 0xcd: /* SET L 1 */
            SET(reg_l, 1);
            break;
        case 0xce: /* SET (HL) 1 */
            SETXX(1, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xcf: /* SET A 1 */
            SET(reg_a, 1);
            break;
        case 0xd0: /* SET B 2 */
            SET(reg_b, 2);
            break;
        case 0xd1: /* SET C 2 */
            SET(reg_c, 2);
            break;
        case 0xd2: /* SET D 2 */
            SET(reg_d, 2);
            break;
        case 0xd3: /* SET E 2 */
            SET(reg_e, 2);
            break;
        case 0xd4: /* SET H 2 */
            SET(reg_h, 2);
            break;
        case 0xd5: /* SET L 2 */
            SET(reg_l, 2);
            break;
        case 0xd6: /* SET (HL) 2 */
            SETXX(2, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xd7: /* SET A 2 */
            SET(reg_a, 2);
            break;
        case 0xd8: /* SET B 3 */
            SET(reg_b, 3);
            break;
        case 0xd9: /* SET C 3 */
            SET(reg_c, 3);
            break;
        case 0xda: /* SET D 3 */
            SET(reg_d, 3);
            break;
        case 0xdb: /* SET E 3 */
            SET(reg_e, 3);
            break;
        case 0xdc: /* SET H 3 */
            SET(reg_h, 3);
            break;
        case 0xdd: /* SET L 3 */
            SET(reg_l, 3);
            break;
        case 0xde: /* SET (HL) 3 */
            SETXX(3, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xdf: /* SET A 3 */
            SET(reg_a, 3);
            break;
        case 0xe0: /* SET B 4 */
            SET(reg_b, 4);
            break;
        case 0xe1: /* SET C 4 */
            SET(reg_c, 4);
            break;
        case 0xe2: /* SET D 4 */
            SET(reg_d, 4);
            break;
        case 0xe3: /* SET E 4 */
            SET(reg_e, 4);
            break;
        case 0xe4: /* SET H 4 */
            SET(reg_h, 4);
            break;
        case 0xe5: /* SET L 4 */
            SET(reg_l, 4);
            break;
        case 0xe6: /* SET (HL) 4 */
            SETXX(4, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xe7: /* SET A 4 */
            SET(reg_a, 4);
            break;
        case 0xe8: /* SET B 5 */
            SET(reg_b, 5);
            break;
        case 0xe9: /* SET C 5 */
            SET(reg_c, 5);
            break;
        case 0xea: /* SET D 5 */
            SET(reg_d, 5);
            break;
        case 0xeb: /* SET E 5 */
            SET(reg_e, 5);
            break;
        case 0xec: /* SET H 5 */
            SET(reg_h, 5);
            break;
        case 0xed: /* SET L 5 */
            SET(reg_l, 5);
            break;
        case 0xee: /* SET (HL) 5 */
            SETXX(5, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xef: /* SET A 5 */
            SET(reg_a, 5);
            break;
        case 0xf0: /* SET B 6 */
            SET(reg_b, 6);
            break;
        case 0xf1: /* SET C 6 */
            SET(reg_c, 6);
            break;
        case 0xf2: /* SET D 6 */
            SET(reg_d, 6);
            break;
        case 0xf3: /* SET E 6 */
            SET(reg_e, 6);
            break;
        case 0xf4: /* SET H 6 */
            SET(reg_h, 6);
            break;
        case 0xf5: /* SET L 6 */
            SET(reg_l, 6);
            break;
        case 0xf6: /* SET (HL) 6 */
            SETXX(6, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xf7: /* SET A 6 */
            SET(reg_a, 6);
            break;
        case 0xf8: /* SET B 7 */
            SET(reg_b, 7);
            break;
        case 0xf9: /* SET C 7 */
            SET(reg_c, 7);
            break;
        case 0xfa: /* SET D 7 */
            SET(reg_d, 7);
            break;
        case 0xfb: /* SET E 7 */
            SET(reg_e, 7);
            break;
        case 0xfc: /* SET H 7 */
            SET(reg_h, 7);
            break;
        case 0xfd: /* SET L 7 */
            SET(reg_l, 7);
            break;
        case 0xfe: /* SET (HL) 7 */
            SETXX(7, HL_WORD(), 4, 4, 7, 2);
            break;
        case 0xff: /* SET A 7 */
            SET(reg_a, 7);
            break;
        default:
            INC_PC(2);
    }
}

#undef INSTS
#define INSTS(IX,IY)         \
    do {                     \
        switch (inst_mode) { \
            case INST_IX:    \
                IX;          \
                break;       \
            case INST_IY:    \
                IY;          \
                break;       \
            default:         \
                break;       \
        }                    \
    } while (0)

#define INSTS_SH(func, XY, cyc, inc) \
    INSTS(func(IX_##XY, 0, 0, cyc, inc), \
          func(IY_##XY, 0, 0, cyc, inc));

#define INSTS_SHREG(func, reg, XY, cyc, inc) \
    INSTS(func(reg, IX_##XY, 0, 0, cyc, inc), \
          func(reg, IY_##XY, 0 ,0, cyc, inc));

#define INSTS_RS(func, bit, XY, cyc, inc) \
    INSTS(func(bit, IX_##XY, 0, 0, cyc, inc), \
          func(bit, IY_##XY, 0, 0, cyc, inc));

#define INSTS_RSREG(func, bit, reg, XY, cyc, inc) \
    INSTS(func(bit, reg, IX_##XY, 0, 0, cyc, inc), \
          func(bit, reg, IY_##XY, 0, 0, cyc, inc));

#define INSTS_BIT(XY, bit, cyc, inc) \
    INSTS(BIT16(LOAD(IX_##XY), bit, 0, cyc, inc), \
          BIT16(LOAD(IY_##XY), bit, 0, cyc, inc));

/* use macros to reduce source code and make this more managable. */
/* "Ir" is either IX or IY. The macros above do the code expansion to
   handle both the IX and IY cases. */
static void opcode_ddfd_cb(uint8_t iip2, uint8_t iip3, uint16_t iip23)
{
    /* effecitively read 0xcb */
    CLK_ADD(CLK, 4);
    INC_PC(1);
    switch (iip3) {
        case 0x00: /* RLC (Ir+d),B */
            INSTS_SHREG(RLCXXREG, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x01: /* RLC (Ir+d),C */
            INSTS_SHREG(RLCXXREG, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x02: /* RLC (Ir+d),D */
            INSTS_SHREG(RLCXXREG, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x03: /* RLC (Ir+d),E */
            INSTS_SHREG(RLCXXREG, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x04: /* RLC (Ir+d),H */
            INSTS_SHREG(RLCXXREG, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x05: /* RLC (Ir+d),L */
            INSTS_SHREG(RLCXXREG, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x06: /* RLC (Ir+d) */
            INSTS_SH(RLCXX, WORD_OFF(iip2), 15, 2);
            break;
        case 0x07: /* RLC (Ir+d),A */
            INSTS_SHREG(RLCXXREG, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x08: /* RRC (Ir+d),B */
            INSTS_SHREG(RRCXXREG, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x09: /* RRC (Ir+d),C */
            INSTS_SHREG(RRCXXREG, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x0a: /* RRC (Ir+d),D */
            INSTS_SHREG(RRCXXREG, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x0b: /* RRC (Ir+d),E */
            INSTS_SHREG(RRCXXREG, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x0c: /* RRC (Ir+d),H */
            INSTS_SHREG(RRCXXREG, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x0d: /* RRC (Ir+d),L */
            INSTS_SHREG(RRCXXREG, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x0e: /* RRC (Ir+d) */
            INSTS_SH(RRCXX, WORD_OFF(iip2), 15, 2);
            break;
        case 0x0f: /* RRC (Ir+d),A */
            INSTS_SHREG(RRCXXREG, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x10: /* RL (Ir+d),B */
            INSTS_SHREG(RLXXREG, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x11: /* RL (Ir+d),C */
            INSTS_SHREG(RLXXREG, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x12: /* RL (Ir+d),D */
            INSTS_SHREG(RLXXREG, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x13: /* RL (Ir+d),E */
            INSTS_SHREG(RLXXREG, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x14: /* RL (Ir+d),H */
            INSTS_SHREG(RLXXREG, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x15: /* RL (Ir+d),L */
            INSTS_SHREG(RLXXREG, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x16: /* RL (Ir+d) */
            INSTS_SH(RLXX, WORD_OFF(iip2), 15, 2);
            break;
        case 0x17: /* RL (Ir+d),A */
            INSTS_SHREG(RLXXREG, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x18: /* RR (Ir+d),B */
            INSTS_SHREG(RRXXREG, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x19: /* RR (Ir+d),C */
            INSTS_SHREG(RRXXREG, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x1a: /* RR (Ir+d),D */
            INSTS_SHREG(RRXXREG, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x1b: /* RR (Ir+d),E */
            INSTS_SHREG(RRXXREG, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x1c: /* RR (Ir+d),H */
            INSTS_SHREG(RRXXREG, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x1d: /* RR (Ir+d),L */
            INSTS_SHREG(RRXXREG, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x1e: /* RR (Ir+d) */
            INSTS_SH(RRXX, WORD_OFF(iip2), 15, 2);
            break;
        case 0x1f: /* RR (Ir+d),A */
            INSTS_SHREG(RRXXREG, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x20: /* SLA (Ir+d),B */
            INSTS_SHREG(SLAXXREG, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x21: /* SLA (Ir+d),C */
            INSTS_SHREG(SLAXXREG, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x22: /* SLA (Ir+d),D */
            INSTS_SHREG(SLAXXREG, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x23: /* SLA (Ir+d),E */
            INSTS_SHREG(SLAXXREG, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x24: /* SLA (Ir+d),H */
            INSTS_SHREG(SLAXXREG, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x25: /* SLA (Ir+d),L */
            INSTS_SHREG(SLAXXREG, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x26: /* SLA (Ir+d) */
            INSTS_SH(SLAXX, WORD_OFF(iip2), 15, 2);
            break;
        case 0x27: /* SLA (Ir+d),A */
            INSTS_SHREG(SLAXXREG, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x28: /* SRA (Ir+d),B */
            INSTS_SHREG(SRAXXREG, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x29: /* SRA (Ir+d),C */
            INSTS_SHREG(SRAXXREG, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x2a: /* SRA (Ir+d),D */
            INSTS_SHREG(SRAXXREG, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x2b: /* SRA (Ir+d),E */
            INSTS_SHREG(SRAXXREG, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x2c: /* SRA (Ir+d),H */
            INSTS_SHREG(SRAXXREG, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x2d: /* SRA (Ir+d),L */
            INSTS_SHREG(SRAXXREG, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x2e: /* SRA (Ir+d) */
            INSTS_SH(SRAXX, WORD_OFF(iip2), 15, 2);
            break;
        case 0x2f: /* SRA (Ir+d),A */
            INSTS_SHREG(SRAXXREG, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x30: /* SLL (Ir+d),B */
            INSTS_SHREG(SLLXXREG, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x31: /* SLL (Ir+d),C */
            INSTS_SHREG(SLLXXREG, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x32: /* SLL (Ir+d),D */
            INSTS_SHREG(SLLXXREG, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x33: /* SLL (Ir+d),E */
            INSTS_SHREG(SLLXXREG, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x34: /* SLL (Ir+d),H */
            INSTS_SHREG(SLLXXREG, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x35: /* SLL (Ir+d),L */
            INSTS_SHREG(SLLXXREG, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x36: /* SLL (Ir+d) */
            INSTS_SH(SLLXX, WORD_OFF(iip2), 15, 2);
            break;
        case 0x37: /* SLL (Ir+d),A */
            INSTS_SHREG(SLLXXREG, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x38: /* SRL (Ir+d),B */
            INSTS_SHREG(SRLXXREG, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x39: /* SRL (Ir+d),C */
            INSTS_SHREG(SRLXXREG, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x3a: /* SRL (Ir+d),D */
            INSTS_SHREG(SRLXXREG, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x3b: /* SRL (Ir+d),E */
            INSTS_SHREG(SRLXXREG, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x3c: /* SRL (Ir+d),H */
            INSTS_SHREG(SRLXXREG, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x3d: /* SRL (Ir+d),L */
            INSTS_SHREG(SRLXXREG, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x3e: /* SRL (Ir+d) */
            INSTS_SH(SRLXX, WORD_OFF(iip2), 15, 2);
            break;
        case 0x3f: /* SRL (Ir+d),A */
            INSTS_SHREG(SRLXXREG, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x40: /* BIT (Ir+d) 0 */
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
            INSTS_BIT(WORD_OFF(iip2), 0, 12, 2);
            break;
        case 0x48: /* BIT (Ir+d) 1 */
        case 0x49:
        case 0x4a:
        case 0x4b:
        case 0x4c:
        case 0x4d:
        case 0x4e:
        case 0x4f:
            INSTS_BIT(WORD_OFF(iip2), 1, 12, 2);
            break;
        case 0x50: /* BIT (Ir+d) 2 */
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
            INSTS_BIT(WORD_OFF(iip2), 2, 12, 2);
            break;
        case 0x58: /* BIT (Ir+d) 3 */
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x5e:
        case 0x5f:
            INSTS_BIT(WORD_OFF(iip2), 3, 12, 2);
            break;
        case 0x60: /* BIT (Ir+d) 4 */
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
            INSTS_BIT(WORD_OFF(iip2), 4, 12, 2);
            break;
        case 0x68: /* BIT (Ir+d) 5 */
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x6f:
            INSTS_BIT(WORD_OFF(iip2), 5, 12, 2);
            break;
        case 0x70: /* BIT (Ir+d) 6 */
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
            INSTS_BIT(WORD_OFF(iip2), 6, 12, 2);
            break;
        case 0x78: /* BIT (Ir+d) 7 */
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
            INSTS_BIT(WORD_OFF(iip2), 7, 12, 2);
            break;
        case 0x80: /* RES (Ir+d),B 0 */
            INSTS_RSREG(RESXXREG, 0, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x81: /* RES (Ir+d),C 0 */
            INSTS_RSREG(RESXXREG, 0, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x82: /* RES (Ir+d),D 0 */
            INSTS_RSREG(RESXXREG, 0, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x83: /* RES (Ir+d),E 0 */
            INSTS_RSREG(RESXXREG, 0, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x84: /* RES (Ir+d),H 0 */
            INSTS_RSREG(RESXXREG, 0, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x85: /* RES (Ir+d),L 0 */
            INSTS_RSREG(RESXXREG, 0, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x86: /* RES (Ir+d) 0 */
            INSTS_RS(RESXX, 0, WORD_OFF(iip2), 15, 2);
            break;
        case 0x87: /* RES (Ir+d),A 0 */
            INSTS_RSREG(RESXXREG, 0, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x88: /* RES (Ir+d),B 1 */
            INSTS_RSREG(RESXXREG, 1, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x89: /* RES (Ir+d),C 1 */
            INSTS_RSREG(RESXXREG, 1, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x8a: /* RES (Ir+d),D 1 */
            INSTS_RSREG(RESXXREG, 1, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x8b: /* RES (Ir+d),E 1 */
            INSTS_RSREG(RESXXREG, 1, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x8c: /* RES (Ir+d),H 1 */
            INSTS_RSREG(RESXXREG, 1, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x8d: /* RES (Ir+d),L 1 */
            INSTS_RSREG(RESXXREG, 1, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x8e: /* RES (Ir+d) 1 */
            INSTS_RS(RESXX, 1, WORD_OFF(iip2), 15, 2);
            break;
        case 0x8f: /* RES (Ir+d),A 1 */
            INSTS_RSREG(RESXXREG, 1, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x90: /* RES (Ir+d),B 2 */
            INSTS_RSREG(RESXXREG, 2, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x91: /* RES (Ir+d),C 2 */
            INSTS_RSREG(RESXXREG, 2, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x92: /* RES (Ir+d),D 2 */
            INSTS_RSREG(RESXXREG, 2, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x93: /* RES (Ir+d),E 2 */
            INSTS_RSREG(RESXXREG, 2, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x94: /* RES (Ir+d),H 2 */
            INSTS_RSREG(RESXXREG, 2, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x95: /* RES (Ir+d),L 2 */
            INSTS_RSREG(RESXXREG, 2, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x96: /* RES (Ir+d) 2 */
            INSTS_RS(RESXX, 2, WORD_OFF(iip2), 15, 2);
            break;
        case 0x97: /* RES (Ir+d),A 2 */
            INSTS_RSREG(RESXXREG, 2, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0x98: /* RES (Ir+d),B 3 */
            INSTS_RSREG(RESXXREG, 3, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0x99: /* RES (Ir+d),C 3 */
            INSTS_RSREG(RESXXREG, 3, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0x9a: /* RES (Ir+d),D 3 */
            INSTS_RSREG(RESXXREG, 3, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0x9b: /* RES (Ir+d),E 3 */
            INSTS_RSREG(RESXXREG, 3, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0x9c: /* RES (Ir+d),H 3 */
            INSTS_RSREG(RESXXREG, 3, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0x9d: /* RES (Ir+d),L 3 */
            INSTS_RSREG(RESXXREG, 3, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0x9e: /* RES (Ir+d) 3 */
            INSTS_RS(RESXX, 3, WORD_OFF(iip2), 15, 2);
            break;
        case 0x9f: /* RES (Ir+d),A 3 */
            INSTS_RSREG(RESXXREG, 3, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa0: /* RES (Ir+d),B 4 */
            INSTS_RSREG(RESXXREG, 4, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa1: /* RES (Ir+d),C 4 */
            INSTS_RSREG(RESXXREG, 4, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa2: /* RES (Ir+d),D 4 */
            INSTS_RSREG(RESXXREG, 4, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa3: /* RES (Ir+d),E 4 */
            INSTS_RSREG(RESXXREG, 4, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa4: /* RES (Ir+d),H 4 */
            INSTS_RSREG(RESXXREG, 4, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa5: /* RES (Ir+d),L 4 */
            INSTS_RSREG(RESXXREG, 4, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa6: /* RES (Ir+d) 4 */
            INSTS_RS(RESXX, 4, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa7: /* RES (Ir+d),A 4 */
            INSTS_RSREG(RESXXREG, 4, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa8: /* RES (Ir+d),B 5 */
            INSTS_RSREG(RESXXREG, 5, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xa9: /* RES (Ir+d),C 5 */
            INSTS_RSREG(RESXXREG, 5, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xaa: /* RES (Ir+d),D 5 */
            INSTS_RSREG(RESXXREG, 5, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xab: /* RES (Ir+d),E 5 */
            INSTS_RSREG(RESXXREG, 5, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xac: /* RES (Ir+d),H 5 */
            INSTS_RSREG(RESXXREG, 5, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xad: /* RES (Ir+d),L 5 */
            INSTS_RSREG(RESXXREG, 5, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xae: /* RES (Ir+d) 5 */
            INSTS_RS(RESXX, 5, WORD_OFF(iip2), 15, 2);
            break;
        case 0xaf: /* RES (Ir+d),A 5 */
            INSTS_RSREG(RESXXREG, 5, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb0: /* RES (Ir+d),B 6 */
            INSTS_RSREG(RESXXREG, 6, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb1: /* RES (Ir+d),C 6 */
            INSTS_RSREG(RESXXREG, 6, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb2: /* RES (Ir+d),D 6 */
            INSTS_RSREG(RESXXREG, 6, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb3: /* RES (Ir+d),E 6 */
            INSTS_RSREG(RESXXREG, 6, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb4: /* RES (Ir+d),H 6 */
            INSTS_RSREG(RESXXREG, 6, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb5: /* RES (Ir+d),L 6 */
            INSTS_RSREG(RESXXREG, 6, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb6: /* RES (Ir+d) 6 */
            INSTS_RS(RESXX, 6, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb7: /* RES (Ir+d),A 6 */
            INSTS_RSREG(RESXXREG, 6, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb8: /* RES (Ir+d),B 7 */
            INSTS_RSREG(RESXXREG, 7, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xb9: /* RES (Ir+d),C 7 */
            INSTS_RSREG(RESXXREG, 7, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xba: /* RES (Ir+d),D 7 */
            INSTS_RSREG(RESXXREG, 7, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xbb: /* RES (Ir+d),E 7 */
            INSTS_RSREG(RESXXREG, 7, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xbc: /* RES (Ir+d),H 7 */
            INSTS_RSREG(RESXXREG, 7, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xbd: /* RES (Ir+d),L 7 */
            INSTS_RSREG(RESXXREG, 7, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xbe: /* RES (Ir+d) 7 */
            INSTS_RS(RESXX, 7, WORD_OFF(iip2), 15, 2);
            break;
        case 0xbf: /* RES (Ir+d),A 7 */
            INSTS_RSREG(RESXXREG, 7, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc0: /* SET (Ir+d),B 0 */
            INSTS_RSREG(SETXXREG, 0, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc1: /* SET (Ir+d),C 0 */
            INSTS_RSREG(SETXXREG, 0, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc2: /* SET (Ir+d),D 0 */
            INSTS_RSREG(SETXXREG, 0, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc3: /* SET (Ir+d),E 0 */
            INSTS_RSREG(SETXXREG, 0, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc4: /* SET (Ir+d),H 0 */
            INSTS_RSREG(SETXXREG, 0, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc5: /* SET (Ir+d),L 0 */
            INSTS_RSREG(SETXXREG, 0, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc6: /* SET (Ir+d) 0 */
            INSTS_RS(SETXX, 0, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc7: /* SET (Ir+d),A 0 */
            INSTS_RSREG(SETXXREG, 0, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc8: /* SET (Ir+d),B 1 */
            INSTS_RSREG(SETXXREG, 1, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xc9: /* SET (Ir+d),C 1 */
            INSTS_RSREG(SETXXREG, 1, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xca: /* SET (Ir+d),D 1 */
            INSTS_RSREG(SETXXREG, 1, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xcb: /* SET (Ir+d),E 1 */
            INSTS_RSREG(SETXXREG, 1, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xcc: /* SET (Ir+d),H 1 */
            INSTS_RSREG(SETXXREG, 1, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xcd: /* SET (Ir+d),L 1 */
            INSTS_RSREG(SETXXREG, 1, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xce: /* SET (Ir+d) 1 */
            INSTS_RS(SETXX, 1, WORD_OFF(iip2), 15, 2);
            break;
        case 0xcf: /* SET (Ir+d),A 1 */
            INSTS_RSREG(SETXXREG, 1, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd0: /* SET (Ir+d),B 2 */
            INSTS_RSREG(SETXXREG, 2, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd1: /* SET (Ir+d),C 2 */
            INSTS_RSREG(SETXXREG, 2, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd2: /* SET (Ir+d),D 2 */
            INSTS_RSREG(SETXXREG, 2, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd3: /* SET (Ir+d),E 2 */
            INSTS_RSREG(SETXXREG, 2, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd4: /* SET (Ir+d),H 2 */
            INSTS_RSREG(SETXXREG, 2, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd5: /* SET (Ir+d),L 2 */
            INSTS_RSREG(SETXXREG, 2, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd6: /* SET (Ir+d) 2 */
            INSTS_RS(SETXX, 2, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd7: /* SET (Ir+d),A 2 */
            INSTS_RSREG(SETXXREG, 2, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd8: /* SET (Ir+d),B 3 */
            INSTS_RSREG(SETXXREG, 3, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xd9: /* SET (Ir+d),C 3 */
            INSTS_RSREG(SETXXREG, 3, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xda: /* SET (Ir+d),D 3 */
            INSTS_RSREG(SETXXREG, 3, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xdb: /* SET (Ir+d),E 3 */
            INSTS_RSREG(SETXXREG, 3, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xdc: /* SET (Ir+d),H 3 */
            INSTS_RSREG(SETXXREG, 3, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xdd: /* SET (Ir+d),L 3 */
            INSTS_RSREG(SETXXREG, 3, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xde: /* SET (Ir+d) 3 */
            INSTS_RS(SETXX, 3, WORD_OFF(iip2), 15, 2);
            break;
        case 0xdf: /* SET (Ir+d),A 3 */
            INSTS_RSREG(SETXXREG, 3, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe0: /* SET (Ir+d),B 4 */
            INSTS_RSREG(SETXXREG, 4, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe1: /* SET (Ir+d),C 4 */
            INSTS_RSREG(SETXXREG, 4, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe2: /* SET (Ir+d),D 4 */
            INSTS_RSREG(SETXXREG, 4, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe3: /* SET (Ir+d),E 4 */
            INSTS_RSREG(SETXXREG, 4, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe4: /* SET (Ir+d),H 4 */
            INSTS_RSREG(SETXXREG, 4, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe5: /* SET (Ir+d),L 4 */
            INSTS_RSREG(SETXXREG, 4, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe6: /* SET (Ir+d) 4 */
            INSTS_RS(SETXX, 4, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe7: /* SET (Ir+d),A 4 */
            INSTS_RSREG(SETXXREG, 4, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe8: /* SET (Ir+d),B 5 */
            INSTS_RSREG(SETXXREG, 5, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xe9: /* SET (Ir+d),C 5 */
            INSTS_RSREG(SETXXREG, 5, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xea: /* SET (Ir+d),D 5 */
            INSTS_RSREG(SETXXREG, 5, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xeb: /* SET (Ir+d),E 5 */
            INSTS_RSREG(SETXXREG, 5, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xec: /* SET (Ir+d),H 5 */
            INSTS_RSREG(SETXXREG, 5, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xed: /* SET (Ir+d),L 5 */
            INSTS_RSREG(SETXXREG, 5, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xee: /* SET (Ir+d) 5 */
            INSTS_RS(SETXX, 5, WORD_OFF(iip2), 15, 2);
            break;
        case 0xef: /* SET (Ir+d),A 5 */
            INSTS_RSREG(SETXXREG, 5, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf0: /* SET (Ir+d),B 6 */
            INSTS_RSREG(SETXXREG, 6, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf1: /* SET (Ir+d),C 6 */
            INSTS_RSREG(SETXXREG, 6, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf2: /* SET (Ir+d),D 6 */
            INSTS_RSREG(SETXXREG, 6, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf3: /* SET (Ir+d),E 6 */
            INSTS_RSREG(SETXXREG, 6, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf4: /* SET (Ir+d),H 6 */
            INSTS_RSREG(SETXXREG, 6, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf5: /* SET (Ir+d),L 6 */
            INSTS_RSREG(SETXXREG, 6, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf6: /* SET (Ir+d) 6 */
            INSTS_RS(SETXX, 6, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf7: /* SET (Ir+d),A 6 */
            INSTS_RSREG(SETXXREG, 6, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf8: /* SET (Ir+d),B 7 */
            INSTS_RSREG(SETXXREG, 7, reg_b, WORD_OFF(iip2), 15, 2);
            break;
        case 0xf9: /* SET (Ir+d),C 7 */
            INSTS_RSREG(SETXXREG, 7, reg_c, WORD_OFF(iip2), 15, 2);
            break;
        case 0xfa: /* SET (Ir+d),D 7 */
            INSTS_RSREG(SETXXREG, 7, reg_d, WORD_OFF(iip2), 15, 2);
            break;
        case 0xfb: /* SET (Ir+d),E 7 */
            INSTS_RSREG(SETXXREG, 7, reg_e, WORD_OFF(iip2), 15, 2);
            break;
        case 0xfc: /* SET (Ir+d),H 7 */
            INSTS_RSREG(SETXXREG, 7, reg_h, WORD_OFF(iip2), 15, 2);
            break;
        case 0xfd: /* SET (Ir+d),L 7 */
            INSTS_RSREG(SETXXREG, 7, reg_l, WORD_OFF(iip2), 15, 2);
            break;
        case 0xfe: /* SET (Ir+d) 7 */
            INSTS_RS(SETXX, 7, WORD_OFF(iip2), 15, 2);
            break;
        case 0xff: /* SET (Ir+d),A 7 */
            INSTS_RSREG(SETXXREG, 7, reg_a, WORD_OFF(iip2), 15, 2);
            break;
        default:
            /* all 256 instances covered */
            break;
    }
}

static void opcode_ed(uint8_t ip1, uint8_t ip2, uint8_t ip3, uint16_t ip12, uint16_t ip23)
{
    switch (ip1) {
        case 0x40: /* IN B BC */
            INBC(reg_b, 10, 2, 2);
            break;
        case 0x41: /* OUT BC B */
            OUTBC(reg_b, 10, 2, 2);
            break;
        case 0x42: /* SBC HL BC */
            SBCHLREG(reg_b, reg_c);
            break;
        case 0x43: /* LD (WORD) BC */
            STW(ip23, reg_b, reg_c, 4, 13, 3, 4);
            break;
        case 0x44: /* NEG */
            NEG();
            break;
        case 0x45: /* RETN */
            RETNI();
            break;
        case 0x46: /* IM0 */
            IM(0);
            break;
        case 0x47: /* LD I A */
            LDREG(reg_i, reg_a, 6, 3, 2);
            break;
        case 0x48: /* IN C BC */
            INBC(reg_c, 10, 2, 2);
            break;
        case 0x49: /* OUT BC C */
            OUTBC(reg_c, 10, 2, 2);
            break;
        case 0x4a: /* ADC HL BC */
            ADCHLREG(reg_b, reg_c);
            break;
        case 0x4b: /* LD BC (WORD) */
            LDIND(ip23, reg_b, reg_c, 4, 4, 12, 4);
            break;
        case 0x4c: /* undoc NEG */
            NEG();
            break;
        case 0x4d: /* RETI */
            RETNI();
            break;
        case 0x4e: /* undoc IM0 */
            IM(0);
            break;
        case 0x4f: /* LD R A FIXME: Not emulated.  */
            NOP(9, 2);
            break;
        case 0x50: /* IN D BC */
            INBC(reg_d, 10, 2, 2);
            break;
        case 0x51: /* OUT BC D */
            OUTBC(reg_d, 10, 2, 2);
            break;
        case 0x52: /* SBC HL DE */
            SBCHLREG(reg_d, reg_e);
            break;
        case 0x53: /* LD (WORD) DE */
            STW(ip23, reg_d, reg_e, 4, 13, 3, 4);
            break;
        case 0x54: /* undoc NEG */
            NEG();
            break;
        case 0x55: /* undoc RETN */
            RETNI();
            break;
        case 0x56: /* IM1 */
            IM(1);
            break;
        case 0x57: /* LD A I */
            LDAIR(reg_i);
            break;
        case 0x58: /* IN E BC */
            INBC(reg_e, 10, 2, 2);
            break;
        case 0x59: /* OUT BC E */
            OUTBC(reg_e, 10, 2, 2);
            break;
        case 0x5a: /* ADC HL DE */
            ADCHLREG(reg_d, reg_e);
            break;
        case 0x5b: /* LD DE (WORD) */
            LDIND(ip23, reg_d, reg_e, 4, 4, 12, 4);
            break;
        case 0x5c: /* undoc NEG */
            NEG();
            break;
        case 0x5d: /* undoc RETN */
            RETNI();
            break;
        case 0x5e: /* IM2 */
            IM(2);
            break;
        case 0x5f: /* LD A R */
            LDAIR((uint8_t)(CLK & 0xff));
            break;
        case 0x60: /* IN H BC */
            INBC(reg_h, 10, 2, 2);
            break;
        case 0x61: /* OUT BC H */
            OUTBC(reg_h, 10, 2, 2);
            break;
        case 0x62: /* SBC HL HL */
            SBCHLREG(reg_h, reg_l);
            break;
        case 0x63: /* LD (WORD) HL */
            STW(ip23, reg_h, reg_l, 4, 13, 3, 4);
            break;
        case 0x64: /* undoc NEG */
            NEG();
            break;
        case 0x65: /* undoc RETN */
            RETNI();
            break;
        case 0x66: /* undoc IM0 */
            IM(0);
            break;
        case 0x67: /* RRD */
            RRD();
            break;
        case 0x68: /* IN L BC */
            INBC(reg_l, 10, 2, 2);
            break;
        case 0x69: /* OUT BC L */
            OUTBC(reg_l, 10, 2, 2);
            break;
        case 0x6a: /* ADC HL HL */
            ADCHLREG(reg_h, reg_l);
            break;
        case 0x6b: /* LD HL (WORD) */
            LDIND(ip23, reg_h, reg_l, 4, 4, 12, 4);
            break;
        case 0x6c: /* undoc NEG */
            NEG();
            break;
        case 0x6d: /* undoc RETN */
            RETNI();
            break;
        case 0x6e: /* undoc IM0 */
            IM(0);
            break;
        case 0x6f: /* RLD */
            RLD();
            break;
        case 0x70: /* IN F BC */
            INBC0(10, 2, 2);
            break;
        case 0x71: /* OUT BC #0 */
            OUTBC(0, 10, 2, 2);
            break;
        case 0x72: /* SBC HL SP */
            SBCHLSP();
            break;
        case 0x73: /* LD (WORD) SP */
            STSPW(ip23, 4, 13, 3, 4);
            break;
        case 0x74: /* undoc NEG */
            NEG();
            break;
        case 0x75: /* undoc RETN */
            RETNI();
            break;
        case 0x76: /* undoc IM1 */
            IM(1);
            break;
        case 0x77: /* undoc NOP */
            NOP(8, 2);
            break;
        case 0x78: /* IN A BC */
            INBC(reg_a, 10, 2, 2);
            break;
        case 0x79: /* OUT BC A */
            OUTBC(reg_a, 10, 2, 2);
            break;
        case 0x7a: /* ADC HL SP */
            ADCHLSP();
            break;
        case 0x7b: /* LD SP (WORD) */
            LDSPIND(ip23, 4, 4, 12, 4);
            break;
        case 0x7c: /* undoc NEG */
            NEG();
            break;
        case 0x7d: /* undoc RETN */
            RETNI();
            break;
        case 0x7e: /* undoc IM2 */
            IM(2);
            break;
        case 0x7f: /* undoc NOP */
            NOP(8, 2);
            break;
        case 0xa0: /* LDI */
            LDDI(INC_DE_WORD(), INC_HL_WORD());
            break;
        case 0xa1: /* CPI */
            CPDI(INC_HL_WORD(),reg_wz++);
            break;
        case 0xa2: /* INI */
            CLK_ADD(CLK, 4);
            INDI(INC_HL_WORD());
            break;
        case 0xa3: /* OUTI */
            OUTDI(INC_HL_WORD());
            break;
        case 0xa8: /* LDD */
            LDDI(DEC_DE_WORD(), DEC_HL_WORD());
            break;
        case 0xa9: /* CPD */
            CPDI(DEC_HL_WORD(),reg_wz--);
            break;
        case 0xaa: /* IND */
            CLK_ADD(CLK, 4);
            INDI(DEC_HL_WORD());
            break;
        case 0xab: /* OUTD */
            OUTDI(DEC_HL_WORD());
            break;
        case 0xb0: /* LDIR */
            LDDIR(INC_DE_WORD(), INC_HL_WORD());
            break;
        case 0xb1: /* CPIR */
            CPDIR(INC_HL_WORD());
            break;
        case 0xb2: /* INIR */
            INDIR(INC_HL_WORD());
            break;
        case 0xb3: /* OTIR */
            OTDIR(INC_HL_WORD());
            break;
        case 0xb8: /* LDDR */
            LDDIR(DEC_DE_WORD(), DEC_HL_WORD());
            break;
        case 0xb9: /* CPDR */
            CPDIR(DEC_HL_WORD());
            break;
        case 0xba: /* INDR */
            INDIR(DEC_HL_WORD());
            break;
        case 0xbb: /* OTDR */
            OTDIR(DEC_HL_WORD());
            break;
        case 0xcb: /* NOP */
            NOP(8, 2);
            break;
        case 0xdd: /* NOP */
            NOP(8, 2);
            break;
        case 0xed: /* NOP */
            NOP(8, 2);
            break;
        case 0xfd: /* NOP */
            NOP(8, 2);
            break;
        default:
            NOP(8, 2);
#ifdef DEBUG_Z80
            log_message(LOG_DEFAULT,
                        "%i PC %04x A%02x F%02x B%02x C%02x D%02x E%02x H%02x L%02x SP%04x OP ED %02x %02x %02x.",
                        (int)(CLK), (unsigned int)(z80_reg_pc), reg_a, reg_f, reg_b, reg_c, reg_d, reg_e, reg_h, reg_l, reg_sp, ip1, ip2, ip3);
#endif
    }
}

/* ------------------------------------------------------------------------- */

/* Z80 mainloop.  */
#undef INSTS
#define INSTS(INONE,IX,IY)   \
    do {                     \
        switch (inst_mode) { \
            case INST_IX:    \
                IX;          \
                break;       \
            case INST_IY:    \
                IY;          \
                break;       \
            default:         \
                INONE;       \
                break;       \
        }                    \
    } while (0)

/* use macros to reduce source code and make this more managable. */
/* INSTS macro handles the base, 0xdd, and 0xfd opcodes. */
/* When a 0xdd or 0xfd is encountered, inst_mode is set and the "next"
   instruction is fetched but sets the proper addressing. */
/* This lets us cover the undocuemented 0xdd and 0xfd opcodes easier too. */
static void z80_maincpu_loop(interrupt_cpu_status_t *cpu_int_status, alarm_context_t *cpu_alarm_context)
{
    opcode_t opcode;

    import_registers();

    Z80_SET_DMA_REQUEST(0)

    do {
        while (CLK >= alarm_context_next_pending_clk(cpu_alarm_context)) {
            alarm_context_dispatch(cpu_alarm_context, CLK);
        }
        {
            enum cpu_int pending_interrupt;

            pending_interrupt = cpu_int_status->global_pending_int;
            if (pending_interrupt != IK_NONE) {
                DO_INTERRUPT(pending_interrupt);
                while (CLK >= alarm_context_next_pending_clk(cpu_alarm_context)) {
                    alarm_context_dispatch(cpu_alarm_context, CLK);
                }
            }
        }

        /* delay IFFs 2 instructions */
        iff1 = iff1_1;
        iff2 = iff2_1;
        iff1_1 = iff1_2;
        iff2_1 = iff2_2;

        if (halt) {
            CLK_ADD(CLK, 4);
            cpu_int_status->num_dma_per_opcode = 0;
            continue;
        }

        SET_LAST_ADDR(z80_reg_pc);
fetchmore:
        FETCH_OPCODE(opcode);

#ifdef DEBUG
        if (debug.maincpu_traceflg) {
            log_message(LOG_DEFAULT,
                        ".%04x %i %-25s A%02x F%02x B%02x C%02x D%02x E%02x H%02x L%02x S%04x",
                        (unsigned int)z80_reg_pc, 0, mon_disassemble_to_string(e_comp_space, z80_reg_pc, p0, p1, p2, p3, 1, "z80"),
                        reg_a, reg_f, reg_b, reg_c, reg_d, reg_e, reg_h, reg_l, reg_sp);
        }
#endif

        SET_LAST_OPCODE(p0);

        switch (p0) {
            case 0x00: /* NOP */
                NOP(4, 1);
                break;
            case 0x01: /* LD BC # */
                LDW(p12, reg_b, reg_c, 10, 0, 3);
                break;
            case 0x02: /* LD (BC) A */
                STREGWZ(BC_WORD(), reg_a, 4, 3, 1);
                break;
            case 0x03: /* INC BC */
                DECINC(INC_BC_WORD(), 6, 1);
                break;
            case 0x04: /* INC B */
                INCREG(reg_b, 4, 1);
                break;
            case 0x05: /* DEC B */
                DECREG(reg_b, 4, 1);
                break;
            case 0x06: /* LD B # */
                LDREG(reg_b, p1, 4, 3, 2);
                break;
            case 0x07: /* RLCA */
                RLCA(4, 1);
                break;
            case 0x08: /* EX AF AF' */
                EXAFAF(4, 1);
                break;
            case 0x09: /* ADD HL BC; ADD IX BC; ADD IY BC */
                INSTS(ADDXXREG(reg_h, reg_l, reg_b, reg_c, 11, 1),
                      ADDXXREG(reg_ixh, reg_ixl, reg_b, reg_c, 11, 1),
                      ADDXXREG(reg_iyh, reg_iyl, reg_b, reg_c, 11, 1));
                break;
            case 0x0a: /* LD A (BC) */
                LDREGWZ(reg_a, BC_WORD(), 4, 3, 1);
                break;
            case 0x0b: /* DEC BC */
                DECINC(DEC_BC_WORD(), 6, 1);
                break;
            case 0x0c: /* INC C */
                INCREG(reg_c, 4, 1);
                break;
            case 0x0d: /* DEC C */
                DECREG(reg_c, 4, 1);
                break;
            case 0x0e: /* LD C # */
                LDREG(reg_c, p1, 4, 3, 2);
                break;
            case 0x0f: /* RRCA */
                RRCA(4, 1);
                break;
            case 0x10: /* DJNZ */
                DJNZ(p1, 2);
                break;
            case 0x11: /* LD DE # */
                LDW(p12, reg_d, reg_e, 10, 0, 3);
                break;
            case 0x12: /* LD (DE) A */
                STREGWZ(DE_WORD(), reg_a, 4, 3, 1);
                break;
            case 0x13: /* INC DE */
                DECINC(INC_DE_WORD(), 6, 1);
                break;
            case 0x14: /* INC D */
                INCREG(reg_d, 4, 1);
                break;
            case 0x15: /* DEC D */
                DECREG(reg_d, 4, 1);
                break;
            case 0x16: /* LD D # */
                LDREG(reg_d, p1, 4, 3, 2);
                break;
            case 0x17: /* RLA */
                RLA(4, 1);
                break;
            case 0x18: /* JR */
                BRANCH(1, p1, 2);
                break;
            case 0x19: /* ADD HL DE; ADD IX DE; ADD IY DE */
                INSTS(ADDXXREG(reg_h, reg_l, reg_d, reg_e, 11, 1),
                      ADDXXREG(reg_ixh, reg_ixl, reg_d, reg_e, 11, 1),
                      ADDXXREG(reg_iyh, reg_iyl, reg_d, reg_e, 11, 1));
                break;
            case 0x1a: /* LD A (DE) */
                LDREG(reg_a, LOAD(DE_WORD()), 4, 3, 1);
                break;
            case 0x1b: /* DEC DE */
                DECINC(DEC_DE_WORD(), 6, 1);
                break;
            case 0x1c: /* INC E */
                INCREG(reg_e, 4, 1);
                break;
            case 0x1d: /* DEC E */
                DECREG(reg_e, 4, 1);
                break;
            case 0x1e: /* LD E # */
                LDREG(reg_e, p1, 4, 3, 2);
                break;
            case 0x1f: /* RRA */
                RRA(4, 1);
                break;
            case 0x20: /* JR NZ */
                BRANCH(!LOCAL_ZERO(), p1, 2);
                break;
            case 0x21: /* LD HL # ; LD IX #; LD IY # */
                INSTS(LDW(p12, reg_h, reg_l, 10, 0, 3),
                      LDW(p12, reg_ixh, reg_ixl, 10, 0, 3),
                      LDW(p12, reg_iyh, reg_iyl, 10, 0, 3));
                break;
            case 0x22: /* LD (WORD) HL; LD (WORD) IX; LD (WORD) IY */
                INSTS(STW(p12, reg_h, reg_l, 4, 9, 3, 3),
                      STW(p12, reg_ixh, reg_ixl, 4, 9, 3, 3),
                      STW(p12, reg_iyh, reg_iyl, 4, 9, 3, 3));
                break;
            case 0x23: /* INC HL ; INC IX; INC IY */
                INSTS(DECINC(INC_HL_WORD(), 6, 1),
                      DECINC(INC_IX_WORD(), 6, 1),
                      DECINC(INC_IY_WORD(), 6, 1));
                break;
            case 0x24: /* INC H ; INC IXH ; INC IYH */
                INSTS(INCREG(reg_h, 4, 1),
                      INCREG(reg_ixh, 4, 1),
                      INCREG(reg_iyh, 4, 1));
                break;
            case 0x25: /* DEC H ; DEC IXH ; DEC IYH */
                INSTS(DECREG(reg_h, 4, 1),
                      DECREG(reg_ixh, 4, 1),
                      DECREG(reg_iyh, 4, 1));
                break;
            case 0x26: /* LD H # ; LD IXH # ; LD IYH # */
                INSTS(LDREG(reg_h, p1, 4, 3, 2),
                      LDREG(reg_ixh, p1, 4, 3, 2),
                      LDREG(reg_iyh, p1, 4, 3, 2));
                break;
            case 0x27: /* DAA */
                DAA(4, 1);
                break;
            case 0x28: /* JR Z */
                BRANCH(LOCAL_ZERO(), p1, 2);
                break;
            case 0x29: /* ADD HL HL ; ADD IX IX ; ADD IY IY */
                INSTS(ADDXXREG(reg_h, reg_l, reg_h, reg_l, 11, 1),
                      ADDXXREG(reg_ixh, reg_ixl, reg_ixh, reg_ixl, 11, 1),
                      ADDXXREG(reg_iyh, reg_iyl, reg_iyh, reg_iyl, 11, 1));
                break;
            case 0x2a: /* LD HL (WORD) ; LD IX (WORD) ; LD IY (WORD) */
                INSTS(LDIND(p12, reg_h, reg_l, 4, 4, 8, 3),
                      LDIND(p12, reg_ixh, reg_ixl, 4, 4, 8, 3),
                      LDIND(p12, reg_iyh, reg_iyl, 4, 4, 8, 3));
                break;
            case 0x2b: /* DEC HL ; DEC IX ; DEC IY */
                INSTS(DECINC(DEC_HL_WORD(), 6, 1),
                      DECINC(DEC_IX_WORD(), 6, 1),
                      DECINC(DEC_IY_WORD(), 6, 1));
                break;
            case 0x2c: /* INC L ; INC IXL ; DEC IYL */
                INSTS(INCREG(reg_l, 4, 1),
                      INCREG(reg_ixl, 4, 1),
                      INCREG(reg_iyl, 4, 1));
                break;
            case 0x2d: /* DEC L ; DEC IXL ; DEC IYL */
                INSTS(DECREG(reg_l, 4, 1),
                      DECREG(reg_ixl, 4, 1),
                      DECREG(reg_iyl, 4, 1));
                break;
            case 0x2e: /* LD L # ; LD IXL # ; LD IYL # */
                INSTS(LDREG(reg_l, p1, 4, 3, 2),
                      LDREG(reg_ixl, p1, 4, 3, 2),
                      LDREG(reg_iyl, p1, 4, 3, 2));
                break;
            case 0x2f: /* CPL */
                CPL(4, 1);
                break;
            case 0x30: /* JR NC */
                BRANCH(!LOCAL_CARRY(), p1, 2);
                break;
            case 0x31: /* LD SP # */
                LDSP(p12, 10, 0, 3);
                break;
            case 0x32: /* LD (WORD) A */
                STREGWZ(p12, reg_a, 10, 3, 3);
                break;
            case 0x33: /* INC SP */
                DECINC(reg_sp++, 6, 1);
                break;
            case 0x34: /* INC (HL) ; INC (IX+d) ; INC (IY+d) */
                INSTS(INCXXIND(HL_WORD(), 4, 4, 3, 1),
                      INCXXIND(IX_WORD_OFF(p1), 0, 7, 12, 2),
                      INCXXIND(IY_WORD_OFF(p1), 0, 7, 12, 2));
                break;
            case 0x35: /* DEC (HL) ; DEC (IX+d) ; DEC (IY+d) */
                INSTS(DECXXIND(HL_WORD(), 4, 4, 3, 1),
                      DECXXIND(IX_WORD_OFF(p1), 0, 7, 12, 2),
                      DECXXIND(IY_WORD_OFF(p1), 0, 7, 12, 2));
                break;
            case 0x36: /* LD (HL) # ; LD (IX+d) # ; LD (IY+d) # */
                INSTS(STREG(HL_WORD(), p1, 8, 2, 2),
                      STREG(IX_WORD_OFF(p1), p2, 8, 7, 3),
                      STREG(IY_WORD_OFF(p1), p2, 8, 7, 3));
                break;
            case 0x37: /* SCF */
                SCF(4, 1);
                break;
            case 0x38: /* JR C */
                BRANCH(LOCAL_CARRY(), p1, 2);
                break;
            case 0x39: /* ADD HL SP ; ADD IX SP ; ADD IY SP */
                INSTS(ADDXXSP(reg_h, reg_l, 11, 1),
                      ADDXXSP(reg_ixh, reg_ixl, 11, 1),
                      ADDXXSP(reg_iyh, reg_iyl, 11, 1));
                break;
            case 0x3a: /* LD A (WORD) */
                LDREGWZ(reg_a, p12, 10, 3, 3);
                break;
            case 0x3b: /* DEC SP */
                DECINC(reg_sp--, 6, 1);
                break;
            case 0x3c: /* INC A */
                INCREG(reg_a, 4, 1);
                break;
            case 0x3d: /* DEC A */
                DECREG(reg_a, 4, 1);
                break;
            case 0x3e: /* LD A # */
                LDREG(reg_a, p1, 4, 3, 2);
                break;
            case 0x3f: /* CCF */
                CCF(4, 1);
                break;
            case 0x40: /* LD B B */
                LDREG(reg_b, reg_b, 0, 4, 1);
                break;
            case 0x41: /* LD B C */
                LDREG(reg_b, reg_c, 0, 4, 1);
                break;
            case 0x42: /* LD B D */
                LDREG(reg_b, reg_d, 0, 4, 1);
                break;
            case 0x43: /* LD B E */
                LDREG(reg_b, reg_e, 0, 4, 1);
                break;
            case 0x44: /* LD B H ; LD B IXH ; LD B IYH */
                INSTS(LDREG(reg_b, reg_h, 0, 4, 1),
                      LDREG(reg_b, reg_ixh, 0, 4, 1),
                      LDREG(reg_b, reg_iyh, 0, 4, 1));
                break;
            case 0x45: /* LD B L ; LD B IXL ; LD B IYL */
                INSTS(LDREG(reg_b, reg_l, 0, 4, 1),
                      LDREG(reg_b, reg_ixl, 0, 4, 1),
                      LDREG(reg_b, reg_iyl, 0, 4, 1));
                break;
            case 0x46: /* LD B (HL) ; LD B (IX+d) ; LD B (IY+d) */
                INSTS(LDREG(reg_b, LOAD(HL_WORD()), 4, 3, 1),
                      LDREG(reg_b, LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      LDREG(reg_b, LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x47: /* LD B A */
                LDREG(reg_b, reg_a, 0, 4, 1);
                break;
            case 0x48: /* LD C B */
                LDREG(reg_c, reg_b, 0, 4, 1);
                break;
            case 0x49: /* LD C C */
                LDREG(reg_c, reg_c, 0, 4, 1);
                break;
            case 0x4a: /* LD C D */
                LDREG(reg_c, reg_d, 0, 4, 1);
                break;
            case 0x4b: /* LD C E */
                LDREG(reg_c, reg_e, 0, 4, 1);
                break;
            case 0x4c: /* LD C H ; LD C IXH ; LD C IYH */
                INSTS(LDREG(reg_c, reg_h, 0, 4, 1),
                      LDREG(reg_c, reg_ixh, 0, 4, 1),
                      LDREG(reg_c, reg_iyh, 0, 4, 1));
                break;
            case 0x4d: /* LD C L ; LD C IXL ; LD C IYL */
                INSTS(LDREG(reg_c, reg_l, 0, 4, 1),
                      LDREG(reg_c, reg_ixl, 0, 4, 1),
                      LDREG(reg_c, reg_iyl, 0, 4, 1));
                break;
            case 0x4e: /* LD C (HL) ; LD C (IX+d) ; LD C (IY+d) */
                INSTS(LDREG(reg_c, LOAD(HL_WORD()), 4, 3, 1),
                      LDREG(reg_c, LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      LDREG(reg_c, LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x4f: /* LD C A */
                LDREG(reg_c, reg_a, 0, 4, 1);
                break;
            case 0x50: /* LD D B */
                LDREG(reg_d, reg_b, 0, 4, 1);
                break;
            case 0x51: /* LD D C */
                LDREG(reg_d, reg_c, 0, 4, 1);
                break;
            case 0x52: /* LD D D */
                LDREG(reg_d, reg_d, 0, 4, 1);
                break;
            case 0x53: /* LD D E */
                LDREG(reg_d, reg_e, 0, 4, 1);
                break;
            case 0x54: /* LD D H ; LD D IXH ; LD D IYH */
                INSTS(LDREG(reg_d, reg_h, 0, 4, 1),
                      LDREG(reg_d, reg_ixh, 0, 4, 1),
                      LDREG(reg_d, reg_iyh, 0, 4, 1));
                break;
            case 0x55: /* LD D L ; LD D IXL ; LD D IYL */
                INSTS(LDREG(reg_d, reg_l, 0, 4, 1),
                      LDREG(reg_d, reg_ixl, 0, 4, 1),
                      LDREG(reg_d, reg_iyl, 0, 4, 1));
                break;
            case 0x56: /* LD D (HL) ; LD D (IX+d) ; LD D (IY+d) */
                INSTS(LDREG(reg_d, LOAD(HL_WORD()), 4, 3, 1),
                      LDREG(reg_d, LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      LDREG(reg_d, LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x57: /* LD D A */
                LDREG(reg_d, reg_a, 0, 4, 1);
                break;
            case 0x58: /* LD E B */
                LDREG(reg_e, reg_b, 0, 4, 1);
                break;
            case 0x59: /* LD E C */
                LDREG(reg_e, reg_c, 0, 4, 1);
                break;
            case 0x5a: /* LD E D */
                LDREG(reg_e, reg_d, 0, 4, 1);
                break;
            case 0x5b: /* LD E E */
                LDREG(reg_e, reg_e, 0, 4, 1);
                break;
            case 0x5c: /* LD E H ; LD E IXH ; LD E IYH */
                INSTS(LDREG(reg_e, reg_h, 0, 4, 1),
                      LDREG(reg_e, reg_ixh, 0, 4, 1),
                      LDREG(reg_e, reg_iyh, 0, 4, 1));
                break;
            case 0x5d: /* LD E L ; LD E IXL ; LD E IYL */
                INSTS(LDREG(reg_e, reg_l, 0, 4, 1),
                      LDREG(reg_e, reg_ixl, 0, 4, 1),
                      LDREG(reg_e, reg_iyl, 0, 4, 1));
                break;
            case 0x5e: /* LD E (HL) ; LD E (IX+d) ; LD E (IY+d) */
                INSTS(LDREG(reg_e, LOAD(HL_WORD()), 4, 3, 1),
                      LDREG(reg_e, LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      LDREG(reg_e, LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x5f: /* LD E A */
                LDREG(reg_e, reg_a, 0, 4, 1);
                break;
            case 0x60: /* LD H B ; LD IXH B ; LD IYH B */
                INSTS(LDREG(reg_h, reg_b, 0, 4, 1),
                      LDREG(reg_ixh, reg_b, 0, 4, 1),
                      LDREG(reg_iyh, reg_b, 0, 4, 1));
                break;
            case 0x61: /* LD H C ; LD IXH C ; LD IYH C */
                INSTS(LDREG(reg_h, reg_c, 0, 4, 1),
                      LDREG(reg_ixh, reg_c, 0, 4, 1),
                      LDREG(reg_iyh, reg_c, 0, 4, 1));
                break;
            case 0x62: /* LD H D ; LD IXH D ; LD IYH D */
                INSTS(LDREG(reg_h, reg_d, 0, 4, 1),
                      LDREG(reg_ixh, reg_d, 0, 4, 1),
                      LDREG(reg_iyh, reg_d, 0, 4, 1));
                break;
            case 0x63: /* LD H E ; LD IXH E ; LD IYH E */
                INSTS(LDREG(reg_h, reg_e, 0, 4, 1),
                      LDREG(reg_ixh, reg_e, 0, 4, 1),
                      LDREG(reg_iyh, reg_e, 0, 4, 1));
                break;
            case 0x64: /* LD H H ; LD IXH IXH; LD IYH IYH */
                INSTS(LDREG(reg_h, reg_h, 0, 4, 1),
                      LDREG(reg_ixh, reg_ixh, 0, 4, 1),
                      LDREG(reg_iyh, reg_iyh, 0, 4, 1));
                break;
            case 0x65: /* LD H L ; LD IXH IXL ; LD IYH IYL */
                INSTS(LDREG(reg_h, reg_l, 0, 4, 1),
                      LDREG(reg_ixh, reg_ixl, 0, 4, 1),
                      LDREG(reg_iyh, reg_iyl, 0, 4, 1));
                break;
            case 0x66: /* LD H (HL) ; LD H (IX+d) ; LD H (IY+d) */
                INSTS(LDREG(reg_h, LOAD(HL_WORD()), 4, 3, 1),
                      LDREG(reg_h, LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      LDREG(reg_h, LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x67: /* LD H A ; LD IXH A ; LD IYH A */
                INSTS(LDREG(reg_h, reg_a, 0, 4, 1),
                      LDREG(reg_ixh, reg_a, 0, 4, 1),
                      LDREG(reg_iyh, reg_a, 0, 4, 1));
                break;
            case 0x68: /* LD L B ; LD IXH B ; LD IYH B */
                INSTS(LDREG(reg_l, reg_b, 0, 4, 1),
                      LDREG(reg_ixl, reg_b, 0, 4, 1),
                      LDREG(reg_iyl, reg_b, 0, 4, 1));
                break;
            case 0x69: /* LD L C ; LD IXH C ; LD IYH C */
                INSTS(LDREG(reg_l, reg_c, 0, 4, 1),
                      LDREG(reg_ixl, reg_c, 0, 4, 1),
                      LDREG(reg_iyl, reg_c, 0, 4, 1));
                break;
            case 0x6a: /* LD L D ; LD IXH D ; LD IYH D */
                INSTS(LDREG(reg_l, reg_d, 0, 4, 1),
                      LDREG(reg_ixl, reg_d, 0, 4, 1),
                      LDREG(reg_iyl, reg_d, 0, 4, 1));
                break;
            case 0x6b: /* LD L E ; LD IXH E ; LD IYH E */
                INSTS(LDREG(reg_l, reg_e, 0, 4, 1),
                      LDREG(reg_ixl, reg_e, 0, 4, 1),
                      LDREG(reg_iyl, reg_e, 0, 4, 1));
                break;
            case 0x6c: /* LD L H ; LD IXL LXH ; LD IYL IYH */
                INSTS(LDREG(reg_l, reg_h, 0, 4, 1),
                      LDREG(reg_ixl, reg_ixh, 0, 4, 1),
                      LDREG(reg_iyl, reg_iyh, 0, 4, 1));
                break;
            case 0x6d: /* LD L L ; LD IXL IXL ; LD IYL IYL */
                INSTS(LDREG(reg_l, reg_l, 0, 4, 1),
                      LDREG(reg_ixl, reg_ixl, 0, 4, 1),
                      LDREG(reg_iyl, reg_iyl, 0, 4, 1));
                break;
            case 0x6e: /* LD L (HL) ; LD L (IX+d) ; LD L (IY+d) */
                INSTS(LDREG(reg_l, LOAD(HL_WORD()), 4, 3, 1),
                      LDREG(reg_l, LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      LDREG(reg_l, LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x6f: /* LD L A ; LD IXL A ; LD IYL A */
                INSTS(LDREG(reg_l, reg_a, 0, 4, 1),
                      LDREG(reg_ixl, reg_a, 0, 4, 1),
                      LDREG(reg_iyl, reg_a, 0, 4, 1));
                break;
            case 0x70: /* LD (HL) B ; LD (IX+d) B ; LD (IY+d) B */
                INSTS(STREG(HL_WORD(), reg_b, 4, 3, 1),
                      STREG(IX_WORD_OFF(p1), reg_b, 4, 11, 2),
                      STREG(IY_WORD_OFF(p1), reg_b, 4, 11, 2));
                break;
            case 0x71: /* LD (HL) C ; LD (IX+d) C ; LD (IY+d) C */
                INSTS(STREG(HL_WORD(), reg_c, 4, 3, 1),
                      STREG(IX_WORD_OFF(p1), reg_c, 4, 11, 2),
                      STREG(IY_WORD_OFF(p1), reg_c, 4, 11, 2));
                break;
            case 0x72: /* LD (HL) D ; LD (IX+d) D ; LD (IY+d) D */
                INSTS(STREG(HL_WORD(), reg_d, 4, 3, 1),
                      STREG(IX_WORD_OFF(p1), reg_d, 4, 11, 2),
                      STREG(IY_WORD_OFF(p1), reg_d, 4, 11, 2));
                break;
            case 0x73: /* LD (HL) E ; LD (IX+d) E ; LD (IY+d) E */
                INSTS(STREG(HL_WORD(), reg_e, 4, 3, 1),
                      STREG(IX_WORD_OFF(p1), reg_e, 4, 11, 2),
                      STREG(IY_WORD_OFF(p1), reg_e, 4, 11, 2));
                break;
            case 0x74: /* LD (HL) H ; LD (IX+d) H ; LD (IY+d) H */
                INSTS(STREG(HL_WORD(), reg_h, 4, 3, 1),
                      STREG(IX_WORD_OFF(p1), reg_h, 4, 11, 2),
                      STREG(IY_WORD_OFF(p1), reg_h, 4, 11, 2));
                break;
            case 0x75: /* LD (HL) L ; LD (IX+d) L ; LD (IY+d) L */
                INSTS(STREG(HL_WORD(), reg_l, 4, 3, 1),
                      STREG(IX_WORD_OFF(p1), reg_l, 4, 11, 2),
                      STREG(IY_WORD_OFF(p1), reg_l, 4, 11, 2));
                break;
            case 0x76: /* HALT */
                HALT();
                break;
            case 0x77: /* LD (HL) A ; LD (IX+d) A ; LD (IY+d) A */
                INSTS(STREG(HL_WORD(), reg_a, 4, 3, 1),
                      STREG(IX_WORD_OFF(p1), reg_a, 4, 11, 2),
                      STREG(IY_WORD_OFF(p1), reg_a, 4, 11, 2));
                break;
            case 0x78: /* LD A B */
                LDREG(reg_a, reg_b, 0, 4, 1);
                break;
            case 0x79: /* LD A C */
                LDREG(reg_a, reg_c, 0, 4, 1);
                break;
            case 0x7a: /* LD A D */
                LDREG(reg_a, reg_d, 0, 4, 1);
                break;
            case 0x7b: /* LD A E */
                LDREG(reg_a, reg_e, 0, 4, 1);
                break;
            case 0x7c: /* LD A H ; LD A IXH ; LD A IYH */
                INSTS(LDREG(reg_a, reg_h, 0, 4, 1),
                      LDREG(reg_a, reg_ixh, 0, 4, 1),
                      LDREG(reg_a, reg_iyh, 0, 4, 1));
                break;
            case 0x7d: /* LD A L ; LD A IXL ; LD A IYL */
                INSTS(LDREG(reg_a, reg_l, 0, 4, 1),
                      LDREG(reg_a, reg_ixl, 0, 4, 1),
                      LDREG(reg_a, reg_iyl, 0, 4, 1));
                break;
            case 0x7e: /* LD A (HL) ; LD A (IX+d) ; LD A (IY+d) */
                INSTS(LDREG(reg_a, LOAD(HL_WORD()), 4, 3, 1),
                      LDREG(reg_a, LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      LDREG(reg_a, LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x7f: /* LD A A */
                LDREG(reg_a, reg_a, 0, 4, 1);
                break;
            case 0x80: /* ADD B */
                ADD(reg_b, 0, 4, 1);
                break;
            case 0x81: /* ADD C */
                ADD(reg_c, 0, 4, 1);
                break;
            case 0x82: /* ADD D */
                ADD(reg_d, 0, 4, 1);
                break;
            case 0x83: /* ADD E */
                ADD(reg_e, 0, 4, 1);
                break;
            case 0x84: /* ADD H ; ADD IXH ; ADD IYH */
                INSTS(ADD(reg_h, 0, 4, 1),
                      ADD(reg_ixh, 0, 4, 1),
                      ADD(reg_iyh, 0, 4, 1));
                break;
            case 0x85: /* ADD L ; ADD IXL ; ADD IYL */
                INSTS(ADD(reg_l, 0, 4, 1),
                      ADD(reg_ixl, 0, 4, 1),
                      ADD(reg_iyl, 0, 4, 1));
                break;
            case 0x86: /* ADD (HL) ; ADD (IX+d) ; ADD (IY+d) */
                INSTS(ADD(LOAD(HL_WORD()), 4, 3, 1),
                      ADD(LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      ADD(LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x87: /* ADD A */
                ADD(reg_a, 0, 4, 1);
                break;
            case 0x88: /* ADC B */
                ADC(reg_b, 0, 4, 1);
                break;
            case 0x89: /* ADC C */
                ADC(reg_c, 0, 4, 1);
                break;
            case 0x8a: /* ADC D */
                ADC(reg_d, 0, 4, 1);
                break;
            case 0x8b: /* ADC E */
                ADC(reg_e, 0, 4, 1);
                break;
            case 0x8c: /* ADC H ; ADC IXH ; ADC IYH */
                INSTS(ADC(reg_h, 0, 4, 1),
                      ADC(reg_ixh, 0, 4, 1),
                      ADC(reg_iyh, 0, 4, 1));
                break;
            case 0x8d: /* ADC L ; ADC IXL ; ADC IYL */
                INSTS(ADC(reg_l, 0, 4, 1),
                      ADC(reg_ixl, 0, 4, 1),
                      ADC(reg_iyl, 0, 4, 1));
                break;
            case 0x8e: /* ADC (HL) ; ADC (IX+d) ; ADC (IY+d) */
                INSTS(ADC(LOAD(HL_WORD()), 4, 3, 1),
                      ADC(LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      ADC(LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x8f: /* ADC A */
                ADC(reg_a, 0, 4, 1);
                break;
            case 0x90: /* SUB B */
                SUB(reg_b, 0, 4, 1);
                break;
            case 0x91: /* SUB C */
                SUB(reg_c, 0, 4, 1);
                break;
            case 0x92: /* SUB D */
                SUB(reg_d, 0, 4, 1);
                break;
            case 0x93: /* SUB E */
                SUB(reg_e, 0, 4, 1);
                break;
            case 0x94: /* SUB H ; SUB IXH ; SUB IYH */
                INSTS(SUB(reg_h, 0, 4, 1),
                      SUB(reg_ixh, 0, 4, 1),
                      SUB(reg_iyh, 0, 4, 1));
                break;
            case 0x95: /* SUB L ; SUB IXL ; SUB IYL */
                INSTS(SUB(reg_l, 0, 4, 1),
                      SUB(reg_ixl, 0, 4, 1),
                      SUB(reg_iyl, 0, 4, 1));
                break;
            case 0x96: /* SUB (HL) ; SUB (IX+d) ; SUB (IY+d) */
                INSTS(SUB(LOAD(HL_WORD()), 4, 3, 1),
                      SUB(LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      SUB(LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x97: /* SUB A */
                SUB(reg_a, 0, 4, 1);
                break;
            case 0x98: /* SBC B */
                SBC(reg_b, 0, 4, 1);
                break;
            case 0x99: /* SBC C */
                SBC(reg_c, 0, 4, 1);
                break;
            case 0x9a: /* SBC D */
                SBC(reg_d, 0, 4, 1);
                break;
            case 0x9b: /* SBC E */
                SBC(reg_e, 0, 4, 1);
                break;
            case 0x9c: /* SBC H ; SBC IXH ; SBC IYH */
                INSTS(SBC(reg_h, 0, 4, 1),
                      SBC(reg_ixh, 0, 4, 1),
                      SBC(reg_iyh, 0, 4, 1));
                break;
            case 0x9d: /* SBC L ; SBC IXL ; SBC IYL */
                INSTS(SBC(reg_l, 0, 4, 1),
                      SBC(reg_ixl, 0, 4, 1),
                      SBC(reg_iyl, 0, 4, 1));
                break;
            case 0x9e: /* SBC (HL) ; SBC (IX+d) ; SBC (IY+d) */
                INSTS(SBC(LOAD(HL_WORD()), 4, 3, 1),
                      SBC(LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      SBC(LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0x9f: /* SBC A */
                SBC(reg_a, 0, 4, 1);
                break;
            case 0xa0: /* AND B */
                AND(reg_b, 0, 4, 1);
                break;
            case 0xa1: /* AND C */
                AND(reg_c, 0, 4, 1);
                break;
            case 0xa2: /* AND D */
                AND(reg_d, 0, 4, 1);
                break;
            case 0xa3: /* AND E */
                AND(reg_e, 0, 4, 1);
                break;
            case 0xa4: /* AND H ; AND IXH ; AND IYH */
                INSTS(AND(reg_h, 0, 4, 1),
                      AND(reg_ixh, 0, 4, 1),
                      AND(reg_iyh, 0, 4, 1));
                break;
            case 0xa5: /* AND L ; AND IXL ; AND IYL */
                INSTS(AND(reg_l, 0, 4, 1),
                      AND(reg_ixl, 0, 4, 1),
                      AND(reg_iyl, 0, 4, 1));
                break;
            case 0xa6: /* AND (HL) ; AND (IX+d) ; AND (IY+d) */
                INSTS(AND(LOAD(HL_WORD()), 4, 3, 1),
                      AND(LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      AND(LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0xa7: /* AND A */
                AND(reg_a, 0, 4, 1);
                break;
            case 0xa8: /* XOR B */
                XOR(reg_b, 0, 4, 1);
                break;
            case 0xa9: /* XOR C */
                XOR(reg_c, 0, 4, 1);
                break;
            case 0xaa: /* XOR D */
                XOR(reg_d, 0, 4, 1);
                break;
            case 0xab: /* XOR E */
                XOR(reg_e, 0, 4, 1);
                break;
            case 0xac: /* XOR H ; XOR IXH ; XOR IYH */
                INSTS(XOR(reg_h, 0, 4, 1),
                      XOR(reg_ixh, 0, 4, 1),
                      XOR(reg_iyh, 0, 4, 1));
                break;
            case 0xad: /* XOR L ; XOR IXL ; XOR IYL */
                INSTS(XOR(reg_l, 0, 4, 1),
                      XOR(reg_ixl, 0, 4, 1),
                      XOR(reg_iyl, 0, 4, 1));
                break;
            case 0xae: /* XOR (HL) ; XOR (IX+d) ; XOR (IY+d) */
                INSTS(XOR(LOAD(HL_WORD()), 4, 3, 1),
                      XOR(LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      XOR(LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0xaf: /* XOR A */
                XOR(reg_a, 0, 4, 1);
                break;
            case 0xb0: /* OR B */
                OR(reg_b, 0, 4, 1);
                break;
            case 0xb1: /* OR C */
                OR(reg_c, 0, 4, 1);
                break;
            case 0xb2: /* OR D */
                OR(reg_d, 0, 4, 1);
                break;
            case 0xb3: /* OR E */
                OR(reg_e, 0, 4, 1);
                break;
            case 0xb4: /* OR H ; OR IXH ; OR IYH */
                INSTS(OR(reg_h, 0, 4, 1),
                      OR(reg_ixh, 0, 4, 1),
                      OR(reg_iyh, 0, 4, 1));
                break;
            case 0xb5: /* OR L ; OR IYL ; OR IYL */
                INSTS(OR(reg_l, 0, 4, 1),
                      OR(reg_ixl, 0, 4, 1),
                      OR(reg_iyl, 0, 4, 1));
                break;
            case 0xb6: /* OR (HL) ; OR (IX+d) ; OR (IY+d) */
                INSTS(OR(LOAD(HL_WORD()), 4, 3, 1),
                      OR(LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      OR(LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0xb7: /* OR A */
                OR(reg_a, 0, 4, 1);
                break;
            case 0xb8: /* CP B */
                CP(reg_b, 0, 4, 1);
                break;
            case 0xb9: /* CP C */
                CP(reg_c, 0, 4, 1);
                break;
            case 0xba: /* CP D */
                CP(reg_d, 0, 4, 1);
                break;
            case 0xbb: /* CP E */
                CP(reg_e, 0, 4, 1);
                break;
            case 0xbc: /* CP H ; CP IXH ; CP IYH */
                INSTS(CP(reg_h, 0, 4, 1),
                      CP(reg_ixh, 0, 4, 1),
                      CP(reg_iyh, 0, 4, 1));
                break;
            case 0xbd: /* CP L ; CP IXL ; CP IYL */
                INSTS(CP(reg_l, 0, 4, 1),
                      CP(reg_ixl, 0, 4, 1),
                      CP(reg_iyl, 0, 4, 1));
                break;
            case 0xbe: /* CP (HL) ; CP (IX+d) ; CP (IY+d) */
                INSTS(CP(LOAD(HL_WORD()), 4, 3, 1),
                      CP(LOAD(IX_WORD_OFF(p1)), 4, 11, 2),
                      CP(LOAD(IY_WORD_OFF(p1)), 4, 11, 2));
                break;
            case 0xbf: /* CP A */
                CP(reg_a, 0, 4, 1);
                break;
            case 0xc0: /* RET NZ */
                RET_COND(!LOCAL_ZERO(), 5, 3, 3, 5, 1);
                break;
            case 0xc1: /* POP BC */
                POP(reg_b, reg_c, 1);
                break;
            case 0xc2: /* JP NZ */
                JMP_COND(p12, !LOCAL_ZERO(), 10, 10);
                break;
            case 0xc3: /* JP */
                JMP(p12, 10);
                break;
            case 0xc4: /* CALL NZ */
                CALL_COND(p12, !LOCAL_ZERO(), 3, 3, 11, 10, 3);
                break;
            case 0xc5: /* PUSH BC */
                PUSH(reg_b, reg_c, 1);
                break;
            case 0xc6: /* ADD # */
                ADD(p1, 4, 3, 2);
                break;
            case 0xc7: /* RST 00 */
                CALL(0x00, 3, 3, 5, 1);
                break;
            case 0xc8: /* RET Z */
                RET_COND(LOCAL_ZERO(), 5, 3, 3, 5, 1);
                break;
            case 0xc9: /* RET */
                RET(4, 4, 2);
                break;
            case 0xca: /* JP Z */
                JMP_COND(p12, LOCAL_ZERO(), 10, 10);
                break;
            case 0xcb: /* OPCODE CB */
                INSTS(opcode_cb((uint8_t)p1, (uint8_t)p2, (uint8_t)p3, (uint16_t)p12, (uint16_t)p23),
                      opcode_ddfd_cb((uint8_t)p1, (uint8_t)p2, (uint16_t)p12),
                      opcode_ddfd_cb((uint8_t)p1, (uint8_t)p2, (uint16_t)p12));
                break;
            case 0xcc: /* CALL Z */
                CALL_COND(p12, LOCAL_ZERO(), 3, 3, 11, 10, 3);
                break;
            case 0xcd: /* CALL */
                CALL(p12, 3, 3, 11, 3);
                break;
            case 0xce: /* ADC # */
                ADC(p1, 4, 3, 2);
                break;
            case 0xcf: /* RST 08 */
                CALL(0x08, 3, 3, 5, 1);
                break;
            case 0xd0: /* RET NC */
                RET_COND(!LOCAL_CARRY(), 5, 3, 3, 5, 1);
                break;
            case 0xd1: /* POP DE */
                POP(reg_d, reg_e, 1);
                break;
            case 0xd2: /* JP NC */
                JMP_COND(p12, !LOCAL_CARRY(), 10, 10);
                break;
            case 0xd3: /* OUT A */
                OUTA(p1, 4, 7, 2);
                break;
            case 0xd4: /* CALL NC */
                CALL_COND(p12, !LOCAL_CARRY(), 3, 3, 11, 10, 3);
                break;
            case 0xd5: /* PUSH DE */
                PUSH(reg_d, reg_e, 1);
                break;
            case 0xd6: /* SUB # */
                SUB(p1, 4, 3, 2);
                break;
            case 0xd7: /* RST 10 */
                CALL(0x10, 3, 3, 5, 1);
                break;
            case 0xd8: /* RET C */
                RET_COND(LOCAL_CARRY(), 5, 3, 3, 5, 1);
                break;
            case 0xd9: /* EXX */
                EXX(4, 1);
                break;
            case 0xda: /* JP C */
                JMP_COND(p12, LOCAL_CARRY(), 10, 10);
                break;
            case 0xdb: /* IN A */
                INA(p1, 4, 7, 2);
                break;
            case 0xdc: /* CALL C */
                CALL_COND(p12, LOCAL_CARRY(), 3, 3, 11, 10, 3);
                break;
            case 0xdd: /*  OPCODE DD */
                INSTS(inst_mode = INST_IX; CLK_ADD(CLK, 4); INC_PC(1); goto fetchmore, NOP(4,1), NOP(4,1));
                break;
            case 0xde: /* SBC # */
                SBC(p1, 4, 3, 2);
                break;
            case 0xdf: /* RST 18 */
                CALL(0x18, 3, 3, 5, 1);
                break;
            case 0xe0: /* RET PO */
                RET_COND(!LOCAL_PARITY(), 5, 3, 3, 5, 1);
                break;
            case 0xe1: /* POP HL ; POP IX; POP IY */
                INSTS(POP(reg_h, reg_l, 1),
                      POP(reg_ixh, reg_ixl, 1),
                      POP(reg_iyh, reg_iyl, 1));
                break;
            case 0xe2: /* JP PO */
                JMP_COND(p12, !LOCAL_PARITY(), 10, 10);
                break;
            case 0xe3: /* EX HL (SP) ; EX IX (SP) ; EX IY (SP) */
                INSTS(EXXXSP(reg_h, reg_l, 4, 4, 4, 4, 3, 1),
                      EXXXSP(reg_ixh, reg_ixl, 4, 4, 4, 4, 3, 1),
                      EXXXSP(reg_iyh, reg_iyl, 4, 4, 4, 4, 3, 1));
                break;
            case 0xe4: /* CALL PO */
                CALL_COND(p12, !LOCAL_PARITY(), 3, 3, 11, 10, 3);
                break;
            case 0xe5: /* PUSH HL ; PUSH IX; PUSH IY */
                INSTS(PUSH(reg_h, reg_l, 1),
                      PUSH(reg_ixh, reg_ixl, 1),
                      PUSH(reg_iyh, reg_iyl, 1));
                break;
            case 0xe6: /* AND # */
                AND(p1, 4, 3, 2);
                break;
            case 0xe7: /* RST 20 */
                CALL(0x20, 3, 3, 5, 1);
                break;
            case 0xe8: /* RET PE */
                RET_COND(LOCAL_PARITY(), 5, 3, 3, 5, 1);
                break;
            case 0xe9: /* LD PC HL ; LD PC IX ; LD PC IY */
                INSTS(JMP((HL_WORD()), 4),
                      JMP((IX_WORD()), 4),
                      JMP((IY_WORD()), 4));
                break;
            case 0xea: /* JP PE */
                JMP_COND(p12, LOCAL_PARITY(), 10, 10);
                break;
            case 0xeb: /* EX DE HL */
                EXDEHL(4, 1);
                break;
            case 0xec: /* CALL PE */
                CALL_COND(p12, LOCAL_PARITY(), 3, 3, 11, 10, 3);
                break;
            case 0xed: /* OPCODE ED */
                opcode_ed((uint8_t)p1, (uint8_t)p2, (uint8_t)p3, (uint16_t)p12, (uint16_t)p23);
                break;
            case 0xee: /* XOR # */
                XOR(p1, 4, 3, 2);
                break;
            case 0xef: /* RST 28 */
                CALL(0x28, 3, 3, 5, 1);
                break;
            case 0xf0: /* RET P */
                RET_COND(!LOCAL_SIGN(), 5, 3, 3, 5, 1);
                break;
            case 0xf1: /* POP AF */
                POP(reg_a, reg_f, 1);
                break;
            case 0xf2: /* JP P */
                JMP_COND(p12, !LOCAL_SIGN(), 10, 10);
                break;
            case 0xf3: /* DI */
                DI(4, 1);
                break;
            case 0xf4: /* CALL P */
                CALL_COND(p12, !LOCAL_SIGN(), 3, 3, 11, 10, 3);
                break;
            case 0xf5: /* PUSH AF */
                PUSH(reg_a, reg_f, 1);
                break;
            case 0xf6: /* OR # */
                OR(p1, 4, 3, 2);
                break;
            case 0xf7: /* RST 30 */
                CALL(0x30, 3, 3, 5, 1);
                break;
            case 0xf8: /* RET M */
                RET_COND(LOCAL_SIGN(), 5, 3, 3, 5, 1);
                break;
            case 0xf9: /* LD SP HL ; LD SP IX ; LD SP IY */
                INSTS(LDSP(HL_WORD(), 4, 2, 1),
                      LDSP(IX_WORD(), 0, 6, 1),
                      LDSP(IY_WORD(), 0, 6, 1));
                break;
            case 0xfa: /* JP M */
                JMP_COND(p12, LOCAL_SIGN(), 10, 10);
                break;
            case 0xfb: /* EI */
                EI(4, 1);
                break;
            case 0xfc: /* CALL M */
                CALL_COND(p12, LOCAL_SIGN(), 3, 3, 11, 10, 3);
                break;
            case 0xfd: /* OPCODE FD */
                INSTS(inst_mode = INST_IY; CLK_ADD(CLK, 4); INC_PC(1); goto fetchmore, NOP(4,1), NOP(4,1));
                break;
            case 0xfe: /* CP # */
                CP(p1, 4, 3, 2);
                break;
            case 0xff: /* RST 38 */
                CALL(0x38, 3, 3, 5, 1);
                break;
        }

        cpu_int_status->num_dma_per_opcode = 0;
        inst_mode = INST_NONE;

        if (maincpu_clk_limit && (CLK > maincpu_clk_limit)) {
            log_error(LOG_DEFAULT, "cycle limit reached.");
            archdep_vice_exit(1);
        }

    } while (Z80_LOOP_COND);

    export_registers();
}
