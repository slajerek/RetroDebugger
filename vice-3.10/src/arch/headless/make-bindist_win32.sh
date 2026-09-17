#!/bin/bash

# make-bindist_w32.sh - Make a binary distribution for the Windows headless build
#
# Written by
#  Bas Wassink <b.wassink@ziggo.nl>
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

# Strip executable
STRIP=":"
# version string for the distrubution name
VICEVERSION=""
# Value of the enable_arch autoconf variable, used to warn about optimization
ENABLEARCH=""
# Archive type ("zip"|"7zip")
ZIPTYPE=""
# Whether to include the deprecated x64 emulator ("yes"|"no")
X64INC="no"
# top source directory
TOPSRCDIR=""
# Top build directory
TOPBUILDDIR=""
# CPU type string ("i686"|"x86_64"|"amd64")
CPU="unknown"
# objdump executable
OBJDUMP=""
# Compiler executable
COMPILER=""

# SVN revision
SVNREVISION="none"
# SVN revision suffix (set if creating dist from svn-managed directory)
SVNSUFFIX=""
# Script name for messages
SCRIPTNAME=$(basename $0)
# Verbose output
VERBOSE=""

EMUS="x64sc x64dtv xscpu64 x128 xvic xplus4 xpet xcbm2 xcbm5x0 vsid"
TOOLS="c1541 tools/cartconv/cartconv tools/petcat/petcat"


# Print usage message on stdout
show_usage()
{
    cat <<EOF
Usage: $SCRIPTNAME <options>

Options:
    -h, --help                  display this message and exit
    --verbose                   verbose output

  The following options are required:
    --build-dir PATH            top build directory
    --compiler-exe PATH         path to the compiler executable
    --cpu-type CPU              host CPU type
    --enable-arch VALUE         value of the enable_arch configure variable
    --objdump-exe PATH          path to objdump executable
    --src-dir PATH              top source directory
    --vice-version VERSION      VICE version string

  The following options are optional:
    --disable-x64               don't include x64 binary (default)
    --enable-x64                include x64 binary
    --strip-exe PATH            path to strip executable (":" can be used to
                                disable stripping)
    --zip-type <zip|7z[ip]>     type of archive to create
EOF
}


# Get SVN revision number, if it exists
# 
# Store SVN revision in SVNREVISION, filename revision suffix in SVNSUFFIX.
# First uses the svnversion command and if that doesn't produce an svn revision
# git-svn is used to try to get an svn revision.
get_svn_revision()
{
    svnrev_string=$(svnversion $TOPSRCDIR)
    if [ "$?" = "0" ]; then
        # Choose the second number (usually higher) if it exists; drop letter suffixes.
        SVNREVISION=`echo "$svnrev_string" | sed 's/^\([0-9]*:\)*\([0-9]*\)*.*/\2/'`
        #echo "svnrev string: $svnrev"
    fi
    if [ -z "$SVNREVISION" ]; then
        # No SVN revision foud, check if we have a git svn ref
        GITSVNHASH=$(git -C "$TOPSRCDIR" log --grep='git-svn-id:' -n 1 --pretty-format:"%H")
        if [[ "$?" = "0" && -n "$GITSVNHASH" ]]; then
            SVNREVISION=$(git svn find-rev $GITSVNHASH)
        fi
    fi
    if [ -n "$SVNREVISION" ]; then
        SVNSUFFIX="-r$SVNREVISION"
    fi
}

# Set variables used to create an archive of the dist directory
#
# Sets ZIPEXE to the executable required, ZIPEXT to the file extension and
# ZIPOPT to the command line options to pass to the ZIPEXE.
set_zip_variables()
{
    if [ "x$ZIPTYPE" = "xzip" ]; then
        ZIPEXE="zip"
        ZIPEXT=".zip"
        ZIPOPT="-r -9"
        if [ -n "$VERBOSE" ]; then
            ZIPOPT="$ZIPOPT -v"
        else
            ZIPOPT="$ZIPOPT -q"
        fi
    elif [[ "x$ZIPTYPE" = "x7z" || "x$ZIPTYPE" = "x7zip" ]]; then
        ZIPEXE="7z"
        ZIPEXT=".7z"
        # No options for verbose/quiet operation
        ZIPOPT="a -t7z -m0=lzma2 -mx=9 -ms=on"
    else
        ZIPEXE="none (don't create archive of dist directory)"
    fi
}


