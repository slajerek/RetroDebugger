/*
 * c64acia1.c - Definitions for a 6551 ACIA interface
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "vice.h"

#include "c64acia.h"

#define mycpu           maincpu
#define myclk           maincpu_clk
#define mycpu_rmw_flag  maincpu_rmw_flag

#define myacia acia1

/* resource defaults */
#define MYACIA          "Acia1"
#define MyDevice        1
#define MyIrq           IK_NMI

#define myaciadev       acia1dev
#define myaciactrl      acia1ctrl

#if 0
void acia1_init(void);
void acia1_reset(void);
int acia1_resources_init(void);
int acia1_cmdline_options_init(void);
int acia1_snapshot_write_module(struct snapshot_s *);
int acia1_snapshot_read_module(struct snapshot_s *);
void acia1_store
#endif


#define myacia_init acia1_init
#define myacia_init_cmdline_options acia1_cmdline_options_init
#define myacia_init_resources acia1_resources_init
#define myacia_snapshot_read_module acia1_snapshot_read_module
#define myacia_snapshot_write_module acia1_snapshot_write_module
#define myacia_peek acia1_peek
#define myacia_reset acia1_reset
#define myacia_store acia1_store

/* function prototype */
int acia1_set_mode(int mode);

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
#define myacia_set_mode(x) acia1_set_mode(x)
#else
#define myacia_set_mode(x) 0
#endif

#include "cartio.h"
#include "cartridge.h"
#include "export.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"

#define mycpu_alarm_context maincpu_alarm_context
#define mycpu_set_irq maincpu_set_irq
#define mycpu_set_nmi maincpu_set_nmi
#define mycpu_set_int_noclk maincpu_set_int

#include "acia.h"

#define ACIA_MODE_HIGHEST   ACIA_MODE_TURBO232

//#include "aciacore.c"

///
///
////// aciacore.c starts here


/*! \file aciacore.c \n
 *  \author Andre Fachat, Spiro Trikaliotis\n
 *  \brief  Template file for ACIA 6551 emulation.
 *
 * Written by
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
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

#include "vice.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "acia.h"
#include "alarm.h"
#include "cmdline.h"
#include "interrupt.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "rs232drv.h"
#include "rs232.h"
#include "snapshot.h"
#include "vicetypes.h"
#include "monitor.h"

uint8_t myacia_read(uint16_t addr);

#undef  DEBUG   /*!< define if you want "normal" debugging output */
#undef  DEBUG_VERBOSE /*!< define if you want very verbose debugging output. */
/* #define DEBUG */
/* #define DEBUG_VERBOSE */

/* #define LOG_MODEM_STATUS */

/*! \brief Helper macro for outputting debugging messages */
#ifdef VICE_DEBUG
# define DEBUG_LOG_MESSAGE(_x) log_message _x
#else
# define DEBUG_LOG_MESSAGE(_x)
#endif

/*! \brief Helper macro for outputting verbose debugging messages */
#ifdef DEBUG_VERBOSE
# define DEBUG_VERBOSE_LOG_MESSAGE(_x) log_message _x
#else
# define DEBUG_VERBOSE_LOG_MESSAGE(_x)
#endif

/*! \brief specify the transmit state the ACIA is currently in
 
 \remark
 The numerical values must remain as they are!
 int_acia_tx() relies on them when testing and decrementing
 the in_tx variable!
 */
enum acia_tx_state {
	ACIA_TX_STATE_NO_TRANSMIT = 0, /*!< currently, there is no transmit processed */
	ACIA_TX_STATE_TX_STARTED = 1,  /*!< the transmit has already begun */
	ACIA_TX_STATE_DR_WRITTEN = 2   /*!< the data register has been written, but the byte written is not yet in transmit */
};

typedef struct acia_struct {
    alarm_t *alarm_tx; /*!< handling of the transmit (TX) alarm */
    alarm_t *alarm_rx; /*!< handling of the receive (RX) alarm */
    unsigned int int_num;     /*!< the (internal) number for the ACIA interrupt as returned by interrupt_cpu_status_int_new(). */

    int ticks;  /*!< number of clock ticks per char */
    int fd;             /*!< file descriptor used to access the RS232 physical device on the host machine */
    enum acia_tx_state in_tx;   /*!< indicates that a transmit is currently ongoing */
    int irq;
    uint8_t cmd;        /*!< value of the 6551 command register */
    uint8_t ctrl;       /*!< value of the 6551 control register */
    uint8_t rxdata;     /*!< data that has been received last */
    uint8_t txdata;     /*!< data prepared to be send */
    uint8_t status;     /*!< value of the 6551 status register */
    uint8_t ectrl;      /*!< value of the extended control register of the turbo232 card */
    uint8_t datamask;   /*!< Word length bitmask used on received and sent data */
    int alarm_active_tx;    /*!< 1 if TX alarm is set; else 0 */
    int alarm_active_rx;    /*!< 1 if RX alarm is set; else 0 */

    log_t log; /*!< the log where to write debugging messages */

    uint8_t last_read;  /*!< the byte read the last time (for RMW) */

    /******************************************************************/

    /*! \brief the clock value the TX alarm has last been set to fire at

     \note
      If alarm_active_tx is set to 1, to alarm is
      actually set. If alarm_active_tx is 0, then
      the alarm either has already fired, or it
      already has been cancelled.
    */
    CLOCK alarm_clk_tx;

    /*! \brief the clock value the RX alarm has last been set to fire at

     \note
      If alarm_active_rx is set to 1, to alarm is
      actually set. If alarm_active_rx is 0, then
      the alarm either has already fired, or it
      already has been cancelled.
    */
    CLOCK alarm_clk_rx;

    /*! \brief the arch-dependant RS232 device to use for this acia implementation */
    int device;

    /*! \brief the type of interrupt implemented by the ACIA

     The ACIA either implements an IRQ (IK_IRQ), an NMI (IK_NMI),
     or no interrupt at all (IK_NONE).

     \note
       As some cartridges can be switched between these modes,
       it is necessary to remember this value.
    */
    enum cpu_int irq_type;

    /*! \brief the type of interrupt implemented by the ACIA,
      as defined in the resources

      Essentially, this is the same info as acia_irq_type. As the
      resource stored in the VICE system is different from
      the actual value in acia_irq_type, the resources value is
      stored here.
    */
    int irq_res;

    /*! \brief the acia variant implemented.
      Specifies if this acia implements a "raw" 6551 device
      (ACIA_MODE_NORMAL), a swiftlink device (ACIA_MODE_SWIFTLINK)
      or a turbo232 device (ACIA_MODE_TURBO232).
    */
    int mode;

    /*! \brief The handshake lines as currently seen by the ACIA */
    enum rs232handshake_out rs232_status_lines;

} acia_type;

/******************************************************************/

static acia_type acia = { NULL, NULL, 0, 0, 0, (enum acia_tx_state)0,
                          0, 0, 0, 0, 0, 0, 0, 0xff, 0, 0, 0, 0, 0, 0, 0,
                          (enum cpu_int)0, 0, 0, (enum rs232handshake_out)0 };

