#!/bin/sh
# vim: set et ts=2 sw=2 sts=2 syntax=bash fdm=marker:

#
# make_bindist_win32.sh -- Make a binary distribution for the Windows ports
#
# Written by
#  Bas Wassink <b.wassink@ziggo.nl>
# Based on code by
#  Marco van den Heuvel <blackystardust68@yahoo.com>
#  Greg King <gregdk@users.sf.net>
#
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

# {{{ Option variables

# --compiler:     The compiler used to build the executables
COMPILER=""

# --cpu:          CPU type
CPU=""

# --cross:        Are we cross compiling?
CROSS=""

# --enable-arch:  Have the executables been built with the --enable-arch
#                 configure flag?
ENABLE_ARCH=""

# --enable-x64:   Enable packaging of the deprecated x64 emulator
ENABLE_X64="no"

# --html-docs:    Enable distributing the documentation in HTML format
HTML_DOCS="no"

# --objdump:      Path to the objdump executable
OBJDUMP=""

# --sdl-config    Path to the sdl-config executable
#                 Only used when cross-compiling
SDL_CONFIG=""

# --strip:        Path to the strip executable
STRIP=":"

# --topbuilddir:  Top build directory
TOPBUILDDIR=""

# --topsrcdir:    Top source directory
TOPSRCDIR=""

# --ui:           User interface type
UI=""

# --verbose       Be verbose
VERBOSE="no"

# --vice-version: VICE version number
VICE_VERSION=""

# --zip-tool:     Compression tool to use
ZIP_TOOL="nozip"
# }}}

# {{{ Runtime variables

# Windows version, either win32 or win64
BINDIST_WINVER=""

# Bindist prefix
BINDIST_PREFIX=""

# Bindist VICE version (including SVN revision if applicable)
BINDIST_VICE_VERSION=""

# Default emulators (x64 gets added when required via --enable-x64)
BINDIST_EMUS="x64sc xscpu64 x64dtv x128 xcbm2 xcbm5x0 xpet xplus4 xvic vsid"

# Tools
BINDIST_TOOLS="c1541 tools/cartconv/cartconv tools/petcat/petcat"

# Emulator data directories (ROMs, ROMSets, keymaps, joymaps)
BINDIST_EMU_DATA_DIRS="C128 C64 C64DTV CBM-II DRIVES PET PLUS4 PRINTER SCPU64 VIC20"

# Bindist DLLs
BINDIST_DLLS=""

# Documentation
BINDIST_DOCS="COPYING NEWS README"

# }}}

# Set bindist Windows version to either 'win32' or 'win64'
set_bindist_windows_version()
{
  case "$CPU" in
    x86_64|amd64) BINDIST_WINVER="win64";;
    *) BINDIST_WINVER="win32";;
  esac
}


# Set bindist prefix to '[SDL|SDL2|GTK3]VICE'
set_bindist_prefix()
{
  BINDIST_PREFIX=$(echo "${UI}VICE" | tr '[:lower:]' '[:upper:]')
}

# Set VICE version including any revision if building from SVN or Git
#
set_bindist_vice_version()
{
  svn_suffix=""

  # check for SVN revision
  svn_rev_string=$(svnversion $TOPSRCDIR)
  if [ $? -eq 0 ]; then
    # okay, we have an SVN revision, get the number
    svn_rev=$(echo "$svn_rev_string" | sed 's/^\([0-9]*:\)*\([0-9]*\)*.*/\2/')
    if [ -n "$svn_rev" ]; then
      svn_suffix="-r$svn_rev"
    fi
  fi

  # If we didn't get an SVN revision, we might be building from a Git Action
  if [ "$svn_suffix" = "" ]; then
    svn_git_hash=$(git -C "$TOPSRCDIR" log --grep='git-svn-id:' -n 1 --pretty=format:"%H")
    if [[ $? -eq 0 && "$svn_git_hash" != "" ]]; then
      svn_suffix="-r$(git svn find-rev $svn_git_hash)"
    fi
  fi

  BINDIST_VICE_VERSION="${VICE_VERSION}${svn_suffix}"
}


# Create bindist dir, remove first if it already exists
function create_bindist_dir()
{
  if [ "$VERBOSE" = "yes" ]; then
    echo "Creating directory $BINDIST_DIR"
  fi
  if [ -e "$BINDIST_DIR" ]; then
    if [ "$VERBOSE" = "yes" ]; then
      echo ".. Removing old directory first"
    fi
    rm -rfd "$BINDIST_DIR"
  fi
  mkdir "$BINDIST_DIR"
  if [ $? -ne 0 ]; then
    echo "$(basename $0): error: failed to remove $BINDIST_DIR"
    exit 1
  fi
  if [ "$VERBOSE" = "yes" ]; then
    echo "OK."
  fi
}


