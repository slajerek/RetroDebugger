#!/bin/sh

#
# make-bindist_win32.sh - Make a binary distribution for the Windows SDL ports.
#
# Written by
#  Marco van den Heuvel <blackystardust68@yahoo.com>
#  Bas Wassink <b.wassink@ziggo.nl>
#  Greg King <gregdk@users.sf.net>
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
# Usage: make-bindist.sh <strip=$1> <vice-version=$2> <--enable-arch=$3> <zip|nozip=$4> <unzipbin=$5>
#                        <x64-included=$6> <top-srcdir=$7> <top-builddir=$8> <cpu=$9> <SDL-version=$10>
#                        <sdl-config=$11> <cross=$12> <objdump=$13> <compiler=$14> <--enable-html-docs=$15>
#                        [<svn-revision-override=$16>]
#

STRIP=$1
VICEVERSION=$2
ENABLEARCH=$3
ZIPKIND=$4
UNZIPBIN=$5
X64INC=$6
TOPSRCDIR=$7
TOPBUILDDIR=$8
CPU=$9

shift   # $10
SDLVERSION=$9

shift   # $11
SDLCONFIG=$9

shift   # $12
CROSS=$9

shift   # $13
OBJDUMP=$9

shift   # $14
COMPILER=$9

shift   # $15
HTML_DOCS=$9

shift   # $16
SVN_REVISION_OVERRIDE=$9


# Try to get the SVN revision

SVN_SUFFIX=""
svnrev_string=$(svnversion $TOPSRCDIR)
if test "$?" = "0"; then
  # Choose the second number (usually higher) if it exists; drop letter suffixes.
  svnrev=`echo "$svnrev_string" | sed 's/^\([0-9]*:\)*\([0-9]*\)*.*/\2/'`
  #echo "svnrev string: $svnrev"
  # Only a number is extracted.
  if test -n "$svnrev"
    then SVN_SUFFIX="-r$svnrev"
  fi
fi

if test "$SVN_SUFFIX" = ""; then
  # No svnversion found, fall back to the revision override, if available
  if test "x$SVN_REVISION_OVERRIDE" != "x"; then
    SVN_SUFFIX="-r$SVN_REVISION_OVERRIDE"
  fi
fi


if test x"$CPU" = "xx86_64" -o x"$CPU" = "xamd64"
then WINXX="win64"
else WINXX="win32"
fi


# Check if we have the old x64.exe binary.
if test x"$X64INC" = "xyes"
then X64FILE="x64"
else X64FILE=""
fi

EMULATORS="$X64FILE x64sc xscpu64 x64dtv x128 xcbm2 xcbm5x0 xpet xplus4 xvic vsid"
CONSOLE_TOOLS="c1541 tools/cartconv/cartconv tools/petcat/petcat"
EXECUTABLES="$EMULATORS $CONSOLE_TOOLS"

unset CONSOLE_TOOLS EMULATORS X64FILE svnrev svnrev_string CPU X64INC

for i in $EXECUTABLES; do
  if [ ! -e $TOPBUILDDIR/src/$i.exe ]; then
    echo 'Error: executable files not found; do a "make" first.'
    exit 1
  fi
done


if test x"$SDLVERSION" = "x2"; then
  echo "Generating a $WINXX SDL2 port binary distribution..."
  SDLNAME="SDL2VICE"
else
  echo "Generating a $WINXX SDL port binary distribution..."
  SDLNAME="SDLVICE"
fi

BINDIST_DIR="$SDLNAME-$VICEVERSION-$WINXX$SVN_SUFFIX"

if test -e "$BINDIST_DIR"; then
    echo "Removing an old $BINDIST_DIR"
    rm -f -r $BINDIST_DIR
fi
mkdir $BINDIST_DIR


# Copy binaries.  Strip them unless VICE is configured with "--enable-debug".
for i in $EXECUTABLES; do
  cp $TOPBUILDDIR/src/$i.exe $BINDIST_DIR
  $STRIP $BINDIST_DIR/$(basename $i).exe
done


if test x"$CROSS" != "xtrue"; then
  # Assume MSYS2 on Windows here.
  dlls=`ntldd -R $BINDIST_DIR/x64sc.exe|gawk '/\\\\bin\\\\/{print $3;}'|cygpath -f -`
  test -n "$dlls"&&cp $dlls $BINDIST_DIR

  # drop unzip.exe and its dependencies in the bin/ =)
  if test x"$UNZIPBIN" != "xno"; then
    cp $UNZIPBIN $BINDIST_DIR
    cp `ntldd -R $UNZIPBIN | gawk '/\\\\bin\\\\/{print $3;}' | cygpath -f -` $BINDIST_DIR
  fi