static void acia_preinit(void)
{
    memset(&acia, 0, sizeof acia);

    acia.ticks = 21111;
    acia.fd = -1;
    acia.in_tx = ACIA_TX_STATE_NO_TRANSMIT;
    acia.log = LOG_DEFAULT;
    acia.irq_type = IK_NONE;
    acia.mode = ACIA_MODE_NORMAL;
}

static void int_acia_tx(CLOCK offset, void *data);
static void int_acia_rx(CLOCK offset, void *data);

/******************************************************************/

/*! \brief the bps rates available in the order of the control register
 
 This array is used to set the bps rate of the 6551.
 For this, the values are set in the same order as
 they are defined in the CONTROL register.
 
 \remark
 the first value is bogus. It should be 16*external clock.
 
 \remark
 swiftlink and turbo232 modes use the same table
 except they double the values.
 */
static const double acia_bps_table[16] = {
	10, 50, 75, 109.92, 134.58, 150, 300, 600, 1200, 1800,
	2400, 3600, 4800, 7200, 9600, 19200
};

/*! \brief the extra bps rates of the turbo232 card
 
 This lists the extra bps rates available in the
 turbo232 card. In the turbo232 card, if the CTRL register
 is set to the bps rate of 10 bps, the extended ctrl
 register determines the bps rates. The extended ctrl
 register is used as an index in this table to get the
 bps rate.
 
 \remark
 the last value is a bogus value and in the real module
 that value is reserved for future use.
 */
static const double t232_bps_table[4] = {
	230400, 115200, 57600, 28800
};

/******************************************************************/

/*! \internal \brief Change the device resource for this ACIA
 
 \param val
 The device no. to use for this ACIA
 
 \param param
 Unused
 
 \return
 0 on success, -1 on error.
 
 \remark
 This function is called whenever the resource
 MYACIA "Dev" is changed.
 */
static int acia_set_device(int val, void *param)
{
	if (val < 0 || val > 3) {
		return -1;
	}
	
	if (acia.fd >= 0) {
		log_error(acia.log,
				  "acia_set_device(): "
				  "Device open, change effective only after close!");
	}
	
	acia.device = val;
	return 0;
}


/*! \internal \brief Generate an ACIA interrupt
 
 This function is used to generate an ACIA interrupt.
 Depending upon the type of interrupt to generate
 (IK_NONE, IK_IRQ or IK_NMI), the appropriate function
 is called (or no function at all if the interrupt type
 is set to IK_NONE).
 
 \param aciairq
 The interrupt type to use.
 Must be one of IK_NONE, IK_IRQ or IK_NMI.
 
 \param int_num
 the (internal) number for the ACIA interrupt as returned
 by interrupt_cpu_status_int_new()
 
 \param value
 The state to set this interrupt to.\n
 
 \remark
 In case of aciairq = IK_NONE, value must be IK_NONE, too.\n
 In case of aciairq = IK_IRQ, value must be one of IK_IRQ or IK_NONE.\n
 In case of aciairq = IK_NMI, value must be one of IK_NMI or IK_NONE.\n
 */
static void acia_set_int(int aciairq, unsigned int int_num, int value)
{
    DEBUG_LOG_MESSAGE((acia.log, "acia_set_int(aciairq=%d, int_num=%u, value=%u",
        aciairq, int_num, value));
    assert((value == aciairq) || (value == IK_NONE));

    if (aciairq == IK_IRQ) {
        mycpu_set_irq(int_num, value);
    }
    if (aciairq == IK_NMI) {
        mycpu_set_nmi(int_num, value);
    }
}

/*! \internal \brief Change the Interrupt resource for this ACIA
 
 \param new_irq_res
   The interrupt type to use:
      0 = none,
      1 = NMI,
      2 = IRQ.

 \param param
 Unused
 
 \return
 0 on success, -1 on error.
 
 \remark
 This function is called whenever the resource
 MYACIA "Irq" is changed.
 */
#if (ACIA_MODE_HIGHEST == ACIA_MODE_TURBO232)
static int acia_set_irq(int new_irq_res, void *param)
{
    enum cpu_int new_irq;
    static const enum cpu_int irq_tab[] = { IK_NONE, IK_NMI, IK_IRQ };

    /*
     * if an invalid interrupt type has been given, return
     * with an error.
     */
    if ((new_irq_res < 0) || (new_irq_res > 2)) {
        return -1;
    }

    new_irq = irq_tab[new_irq_res];

    if (acia.irq_type != new_irq) {
        acia_set_int(acia.irq_type, acia.int_num, IK_NONE);
        if (new_irq != IK_NONE) {
            acia_set_int(new_irq, acia.int_num, new_irq);
        }
    }
    acia.irq_type = new_irq;
    acia.irq_res = new_irq_res;

    return 0;
}
#endif

/*! \internal \brief get the bps rate ("baud rate") of the ACIA
 
 \return
 the bps rate the acia is currently programmed to.
 */
static double get_acia_bps(void)
{
    switch (acia.mode) {
        case ACIA_MODE_NORMAL:
            return acia_bps_table[acia.ctrl & ACIA_CTRL_BITS_BPS_MASK];

        case ACIA_MODE_SWIFTLINK:
            return acia_bps_table[acia.ctrl & ACIA_CTRL_BITS_BPS_MASK] * 2;

        case ACIA_MODE_TURBO232:
            if ((acia.ctrl & ACIA_CTRL_BITS_BPS_MASK) == ACIA_CTRL_BITS_BPS_16X_EXT_CLK) {
                return t232_bps_table[acia.ectrl & T232_ECTRL_BITS_EXT_BPS_MASK];
            } else {
                return acia_bps_table[acia.ctrl & ACIA_CTRL_BITS_BPS_MASK] * 2;
            }

        default:
            log_message(acia.log, "Invalid acia.mode = %d in get_acia_bps()", acia.mode);
            return acia_bps_table[0]; /* return dummy value */
    }
}

/*! \internal \brief set the ticks between characters of the ACIA according to the bps rate
 
 Set the ticks that will pass for one character to be transferred
 according to the current ACIA settings.
 */
static void set_acia_ticks(void)
{
    unsigned int bits;

    DEBUG_LOG_MESSAGE((acia.log, "Setting ACIA to %u bps",
                       (unsigned int) get_acia_bps()));

    switch (acia.ctrl & ACIA_CTRL_BITS_WORD_LENGTH_MASK) {
        case ACIA_CTRL_BITS_WORD_LENGTH_8:
            bits = 8;
            break;
        case ACIA_CTRL_BITS_WORD_LENGTH_7:
            bits = 7;
            break;
        case ACIA_CTRL_BITS_WORD_LENGTH_6:
            bits = 6;
            break;
        default:
        /*
         * this case is only for gcc to calm down, as it wants to warn that
         * bits is used uninitialised - which it is not.
         */
        /* FALL THROUGH */
        case ACIA_CTRL_BITS_WORD_LENGTH_5:
            bits = 5;
            break;
    }

    /* set the data bitmask */

    acia.datamask = 0xff >> (8 - bits);

    /*
     * we neglect the fact that we might have 1.5 stop bits instead of 2
     *
     * FIX ME!!! 8-bit mode with parity should have only 1 stop bit
     */
    bits += 1 /* the start bit */
            + ((acia.cmd & ACIA_CMD_BITS_PARITY_ENABLED) ? 1 : 0) /* parity or not */
            + ((acia.ctrl & ACIA_CTRL_BITS_2_STOP) ? 2 : 1);   /* 1 or 2 stop bits */

    /*
     * calculate time in ticks for the data bits
     * including start, stop and parity bits.
     */
    acia.ticks = (int) (machine_get_cycles_per_second() / get_acia_bps() * bits);


    /* adjust the alarm rate for reception */
    if (acia.alarm_active_rx) {
        acia.alarm_clk_rx = myclk + acia.ticks;
        alarm_set(acia.alarm_rx, acia.alarm_clk_rx);
        acia.alarm_active_rx = 1;
    }

    /*
     * set the baud rate of the physical device
     */
    if (acia.fd >= 0) {
        rs232drv_set_bps(acia.fd, (unsigned int)get_acia_bps());
    }
}