# Debug hook: show values of the option variables
#
show_option_values()
{
  cat <<EOF
  Option values:
    compiler    : $COMPILER
    cpu         : $CPU
    cross       : $CROSS
    enable-arch : $ENABLE_ARCH
    enable-x64  : $ENABLE_X64
    html_docs   : $HTML_DOCS
    objdump     : $OBJDUMP
    sdl-config  : $SDL_CONFIG
    strip       : $STRIP
    topbuilddir : $TOPBUILDDIR
    topsrcdir   : $TOPSRCDIR
    ui          : $UI
    verbose     : $VERBOSE
    vice-version: $VICE_VERSION
    zip-tool    : $ZIP_TOOL
EOF
}


# Show usage message
usage()
{
  cat <<EOF
Usage: $(basename $0) [options]

The following options are available:

  --help            Show this text and exit

  --compiler        Compiler used (required)
  --cpu             CPU targeted
  --cross           VICE is being cross-compiled [yes, no] (required)
  --enable-arch     Status of the --enable-arch configure flag
  --enable-x64      Provide the deprecated x64 emulator (default=no)
  --html-docs       Provide the documentation in HTML format (default=no)
  --objdump         Path to the objdump executable (required)
  --sdl-config      Path to the sdl-config executable (required)
  --strip           Path to the strip executable (default=no)
  --topbuilddir     Top build directory (required)
  --topsrcdir       Top source directory (required)
  --ui              Select UI [sdl1, sdl2, gtk3] (required)
  --verbose         Verbose output (default=no)
  --vice-version    VICE version string without SVN revision (required)
  --zip-tool        Compression tool to use (default=nozip)
EOF
  exit 0
}


# Create the directory tree inside the bindist directory
create_bindist_subdirs()
{
  create_dir()
  {
    if [[ ! -z "$1" && ! "$1" = "." ]]; then
      if [ "$VERBOSE" = "yes" ]; then
        echo ".. creating dir '$BINDIST_DIR/$1'"
      fi
      mkdir -p "$BINDIST_DIR/$1"
    fi
  }

  create_dir "$BINDIST_EXE_DIR"
  create_dir "$BINDIST_DLL_DIR"
  create_dir "$BINDIST_DATA_DIR"
  create_dir "$BINDIST_DOC_DIR"
}


# Copy emulator executables from the build directory to the bindist directory
#
# @param $1: string with emulator names
#
copy_emulators()
{
  if [ "$VERBOSE" = "yes" ]; then
    echo "Copying emulators: $1"
  fi
  for emu in $1; do
    if [ "$VERBOSE" = "yes" ]; then
      echo ".. copying $TOPBUILDDIR/src/${emu}.exe"
    fi
    cp "$TOPBUILDDIR/src/$emu" $BINDIST_DIR/$BINDIST_EXE_DIR
    $STRIP $BINDIST_DIR/$BINDIST_EXE_DIR/${emu}.exe
  done
}


# Copy tool executables from the build directory to the bindist directory
#
# @param $1: string with tool names/paths
#
copy_tools()
{
  if [ "$VERBOSE" = "yes" ]; then
    echo "Copying tools: $1"
  fi
  for tool in $1; do
    if [ "$VERBOSE" = "yes" ]; then
      echo ".. copying $TOPBUILDDIR/src/${tool}.exe"
    fi
    cp "$TOPBUILDDIR/src/${tool}.exe" $BINDIST_DIR/$BINDIST_EXE_DIR
    $STRIP $BINDIST_DIR/$BINDIST_EXE_DIR/$(basename $tool).exe
  done
}


# Copy required DLLs into the bindist directory
copy_dlls()
{
  if [ "$VERBOSE" = "yes" ]; then
    echo "Copying DLLs"
  fi
  BINDIST_DLLS=""
  archdep_find_dlls
  for dll in $BINDIST_DLLS; do
    if [ "$VERBOSE" = "yes" ]; then
      echo ".. copying $dll"
    fi
    cp $dll $BINDIST_DIR/$BINDIST_DLL_DIR
  done
}


# Copy data directories of the emulators (ROMs, keymaps etc)
# Also remove unwanted files (Makefile*, other UI's keymaps)
#
copy_emu_data_dirs()
{
  if [ "$VERBOSE" = "yes" ]; then
    echo "Copying data directories"
  fi
  for datadir in $BINDIST_EMU_DATA_DIRS; do
    if [ "$VERBOSE" = "yes" ]; then
      echo ".. copying data from $datadir"
    fi
    # Copy all files (including Makefiles and files used for other UIs/ports)
    cp -R $TOPSRCDIR/data/$datadir $BINDIST_DIR/$BINDIST_DATA_DIR
    # Clean up
    archdep_clean_emu_data_dir "$BINDIST_DIR/$BINDIST_DATA_DIR/$datadir"
  done
}


