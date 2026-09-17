#!/bin/bash

#
# mkdoxy.sh - Generate doxygen documentation
#
# Written by
#  groepaz <groepaz@gmx.net>
#  compyx <b.wassink@ziggo.nl>
#
# This file is part of VICE, the Versatile Commodore Emulator.
# See README for copyright notice.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#  02111-1307  USA.
#

################################################################################
#
# to get the most out of doxygen we must make sure that it only "sees" the files
# which belong to a respective configuration. this is done by first specifying
# all directories which contain actual sourcefiles in the "getinputs" function,
# and then excluding files which are not required (eg for a specific emulator)
# in the "getexcludes" function.
#
# adding a new configuration:
# - update "getinputs" and make sure that for that config only directories which
#   contain sourcefiles for this config go into the INPUT variable.
# - generate the documentation, and look at the file list
# - update "getexcludes" and make sure that for this config all files which do
#   not belong to it are excluded and end up in the EXCLUDE variable.
# - generate the documentation again and verify all is ok :) 
# - add this config to the list of working ones in the howto
#
# http://www.stack.nl/~dimitri/doxygen/config.html - Doyxgen config
# http://qof.sourceforge.net/doxy/reference.html - Doxygen reference
################################################################################

################################################################################
# this function returns all directories which contain input files for a specifc
# configuration.
#
# $1    machine
# $2    port
# $3    ui
#
# returns path(es) in INPUT
################################################################################
function getinputs
{
INPUT=" ../src"
INPUT+=" mainpage.dox"

ARCH_INPUT=" ../src/arch"
ARCH_INPUT+=" ../src/arch/shared"
ARCH_INPUT+=" ../src/arch/shared/hotkeys"
ARCH_INPUT+=" ../src/arch/shared/hwsiddrv"
ARCH_INPUT+=" ../src/arch/shared/mididrv"
ARCH_INPUT+=" ../src/arch/shared/socketdrv"
ARCH_INPUT+=" ../src/arch/shared/sounddrv"

ARCH_GTK3_INPUT=" ../src/arch/gtk3"
ARCH_GTK3_INPUT+=" ../src/arch/gtk3/widgets"
ARCH_GTK3_INPUT+=" ../src/arch/gtk3/widgets/base"
ARCH_GTK3_INPUT+=" ../src/arch/gtk3/joystickdrv"

ARCH_SDL_INPUT=" ../src/arch/sdl"

INPUT+=" ../src/core"
INPUT+=" ../src/diag"
INPUT+=" ../src/diskimage"
INPUT+=" ../src/drive"
INPUT+=" ../src/drive/iec"
INPUT+=" ../src/fileio"
INPUT+=" ../src/fsdevice"
INPUT+=" ../src/gfxoutputdrv"
INPUT+=" ../src/hwsiddrv"
INPUT+=" ../src/iecbus"
INPUT+=" ../src/imagecontents"
INPUT+=" ../src/joyport"
INPUT+=" ../src/monitor"
INPUT+=" ../src/parallel"
INPUT+=" ../src/printerdrv"
INPUT+=" ../src/raster"
INPUT+=" ../src/rs232drv"
INPUT+=" ../src/rtc"
INPUT+=" ../src/samplerdrv"
INPUT+=" ../src/serial"
INPUT+=" ../src/sounddrv"
INPUT+=" ../src/tape"
INPUT+=" ../src/tapeport"
INPUT+=" ../src/userport"
INPUT+=" ../src/vdrive"
INPUT+=" ../src/video"

# external libs
LIB_INPUT=" ../src/lib"
LIB_INPUT+=" ../src/lib/linenoise-ng"
LIB_INPUT+=" ../src/lib/p64"

# chips
CRTC_INPUT=" ../src/crtc"
VDC_INPUT=" ../src/vdc"
VICII_INPUT=" ../src/vicii"
VICIISC_INPUT=" ../src/viciisc"

SID_INPUT=" ../src/resid"
SID_INPUT+=" ../src/sid"

DTVSID_INPUT=" ../src/resid-dtv"
DTVSID_INPUT+=" ../src/sid"

# machines

VSID_INPUT=" ../src/c64"

C64_INPUT=" ../src/c64"
C64_INPUT+=" ../src/c64/cart"
C64_INPUT+=" ../src/drive/iec/c64exp"
C64_INPUT+=" ../src/drive/iecieee"
C64_INPUT+=" ../src/drive/ieee"
C64_INPUT+=" ../src/drive/tcbm"

C128_INPUT=" ../src/c128"
C128_INPUT+=" ../src/c128/cart"
C128_INPUT+=" ../src/drive/iec128dcr"
C128_INPUT+=" ../src/drive/iecieee"
C128_INPUT+=" ../src/drive/ieee"
C128_INPUT+=" ../src/drive/tcbm"

DTV_INPUT=" ../src/c64dtv"
DTV_INPUT+=" ../src/drive/iecieee"
DTV_INPUT+=" ../src/drive/ieee"
DTV_INPUT+=" ../src/drive/tcbm"

CBM2_INPUT=" ../src/cbm2"
CBM2_INPUT+=" ../src/cbm2/cart"
CBM2_INPUT+=" ../src/drive/iecieee"
CBM2_INPUT+=" ../src/drive/ieee"
CBM2_INPUT+=" ../src/drive/tcbm"

PET_INPUT=" ../src/pet"
PET_INPUT+=" ../src/drive/iecieee"
PET_INPUT+=" ../src/drive/ieee"
PET_INPUT+=" ../src/drive/tcbm"

PLUS4_INPUT=" ../src/plus4"
PLUS4_INPUT+=" ../src/plus4/cart"
PLUS4_INPUT+=" ../src/drive/iec/plus4exp"
PLUS4_INPUT+=" ../src/drive/iecieee"
PLUS4_INPUT+=" ../src/drive/ieee"
PLUS4_INPUT+=" ../src/drive/tcbm"

VIC20_INPUT=" ../src/vic20"
VIC20_INPUT+=" ../src/vic20/cart"
VIC20_INPUT+=" ../src/drive/iecieee"
VIC20_INPUT+=" ../src/drive/ieee"
VIC20_INPUT+=" ../src/drive/tcbm"

C1541_INPUT=" ../src"

CARTCONV_INPUT=" ../src/tools/cartconv"

PETCAT_INPUT=" ../src/tools/petcat"

# machine
case "$1" in
"vsid")
    INPUT+="$VSID_INPUT"
    INPUT+="$SID_INPUT $VICII_INPUT"
   ;;