/*! \internal \brief Change the emulation mode for this ACIA
 
 \param new_mode
 The exact device type to emulate.
 ACIA_MODE_NORMAL for a "normal" 6551 device,
 ACIA_MODE_SWIFTLINK for a SwiftLink device, or
 ACIA_MODE_TURBO232 for a Turbo232 device.
 
 \param param
 Unused
 
 \return
 0 on success, -1 on error.
 
 \remark
 This function is called whenever the resource
 MYACIA "Mode" is changed.
 */
#if (ACIA_MODE_HIGHEST == ACIA_MODE_TURBO232)
static int acia_set_mode(int new_mode, void *param)
{
	if (new_mode < ACIA_MODE_LOWEST || new_mode > ACIA_MODE_HIGHEST) {
		return -1;
	}
	
	if (myacia_set_mode(new_mode) == 0 && new_mode != ACIA_MODE_LOWEST) {
		return -1;
	}
	
	acia.mode = new_mode;
	set_acia_ticks();
	return 0;
}
#endif

/*! \brief integer resources used by the ACIA module */
static const resource_int_t resources_int[] = {
	{ MYACIA "Dev", MyDevice, RES_EVENT_NO, NULL,
		&acia.device, acia_set_device, NULL },
	RESOURCE_INT_LIST_END
};

/*! \brief initialize the ACIA resources
 
 \return
 0 on success, else -1.
 
 \remark
 Registers the integer resources
 */
int myacia_init_resources(void)
{
    acia_preinit();

    acia.irq_res = MyIrq;
    acia.irq_type = MyIrq;
    acia.mode = ACIA_MODE_NORMAL;

    return resources_register_int(resources_int);
}

/*! \brief the command-line options available for the ACIA */
static const cmdline_option_t cmdline_options[] =
{
    { "-myaciadev", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, MYACIA "Dev", NULL,
      "<0-3>", "Specify RS232 device this ACIA should work on" },
    CMDLINE_LIST_END
};

/*! \brief initialize the command-line options
 
 \return
 0 on success, else -1.
 
 \remark
 Registers the command-line options
 */
int myacia_init_cmdline_options(void)
{
	return cmdline_register_options(cmdline_options);
}

/******************************************************************/
/* auxiliary functions */

/*! \internal \brief Get the modem status and set the status register accordingly
 
 This function reads the physical modem status lines (DSR, DCD)
 and sets the emulated ACIA status register accordingly.
 
 \return
   The new value of the status register

 \todo
   Changes in DSR and DCD should trigger an interrupt
*/
static int acia_get_status(void)
{
    enum rs232handshake_in modem_status = 0;
#ifdef LOG_MODEM_STATUS
    static int oldstatus = -1;
#endif

    if (acia.fd >= 0) {
        modem_status = rs232drv_get_status(acia.fd);
    }
    acia.status &= ~(ACIA_SR_BITS_DCD | ACIA_SR_BITS_DSR);
#if 0
	/*
	 * CTS is very different from DCD.
	 * In the 6551, CTS is handled completely autonomously.
	 * It is not possible to determine its state from Software.
	 */
	if (modem_status & RS232_HSI_CTS) {
		acia.status |= ACIA_SR_BITS_DCD; /* we treat CTS like DCD */
	}
#endif
    if (!(modem_status & RS232_HSI_DCD)) {
        /* DCD mirrors DSR for C64/128 machines */
        switch (machine_class) {
            case VICE_MACHINE_C64:      /* fall through */
            case VICE_MACHINE_C64SC:    /* fall through */
            case VICE_MACHINE_SCPU64:   /* fall through */
            case VICE_MACHINE_C128:     /* fall through */
                acia.status |= ACIA_SR_BITS_DSR;
            break;
        /* Real DCD for other machines */
            default:
                acia.status |= ACIA_SR_BITS_DCD;
        }
    }

    if (!(modem_status & RS232_HSI_DSR)) {
        acia.status |= ACIA_SR_BITS_DSR;
    }


#ifdef LOG_MODEM_STATUS
    if (acia.status != oldstatus) {
        printf("acia_get_status(fd:%d): modem_status:%02x dcd:%d dsr:%d status:%02x dcd:%d dsr:%d\n",
               acia.fd, modem_status,
               modem_status & RS232_HSI_DCD ? 1 : 0,
               modem_status & RS232_HSI_DSR ? 1 : 0,
               acia.status,
               acia.status & ACIA_SR_BITS_DCD ? 1 : 0,
               acia.status & ACIA_SR_BITS_DSR ? 1 : 0
        );
        oldstatus = acia.status;
    }
#endif
    return acia.status;
}

/*! \internal \brief Set the handshake (output) lines to the status of the cmd register
 
 This function sets the physical handshake lines accordingly
 to the state of the emulated ACIA cmd register.
 */
static void acia_set_handshake_lines(void)
{
#ifdef LOG_MODEM_STATUS
    static int oldstatus = -1;
#endif
    switch (acia.cmd & ACIA_CMD_BITS_TRANSMITTER_MASK) {
        case ACIA_CMD_BITS_TRANSMITTER_NO_RTS:
            /* unset RTS, we are NOT ready to receive */
            acia.rs232_status_lines &= ~RS232_HSO_RTS;
            if (acia.alarm_active_rx) {
                /* disable RX alarm */
                /* receiver gets disabled after current character is completed */
                acia.alarm_active_rx = 0;
                /* disable and unset TX alarm */
                /* transmitter is disabled immediately */
                acia.alarm_active_tx = 0;
                alarm_unset(acia.alarm_tx);
            }
            break;

        case ACIA_CMD_BITS_TRANSMITTER_BREAK:
        /* FALL THROUGH */
        case ACIA_CMD_BITS_TRANSMITTER_TX_WITH_IRQ:
        /* FALL THROUGH */
        case ACIA_CMD_BITS_TRANSMITTER_TX_WO_IRQ:
            /* set RTS, we are ready to receive */
            acia.rs232_status_lines |= RS232_HSO_RTS;

            if (!acia.alarm_active_rx) {
                /* enable RX alarm */
                acia.alarm_active_rx = 1;
                set_acia_ticks();
            }
            /* start tx alarm */
            if (acia.alarm_active_tx == 0) {
                    acia.alarm_clk_tx = myclk + acia.ticks;
                    alarm_set(acia.alarm_tx, acia.alarm_clk_tx);
                    acia.alarm_active_tx = 1;
            }
            break;
    }

    if (acia.cmd & ACIA_CMD_BITS_DTR_ENABLE_RECV_AND_IRQ) {
        /* set DTR, we are ready to transmit and receive */
        acia.rs232_status_lines |= RS232_HSO_DTR;
    } else {
        /* unset DTR, we are NOT ready to receive or to transmit */
        acia.rs232_status_lines &= ~RS232_HSO_DTR;
    }

#ifdef LOG_MODEM_STATUS
    if (acia.rs232_status_lines != oldstatus) {
        printf("acia_set_handshake_lines(fd:%d): rs232 status:%02x dtr:%d rts:%d\n",
               acia.fd,
               acia.rs232_status_lines,
               acia.rs232_status_lines & RS232_HSO_DTR ? 1 : 0,
               acia.rs232_status_lines & RS232_HSO_RTS ? 1 : 0
        );
        oldstatus = acia.rs232_status_lines;
    }
#endif
    /* set the RTS and the DTR status */
    if (acia.fd >= 0) {
        rs232drv_set_status(acia.fd, acia.rs232_status_lines);
    }
}