# Copy documentation
#
copy_docs()
{
  if [ "$VERBOSE" = "yes" ]; then
    echo "Copying docs"
  fi

  # Copy docs independent of port
  for doc in $BINDIST_DOCS; do
    if [ "$VERBOSE" = "yes" ]; then
      echo ".. copying $TOPSRCDIR/$doc"
    fi
    cp $TOPSRCDIR/$doc $BINDIST_DIR
  done

  # Copy UI-dependent files
  archdep_copy_docs
}


#------------------------------------------------------------------------------
# Script entry point
#------------------------------------------------------------------------------

# Check for no args
if [ $# -eq 0 ]; then
  usage   # calls exit 0
fi


# Parse command line options
while [ $# -gt 0 ]; do
  case "$1" in
    -h|--help) usage;;
    --verbose)      shift; VERBOSE="yes";;
    --compiler)     shift; COMPILER="$1"; shift;;
    --cpu)          shift; CPU="$1"; shift;;
    --cross)        shift; CROSS="$1"; shift;;
    --enable-arch)  shift; ENABLE_ARCH="$1"; shift;;
    --enable-x64)   shift; ENABLE_X64="$1"; shift;;
    --html-docs)    shift; HTML_DOCS="$1"; shift;;
    --objdump)      shift; OBJDUMP="$1" shift;;
    --sdl-config)   shift; SDL_CONFIG="$1"; shift;;
    --strip)        shift; STRIP="$1"; shift;;
    --topbuilddir)  shift; TOPBUILDDIR="$1"; shift;;
    --topsrcdir)    shift; TOPSRCDIR="$1"; shift;;
    --ui)           shift; UI="$1"; shift;;
    --verbose)      shift; VERBOSE="yes";;
    --vice-version) shift; VICE_VERSION="$1"; shift;;
    --zip-tool)     shift; ZIP_TOOL="$1"; shift;;
    -*) echo "$(basename $0): unknown option $1, aborting."; exit 1;;
  esac
done

if [ "$VERBOSE" = "yes" ]; then
  echo "Creating $UI bindist"
fi

if [ "$VERBOSE" = "yes" ]; then
  show_option_values
fi


set_bindist_windows_version
set_bindist_prefix
set_bindist_vice_version

# Set bindist dir
BINDIST_DIR="$BINDIST_PREFIX-$BINDIST_VICE_VERSION-$BINDIST_WINVER"
# Create bindist dir, optionally deleting the old one
create_bindist_dir

# Add old x64 if requested
if [ "$ENABLE_X64" = "yes" ]; then
  BINDIST_EMUS="x64 $BINDIST_EMUS"
fi

# Now it's time to actually source some stuff depending on UI...
case "$UI" in
  sdl1|sdl2)
    echo "[debug] Sourcing make_bindist_win32_sdl.sh"
    source "$TOPSRCDIR/src/arch/sdl/make_bindist_win32_sdl.sh";;
  gtk3) source "$TOPSRCDIR/src/arch/gtk3/make_bindist_win32_gtk3.sh";;
  *) echo "($basename $0): invalid UI, aborting."; exit;;
esac


# Create directory structure inside the bindist dir
create_bindist_subdirs

# Copy files
copy_emulators "$BINDIST_EMUS"
copy_tools "$BINDIST_TOOLS"
copy_dlls
copy_emu_data_dirs "$BINDIST_DATA_DIRS"
copy_docs


# Create archive, if requested
echo "[debug] All files copied, do we need to zip?"
case "$ZIP_TOOL" in
  zip)
    BINDIST_ZIP="$BINDIST_DIR.zip"
    rm -f $BINDIST_ZIP
    echo "[debug] Creating .zip archive"
    zip -r -9 -q $BINDIST_ZIP $BINDIST_DIR
    ;;
  7zip)
    BINDIST_ZIP="$BINDIST_DIR.7z"
    echo "[debug] Creating .7z archive";
    rm -f $BINDIST_ZIP
    7z a -t7z -m0=lzma2 -mx=9 -ms=on $BINDIST_ZIP $BINDIST_DIR
    ;;
  *)
    echo "No zip";;
esac


# Report
if [ ! -z "$BINDIST_ZIP" ]; then
  echo "Created archive $BINDIST_ZIP"
fi
echo "Done"


      