"x128")
    INPUT+="$C128_INPUT"
    INPUT+="$SID_INPUT $VICII_INPUT $VDC_INPUT"
   ;;
"x64")
    INPUT+="$C64_INPUT"
    INPUT+="$SID_INPUT $VICII_INPUT"
   ;;
"x64dtv")
    INPUT+="$DTV_INPUT"
    INPUT+="$DTVSID_INPUT $SID_INPUT"
   ;;
"x64sc")
    INPUT+="$C64_INPUT"
    INPUT+="$SID_INPUT $VICIISC_INPUT"
   ;;
"xcbm2")
    INPUT+="$CBM2_INPUT"
    INPUT+="$CRTC_INPUT"
   ;;
"xcbm5x0")
    INPUT+="$CBM2_INPUT"
    INPUT+="$SID_INPUT $VICII_INPUT"
   ;;
"xpet")
    INPUT+="$PET_INPUT"
    INPUT+="$CRTC_INPUT"
   ;;
"xplus4")
    INPUT+="$PLUS4_INPUT"
    INPUT+="$TED_INPUT"
   ;;
"xvic")
    INPUT+="$VIC20_INPUT"
    INPUT+="$VIC_INPUT"
   ;;
"xscpu64")
    INPUT+="$C64_INPUT"
    INPUT+="$SID_INPUT $VICII_INPUT"
   ;;
"cartconv")
    INPUT+="$CARTCONV_INPUT"
   ;;
"c1541")
    INPUT+="$C1541_INPUT"
   ;;
"petcat")
    INPUT+="$PETCAT_INPUT"
   ;;
*)
   ;;
esac


# port
case "$2" in
"linux")
    ARCH_INPUT+="$ARCH_UNIX_INPUT"
    ;;
"win32")
    ARCH_INPUT+="$ARCH_WIN32_INPUT"
    ;;
"osx")
    ARCH_INPUT+="$ARCH_OSX_INPUT"
    ;;
*)
    ;;
esac

# gui
case "$3" in
"gtk3")
    ARCH_INPUT+="$ARCH_GTK3_INPUT"
   ;;
"sdl")
    ARCH_INPUT+="$ARCH_SDL_INPUT"
   ;;
*)
   ;;
esac

    INPUT+="$ARCH_INPUT $LIB_INPUT"
    INCLUDE="$INPUT"
}