/*! \brief initialize the ACIA */
void myacia_init(void)
{
    acia.int_num = interrupt_cpu_status_int_new(maincpu_int_status, MYACIA);

    acia.alarm_tx = alarm_new(mycpu_alarm_context, MYACIA, int_acia_tx, NULL);
    acia.alarm_rx = alarm_new(mycpu_alarm_context, MYACIA, int_acia_rx, NULL);

    if (acia.log == LOG_DEFAULT) {
        acia.log = log_open(MYACIA);
    }
}

/*! \brief reset the ACIA */
void myacia_reset(void)
{
    DEBUG_LOG_MESSAGE((acia.log, "reset_myacia"));

    acia.rs232_status_lines = 0;
    if (acia.fd >= 0) {
        rs232drv_set_status(acia.fd, acia.rs232_status_lines);
    }

    acia.cmd = ACIA_CMD_DEFAULT_AFTER_HW_RESET;
    acia.ctrl = ACIA_CTRL_DEFAULT_AFTER_HW_RESET;
    acia.ectrl = T232_ECTRL_DEFAULT_AFTER_HW_RESET;

    set_acia_ticks();

    /* ACIA Status Register after HW reset = 0xx10000 */
    acia.status &= ACIA_SR_BITS_DSR;    /* Keep DSR from emulated modem */
    acia.status |= ACIA_SR_BITS_DCD | ACIA_SR_DEFAULT_AFTER_HW_RESET; /* But disable DCD, closing the rs232drv will bring it up anyways */

    acia.in_tx = ACIA_TX_STATE_NO_TRANSMIT;

    if (acia.fd >= 0) {
        rs232drv_close(acia.fd);
    }
    acia.fd = -1;

    if (acia.alarm_tx) {
        alarm_unset(acia.alarm_tx);
    }
    if (acia.alarm_rx) {
        alarm_unset(acia.alarm_rx);
    }
    acia.alarm_active_tx = 0;
    acia.alarm_active_rx = 0;

    acia_set_int(acia.irq_type, acia.int_num, IK_NONE);
    acia.irq = 0;
}

/******************************************************************/
/* dump definitions and functions */

/* -------------------------------------------------------------------------- */
/* The dump format has a module header and the data generated by the
 * chip...
 *
 * The version of this dump description is 1.0
 */

#define ACIA_DUMP_VER_MAJOR      1 /*!< the major version number of the dump data */
#define ACIA_DUMP_VER_MINOR      1 /*!< the minor version number of the dump data */

/*
 * Layout of the dump data:
 *
 * UBYTE        TDR     Transmit Data Register
 * UBYTE        RDR     Receiver Data Register
 * UBYTE        SR      Status Register (includes state of IRQ line)
 * UBYTE        CMD     Command Register
 * UBYTE        CTRL    Control Register
 *
 * UBYTE        IN_TX   0 = no data to tx; 2 = TDR valid; 1 = in transmit (cf. enum acia_tx_state)
 *
 * QWORD        TICKSTX ticks till the next TDR empty interrupt
 *
 * QWORD        TICKSRX ticks till the next RDF empty interrupt
 *                      TICKSRX has been added with 2.0.9; if it does not
 *                      exist on read, it is assumed that it has the same
 *                      value as TICKSTX to emulate the old behaviour.
 */

/*! the name of this module */
static const char module_name[] = MYACIA;

/*! \brief Write the snapshot module for the ACIA
 
 \param p
 Pointer to the snapshot data
 
 \return
 0 on success, -1 on error
 
 \remark
 Is it sensible to put the ACIA into a snapshot? It is unlikely
 the "other side" of the connection will be able to handle this
 case if a transfer was under way, anyway.
 
 \todo FIXME!!!  Error check.
 
 \todo FIXME!!!  If no connection, emulate carrier lost or so.
 */