else
  # Assume a cross-builder for Windows here.
  get_dll_deps()
  {
    for j in `find $BINDIST_DIR -name '*.dll'`; do
      dlls=`$OBJDUMP -p $j|gawk '/^\\tDLL N/{print " -o -name",$3;}'`
      dlls=`find $dlldirs -false$dlls`
      test -n "$dlls"&&cp -u $dlls $BINDIST_DIR||:
    done
  }

  # Find the paths of DLL directories.
  libm=`$COMPILER -print-file-name=libm.a`
  location=`dirname $libm`
  loc=`dirname $location`
  dlldirs="$loc/bin $location"
  test -d "$loc/dll"&&dlldirs="$dlldirs $loc/dll"
  libgcc=`$COMPILER -print-file-name=libgcc.a`
  gccdir=`dirname $libgcc`
  sdldir=`$SDLCONFIG --exec-prefix`/bin
  dlldirs="$dlldirs $gccdir $sdldir"
  # Find the DLLs that are needed by VICE.
  dlls=`$OBJDUMP -p $BINDIST_DIR/x64sc.exe|gawk '/^\\tDLL N/{print " -o -name",$3;}'`
  echo "dlls = $dlls"
  dlls=`find $dlldirs -false $dlls`
  echo "dlls = $dlls"
  test -n "$dlls"&&cp $dlls $BINDIST_DIR
  # Find the DLLs that are needed by other DLLs.
  get_dll_deps
fi


cp -a $TOPSRCDIR/data/C128 $TOPSRCDIR/data/C64 $BINDIST_DIR
cp -a $TOPSRCDIR/data/C64DTV $TOPSRCDIR/data/CBM-II $BINDIST_DIR
cp -a $TOPSRCDIR/data/DRIVES $TOPSRCDIR/data/PET $BINDIST_DIR
cp -a $TOPSRCDIR/data/PLUS4 $TOPSRCDIR/data/PRINTER $BINDIST_DIR
cp -a $TOPSRCDIR/data/SCPU64 $TOPSRCDIR/data/VIC20 $BINDIST_DIR
rm -f `find $BINDIST_DIR -name 'Makefile*'`
rm -f `find $BINDIST_DIR -name 'gtk3*'`
mkdir $BINDIST_DIR/hotkeys
cp -a $TOPSRCDIR/data/hotkeys/*.vhk $BINDIST_DIR/hotkeys

# Icon files for SDL1
if test x"$SDLVERSION" = "x1"; then
    cp -a $TOPSRCDIR/data/common/*_32.png $BINDIST_DIR/common
fi

mkdir $BINDIST_DIR/doc
#if test x"$HTML_DOCS" = "xyes"; then
#    cp -a $TOPSRCDIR/doc/html/* $BINDIST_DIR/doc
#    cp -a -u $TOPBUILDDIR/doc/html/* $BINDIST_DIR/doc
#fi
rm -f $BINDIST_DIR/doc/Makefile* $BINDIST_DIR/doc/texi2html
rm -f $BINDIST_DIR/doc/checklinks.sh $BINDIST_DIR/doc/sitemap.xml
rm -f $BINDIST_DIR/doc/robots.txt $BINDIST_DIR/doc/COPYING $BINDIST_DIR/doc/NEWS
cp $TOPSRCDIR/NEWS $TOPSRCDIR/COPYING $TOPSRCDIR/README $BINDIST_DIR
cp $TOPSRCDIR/doc/readmes/Readme-SDL.txt $BINDIST_DIR
test -e "$TOPBUILDDIR/doc/vice.pdf"&&cp "$TOPBUILDDIR/doc/vice.pdf" $BINDIST_DIR/doc


if test x"$ZIPKIND" = "xzip" -o x"$ZIPKIND" = "x7zip"; then
  if test x"$ZIPKIND" = "xzip" -o x"$ZIPKIND" = "x"; then
    ZIPEXT=zip
  else
    ZIPEXT=7z
  fi
  rm -f $BINDIST_DIR.$ZIPEXT

  if test x"$ZIPKIND" = "x7zip"; then
    7z a -t7z -m0=lzma2 -mx=9 -ms=on $BINDIST_DIR.$ZIPEXT $BINDIST_DIR
  else
    if test x"$ZIP" = "x"; then
      zip -r -9 -q $BINDIST_DIR.zip $BINDIST_DIR
    else
      $ZIP $BINDIST_DIR.zip $BINDIST_DIR
    fi
  fi
  rm -f -r $BINDIST_DIR
  echo $WINXX SDL$SDLVERSION port binary distribution archive generated as
  echo "(Bash path): $BINDIST_DIR.zip"
  test x"$CROSS" != "xtrue"&&echo "(Windows path): '`cygpath -wa \"$BINDIST_DIR.zip\"`'"
else
  echo $WINXX SDL$SDLVERSION port binary distribution directory generated as
  echo "(Bash path): $BINDIST_DIR/"
  test x"$CROSS" != "xtrue"&&echo "(Windows path): '`cygpath -wa \"$BINDIST_DIR/\"`'"
fi
if test x"$ENABLEARCH" = "xyes"; then
  echo ''
  echo 'Warning: The binaries are optimized for your system.'
  echo 'They might not run on a different system.'
  echo 'Configure with --disable-arch to avoid it.'
  echo ''
fi