################################################################################
# this function returns all files which should be excluded from the input for a
# specific configuration.
#
# $1    machine
# $2    port
# $3    ui
#
# returns file(s) in EXCLUDE
################################################################################
function getexcludes
{
#echo "getting excludes for" $1 $2 $3

ALWAYS_EXCLUDE=" ../src/monitor/mon_lex.c"
ALWAYS_EXCLUDE+=" ../src/monitor/mon_parse.c"

ARCH_LINUX_EXCLUDE=" ../src/arch/shared/sounddrv/soundbeos.cc"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/sounddrv/soundbsp.cc"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/sounddrv/soundcoreaudio.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/sounddrv/sounddx.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/sounddrv/soundsun.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/sounddrv/soundwmm.c"
# FIXME: add non unix GTK3 stuff
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/archdep_win32.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/archdep_win32.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/dynlib-win32.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-osx-hid.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-osx-hid.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-osx-hidlib.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-osx-hidmgr.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-osx-hidutil.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-osx.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-osx.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-win32-dinput-handle.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-win32-dinput-handle.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-win32.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/joy-win32.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/rawnetarch_win32.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/gtk3/rs232-win32-dev.c"
# FIXME: add non unix SDL stuff

# FIXME: add non unix shared stuff
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/archdep_is_haiku.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/archdep_is_haiku.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/archdep_is_macos_bindist.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/archdep_is_macos_bindist.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/archdep_is_windows_nt.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/archdep_is_windows_nt.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/archdep_win32.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/dynlib-win32.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/macOS-util.h"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/macOS-util.m"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/rawnetarch_win32.c"
ARCH_LINUX_EXCLUDE+=" ../src/arch/shared/rs232-win32-dev.c"

GUI_GTK3_EXCLUDE+=" ../src/vice_sdl.h"

C64EXCLUDE+=" ../src/monitor/asm6502dtv.c"
C64EXCLUDE+=" ../src/monitor/asmz80.c"
C64EXCLUDE+=" ../src/monitor/mon_assemblez80.c"
C64EXCLUDE+=" ../src/c1541.c"
C64EXCLUDE+=" ../src/bin2c.c"
C64EXCLUDE+=" ../src/cartconv.c"
C64EXCLUDE+=" ../src/petcat.c"
C64EXCLUDE+=" ../src/c128ui.h"
C64EXCLUDE+=" ../src/cbm2ui.h"
C64EXCLUDE+=" ../src/petui.h"
C64EXCLUDE+=" ../src/plus4ui.h"
C64EXCLUDE+=" ../src/vic20ui.h"
C64EXCLUDE+=" ../src/z80regs.h"
C64EXCLUDE+=" ../src/mainviccpu.c"

X64EXCLUDE="$C64EXCLUDE"
X64EXCLUDE+=" ../src/c64/vsidstubs.c"
X64EXCLUDE+=" ../src/c64/vsidmem.c"
X64EXCLUDE+=" ../src/c64/vsid.c"
X64EXCLUDE+=" ../src/vsidui.h"

X64SCEXCLUDE="$C64EXCLUDE"
X64SCEXCLUDE+=" ../src/c64/vsidstubs.c"
X64SCEXCLUDE+=" ../src/c64/vsidmem.c"
X64SCEXCLUDE+=" ../src/c64/vsid.c"
X64SCEXCLUDE+=" ../src/c64/c64mem.c"
X64SCEXCLUDE+=" ../src/c64/c64model.c"
X64SCEXCLUDE+=" ../src/6510core.c"
X64SCEXCLUDE+=" ../src/vsidui.h"
X64SCEXCLUDE+=" ../src/maincpu.c"

VSIDEXCLUDE="$C64EXCLUDE"

TOOLSEXCLUDE=" ../src/6510core.c"
TOOLSEXCLUDE+=" ../src/6510dtvcore.c"
TOOLSEXCLUDE+=" ../src/65816core.c"
TOOLSEXCLUDE+=" ../src/65c02core.c"
TOOLSEXCLUDE+=" ../src/aciacore.c"
TOOLSEXCLUDE+=" ../src/alarm.c"
TOOLSEXCLUDE+=" ../src/attach.c"
TOOLSEXCLUDE+=" ../src/autostart.c"
TOOLSEXCLUDE+=" ../src/autostart-prg.c"
TOOLSEXCLUDE+=" ../src/clipboard.c"
TOOLSEXCLUDE+=" ../src/clkguard.c"
TOOLSEXCLUDE+=" ../src/cmdline.c"
TOOLSEXCLUDE+=" ../src/color.c"
TOOLSEXCLUDE+=" ../src/crc32.c"
TOOLSEXCLUDE+=" ../src/debug.c"
TOOLSEXCLUDE+=" ../src/digimaxcore.c"
TOOLSEXCLUDE+=" ../src/dma.c"
TOOLSEXCLUDE+=" ../src/embedded.c"
TOOLSEXCLUDE+=" ../src/event.c"
TOOLSEXCLUDE+=" ../src/findpath.c"
TOOLSEXCLUDE+=" ../src/fixpoint.c"
TOOLSEXCLUDE+=" ../src/fliplist.c"
TOOLSEXCLUDE+=" ../src/gcr.c"
TOOLSEXCLUDE+=" ../src/info.c"
TOOLSEXCLUDE+=" ../src/init.c"
TOOLSEXCLUDE+=" ../src/initcmdline.c"
TOOLSEXCLUDE+=" ../src/interrupt.c"
TOOLSEXCLUDE+=" ../src/ioutil.c"
TOOLSEXCLUDE+=" ../src/kbdbuf.c"
TOOLSEXCLUDE+=" ../src/keyboard.c"
TOOLSEXCLUDE+=" ../src/lib.c"
TOOLSEXCLUDE+=" ../src/machine-bus.c"
TOOLSEXCLUDE+=" ../src/machine.c"
TOOLSEXCLUDE+=" ../src/main65816cpu.c"
TOOLSEXCLUDE+=" ../src/main.c"
TOOLSEXCLUDE+=" ../src/mainc64cpu.c"
TOOLSEXCLUDE+=" ../src/maincpu.c"
TOOLSEXCLUDE+=" ../src/mainlock.c"
TOOLSEXCLUDE+=" ../src/mainviccpu.c"
TOOLSEXCLUDE+=" ../src/midi.c"
TOOLSEXCLUDE+=" ../src/network.c"
TOOLSEXCLUDE+=" ../src/opencbmlib.c"
TOOLSEXCLUDE+=" ../src/palette.c"
TOOLSEXCLUDE+=" ../src/piacore.c"
TOOLSEXCLUDE+=" ../src/ps2mouse.c"
TOOLSEXCLUDE+=" ../src/ram.c"
TOOLSEXCLUDE+=" ../src/rawfile.c"
TOOLSEXCLUDE+=" ../src/rawnet.c"
TOOLSEXCLUDE+=" ../src/resources.c"
TOOLSEXCLUDE+=" ../src/romset.c"
TOOLSEXCLUDE+=" ../src/screenshot.c"
TOOLSEXCLUDE+=" ../src/snapshot.c"
TOOLSEXCLUDE+=" ../src/socket.c"
TOOLSEXCLUDE+=" ../src/sound.c"
TOOLSEXCLUDE+=" ../src/sysfile.c"
TOOLSEXCLUDE+=" ../src/traps.c"
TOOLSEXCLUDE+=" ../src/usleep.c"
TOOLSEXCLUDE+=" ../src/util.c"
TOOLSEXCLUDE+=" ../src/vicefeatures.c"
TOOLSEXCLUDE+=" ../src/vsync.c"

TOOLSEXCLUDE+=" ../src/6510core.h"
TOOLSEXCLUDE+=" ../src/acia.h"
TOOLSEXCLUDE+=" ../src/alarm.h"
TOOLSEXCLUDE+=" ../src/archapi.h"
TOOLSEXCLUDE+=" ../src/attach.h"
TOOLSEXCLUDE+=" ../src/autostart.h"
TOOLSEXCLUDE+=" ../src/autostart-prg.h"
TOOLSEXCLUDE+=" ../src/c128ui.h"
TOOLSEXCLUDE+=" ../src/c64ui.h"
TOOLSEXCLUDE+=" ../src/cartio.h"
TOOLSEXCLUDE+=" ../src/catweaselmkiii.h"
TOOLSEXCLUDE+=" ../src/cbm2ui.h"
TOOLSEXCLUDE+=" ../src/cbmdos.h"
TOOLSEXCLUDE+=" ../src/cbmimage.h"
TOOLSEXCLUDE+=" ../src/cia.h"
TOOLSEXCLUDE+=" ../src/clipboard.h"
TOOLSEXCLUDE+=" ../src/clkguard.h"
TOOLSEXCLUDE+=" ../src/color.h"
TOOLSEXCLUDE+=" ../src/config.h"
TOOLSEXCLUDE+=" ../src/console.h"
TOOLSEXCLUDE+=" ../src/crc32.h"
TOOLSEXCLUDE+=" ../src/debug.h"
TOOLSEXCLUDE+=" ../src/diskconstants.h"
TOOLSEXCLUDE+=" ../src/diskimage.h"
TOOLSEXCLUDE+=" ../src/dma.h"
TOOLSEXCLUDE+=" ../src/dynlib.h"
TOOLSEXCLUDE+=" ../src/embedded.h"
TOOLSEXCLUDE+=" ../src/export.h"
TOOLSEXCLUDE+=" ../src/fileio.h"
TOOLSEXCLUDE+=" ../src/findpath.h"
TOOLSEXCLUDE+=" ../src/fixpoint.h"
TOOLSEXCLUDE+=" ../src/flash040.h"
TOOLSEXCLUDE+=" ../src/fliplist.h"
TOOLSEXCLUDE+=" ../src/fsdevice.h"
TOOLSEXCLUDE+=" ../src/fullscreen.h"
TOOLSEXCLUDE+=" ../src/gcr.h"
TOOLSEXCLUDE+=" ../src/gfxoutput.h"
TOOLSEXCLUDE+=" ../src/h6809regs.h"
TOOLSEXCLUDE+=" ../src/hardsid.h"
TOOLSEXCLUDE+=" ../src/iecbus.h"
TOOLSEXCLUDE+=" ../src/iecdrive.h"
TOOLSEXCLUDE+=" ../src/imagecontents.h"
TOOLSEXCLUDE+=" ../src/infocontrib.h"
TOOLSEXCLUDE+=" ../src/info.h"
TOOLSEXCLUDE+=" ../src/initcmdline.h"
TOOLSEXCLUDE+=" ../src/init.h"
TOOLSEXCLUDE+=" ../src/interrupt.h"
TOOLSEXCLUDE+=" ../src/ioutil.h"
TOOLSEXCLUDE+=" ../src/kbdbuf.h"
TOOLSEXCLUDE+=" ../src/keyboard.h"
TOOLSEXCLUDE+=" ../src/lib.h"
TOOLSEXCLUDE+=" ../src/log.h"
TOOLSEXCLUDE+=" ../src/machine-bus.h"
TOOLSEXCLUDE+=" ../src/machine-drive.h"
TOOLSEXCLUDE+=" ../src/machine.h"
TOOLSEXCLUDE+=" ../src/machine-printer.h"
TOOLSEXCLUDE+=" ../src/machine-video.h"
TOOLSEXCLUDE+=" ../src/main65816cpu.h"
TOOLSEXCLUDE+=" ../src/mainc64cpu.h"
TOOLSEXCLUDE+=" ../src/maincpu.h"
TOOLSEXCLUDE+=" ../src/main.h"
TOOLSEXCLUDE+=" ../src/mainlock.h"
TOOLSEXCLUDE+=" ../src/mem.h"
TOOLSEXCLUDE+=" ../src/mididrv.h"
TOOLSEXCLUDE+=" ../src/midi.h"
TOOLSEXCLUDE+=" ../src/monitor.h"
TOOLSEXCLUDE+=" ../src/mos6510dtv.h"
TOOLSEXCLUDE+=" ../src/mos6510.h"
TOOLSEXCLUDE+=" ../src/network.h"
TOOLSEXCLUDE+=" ../src/opencbm.h"
TOOLSEXCLUDE+=" ../src/opencbmlib.h"
TOOLSEXCLUDE+=" ../src/palette.h"
TOOLSEXCLUDE+=" ../src/parallel.h"
TOOLSEXCLUDE+=" ../src/parsid.h"
TOOLSEXCLUDE+=" ../src/petui.h"
TOOLSEXCLUDE+=" ../src/piacore.h"
TOOLSEXCLUDE+=" ../src/plus4ui.h"
TOOLSEXCLUDE+=" ../src/printer.h"
TOOLSEXCLUDE+=" ../src/ps2mouse.h"
TOOLSEXCLUDE+=" ../src/r65c02.h"
TOOLSEXCLUDE+=" ../src/ram.h"
TOOLSEXCLUDE+=" ../src/rawfile.h"
TOOLSEXCLUDE+=" ../src/rawnetarch.h"
TOOLSEXCLUDE+=" ../src/rawnet.h"
TOOLSEXCLUDE+=" ../src/resources.h"
TOOLSEXCLUDE+=" ../src/riot.h"
TOOLSEXCLUDE+=" ../src/romset.h"
TOOLSEXCLUDE+=" ../src/scpu64ui.h"
TOOLSEXCLUDE+=" ../src/screenshot.h"
TOOLSEXCLUDE+=" ../src/serial.h"
TOOLSEXCLUDE+=" ../src/sidcart.h"
TOOLSEXCLUDE+=" ../src/signals.h"
TOOLSEXCLUDE+=" ../src/snapshot.h"
TOOLSEXCLUDE+=" ../src/sound.h"
TOOLSEXCLUDE+=" ../src/ssi2001.h"
TOOLSEXCLUDE+=" ../src/sysfile.h"
TOOLSEXCLUDE+=" ../src/tape.h"
TOOLSEXCLUDE+=" ../src/tap.h"
TOOLSEXCLUDE+=" ../src/tpi.h"
TOOLSEXCLUDE+=" ../src/traps.h"
TOOLSEXCLUDE+=" ../src/types.h"
TOOLSEXCLUDE+=" ../src/uiapi.h"
TOOLSEXCLUDE+=" ../src/uicmdline.h"
TOOLSEXCLUDE+=" ../src/uicolor.h"
TOOLSEXCLUDE+=" ../src/uimon.h"
TOOLSEXCLUDE+=" ../src/usleep.h"
TOOLSEXCLUDE+=" ../src/util.h"
TOOLSEXCLUDE+=" ../src/via.h"
TOOLSEXCLUDE+=" ../src/vic20ui.h"
TOOLSEXCLUDE+=" ../src/vice-event.h"
TOOLSEXCLUDE+=" ../src/vicefeatures.h"
TOOLSEXCLUDE+=" ../src/vicemaxpath.h"
TOOLSEXCLUDE+=" ../src/vice_sdl.h"
TOOLSEXCLUDE+=" ../src/vicesocket.h"
TOOLSEXCLUDE+=" ../src/vicii.h"
TOOLSEXCLUDE+=" ../src/video.h"
TOOLSEXCLUDE+=" ../src/viewport.h"
TOOLSEXCLUDE+=" ../src/vsidui.h"
TOOLSEXCLUDE+=" ../src/vsyncapi.h"
TOOLSEXCLUDE+=" ../src/vsync.h"
TOOLSEXCLUDE+=" ../src/wdc65816.h"
TOOLSEXCLUDE+=" ../src/z80regs.h"
TOOLSEXCLUDE+=" ../src/zfile.h"
TOOLSEXCLUDE+=" ../src/zipcode.h"

TOOLSEXCLUDE+=" ../src/core"
TOOLSEXCLUDE+=" ../src/diag"
TOOLSEXCLUDE+=" ../src/diskimage"
TOOLSEXCLUDE+=" ../src/drive"
TOOLSEXCLUDE+=" ../src/drive/iec"
TOOLSEXCLUDE+=" ../src/fileio"
TOOLSEXCLUDE+=" ../src/gfxoutputdrv"
TOOLSEXCLUDE+=" ../src/hwsiddrv"
TOOLSEXCLUDE+=" ../src/iecbus"
TOOLSEXCLUDE+=" ../src/imagecontents"
TOOLSEXCLUDE+=" ../src/monitor"
TOOLSEXCLUDE+=" ../src/parallel"
TOOLSEXCLUDE+=" ../src/printerdrv"
TOOLSEXCLUDE+=" ../src/raster"
TOOLSEXCLUDE+=" ../src/rs232drv"
TOOLSEXCLUDE+=" ../src/rtc"
TOOLSEXCLUDE+=" ../src/samplerdrv"
TOOLSEXCLUDE+=" ../src/serial"
TOOLSEXCLUDE+=" ../src/sounddrv"
TOOLSEXCLUDE+=" ../src/tape"
TOOLSEXCLUDE+=" ../src/tapeport"
TOOLSEXCLUDE+=" ../src/userport"
TOOLSEXCLUDE+=" ../src/video"
TOOLSEXCLUDE+=" ../src/lib"
TOOLSEXCLUDE+=" ../src/lib/linenoise-ng"
# FIXME: add subdirs
TOOLSEXCLUDE+=" ../src/lib/p64"

TOOLSEXCLUDE+=" ../src/arch/gtk3"
TOOLSEXCLUDE+=" ../src/arch/gtk3/widgets"
TOOLSEXCLUDE+=" ../src/arch/gtk3/widgets/base"
TOOLSEXCLUDE+=" ../src/arch/sdl"

CARTCONVEXCLUDE="$TOOLSEXCLUDE"
CARTCONVEXCLUDE+=" ../src/c1541.c"
CARTCONVEXCLUDE+=" ../src/petcat.c"
CARTCONVEXCLUDE+=" ../src/vdrive"
CARTCONVEXCLUDE+=" ../src/fsdevice"
CARTCONVEXCLUDE+=" ../src/cbmdos.c"
CARTCONVEXCLUDE+=" ../src/cbmimage.c"
CARTCONVEXCLUDE+=" ../src/charset.c"
CARTCONVEXCLUDE+=" ../src/log.c"
CARTCONVEXCLUDE+=" ../src/zfile.c"
CARTCONVEXCLUDE+=" ../src/zipcode.c"
#CARTCONVEXCLUDE+=" ../src/charset.h"
#CARTCONVEXCLUDE+=" ../src/cmdline.h"

PETCATEXCLUDE="$TOOLSEXCLUDE"
PETCATEXCLUDE+=" ../src/c1541.c"
PETCATEXCLUDE+=" ../src/cartconv.c"
PETCATEXCLUDE+=" ../src/vdrive"
PETCATEXCLUDE+=" ../src/fsdevice"
PETCATEXCLUDE+=" ../src/cbmdos.c"
PETCATEXCLUDE+=" ../src/cbmimage.c"
#PETCATEXCLUDE+=" ../src/charset.c"
PETCATEXCLUDE+=" ../src/log.c"
PETCATEXCLUDE+=" ../src/zfile.c"
PETCATEXCLUDE+=" ../src/zipcode.c"
PETCATEXCLUDE+=" ../src/cartridge.h"
#PETCATEXCLUDE+=" ../src/charset.h"
#PETCATEXCLUDE+=" ../src/cmdline.h"

C1541EXCLUDE="$TOOLSEXCLUDE"
C1541EXCLUDE+=" ../src/cartconv.c"
C1541EXCLUDE+=" ../src/petcat.c"
#C1541EXCLUDE+=" ../src/cbmdos.c"
#C1541EXCLUDE+=" ../src/cbmimage.c"
#C1541EXCLUDE+=" ../src/charset.c"
#C1541EXCLUDE+=" ../src/log.c"
#C1541EXCLUDE+=" ../src/zfile.c"
#C1541EXCLUDE+=" ../src/zipcode.c"
C1541EXCLUDE+=" ../src/cartridge.h"
C1541EXCLUDE+=" ../src/charset.h"
C1541EXCLUDE+=" ../src/cmdline.h"

C64_GUI_GTK3_EXCLUDE=" ../src/arch/gtk3/c64dtvui.c"
C64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/c128ui.c"
C64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/cbm2ui.c"
C64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/cbm5x0ui.c"
C64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/petui.c"
C64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/plus4ui.c"
C64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/scpu64ui.c"
C64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/vic20ui.c"

X64_GUI_GTK3_EXCLUDE="$C64_GUI_GTK3_EXCLUDE"
X64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/c64scui.c"
X64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/vsidui.c"
X64_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/vsidui.h"

X64SC_GUI_GTK3_EXCLUDE="$C64_GUI_GTK3_EXCLUDE"
X64SC_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/c64ui.c"
X64SC_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/vsidui.c"
X64SC_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/vsidui.h"

VSID_GUI_GTK3_EXCLUDE="$C64_GUI_GTK3_EXCLUDE"
VSID_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/c64ui.c"
VSID_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/c64scui.c"

TOOLS_GUI_GTK3_EXCLUDE="$C64_GUI_GTK3_EXCLUDE"
TOOLS_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/vsidui.c"
TOOLS_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/vsidui.h"
TOOLS_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/c64ui.c"
TOOLS_GUI_GTK3_EXCLUDE+=" ../src/arch/gtk3/c64scui.c"

CARTCONV_GUI_GTK3_EXCLUDE="$TOOLS_GUI_GTK3_EXCLUDE"

C1541_GUI_GTK3_EXCLUDE="$TOOLS_GUI_GTK3_EXCLUDE"

PETCONV_GUI_GTK3_EXCLUDE="$TOOLS_GUI_GTK3_EXCLUDE"


# machine
case "$1" in
"vsid")
    MACHINE_EXCLUDE="$VSIDEXCLUDE"
    GUI_GTK3_EXCLUDE+="$VSID_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$VSID_GUI_SDL_EXCLUDE"
   ;;