int myacia_snapshot_write_module(snapshot_t *p)
{
    snapshot_module_t *m;
    CLOCK act;
    CLOCK aar;

    m = snapshot_module_create(p, module_name, ACIA_DUMP_VER_MAJOR, ACIA_DUMP_VER_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (acia.alarm_active_tx) {
        act = acia.alarm_clk_tx - myclk;
    } else {
        act = 0;
    }

    if (acia.alarm_active_rx) {
        aar = acia.alarm_clk_rx - myclk;
    } else {
        aar = 0;
    }

    if (SMW_B(m, acia.txdata) < 0
            || SMW_B(m, acia.rxdata) < 0
            || SMW_B(m, (uint8_t)(acia_get_status()
                    | (acia.irq ? ACIA_SR_BITS_IRQ : 0))) < 0
            || SMW_B(m, acia.cmd) < 0
            || SMW_B(m, acia.ctrl) < 0
            || SMW_B(m, (uint8_t)(acia.in_tx)) < 0
            || SMW_CLOCK(m, act) < 0
            || SMW_CLOCK(m, aar) < 0) {
        snapshot_module_close(m);
        return -1;
    }


    return snapshot_module_close(m);
}

/*! \brief Read the snapshot module for the ACIA
 
 \param p
 Pointer to the snapshot data
 
 \return
 0 on success, -1 on error
 
 \remark
 The format has been extended in VICE 2.0.9.
 However, the version number remained the same.
 This function tries to read the new values.
 If they are not available, it mimics the old
 behaviour and reports success, anyway.
 
 \remark
 It is unclear if it is sensible to mimic the
 old behaviour, as the old implementation was
 severely broken.
 */
int myacia_snapshot_read_module(snapshot_t *p)
{
    uint8_t vmajor, vminor;
    uint8_t byte;
    CLOCK qword1;
    CLOCK qword2;
    snapshot_module_t *m;

    alarm_unset(acia.alarm_tx);   /* just in case we don't find module */
    alarm_unset(acia.alarm_rx);   /* just in case we don't find module */
    acia.alarm_active_tx = 0;
    acia.alarm_active_rx = 0;

    mycpu_set_int_noclk(acia.int_num, 0);

    m = snapshot_module_open(p, module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, ACIA_DUMP_VER_MAJOR, ACIA_DUMP_VER_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        snapshot_module_close(m);
        return -1;
    }

    if (SMR_B(m, &acia.txdata) < 0
            || SMR_B(m, &acia.rxdata) < 0
            || SMR_B(m, &acia.status) < 0
            || SMR_B(m, &acia.cmd) < 0
            || SMR_B(m, &acia.ctrl) < 0
            || SMR_B(m, &byte) < 0
            || SMR_CLOCK(m, &qword1) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    acia.irq = 0;
    if (acia.status & ACIA_SR_BITS_IRQ) {
        acia.status &= ~ACIA_SR_BITS_IRQ;
        acia.irq = 1;
        mycpu_set_int_noclk(acia.int_num, acia.irq_type);
    } else {
        mycpu_set_int_noclk(acia.int_num, 0);
    }

    if ((acia.cmd & ACIA_CMD_BITS_DTR_ENABLE_RECV_AND_IRQ) && (acia.fd < 0)) {
        acia.fd = rs232drv_open(acia.device);
        acia_set_handshake_lines();
    } else {
        if ((acia.fd >= 0) && !(acia.cmd & ACIA_CMD_BITS_DTR_ENABLE_RECV_AND_IRQ) && !rs232_useip232[acia.device]) {
            rs232drv_close(acia.fd);
            acia.fd = -1;
        }
    }

    set_acia_ticks();

    acia.in_tx = byte;

    if (qword1) {
        acia.alarm_clk_tx = myclk + qword1;
        alarm_set(acia.alarm_tx, acia.alarm_clk_tx);
        acia.alarm_active_tx = 1;

        /*
         * for compatibility reasons of old snapshots with new ones,
         * set the RX alarm to the same value.
         * if we have a new snapshot (2.0.9 and up), this will be
         * overwritten directly afterwards.
         */
        acia.alarm_clk_rx = myclk + qword1;
        alarm_set(acia.alarm_rx, acia.alarm_clk_rx);
        acia.alarm_active_rx = 1;
    }

    /*
     * this is new with VICE 2.0.9; thus, only use the settings
     * if it does exist.
     */
    if (SMR_CLOCK(m, &qword2) >= 0) {
        if (qword2) {
            acia.alarm_clk_rx = myclk + qword2;
            alarm_set(acia.alarm_rx, acia.alarm_clk_rx);
            acia.alarm_active_rx = 1;
        } else {
            alarm_unset(acia.alarm_rx);
            acia.alarm_active_rx = 0;
        }
    }

    return snapshot_module_close(m);
}


/*! \brief write the ACIA register values
  This function is used to write the ACIA values from the computer.

  \param addr
    The address of the ACIA register to write

  \param byte
    The value to set the register to
*/
void myacia_store(uint16_t addr, uint8_t byte)
{
    int acia_register_size;

    DEBUG_LOG_MESSAGE((acia.log, "store_myacia(%04x,%02x)", addr, byte));

    if (mycpu_rmw_flag) {
        myclk--;
        mycpu_rmw_flag = 0;
        myacia_store(addr, acia.last_read);
        myclk++;
    }

    if (acia.mode == ACIA_MODE_TURBO232) {
        acia_register_size = 7;
    } else {
        acia_register_size = 3;
    }

    switch (addr & acia_register_size) {
        case ACIA_DR:
            acia.txdata = byte;
            if (acia.cmd & ACIA_CMD_BITS_DTR_ENABLE_RECV_AND_IRQ) {
                if (acia.in_tx == ACIA_TX_STATE_DR_WRITTEN) {
                    log_message(acia.log, "ACIA: data register written "
                                "although data has not been sent yet.");
                }
                DEBUG_LOG_MESSAGE((acia.log, "DR write at %d: 0x%02x", myclk, acia.txdata));
                acia.in_tx = ACIA_TX_STATE_DR_WRITTEN;
                if (acia.alarm_active_tx == 0) {
                    acia.alarm_clk_tx = myclk + 1;
                    alarm_set(acia.alarm_tx, acia.alarm_clk_tx);
                    acia.alarm_active_tx = 1;
                }
                acia.status &= ~ACIA_SR_BITS_TRANSMIT_DR_EMPTY; /* clr TDRE */
            }
            break;
        case ACIA_SR:
            /* According the CSG and WDC data sheets, this is a programmed reset! */

            if ((acia.fd >= 0) && !rs232_useip232[acia.device]) {
                rs232drv_close(acia.fd);
                acia.fd = -1;
            }

            /* Status Register programmed reset = xxxxx0xx */
            acia.status &= ~ACIA_SR_BITS_OVERRUN_ERROR;
            /* Command Register programmed reset = xxx00000 */
            acia.cmd &= ACIA_CMD_BITS_PARITY_TYPE_MASK | ACIA_CMD_BITS_PARITY_ENABLED;
            /* This bit is set only in the MOS 6551, not the Rockwell 6551 or the 65c51 versions */
            /* acia.cmd |= ACIA_CMD_BITS_IRQ_DISABLED; */

            acia.in_tx = ACIA_TX_STATE_NO_TRANSMIT;
            acia_set_int(acia.irq_type, acia.int_num, IK_NONE);
            acia.irq = 0;
            if (acia.alarm_tx) {
                alarm_unset(acia.alarm_tx);
            }
            acia.alarm_active_tx = 0;
            acia_set_handshake_lines();
            break;
        case ACIA_CTRL:
            acia.ctrl = byte;
            set_acia_ticks();
            break;
        case ACIA_CMD:
            acia.cmd = byte;
            if ((acia.cmd & ACIA_CMD_BITS_DTR_ENABLE_RECV_AND_IRQ) && (acia.fd < 0)) {
                acia.fd = rs232drv_open(acia.device);
                /* enable RX alarm */
                acia.alarm_active_rx = 1;
                set_acia_ticks();
                /* Set Tx alarm if Tx IRQs are enabled */
                if ((acia.cmd & ACIA_CMD_BITS_TRANSMITTER_MASK) == ACIA_CMD_BITS_TRANSMITTER_TX_WITH_IRQ) {
                    acia.alarm_clk_tx = myclk + acia.ticks;
                    alarm_set(acia.alarm_tx, acia.alarm_clk_tx);
                    acia.alarm_active_tx = 1;
                    acia.in_tx = ACIA_TX_STATE_NO_TRANSMIT;
                }
            } else
            if ((acia.fd >= 0) && !(acia.cmd & ACIA_CMD_BITS_DTR_ENABLE_RECV_AND_IRQ) && !rs232_useip232[acia.device]) {
                rs232drv_close(acia.fd);
                alarm_unset(acia.alarm_tx);
                acia.alarm_active_tx = 0;
                acia.fd = -1;
            }
            /* Moved here so rs232drv status is always updated */
            acia_set_handshake_lines();
            break;
        case T232_ECTRL:
            if ((acia.ctrl & ACIA_CTRL_BITS_BPS_MASK) == ACIA_CTRL_BITS_BPS_16X_EXT_CLK) {
                acia.ectrl = byte;
                set_acia_ticks();
            }
    }
}

/*! \brief read the ACIA register values

  This function is used to read the ACIA values from the computer.
  All side-effects are executed.

  \param addr
    The address of the ACIA register to read

  \return
    The value the register has
*/
uint8_t myacia_read(uint16_t addr)
{
#if 0 /* def DEBUG */
    static uint8_t myacia_read_(uint16_t);
    uint8_t byte = myacia_read_(addr);
    static uint16_t last_addr = 0;
    static uint8_t last_byte = 0;

    if ((addr != last_addr) || (byte != last_byte)) {
        DEBUG_LOG_MESSAGE((acia.log, "read_myacia(%04x) -> %02x", addr, byte));
    }
    last_addr = addr; last_byte = byte;
    return byte;
}
static uint8_t myacia_read_(uint16_t addr)
{
#endif
    int acia_register_size;

    if (acia.mode == ACIA_MODE_TURBO232) {
        acia_register_size = 7;
    } else {
        acia_register_size = 3;
    }

    switch (addr & acia_register_size) {
        case ACIA_DR:
            DEBUG_LOG_MESSAGE((acia.log, "DR read at %d: 0x%02x", myclk, acia.rxdata));
            acia.status &= ~(ACIA_SR_BITS_OVERRUN_ERROR | ACIA_SR_BITS_PARITY_ERROR | ACIA_SR_BITS_FRAMING_ERROR | ACIA_SR_BITS_RECEIVE_DR_FULL);
            acia.last_read = acia.rxdata;
            return acia.rxdata;
        case ACIA_SR:
            {
                uint8_t c = acia_get_status() | (acia.irq ? ACIA_SR_BITS_IRQ : 0);
                DEBUG_LOG_MESSAGE((acia.log, "SR read at %d: 0x%02x", myclk,c));
                acia_set_int(acia.irq_type, acia.int_num, IK_NONE);
                acia.irq = 0;
                acia.last_read = c;
                return c;
            }
        case ACIA_CTRL:
            acia.last_read = acia.ctrl;
            return acia.ctrl;
        case ACIA_CMD:
            acia.last_read = acia.cmd;
            return acia.cmd;
        case T232_NDEF1:
        case T232_NDEF2:
        case T232_NDEF3:
            return 0xff;
        case T232_ECTRL:
            return acia.ectrl
                   + (((acia.ctrl & ACIA_CTRL_BITS_BPS_MASK) == ACIA_CTRL_BITS_BPS_16X_EXT_CLK)
                      ? T232_ECTRL_BITS_EXT_ACTIVE
                      : 0);
    }
    /* should never happen */
    return 0;
}

/*! \brief read the ACIA register values without side effects
  This function reads the ACIA values, so they can be accessed like
  an array of bytes. No side-effects that would be performed if a real
  read access would occur are executed.

  \param addr
    The address of the ACIA register to read

  \return
    The value the register has
*/
uint8_t myacia_peek(uint16_t addr)
{
    switch (addr & 3) {
        case ACIA_DR:
            return acia.rxdata;
        case ACIA_SR:
            {
                uint8_t c = acia.status | (acia.irq ? ACIA_SR_BITS_IRQ : 0);
                return c;
            }
        case ACIA_CTRL:
            return acia.ctrl;
        case ACIA_CMD:
            return acia.cmd;
    }
    return 0;
}

/******************************************************************/
/* alarm functions */

/*! \internal \brief Transmit (TX) alarm function
 
 This function is called when the transmit alarm fires.
 It checks if there is any data to send. If there is some,
 this data is sent to the physical RS232 device.
 
 \param offset
 The clock offset this alarm is executed at.
 
 The current implementation of the emulation core does
 not allow to guarantee that the alarm will fire exactly
 at the time it was scheduled at. The offset tells the
 alarm function how many cycles have passed since the
 time the alarm was scheduled to fire. Thus, (myclk - offset)
 yiels the clock count which the alarm was scheduled to.
 
 \param data
 Additional data defined in the call to alarm_new().
 For the acia implementation, this is always NULL.
 
 \remark
   If we just transmitted a value, the alarm is re-scheduled for
   the time when the transmission has completed. This way, we
   ensure that we do not send out faster than a real ACIA could
   do.

*/
static void int_acia_tx(CLOCK offset, void *data)
{
    DEBUG_VERBOSE_LOG_MESSAGE((acia.log, "int_acia_tx(offset=%ld, myclk=%d", offset, myclk));

    assert(data == NULL);

    if ((acia.in_tx == ACIA_TX_STATE_DR_WRITTEN) && (acia.fd >= 0)) {
        rs232drv_putc(acia.fd, acia.txdata & acia.datamask);

        /* tell the status register that the transmit register is empty */
        acia.status |= ACIA_SR_BITS_TRANSMIT_DR_EMPTY;
    }

    /* generate an interrupt if the ACIA was programmed to generate one */
    /* interrupt will occur everytime even if no data was transmitted */
    if ((acia.cmd & ACIA_CMD_BITS_TRANSMITTER_MASK) == ACIA_CMD_BITS_TRANSMITTER_TX_WITH_IRQ) {
        acia_set_int(acia.irq_type, acia.int_num, acia.irq_type);
        acia.irq = 1;
    }

    if (acia.in_tx != ACIA_TX_STATE_NO_TRANSMIT) {
        /*
         * ACIA_TX_STATE_DR_WRITTEN is decremented to ACIA_TX_STATE_TX_STARTED
         * ACIA_TX_STATE_TX_STARTED is decremented to ACIA_TX_STATE_NO_TRANSMIT
         */
        acia.in_tx--;
    }

    if ((acia.in_tx != ACIA_TX_STATE_NO_TRANSMIT) || ((acia.cmd & ACIA_CMD_BITS_TRANSMITTER_MASK) == ACIA_CMD_BITS_TRANSMITTER_TX_WITH_IRQ)) {
        /* re-schedule alarm */
        acia.alarm_clk_tx = myclk + acia.ticks;
        alarm_set(acia.alarm_tx, acia.alarm_clk_tx);
        acia.alarm_active_tx = 1;
    } else {
        alarm_unset(acia.alarm_tx);
        acia.alarm_active_tx = 0;
    }
}

/*! \internal \brief Receive (RX) alarm function
 
 This function is called when the receive alarm fires.
 It checks if there is any data received. If there is some,
 this data is made available in the ACIA data register (DR)
 
 \param offset
 The clock offset this alarm is executed at.
 
 The current implementation of the emulation core does
 not allow to guarantee that the alarm will fire exactly
 at the time it was scheduled at. The offset tells the
 alarm function how many cycles have passed since the
 time the alarm was scheduled to fire. Thus, (myclk - offset)
 yiels the clock count which the alarm was scheduled to.
 
 \param data
 Additional data defined in the call to alarm_new().
 For the acia implementation, this is always NULL.
 
 \remark
 The alarm is re-scheduled for the time when the reception
 has completed.
 */
static void int_acia_rx(CLOCK offset, void *data)
{
    DEBUG_VERBOSE_LOG_MESSAGE((acia.log, "int_acia_rx(offset=%ld, myclk=%d", offset, myclk));

    assert(data == NULL);

    do {
        uint8_t received_byte;

        if (acia.fd < 0) {
            break;
        }

        if (!rs232drv_getc(acia.fd, &received_byte)) {
            break;
        }

        DEBUG_LOG_MESSAGE((acia.log, "received byte: %u = '%c'.",
                           (unsigned) received_byte, received_byte));

        /* Datasheet (https://downloads.reactivemicro.com/Electronics/Interface%20Adapters/R65C51.pdf)
         * says that new data is discarded on overrun */
        if (!(acia.status & ACIA_SR_BITS_RECEIVE_DR_FULL)) {
            acia.rxdata = received_byte & acia.datamask;
        } else {
            acia.status |= ACIA_SR_BITS_OVERRUN_ERROR;
            DEBUG_LOG_MESSAGE((acia.log, "Overrun! Discarding received byte",
                           (unsigned) acia.rxdata, acia.rxdata));
        }

        /* generate an interrupt if the ACIA was configured to generate one */
        if (!(acia.cmd & ACIA_CMD_BITS_IRQ_DISABLED)) {
            acia_set_int(acia.irq_type, acia.int_num, acia.irq_type);
            acia.irq = 1;
        }


        acia.status |= ACIA_SR_BITS_RECEIVE_DR_FULL;
    } while (0);

    if (acia.alarm_active_rx == 1) {
        acia.alarm_clk_rx = myclk + acia.ticks;
        alarm_set(acia.alarm_rx, acia.alarm_clk_rx);
        /*acia.alarm_active_rx = 1;*/
    } else {
        alarm_unset(acia.alarm_rx);
    }
}

int acia_dump(void)
{
    uint8_t st;
    uint8_t wl;
    char* sb;
    const char* parity = "NONENMNS";
    char p;

    /* Status register */
    st = myacia_peek(0x01);
    /* Word length */
    wl = 8 - ((myacia_peek(0x03) & 0x60) >> 5);
    /* Parity bit */
    p = parity[(myacia_peek(0x02) & 0xE0) >> 5];
    /* Stop bit(s) */
    if (myacia_peek(0x03) & 0x80) {
        switch (wl) {
            case 8:
                if (p != 'N') {
                    sb = "1";
                } else {
                    sb = "2";
                }
                break;
            case 5:
                if (p == 'N') {
                    sb = "1.5";
                } else {
                    sb = "2";
                }
                break;
            default:
                sb = "2";
        }
    } else {
        sb = "1";
    }

    mon_out("Receive Interrupt: %s\n", (myacia_peek(0x02) & 0x02) ? "off" : "on");
    mon_out("DR Rx: %02x Status: %s\t%s\t%s\t%s\n", myacia_peek(0x00), (st & 0x08) ? "[Full]" : "[Not Full]", (st & 0x01) ? "[Parity Error]" : "",
        (st & 0x02) ? "[Framming Error]" : "", (st & 0x04) ? "[Overrun]" : "");
    mon_out("\nTransmit Interrupt: %s\n", ((myacia_peek(0x02) & 0x0c) == 0x04) ? "on" : "off");
    mon_out("DR Tx: %02x Status: %s\n", acia.txdata, (st & 0x10) ? "[Empty]" : "[Not Empty]");
    mon_out("\nRTS: %s\tDTR: %s\n", (myacia_peek(0x02) & 0x0c) ? "Low" : "High", (myacia_peek(0x02) & 0x01) ? "Low" : "High");
    mon_out("DCD: %s\tDSR: %s\n", (st & 0x20) ? "High" : "Low", (st & 0x40) ? "High" : "Low");
    mon_out("\nSpeed/format: %g bps / %u-%c-%s\n", get_acia_bps(), wl, p, sb);
    mon_out("Echo: %s\n", (myacia_peek(0x02) & 0x10) ? "On" : "Off");
    return 0;
}



////// aciacore.c ends here
///
///

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
/* Flag: Do we enable the ACIA RS232 interface emulation?  */
static int acia_enabled = 0;

/* Base address of the ACIA RS232 interface */
static int acia_base = 0xde00;

static char *acia_base_list = NULL;

#endif

/* ------------------------------------------------------------------------- */

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)

/* a prototype is needed (not anymore) */
uint8_t aciacart_read(uint16_t addr);
uint8_t aciacart_peek(uint16_t addr);

static io_source_t acia_device = {
    CARTRIDGE_NAME_ACIA,  /* name of the device */
    IO_DETACH_RESOURCE,   /* use resource to detach the device when involved in a read-collision */
    "Acia1Enable",        /* resource to set to '0' */
    0xde00, 0xde07, 0x07, /* range for the device, can be changed to other ranges */
    0,                    /* read validity is determined by the device upon a read */
    acia1_store,          /* store function */
    NULL,                 /* NO poke function */
    aciacart_read,        /* read function */
    aciacart_peek,        /* peek function */
    acia_dump,            /* device state information dump function */
    CARTRIDGE_ACIA,       /* cartridge ID */
    IO_PRIO_NORMAL,       /* normal priority, device read needs to be checked for collisions */
    0,                    /* insertion order, gets filled in by the registration function */
    IO_MIRROR_NONE        /* NO mirroring */
};

static io_source_list_t *acia_list_item = NULL;

static const export_resource_t export_res = {
    CARTRIDGE_NAME_TURBO232, 0, 0, &acia_device, NULL, CARTRIDGE_TURBO232
};
#endif

/* ------------------------------------------------------------------------- */

int aciacart_cart_enabled(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    return acia_enabled;
#else
    return 0;
#endif
}

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
static int acia1_enable(void)
{
    if (export_add(&export_res) < 0) {
        return -1;
    }
    acia_list_item = io_source_register(&acia_device);
    return 0;
}

static void acia1_disable(void)
{
    export_remove(&export_res);
    io_source_unregister(acia_list_item);
    acia_list_item = NULL;
}

static int set_io_source_base(int address)
{
    int adr = address;

    if (adr == 0xffff) {
        switch (machine_class) {
            case VICE_MACHINE_VIC20:
                adr = 0x9800;
                break;
            default:
                adr = 0xde00;
                break;
        }
    }

    switch (adr) {
        case 0xde00:
        case 0xdf00:
            acia_base = adr;
            acia_device.start_address = acia_base;
            if (acia_device.cart_id == CARTRIDGE_TURBO232) {
                acia_device.end_address = acia_base + 7;
            } else {
                acia_device.end_address = acia_base + 3;
            }
            return 0;
        case 0xd700:
            if (machine_class != VICE_MACHINE_C128) {
                return -1;
            }
            acia_base = adr;
            acia_device.start_address = acia_base;
            if (acia_device.cart_id == CARTRIDGE_TURBO232) {
                acia_device.end_address = acia_base + 7;
            } else {
                acia_device.end_address = acia_base + 3;
            }
            return 0;
        case 0x9800:
        case 0x9c00:
            if (machine_class != VICE_MACHINE_VIC20) {
                return -1;
            }
            acia_base = adr;
            acia_device.start_address = acia_base;
            if (acia_device.cart_id == CARTRIDGE_TURBO232) {
                acia_device.end_address = acia_base + 7;
            } else {
                acia_device.end_address = acia_base + 3;
            }
            return 0;
    }
    return -1;
}

static int set_acia_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if ((val) && (!acia_enabled)) {
        if (acia1_enable() < 0) {
            return -1;
        }
        acia_enabled = 1;
        acia1_reset();
    } else if ((!val) && (acia_enabled)) {
        acia1_disable();
        acia_enabled = 0;
    }
    return 0;
}

