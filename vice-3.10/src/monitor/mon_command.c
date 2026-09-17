/*
 * mon_command.c - The VICE built-in monitor command table.
 *
 * Written by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "console.h"
#include "lib.h"
#include "mon_command.h"
#include "asm.h"
#include "montypes.h"

#include "mon_parse.h"
#include "mon_util.h"
#include "uimon.h"
#include "util.h"

/* FIXME:
 * Removing `const` from `str` and `abbrev` fixes this warning:
../../../../vice/src/arch/gtk3/uimon.c: In function 'console_init':
../../../../vice/src/arch/gtk3/uimon.c:710:17: warning: to be safe all intermediate pointers in cast from 'char **' to 'const char **' must be 'const' qualified [-Wcast-qual
(const char **)&full_name,

  Also had to do some additional hacking, would be nice to see a better solution.
  See r36839.
*/
typedef struct mon_cmds_s {
    char *str;
    char *abbrev;
    const char *param_names;
    const char *description;
    const int filename_as_arg;
} mon_cmds_t;

#define NO_FILENAME_ARG    0
#define FILENAME_ARG       1

static const mon_cmds_t mon_cmd_array[] = {
    { "help", "?",
      "[<Command>]",
      "If no argument is given, prints out a list of all available commands."
      " If an argument is given, prints out specific help for that command.",
      NO_FILENAME_ARG
    },

    { "~", "",
      "<number>",
      "Display the specified number in decimal, hex, octal and binary.",
      NO_FILENAME_ARG
    },

    { "print", "p",
      "<expression>",
      "Evaluate the specified expression and output the result.",
      NO_FILENAME_ARG
    },

    { "", "",
      "",
      "Monitor state commands:",
      NO_FILENAME_ARG
    },

    { "device", "dev",
      "[c:|8:|9:|10:|11:]",
      "Set the default memory device to either the computer `c:' or the"
      " specified disk drive (`8:', `9:', `10:', `11:').\n"
      "Switches to computer when not given a device.\n",
      NO_FILENAME_ARG
    },

    { "exit", "x",
      NULL,
      "Leave the monitor and return to execution.",
      NO_FILENAME_ARG
    },

    { "quit", "q",
      NULL,
      "Exit the emulator immediately.",
      NO_FILENAME_ARG
    },

    { "radix", "rad",
      "[H|D|O|B]",
      "Set the default radix to hex, decimal, octal, or binary.  With no"
      " argument, the current radix is printed.",
      NO_FILENAME_ARG
    },

    { "sidefx", "sfx",
      "[on|off|toggle]",
      "Control how monitor generated reads affect memory locations that have"
      " read side-effects. If the argument is 'on' then reads may cause"
      " side-effects. If the argument is 'off' then reads don't cause"
      " side-effects. If the argument is 'toggle' then the current mode is"
      " switched. No argument displays the current state.",
      NO_FILENAME_ARG
    },

    { "log", "",
      "[on|off|toggle]",
      "Control whether the monitor output is logged into a logfile. If the"
      " argument is 'on' then all output will be written into the logfile. If"
      " the argument is 'off' then no log is produced. If the argument is"
      " 'toggle' then the current mode is switched. No argument displays the"
      " current state.",
      NO_FILENAME_ARG
    },

    { "logname", "",
      "\"<filename>\"",
      "Sets the filename of the logfile.",
      FILENAME_ARG
    },

#ifdef DEBUG
    { "maincpu_trace", "",
      "[on|off|toggle]",
      "Turn tracing of every instruction executed by the main CPU"
      " on or off. If the argument is 'toggle' then the current mode"
      " is switched.",
      NO_FILENAME_ARG
    },
#endif

    { "", "",
      "",
      "Machine state commands:",
      NO_FILENAME_ARG
    },

    { "dump", "",
      "\"<filename>\"",
      "Write a snapshot of the machine into the file specified."
      " This snapshot is compatible with a snapshot written out by the UI.\n"
      "Note: No ROM images are included into the dump.",
      FILENAME_ARG
    },

    { "undump", "",
      "\"<filename>\"",
      "Read a snapshot of the machine from the file specified.",
      FILENAME_ARG
    },

    { "bank", "",
      "[<memspace>] [bankname]",
      "If bankname is not given, print the possible banks for the memspace.\n"
      "If bankname is given set the current bank in the memspace to the given"
      " bank.",
      NO_FILENAME_ARG
    },

    { "cpu", "",
      "<Type>",
      "Specify the type of CPU currently used (6502/z80).",
      NO_FILENAME_ARG
    },

    { "backtrace", "bt",
      NULL,
      "Print JSR call chain (most recent call first). Stack offset"
      " relative to SP+1 is printed in parentheses. This is a best guess"
      " only.",
      NO_FILENAME_ARG
    },

    { "cpuhistory", "chis",
      "[<count>] [c:] [8:] [9:] [10:] [11:]",
      "Show <count> last executed commands on up to five devices."
      " If no devices are specified, then the default device is shown.\n"
      "VICE emulation runs each CPU for a variable number of cycles before"
      " switching between them. They will be synchronized when communication"
      " between them occurs.",
      NO_FILENAME_ARG
    },

    { "registers", "r",
      "[<reg_name> = <number> [, <reg_name> = <number>]*]",
      "Assign respective registers (use FL for status flags).  With no"
      " parameters, display register values.",
      NO_FILENAME_ARG
    },

    { "stopwatch", "sw",
      NULL,
      "Print the CPU cycle counter of the current device. 'reset' sets the counter to 0.",
      NO_FILENAME_ARG
    },

    { "goto", "g",
      "<address>",
      "Change the PC to ADDRESS and continue execution",
      NO_FILENAME_ARG
    },

    { "next", "n",
      "[<count>]",
      "Advance to the next instruction(s).  COUNT allows stepping"
      " more than a single instruction at a time. Subroutines are"
      " treated as a single instruction (\"step over\").",
      NO_FILENAME_ARG
    },

    { "return", "ret",
      NULL,
      "Continues execution and returns to the monitor just after the next"
      " RTS or RTI is executed (\"step out\").",
      NO_FILENAME_ARG
    },

    { "step", "z",
      "[<count>]",
      "Single-step through instructions. COUNT allows stepping"
      " more than a single instruction at a time (\"step into\").",
      NO_FILENAME_ARG
    },


    { "profile", "prof",
      "[on|off]|[flat [num]]|[graph [context] [depth]]|[func <function>]",
      "Main CPU profiling functions. Commands:\n"
      "prof on - Start profiling and flush old profiling data.\n"
      "prof off - Stop profiling.\n"
      "prof flat [<num=20>] - Show flat summary of 'num' top functions sorted by self time.\n"
      "prof graph [<ctx>] [depth <d>] Show callgraph up to 'd' levels deep. If 'ctx' is given, zoom on that subtree.\n"
      "prof func <function> - Show aggregate statistics for a function including callers and callees.\n"
      "prof disass <function> - Per-instruction profiling for function.\n"
      "prof context <ctx> - Detailed context information including "
      " per-instruction profiling for function"
      " in a call graph context.\n"
      "prof clear <function> - Clears all profiling stats for function.\n",
      NO_FILENAME_ARG
    },

    { "reset", "",
      "[<Type>]",
      "Reset the machine or drive. Type: 0 = reset, 1 = power cycle, 8-11 = reset drive.",
      NO_FILENAME_ARG
    },

    { "export", "exp",
      NULL,
      "Print out list of attached expansion port devices.",
      NO_FILENAME_ARG
    },

    { "cartfreeze", "",
      NULL,
      "Use cartridge freeze.",
      NO_FILENAME_ARG
    },

    { "updb", "",
      "<value>",
      "Update the simulated userport output value.",
      NO_FILENAME_ARG
    },

    { "jpdb", "",
      "<port> <value>",
      "Update the simulated joyport output value.",
      NO_FILENAME_ARG
    },

    { "keybuf", "",
      "<string>",
      "Put the specified string into the keyboard buffer.",
      NO_FILENAME_ARG
    },

    { "warp", "",
      "[on|off|toggle]",
      "Turn warp mode on or off. If the argument is 'toggle' then the current mode"
      " is toggled. When no argument is given the current mode is displayed.",
      NO_FILENAME_ARG
    },

    { "", "",
      "",
      "Symbol table commands:",
      NO_FILENAME_ARG
    },

    { "add_label", "al",
      "[<memspace>] <address> <label>",
      "<memspace> is one of: C: 8: 9: 10: 11:\n"
      "<address>  is the address which should get the label.\n"
      "<label>    is the name of the label; it must start with a dot (\".\").\n"
      "\n"
      "Map a given address to a label.  This label can be used when entering"
      " assembly code and is shown during disassembly.  Additionally, it can"
      " be used whenever an address must be specified.",
      NO_FILENAME_ARG
    },

    { "delete_label", "dl",
      "[<memspace>] <label>",
      "<memspace> is one of: C: 8: 9: 10: 11:\n"
      "<label>    is the name of the label; it must start with a dot (\".\").\n"
      "\n"
      "Delete a previously defined label.",
      NO_FILENAME_ARG
    },

    { "load_labels", "ll",
      "[<memspace>] \"<filename>\"",
      "Load a file containing a mapping of labels to addresses.  If no memory"
      " space is specified, the default readspace is used.\n"
      "\n"
      "The format of the file is the one written out by the `save_labels' command;"
      " it consists of some `add_label' commands, written one after the other.",
      FILENAME_ARG
    },

    { "save_labels", "sl",
      "[<memspace>] \"<filename>\"",
      "Save labels to a file.  If no memory space is specified, all of the"
      " labels are saved.",
      FILENAME_ARG
    },

    { "show_labels", "shl",
      "[<memspace>]",
      "Display current label mappings.  If no memory space is specified, show"
      " all labels from default address space.",
      NO_FILENAME_ARG
    },

    { "clear_labels", "cl",
      "[<memspace>]",
      "Clear current label mappings.  If no memory space is specified, clear"
      " all labels from default address space.",
      NO_FILENAME_ARG
    },

    { "", "",
      "",
      "Assembler and memory commands:",
      NO_FILENAME_ARG
    },

    { ">", "",
      "[<address>] <data_list>",
      "Write the specified data at `address'.",
      NO_FILENAME_ARG
    },

    { "a", "",
      "<address> [ <instruction> [: <instruction>]* ]",
      "Assemble instructions to the specified address.  If only one"
      " instruction is specified, enter assembly mode (enter an empty line to"
      " exit assembly mode).",
      NO_FILENAME_ARG
    },

    { "compare", "c",
      "<address_range> <address>",
      "Compare memory from the source specified by the address range to the"
      " destination specified by the address.  The regions may overlap.  Any"
      " values that miscompare are displayed using the default displaytype.",
      NO_FILENAME_ARG
    },

    { "disass", "d",
      "[<address> [<address>]]",
      "Disassemble instructions.  If two addresses are specified, they are"
      " used as a start and end address.  If only one is specified, it is"
      " treated as the start address and a default number of instructions are"
      " disassembled.  If no addresses are specified, a default number of"
      " instructions are disassembled from the dot address.",
      NO_FILENAME_ARG
    },

    { "fill", "f",
      "<address_range> <data_list>",
      "Fill memory in the specified address range with the data in"
      " <data_list>.  If the size of the address range is greater than the size"
      " of the data_list, the data_list is repeated.",
      NO_FILENAME_ARG
    },

    { "hunt", "h",
      "<address_range> <data_list>",
      "Hunt memory in the specified address range for the data in"
      " <data_list>.  If the data is found, the starting address of the match"
      " is displayed.  The entire range is searched for all possible matches."
      " xx can be used as a wildcard for a single byte.",
      NO_FILENAME_ARG
    },

    { "i", "",
      "<address_opt_range>",
      "Display memory contents as PETSCII text.",
      NO_FILENAME_ARG
    },

    { "ii", "",
      "<address_opt_range>",
      "Display memory contents as screen code text.",
      NO_FILENAME_ARG
    },

    { "mem", "m",
      "[<data_type>] [<address_opt_range>]",
      "Display the contents of memory.  If no datatype is given, the default"
      " is used.  If only one address is specified, the length of data"
      " displayed is based on the datatype.  If no addresses are given, the"
      " 'dot' address is used.\n"
      "Please note: due to the ambiguous meaning of 'b' and 'd' these data-"
      "types must be given in uppercase!",
      NO_FILENAME_ARG
    },

    { "memchar", "mc",
      "[<data_type>] [<address_opt_range>]",
      "Display the contents of memory as character data.  If only one address"
      " is specified, only one character is displayed.  If no addresses are"
      " given, the ``dot'' address is used.",
      NO_FILENAME_ARG
    },

    { "memmapsave", "mmsave",
      "\"<filename>\" <Format>",
      "Save the memmap as a picture. Format is:"
      " 0 = BMP, 1 = PCX, 2 = PNG, 3 = GIF, 4 = IFF.",
      FILENAME_ARG
    },

    { "memmapshow", "mmsh",
      "[<mask>] [<address_opt_range>]",
      "Show the memmap. The mask can be specified to show only those"
      " locations with accesses of certain type(s). The mask is a number"
      " with the bits \"ioRWXrwx\", where RWX are for ROM and rwx for RAM."
      " Optionally, an address range can be specified.",
      NO_FILENAME_ARG
    },

    { "memmapzap", "mmzap",
      NULL,
      "Clear the memmap.",
      NO_FILENAME_ARG
    },

    { "memsprite", "ms",
      "[<data_type>] [<address_opt_range>]",
      "Display the contents of memory as sprite data.  If only one address is"
      " specified, only one sprite is displayed.  If no addresses are given,"
      " the ``dot'' address is used.",
      NO_FILENAME_ARG
    },

    { "move", "t",
      "<address_range> <address>",
      "Move memory from the source specified by the address range to"
      " the destination specified by the address.  The regions may overlap.",
      NO_FILENAME_ARG
    },

    { "io", "",
      "<address>",
      "Print out the I/O area of the emulated machine.",
      NO_FILENAME_ARG
    },

    { "screen", "sc",
      NULL,
      "Displays the contents of the screen.",
      NO_FILENAME_ARG
    },


    { "", "",
      "",
      "Checkpoint commands:",
      NO_FILENAME_ARG
    },

    { "break", "bk",
      "[load|store|exec] [address [address] [if <cond_expr>]]",
      "Set a breakpoint, If no address is given, the currently valid break-"
      " points are printed.\n"
      " If an address is given, a breakpoint is set for that address and the"
      " breakpoint number is printed.\n"
      "`load|store|exec' is either `load', `store' or `exec' (or any combina"
      "tion of these) to specify on which operation the monitor breaks. If"
      " not specified, the monitor breaks on `exec'.\n"
      "A conditional expression can also be specified for the breakpoint."
      " For more information on conditions, see the CONDITION command.",
      NO_FILENAME_ARG
    },

    { "command", "",
      "<checknum> \"<Command>\"",
      "Specify `command' as the command to execute when checkpoint `checknum'"
      " is hit.  Note that the `x' command is not yet supported as a"
      " command argument.",
      NO_FILENAME_ARG
    },

    { "condition", "cond",
      "<checknum> if <cond_expr>",
    /* 12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
      "Each time the specified checkpoint is examined, the condition is evaluated. If"
      " it evalutes to true, the checkpoint is activated. Otherwise, it is ignored. If"
      " registers are specified in the expression, the values used are those at the"
      " time the checkpoint is examined, not when the condition is set.\n"
      "The condition can use registers (A, X, Y, PC, SP, FL and other cpu specific"
      " registers (see manual)) and compare them (==, !=, <, >, <=, >=) against other"
      " registers or constants. RL can be used to refer to the current rasterline,"
      " and CY refers to the current cycle in the line.\n"
      "Full expressions are also supported (+, -, *, /, &&, ||). This let's you f.e."
      " to check specific bits in the FL register using the bitwise boolean operators."
      " Paranthises are also supported in the expression."
      " Registers can be the registers of other devices; this is denoted by a memspace"
      " prefix (i.e., c:, 8:, 9:, 10:, 11:"
      " Examples: A == $0, X == Y, 8:X == X)\n"
      " You can also compare against the value of a memory location in a specific bank,"
      " i.e you can break only if the vic register $d020 is $f0."
      " use the form @[bankname]:[$<address>] | [.label].\n"
      " Note this is for the C : memspace only.\n"
      " Examples : if @io:$d020 == $f0, if @io:.vicBorder == $f0",
      NO_FILENAME_ARG
    },

    { "delete", "del",
      "<checknum>",
      "Delete checkpoint `checknum'. If no checkpoint is specified delete all checkpoints.",
      NO_FILENAME_ARG
    },

    { "disable", "dis",
      "<checknum>",
      "Disable checkpoint `checknum'. If no checkpoint is given all checkpoints will be disabled.",
      NO_FILENAME_ARG
    },

    { "enable", "en",
      "<checknum>",
      "Enable checkpoint `checknum'. If no checkpoint is given all checkpoints will be enabled.",
      NO_FILENAME_ARG
    },

    { "ignore", "",
      "<checknum> [<count>]",
      "Ignore a checkpoint a given number of crossings.  If no count is given,\n"
      "the default value is 1.",
      NO_FILENAME_ARG
    },

    { "until", "un",
      "[<address>]",
      "If no address is given, the currently valid breakpoints are printed."
      " If an address is given, a temporary breakpoint is set for that address"
      " and the breakpoint number is printed.  Control is returned to the"
      " emulator by this command.  The breakpoint is deleted once it is hit.",
      NO_FILENAME_ARG
    },

    { "watch", "w",
      "[load|store|exec] [address [address] [if <cond_expr>]]",
      "Set a watchpoint. If no address is given, the currently valid watch-"
      " points are printed. If a single address is specified, set a watchpoint"
      " for that address.  If two addresses are specified, set a watchpoint"
      " for the memory locations between the two addresses.\n"
      " `load|store|exec' is either `load', `store' or `exec' (or any combina"
      "tion of these) to specify on which operation the monitor breaks. If"
      " not specified, the monitor breaks on `load' and `store'.",
      NO_FILENAME_ARG
    },

    { "trace", "tr",
      "[load|store|exec] [address [address] [if <cond_expr>]]",
      "Set a tracepoint. If no address is given, the currently valid trace-"
      " points are printed. If a single address is specified, set a tracepoint"
      " for that address.  If two addresses are specified, set a tracepoint"
      " for the memory locations between the two addresses.\n"
      "`load|store|exec' is either `load', `store' or `exec' (or any combina"
      "tion of these) to specify on which operation the monitor breaks. If"
      " not specified, the monitor traces all three operations.",
      NO_FILENAME_ARG
    },

    { "dummy", "",
      "[on|off|toggle]",
      "Control whether the checkpoints will trigger on dummy accesses."
      " If the argument is 'on' then dummy accesses will cause checkpoints"
      " to trigger. If the argument is 'off' then dummy accesses will not"
      " trigger any checkpoints. If the argument is 'toggle' then the current"
      " mode is switched.  No argument displays the current state.",
      NO_FILENAME_ARG
    },

    { "", "",
      "",
      "File commands:",
      NO_FILENAME_ARG
    },

    { "cd", "",
      "<Directory>",
      "Change current working directory.",
      NO_FILENAME_ARG
    },

    { "pwd", "",
      NULL,
      "Show current working directory.",
      NO_FILENAME_ARG
    },

    { "mkdir", "",
      "<Directory>",
      "Create directory.",
      NO_FILENAME_ARG
    },

    { "rmdir", "",
      "<Directory>",
      "Remove directory.",
      NO_FILENAME_ARG
    },

    { "@", "",
      "<disk command>",
      "Perform a disk command on the currently attached disk image on virtual"
      " drive 8.",
      NO_FILENAME_ARG
    },

    { "attach", "",
      "\"<filename>\" <device>",
      "Attach file to device. (device 32 = cart)",
      FILENAME_ARG
    },

    { "autostart", "",
      "\"<filename>\" [file_index]",
      "Autostart a given disk/tape image or program.",
      FILENAME_ARG
    },

    { "autoload", "",
      "\"<filename>\" [file_index]",
      "Autoload given disk/tape image or program.",
      FILENAME_ARG
    },

    { "bload", "bl",
      "\"<filename>\" <device> <address>",
      "Load the specified file into memory at the specified address."
      " If device is 0, the file is read from the file system.",
      FILENAME_ARG
    },

    { "block_read", "br",
      "<track> <sector> [<address>]",
      "Read the block at the specified track and sector.  If an address is"
      " specified, the data is loaded into memory.  If no address is given, the"
      " data is displayed using the default datatype.",
      NO_FILENAME_ARG
    },

    { "bsave", "bs",
      "\"<filename>\" <device> <address1> <address2>",
      "Save the memory from address1 to address2 to the specified file."
      " If device is 0, the file is written to the file system.",
      FILENAME_ARG
    },

    { "bverify", "bv",
      "\"<filename>\" <device> <address>",
      "Compare the specified file with memory at the specified address."
      " If device is 0, the file is read from the file system.",
      FILENAME_ARG
    },

    { "block_write", "bw",
      "<track> <sector> <address>",
      "Write a block of data at `address' on the specified track and sector"
      " of disk in drive 8.",
      NO_FILENAME_ARG
    },

    { "detach", "",
      "<device>",
      "Detach file from device. (device 32 = cart)",
      NO_FILENAME_ARG
    },

    { "dir", "ls",
      "[<Directory>]",
      "Display the directory contents.",
      NO_FILENAME_ARG
    },

    { "list", "",
      "[<device>]",
      "List disk contents.",
      NO_FILENAME_ARG
    },

    { "load", "l",
      "\"<filename>\" <device> [<address>]",
      "Load the specified file into memory at the specified address."
      " Use (otherwise ignored) two-byte load address from file if no address"
      " specified.\n"
      "If device is 0, the file is read from the file system.",
      FILENAME_ARG
    },

    { "loadbasic", "ldb",
      "\"<filename>\" <device> [<address>]",
      "Load the specified file into memory at the specified address. Set BASIC"
      " pointers appropriately if loaded into computer memory (not all emulators)."
      " Use (otherwise ignored) two-byte load address from file if no address"
      " specified.\n"
      "If device is 0, the file is read from the file system.",
      FILENAME_ARG
    },

    { "save", "s",
      "\"<filename>\" <device> <address1> <address2>",
      "Save the memory from address1 to address2 to the specified file."
      " Write two-byte load address."
      " If device is 0, the file is written to the file system.",
      FILENAME_ARG
    },

    { "verify", "v",
      "\"<filename>\" <device> [<address>]",
      "Compare the specified file with memory at the specified address."
      " If device is 0, the file is read from the file system.",
      FILENAME_ARG
    },

    { "screenshot", "scrsh",
      "\"<filename>\" [<Format>]",
      "Take a screenshot. Format is:"
      " default = BMP, 1 = PCX, 2 = PNG, 3 = GIF, 4 = IFF.",
      FILENAME_ARG
    },

    { "tapectrl", "",
      "<Command>",
      "Control the datasette. Valid commands: "
      " 0 = stop, 1 = start, 2 = forward, 3 = rewind, 4 = record,"
      " 5 = reset, 6 = reset counter.",
      NO_FILENAME_ARG
    },

    { "tapeoffs", "",
      "<Offset>",
      "Sets attached .tap to given offset. When no offset is given, show the current offset.",
      NO_FILENAME_ARG
    },

    { "", "",
      "",
      "Command file commands:",
      NO_FILENAME_ARG
    },

    { "playback", "pb",
      "\"<filename>\"",
      "Monitor commands from the specified file are read and executed.  This"
      " command stops at the end of file or when a STOP command is read.",
      FILENAME_ARG
    },

    { "record", "rec",
      "\"<filename>\"",
      "After this command, all commands entered are written to the specified"
      " file until the STOP command is entered.",
      FILENAME_ARG
    },

    { "stop", "",
      NULL,
      "Stop recording commands.  See `record'.",
      NO_FILENAME_ARG
    },


    { "", "",
      "",
      "Resource state commands:",
      NO_FILENAME_ARG
    },

    { "resourceget", "resget",
      "\"<resource>\"",
      "Displays the value of the resource.",
      NO_FILENAME_ARG
    },

    { "resourceset", "resset",
      "\"<resource>\" \"<value>\"",
      "Sets the value of the resource.",
      NO_FILENAME_ARG
    },

    { "load_resources", "resload",
      "\"<filename>\"",
      "Loads resources from file.",
      FILENAME_ARG
    },

    { "save_resources", "ressave",
      "\"<filename>\"",
      "Saves resources to file.",
      FILENAME_ARG
    },

    { NULL, NULL, NULL, NULL, 0 }
};

int mon_get_nth_command(int index, char **full_name, char **short_name,
        int *takes_filename_as_arg)
{
    if (index < 0 || index >= sizeof(mon_cmd_array) / sizeof(*mon_cmd_array) - 1) {
        return 0;
    }
    *full_name = mon_cmd_array[index].str;
    *short_name = mon_cmd_array[index].abbrev;
    *takes_filename_as_arg = mon_cmd_array[index].filename_as_arg;
    return 1;
}

static int mon_command_lookup_index(const char *str)
{
    int num = 0;

    if (str == NULL) {
        return -1;
    }

    do {
        if ((util_strcasecmp(str, mon_cmd_array[num].str) == 0) ||
            (util_strcasecmp(str, mon_cmd_array[num].abbrev) == 0)) {
            return num;
        }
        num++;
    } while (mon_cmd_array[num].str != NULL);

    return -1;
}

void mon_out_wordwrap(const char *text);

/* FIXME: we should support some kind of token for marking suggested syllables */
void mon_out_wordwrap(const char *text)
{
    const char *cur = text;
    const char *end = text;
    int pos;
    /* FIXME: this should really be handled by the UI instead, ie the UI should
              always just give us valid numbers */
    static int last_known_xres = 40;
    if (console_log) {
        last_known_xres = console_log->console_xres;
    }
    while (*cur) {

        /* at the start of each line, first skip all white space */
        while ((*cur != 0) && ((*cur == ' ') || (*cur == '\n') || (*cur == '\r'))) {
            cur++;
        }

        /* first scan forward from current position, until we either hit a 0,
           a linebreak, or exceed the maximum width */
        end = cur;

        pos = 0;
        while ((*end != 0) && (*end != '\n') && (*end != '\r')) {
            end++;
            pos++;
            if (pos >= last_known_xres) {
                /* if we exceeded the maximum length, scan backwards,
                   until we hit a space, or the start of the line */
                if ((*end != 0) && (*end != 32) && (*end != '\n') && (*end != '\r')) {
                    while (*end != 32) {
                        end--;
                        pos--;
                        if (pos == 0) {
                            break;
                        }
                    }
                }
                break;
            }
        }
        if (pos == 0) {
            break;
        }
        if (*cur == 0) {
            break;
        }
        while ((*cur != 0) && (*cur != '\n') && (*cur != '\r')) {
            if (pos == 0) {
                break;
            }
            mon_out("%c", *cur);
            cur++;
            pos--;
        }
        mon_out("\n");
    }
}

static void print_help_details(const char *cmd, int cmd_num)
{
    const mon_cmds_t *mc;
    char *parameters;

    mc = &mon_cmd_array[cmd_num];

    if (mc->description == NULL) {
        mon_out("FIXME: No help available for `%s'\n", cmd);
        return;
    }

    if (mc->param_names == NULL) {
        parameters = NULL;
    } else {
        parameters = lib_strdup(mc->param_names);
    }

    mon_out("\nSyntax: %s %s\n", mc->str, parameters != NULL ? parameters : "");
    lib_free(parameters);
    if (!util_check_null_string(mc->abbrev)) {
        mon_out("Abbreviation: %s\n", mc->abbrev);
    }
    mon_out_wordwrap(mc->description);
}

static void print_help_topics(const char *filter)
{
    const mon_cmds_t *c;
    int column;
    int len;
    int longest;
    int max_col;
    size_t filter_len = 0;
    int isfirst = 1;
    int count = 0;
    int cmd_num = 0;
    int first_cmd_num = 0;
    const char *first_cmd = NULL;

    /* FIXME: this should really be handled by the UI instead, ie the UI should
              always just give us valid numbers */
    static int last_known_xres = 40;
    if (console_log) {
        last_known_xres = console_log->console_xres;
    }

    /* first find out what the longest string is */
    longest = 0;
    for (c = mon_cmd_array; c->str != NULL; c++) {
        len = (int)strlen(c->str);
        if (!util_check_null_string(c->abbrev)) {
            len += 3 + (int)strlen(c->abbrev); /* 3 => " ()" */
        }
        if (len > longest) {
            longest = len;
        }
    }
    longest += 1; /* one trailing space */
    max_col = last_known_xres / longest;

    if (filter) {
        filter_len = strlen(filter);
    }

    /* now print the list of commands */
    column = 0;
    for (c = mon_cmd_array; c->str != NULL; c++, cmd_num++) {
        int tot = (int)strlen(c->str);

        /* if filter is enabled, and neither the command, nor abbreviation,
           matches the filter, skip this line */
        if ((filter != NULL) &&
            strncmp(filter, c->str, filter_len) &&
            strncmp(filter, c->abbrev, filter_len)) {
            continue;
        }

        if (isfirst) {
            mon_out("Available commands are:\n\n");
            isfirst = 0;
        }

        /* "Empty" command, that's a head line, followed by newline  */
        if (tot == 0) {
            if (column != 0) {
                mon_out("\n");
                column = 0;
            }
            mon_out("\n%s\n", c->description);
            continue;
        }

        /* Command, followed by abbreviation */
        mon_out("%s", c->str);

        if (!util_check_null_string(c->abbrev)) {
            mon_out(" (%s)", c->abbrev);
            tot += 3 + (int)strlen(c->abbrev);
        }

        count++;
        if (count == 1) {
            first_cmd_num = cmd_num;
            first_cmd = c->str;
        }

        /* padding to next column */
        for (; tot < longest; tot++) {
            mon_out(" ");
        }
        column++;

        /* newline after max_col columns */
        if (column >= max_col) {
            mon_out("\n");
            column = 0;
        }

        if (mon_stop_output != 0) {
            break;
        }
    }
    mon_out("\n");

    if ((count == 0) && (filter != NULL)) {
        mon_out("No help available for '%s'. Use \"help\" to get the full list.\n", filter);
    } else if (count == 1) {
        print_help_details(first_cmd, first_cmd_num);
    } else if (count > 1) {
        mon_out("\nUse \"help <command>\" for details.\n");
    }
}

void mon_command_print_help(const char *cmd)
{
    if (cmd == NULL) {

        print_help_topics(NULL);

    } else {
        int cmd_num;

        cmd_num = mon_command_lookup_index(cmd);

        if (cmd_num == -1) {
            /* mon_out("Command `%s' unknown.\n", cmd); */
            print_help_topics(cmd);
        } else {
            print_help_details(cmd, cmd_num);
        }
    }
}