"x128")
    MACHINE_EXCLUDE="$X128EXCLUDE"
    GUI_GTK3_EXCLUDE+="$X128_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$X128_GUI_SDL_EXCLUDE"
   ;;
"x64")
    MACHINE_EXCLUDE="$X64EXCLUDE"
    GUI_GTK3_EXCLUDE+="$X64_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$X64_GUI_SDL_EXCLUDE"
   ;;
"x64dtv")
    MACHINE_EXCLUDE="$X64DTVEXCLUDE"
    GUI_GTK3_EXCLUDE+="$X64DTV_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$X64DTV_GUI_SDL_EXCLUDE"
   ;;
"x64sc")
    MACHINE_EXCLUDE="$X64SCEXCLUDE"
    GUI_GTK3_EXCLUDE+="$X64SC_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$X64SC_GUI_SDL_EXCLUDE"
   ;;
"xcbm2")
    MACHINE_EXCLUDE="$XCBM2EXCLUDE"
    GUI_GTK3_EXCLUDE+="$XCBM2_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$XCBM2_GUI_SDL_EXCLUDE"
   ;;
"xcbm5x0")
    MACHINE_EXCLUDE="$XCBM5X0EXCLUDE"
    GUI_GTK3_EXCLUDE+="$XCBM5X0_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$XCBM5X0_GUI_SDL_EXCLUDE"
   ;;