static int set_acia_base(int val, void *param)
{
    int temp;

    if (acia_enabled) {
        set_acia_enabled(0, NULL);
        temp = set_io_source_base(val);
        set_acia_enabled(1, NULL);
    } else {
        temp = set_io_source_base(val);
    }
    return temp;
}

static void set_io_source_mode(int mode)
{
    switch (mode) {
        default:
        case ACIA_MODE_NORMAL:
            acia_device.name = CARTRIDGE_NAME_ACIA;
            acia_device.start_address = acia_base;
            acia_device.end_address = acia_base + 3;
            acia_device.address_mask = 3;
            acia_device.cart_id = CARTRIDGE_ACIA;
            break;
        case ACIA_MODE_SWIFTLINK:
            acia_device.name = CARTRIDGE_NAME_SWIFTLINK;
            acia_device.start_address = acia_base;
            acia_device.end_address = acia_base + 3;
            acia_device.address_mask = 3;
            acia_device.cart_id = CARTRIDGE_SWIFTLINK;
            break;
        case ACIA_MODE_TURBO232:
            acia_device.name = CARTRIDGE_NAME_TURBO232;
            acia_device.start_address = acia_base;
            acia_device.end_address = acia_base + 7;
            acia_device.address_mask = 7;
            acia_device.cart_id = CARTRIDGE_TURBO232;
            break;
    }
}