# Determine the string used in the filename for the Windows CPU architecture
#
# Note: the ARM stuff is pure guesswork, MinGW doesn't yet support ARM.
get_windows_arch()
{
    case "$CPU" in
        x86_64 | amd64)
            WINARCH="win64"
            ;;
        arm64)
            WINARCH="winarm64"
            ;;
        arm*)
            WINARCH="winarm"
            ;;
        *)
            WINARCH="win32"
            ;;
    esac
}

# Copy binaries: emulators and tools
copy_bins()
{
    if [ "x$X64INC" = "xyes" ]; then
        binaries="x64 $EMUS $TOOLS"
    else
        binaries="$EMUS $TOOLS"
    fi
    for b in $binaries; do
        binary="$TOPBUILDDIR/src/$b.exe"
        # check the binary exists
        if [ ! -f $binary ]; then
            echo "$SCRIPTNAME: error: missing binary $binary, run make first"
            exit 1
        fi
        verbose_echo ".. Copying $binary"
        cp $binary $BINDISTDIR
        if [ -n "$STRIP" ]; then
            verbose_echo ".. Stripping $binary"
            $STRIP $BINDISTDIR/$(basename $b).exe
        fi
    done
}


# Copy DLLs that are *not* inside %WINDIR% or it's subdirectories
copy_libs()
{
    windir=$(cygpath -wW)
    for lib in $(ntldd -R $TOPBUILDDIR/src/x64sc.exe \
                 | sed -n 's/^.*=> \(.*\(dll\|DLL\)\).*$/\1/p'); do
        echo "$lib" | grep -Fq "$windir"
        if [ "$?" != "0" ]; then
            verbose_echo ".. Copying $lib"
            cp $lib $BINDISTDIR
        fi
    done
}