"xpet")
    MACHINE_EXCLUDE="$XPETEXCLUDE"
    GUI_GTK3_EXCLUDE+="$XPET_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$XPET_GUI_SDL_EXCLUDE"
   ;;
"xplus4")
    MACHINE_EXCLUDE="$XPLUS4EXCLUDE"
    GUI_GTK3_EXCLUDE+="$XPLUS4_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$XPLUS4_GUI_SDL_EXCLUDE"
   ;;
"xvic")
    MACHINE_EXCLUDE="$XVICEXCLUDE"
    GUI_GTK3_EXCLUDE+="$XVIC_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$XVIC_GUI_SDL_EXCLUDE"
   ;;
"xscpu64")
    MACHINE_EXCLUDE="$XSCPU64EXCLUDE"
    GUI_GTK3_EXCLUDE+="$XSCPU64_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$XSCPU64_GUI_SDL_EXCLUDE"
   ;;
"cartconv")
    MACHINE_EXCLUDE="$CARTCONVEXCLUDE"
    GUI_GTK3_EXCLUDE+="$CARTCONV_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$CARTCONV_GUI_SDL_EXCLUDE"
   ;;
"c1541")
    MACHINE_EXCLUDE="$C1541EXCLUDE"
    GUI_GTK3_EXCLUDE+="$C1541_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$C1541_GUI_SDL_EXCLUDE"
   ;;