int acia1_set_mode(int mode)
{
    if (acia_enabled) {
        set_acia_enabled(0, NULL);
        set_io_source_mode(mode);
        set_acia_enabled(1, NULL);
    } else {
        set_io_source_mode(mode);
    }
    return 1;
}
#endif

/* ------------------------------------------------------------------------- */

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
static resource_int_t resources_i[] = {
    { "Acia1Enable", 0, RES_EVENT_STRICT, 0,
      &acia_enabled, set_acia_enabled, NULL },
    { "Acia1Irq", MyIrq, RES_EVENT_NO, NULL,
      &acia.irq_res, acia_set_irq, NULL },
    { "Acia1Mode", ACIA_MODE_SWIFTLINK, RES_EVENT_NO, NULL,
      &acia.mode, acia_set_mode, NULL },
    /* CAUTION: position is hardcoded below */
    { "Acia1Base", 0xffff, RES_EVENT_STRICT, vice_int_to_ptr(0xffff),
      &acia_base, set_acia_base, NULL },
    RESOURCE_INT_LIST_END
};
#endif

int aciacart_resources_init(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    /* Set the default factory value depending on the machine. We do this
       here so the default value will not end up in the config file. */
    switch (machine_class) {
        case VICE_MACHINE_VIC20:
            resources_i[3].factory_value = 0x9800;
            break;
        default:
            resources_i[3].factory_value = 0xde00;
            break;
    }

    if (acia1_resources_init() < 0) {
        return -1;
    }
    return resources_register_int(resources_i);
#else
    return 0;
#endif
}