# Copy data files: ROMs, romset files, palettes etc
copy_data()
{
    # create data/ dir to avoid deleting files in the root we might want to keep
    verbose_echo ".. Creating temporary data directory"
    mkdir $BINDISTDIR/data
    verbose_echo ".. Copying $TOPSRCDIR/data/*"
    cp -R $TOPSRCDIR/data $BINDISTDIR
    verbose_echo ".. Deleting unwanted files"
    find $BINDISTDIR/data -type f \
        \( -name 'Makefile*' -o -name '*.vhk' -o -name '*.vjk' -o \
           -name '*.vjm' -o -name '*.vkm' -o -name '*.rc' -o -name '*.png' -o \
           -name '*.svg' -o -name '*.xml' -o -name '*.ttf' \) \
        -exec rm {} \;
    rm -rfd $BINDISTDIR/data/GLSL
    mv $BINDISTDIR/data/* $BINDISTDIR
    rmdir $BINDISTDIR/data
}

# Copy documentation files
#
# TODO: We should probably add a 'Readme-Headless.txt' or so
copy_docs()
{
    docs="$TOPSRCDIR/COPYING $TOPSRCDIR/NEWS $TOPSRCDIR/README $TOPBUILDDIR/doc/vice.pdf"
    mkdir $BINDISTDIR/doc
    for d in $docs; do
        if [ -f $d ]; then
            verbose_echo ".. Copying $d"
            cp $d $BINDISTDIR/doc
        fi
    done
}


create_archive()
{
    if [ -f "$BINDISTZIP" ]; then
        rm $BINDISTZIP
    fi

    # although we've set -q/-v in the info-zip command line options to enable
    # or disable output, 7z doesn't have any of those options, so we still have
    # to redirect to nul:
    if [ -z "$VERBOSE" ]; then
        $ZIPEXE $ZIPOPT $BINDISTZIP $BINDISTDIR > /dev/null
    else
        $ZIPEXE $ZIPOPT $BINDISTZIP $BINDISTDIR
    fi
    if [ "$?" = "0" ]; then
        rm -rfd $BINDISTDIR
    fi
}


# Echo text only if --verbose was passed to the script
#
# Params:   $1  text to print
verbose_echo()
{
    if [ -n "$VERBOSE" ]; then
        echo "$1"
    fi
}


# Exit with status 1 if a command line option is missing its required argument
#
# Params:   $1  option name
#           $2  option argument
option_needs_arg()
{
    if [ -z "$2" ]; then
        echo "$SCRIPTNAME: error: missing argument for $1"
        exit 1
    fi
}

# Handle command line arguments
echo "$@"
if [ $# -eq 0 ]; then
    show_usage
    exit 0
fi

while [ -n "$1" ]; do
    case "$1" in
        --build-dir)
            option_needs_arg "$1" "$2"
            TOPBUILDDIR="$2"
            shift; shift
            ;;
        --compiler-exe)
            option_needs_arg "$1" "$2"
            COMPILER="$2"
            shift; shift
            ;;
        --cpu-type)
            option_needs_arg "$1" "$2"
            CPU="$2"
            shift; shift
            ;;
        --disable-x64)
            X64INC="no"
            shift
            ;;
        --enable-arch)
            option_needs_arg "$1" "$2"
            ENABLEARCH="$2"
            shift; shift
            ;;
        --enable-x64)
            X64INC="yes"
            shift
            ;;
        --objdump-exe)
            option_needs_arg "$1" "$2"
            OBJDUMP="$2"
            shift; shift
            ;;
        --src-dir)
            option_needs_arg "$1" "$2"
            TOPSRCDIR="$2"
            shift; shift
            ;;
        --strip-exe)
            option_needs_arg "$1" "$2"
            STRIP="$2"
            shift; shift
            ;;
        --verbose)
            VERBOSE="yes"
            shift
            ;;
        --vice-version)
            option_needs_arg "$1" "$2"
            VICEVERSION="$2"
            shift; shift
            ;;
        --zip-type)
            option_needs_arg "$1" "$2"
            ZIPTYPE="$2"
            shift; shift
            ;;
        -h | --help)
            show_usage
            exit 0
            ;;
        *)
            echo "$SCRIPTNAME: error: unknown option $1"
            show_usage
            exit 1
            ;;
    esac
done


# Determine SVN revision, if any
get_svn_revision
# Determine archive format, if any, and set appropiate variables
set_zip_variables
# Determine Windows arch string
get_windows_arch
# Generate dist directory name and archive filename
BINDISTDIR="HeadlessVICE-$VICEVERSION-$WINARCH$SVNSUFFIX"
if [ -n "$ZIPEXT" ]; then
    BINDISTZIP="$BINDISTDIR.$ZIPEXE"

fi

if [ -n "$VERBOSE" ]; then
    echo "----"
    echo "compiler executable     = $COMPILER"
    echo "objdump executable      = $OBJDUMP"
    echo "strip executable        = $STRIP"
    echo "archive type            = $ZIPEXE"
    echo "archive options         = $ZIPOPT"
    echo "VICE version string     = $VICEVERSION"
    echo "CPU type                = $CPU"
    echo "configure --enable-arch = $ENABLEARCH"
    echo "Include x64 emulator    = $X64INC"
    echo "Top source directory    = $TOPSRCDIR"
    echo "Top build directory     = $TOPBUILDDIR"

    echo "SVN revision            = $SVNREVISION"
    echo "Windows CPU arch        = $WINARCH"
    echo "Distribution directory  = $BINDISTDIR"
    echo "Distribution archive    = $BINDISTZIP"
    echo "----"
fi


# Create dist dir
if [ -d $BINDISTDIR ]; then
    verbose_echo "Removing dist dir $BINDISTDIR"
    rm -rfd $BINDISTDIR
fi
verbose_echo "Creating dist dir $BINDISTDIR"
mkdir $BINDISTDIR

verbose_echo "Copying emulator binaries and tools"
copy_bins
verbose_echo "Copying libaries"
copy_libs
verbose_echo "Copying ROMs, romsets, palettes and icons"
copy_data
verbose_echo "Copying documentation"
copy_docs

if [ -n "$ZIPEXT" ]; then
    verbose_echo "Creating archive"
    create_archive
fi

if [ "$?" = "0" ]; then
    if [ -z "$ZIPEXT" ]; then
        verbose_echo "OK: binary distribution directory created as $BINDISTDIR"
    else
        verbose_echo "OK: binary distribution archive created as $BINDISTZIP"
    fi
fi

if [ "x$ENABLEARCH" != "xno" ]; then
    echo "Warning: binaries were created with buildhost-specific optimizations and may not"
    echo "work on other systems."
fi