"petcat")
    MACHINE_EXCLUDE="$PETCATEXCLUDE"
    GUI_GTK3_EXCLUDE+="$PETCAT_GUI_GTK3_EXCLUDE"
    GUI_SDL_EXCLUDE+="$PETCAT_GUI_SDL_EXCLUDE"
   ;;
*)
   ;;
esac

# port
case "$2" in
"linux")
    ARCH_EXCLUDE="$ARCH_LINUX_EXCLUDE"
   ;;
"win32")
    ARCH_EXCLUDE="$ARCH_WIN32_EXCLUDE"
   ;;
"osx")
    ARCH_EXCLUDE="$ARCH_OSX_EXCLUDE"
   ;;
*)
   ;;
esac

# gui
case "$3" in
"gtk3")
    GUI_EXCLUDE="$GUI_GTK3_EXCLUDE"
   ;;
"sdl")
    GUI_EXCLUDE="$GUI_SDL_EXCLUDE"
   ;;
*)
   ;;
esac

EXCLUDE="$ALWAYS_EXCLUDE $MACHINE_EXCLUDE $ARCH_EXCLUDE $GUI_EXCLUDE"

}

################################################################################
# this function creates the documentation for one specific configuration
#
# $1    machine
# $2    port
# $3    ui
################################################################################
function makedocs
{
    echo "making docs for "$1" source ("$2", "$3") ["$VERSION"]" 
    OUTPUT="./doxy/"$1"/"
    rm -rf $OUTPUT
    mkdir -p $OUTPUT
    getinputs $1 $2 $3
    getexcludes $1 $2 $3
    echo "INPUT="$INPUT
    echo "INCLUDE_PATH="$INCLUDE
    echo "EXCLUDE="$EXCLUDE
    echo "PREDEFINED="$PREDEFINED
(cat Doxyfile ;\
     echo "INPUT=" $INPUT ;\
     echo "OUTPUT_DIRECTORY=" $OUTPUT ;\
     echo "INCLUDE_PATH=" $INCLUDE ;\
     echo "EXCLUDE=" $EXCLUDE ;\
     echo "PREDEFINED="$PREDEFINE ;\
     echo "PROJECT_NAME="$1 ;\
     echo "PROJECT_NUMBER="$VERSION ;\
    ) | doxygen -
}