void aciacart_resources_shutdown(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (acia_base_list) {
        lib_free(acia_base_list);
    }
#endif
}

/* ------------------------------------------------------------------------- */

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
uint8_t aciacart_read(uint16_t addr)
{
    acia_device.io_source_valid = 0;
    if (acia.mode == ACIA_MODE_TURBO232 && (addr & 7 ) > 3 && (addr & 7) != 7) {
        return 0;
    }
    acia_device.io_source_valid = 1;
    return myacia_read(addr);
}

uint8_t aciacart_peek(uint16_t addr)
{
    if (acia.mode == ACIA_MODE_TURBO232 && (addr & 7 ) > 3 && (addr & 7) != 7) {
        return 0;
    }
    return acia1_peek(addr);
}
#endif

void aciacart_reset(void)
{
    acia1_reset();
}

void aciacart_init(void)
{
    acia1_init();
}

/* ------------------------------------------------------------------------- */

#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
static const cmdline_option_t cart_cmdline_options[] =
{
    { "-acia1irq", SET_RESOURCE, 1,
      NULL, NULL, "Acia1Irq", NULL,
      "<interrupt>", "Set the ACIA interrupt (0: None, 1: NMI, 2: IRQ)" },
    { "-acia1mode", SET_RESOURCE, 1,
      NULL, NULL, "Acia1Mode", NULL,
      "<Mode>", "Set the ACIA mode (0: Normal, 1: Swiftlink, 2: Turbo232)" },
    CMDLINE_LIST_END
};

static cmdline_option_t base_cmdline_options[] =
{
    { "-acia1base", SET_RESOURCE, 1,
      NULL, NULL, "Acia1Base", NULL,
      "<Base address>", NULL },
    CMDLINE_LIST_END
};
#endif

int aciacart_cmdline_options_init(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (machine_class == VICE_MACHINE_C128) {
        acia_base_list = lib_strdup("Set the base address of the ACIA cartridge. (0xD700, 0xDE00, 0xDF00)");
    } else if (machine_class == VICE_MACHINE_VIC20) {
        acia_base_list = lib_strdup("Set the base address of the ACIA cartridge. (0x9800, 0x9C00)");
    } else {
        acia_base_list = lib_strdup("Set the base address of the ACIA cartridge. (0xDE00, 0xDF00)");
    }

    base_cmdline_options[0].description = acia_base_list;

    if (cmdline_register_options(base_cmdline_options) < 0) {
          return -1;
    }

    if (cmdline_register_options(cart_cmdline_options) < 0) {
          return -1;
    }
#endif

    return acia1_cmdline_options_init();
}

/* ------------------------------------------------------------------------- */

void aciacart_detach(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    set_acia_enabled(0, NULL);
#endif
}

int aciacart_enable(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    return set_acia_enabled(1, NULL);
#else
    return 0;
#endif
}


int aciacart_disable(void)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    return set_acia_enabled(0, NULL);
#else
    return 0;
#endif
}


/* ------------------------------------------------------------------------- */

int aciacart_snapshot_write_module(struct snapshot_s *p)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (acia1_snapshot_write_module(p) < 0) {
        return -1;
    }
#endif
    return 0;
}

int aciacart_snapshot_read_module(struct snapshot_s *p)
{
#if defined(HAVE_RS232DEV) || defined(HAVE_RS232NET)
    if (acia1_snapshot_read_module(p) < 0) {
        acia_enabled = 0;
        return -1;
    }
    /* FIXME: Why do we need to do so???  */
    if (acia1_enable() == 0) {
        aciacart_reset();          /* Clear interrupts.  */
        acia_enabled = 1;
    }
#endif
    return 0;
}