################################################################################
# this function creates and index.html entry page in ./doxy
################################################################################

function makeindex
{
    echo "making index.html"
    OUTPUT="./doxy/index.html"

    echo "<html><head>" > $OUTPUT
    echo "<title>VICE doxy</title></head><body>" >> $OUTPUT
    echo "<h1>VICE doxy</h1>" >> $OUTPUT

    for I in x64 x64sc x64dtv xscpu64 x128 xcbm2 xcbm5x0 xpet xplus4 xvic vsid c1541 cartconv petcat; do \
        if [ -a ./doxy/$I ]; then \
            echo $I; \
            echo "<a href=\""$I"/html/index.html\">"$I"</a>" >> $OUTPUT; \
        fi; \
    done

    echo "</body></html>" >> $OUTPUT
}

################################################################################
# handle commandline arguments
################################################################################

# TODO: detect port/ui automatically
# TODO: optionally enable call graphs
# TODO: optionally enable source browser

# defaults
MACHINE="all"
PORT="linux"
GUI="gtk3"


# Check for --help
if [ "$1" = "--help" ]; then
    cat << EOF
Usage: ./mkdoxy.sh [machine=$MACHINE [port=$PORT [gui=$GUI]]] | clean | --help

    Where "machine" is one of 'all', 'tools', 'vsid', 'x128, 'x64', 'x64dtv',
    'x64sc', 'xcbm2', 'xcbm5x0', 'xpet', 'xplus4', 'xvic', 'xscpu64',
    'c1541', 'cartconv' or 'petcat', "port" is one of 'linux', 'win32' or
    'osx2', and "gui" is one of 'gtk3' or 'sdl'.

    When given '--help', this text is shown, when given 'clean', the generated
    files in doc/doxy are deleted.
EOF
    exit 0
fi


# Check for clean
if [ "$1" = "clean" ]; then
    echo "Cleaning Doxygen docs."
    rm -rfd ./doxy/*
    exit 0
fi


# machine
case "$1" in
"all")
   MACHINE="all"
   ;;
"tools")
   MACHINE="tools"
   ;;
"vsid")
   MACHINE="vsid"
   ;;
"x128")
   MACHINE="x128"
   ;;
"x64")
   MACHINE="x64"
   ;;
"x64dtv")
   MACHINE="x64dtv"
   ;;
"x64sc")
   MACHINE="x64sc"
   ;;
"xcbm2")
   MACHINE="xcbm2"
   ;;
"xcbm5x0")
   MACHINE="xcbm5x0"
   ;;
"xpet")
   MACHINE="xpet"
   ;;
"xplus4")
   MACHINE="xplus4"
   ;;
"xvic")
   MACHINE="xvic"
   ;;
"xscpu64")
   MACHINE="xscpu64"
   ;;
"cartconv")
   MACHINE="cartconv"
   ;;
"petcat")
   MACHINE="petcat"
   ;;
"c1541")
   MACHINE="c1541"
   ;;
*)
    # make sure we don't error out on an empty MACHINE, empty means 'all':
    if [ x"$1" != "x" ]; then
        echo "Error: unknown machine '$1'."
        echo "Use ./mkdoxy.sh --help to get a list of supported machines."
        exit 1
    fi
   ;;
esac


# port
case "$2" in
"linux")
   PORT="linux"
   GUI="gtk3"
   ;;
"win32")
   PORT="win32"
   GUI="gtk3"
   ;;
"osx")
   PORT="osx"
   GUI="gtk3"
   ;;
*)
   ;;
esac

# gui
case "$3" in
"gtk3")
   GUI="gtk3"
   ;;
"sdl")
   GUI="sdl"
   ;;
*)
   ;;
esac

VERSION=`grep " VERSION " ../src/config.h | sed 's:#define VERSION \"\(.*\)\":\1:'`
PREDEFINED=`cpp -dD < ../src/config.h | grep define | sed -s 's:#define ::g' | sed -s 's: :=:' | sed -s 's:=$::' | grep -v '__' | tr '\n' ' '`

#echo $MACHINE $PORT $GUI

case "$MACHINE" in
"all")
    makedocs "vsid" $PORT $GUI
    makedocs "x128" $PORT $GUI
    makedocs "x64" $PORT $GUI
    makedocs "x64dtv" $PORT $GUI
    makedocs "x64sc" $PORT $GUI
    makedocs "xcbm2" $PORT $GUI
    makedocs "xcbm5x0" $PORT $GUI
    makedocs "xpet" $PORT $GUI
    makedocs "xplus4" $PORT $GUI
    makedocs "xvic" $PORT $GUI
    makedocs "xscpu64" $PORT $GUI
    makedocs "petcat" $PORT $GUI
    makedocs "cartconv" $PORT $GUI
    makedocs "c1541" $PORT $GUI
   ;;
"tools")
    makedocs "petcat" $PORT $GUI
    makedocs "cartconv" $PORT $GUI
    makedocs "c1541" $PORT $GUI
   ;;
*)
    makedocs $MACHINE $PORT $GUI
   ;;
esac

makeindex
