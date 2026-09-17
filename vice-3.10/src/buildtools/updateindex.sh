#!/bin/bash

###############################################################################
# updateindex.sh   - update VICE version / date in the index.html file
###############################################################################

#VERBOSE=1

if sed -i 'p' $(mktemp) 2>/dev/null
then
    # GNU sed
    SED_I="sed -i"
else
    # BSD sed
    SED_I="sed -i ''"
fi

README=doc/html/index.html
CONFIG=configure.ac
VICEDATE=src/vicedate.h

if [ "x$1" = "x" ]; then
echo Filename for index.html not defined, using: $README
else
README=$1
fi

if [ "x$2" = "x" ]; then
echo Filename for configure.ac not defined, using: $CONFIG
else
CONFIG=$2
fi

if [ "x$3" = "x" ]; then
echo Filename for vicedate.h not defined, using: $VICEDATE
else
VICEDATE=$3
fi

if [ "x$VERBOSE" = "x1" ]; then
echo readme:$README
echo config:$CONFIG
echo vicedate:$VICEDATE
fi

VMAJOR=`grep "m4_define.*(.*vice_version_major" $CONFIG | sed "s:m4_define.*(.*vice_version_major.*, \([0-9]*\)):\1:g"`
VMINOR=`grep "m4_define.*(.*vice_version_minor" $CONFIG | sed "s:m4_define.*(.*vice_version_minor.*, \([0-9]*\)):\1:g"`
VBUILD=`grep "m4_define.*(.*vice_version_build" $CONFIG | sed "s:m4_define.*(.*vice_version_build.*, \([0-9]*\)):\1:g"`
VDEV=`grep "m4_define.*(.*vice_version_label" $CONFIG | sed "s:m4_define.*(.*vice_version_label.*, \([a-z]*\)):\1:g"`

#VMINOR=0
#VMINOR=14
#VBUILD=0
#VBUILD=14

if [ "x$VERBOSE" = "x1" ]; then
echo major: $VMAJOR
echo minor: $VMINOR
echo build: $VBUILD
echo dev: $VDEV
fi

if [ "x$VDEV" = "xdev" ]; then
    echo "release date/version in index.html is not updated in dev versions"
    exit 0
fi

DAY=`grep "VICEDATE_DAY " $VICEDATE | cut -d " " -f 3`
MONTH=`grep "VICEDATE_MONTH_LONG " $VICEDATE | cut -d '"' -f 2`
YEAR=`grep "VICEDATE_YEAR " $VICEDATE | cut -d " " -f 3`

if [ "x$VERBOSE" = "x1" ]; then
    echo "day $DAY"
    echo "month: $MONTH"
    echo "year: $YEAR"
fi

# "(24 January 2022) Version 3.6.1 released"
TOPLINE=`grep "([0-9]\+ [A-Z][a-z]* 20[0-9][0-9]) Version [0-9]\+\.[0-9]\+[\.]*[0-9]* \+released" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 1 old: $TOPLINE"
fi

if [ "x$TOPLINE" = "x" ]; then
    echo "WARNING: patch line 1 of index.html not found, version/date NOT updated."
else
    TOPLINE="($DAY $MONTH $YEAR) Version $VMAJOR.$VMINOR"
    if [ "$VBUILD" != "0" ]; then
        TOPLINE="$TOPLINE.$VBUILD"
    fi
    TOPLINE="$TOPLINE released"
    LC_ALL=C $SED_I -e "s:[\(][0-9]\+ [A-Z][a-z]* 20[0-9][0-9][\)] Version [0-9]\+\.[0-9]\+[\.]*[0-9]* \+released:$TOPLINE:g" $README
fi

TOPLINE=`grep "([0-9]\+ [A-Z][a-z]* 20[0-9][0-9]) Version [0-9]\+\.[0-9]\+[\.]*[0-9]* \+released" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 1 new: $TOPLINE"
fi

# <a href="https://sourceforge.net/projects/vice-emu/files/releases/vice-3.6.1.tar.gz/download">vice-3.6.1.tar.gz</a>

LINE=`grep "vice-[0-9]\+\.[0-9]\+\.*[0-9]*\.tar\.gz" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 2 old: $LINE"
fi

if [ "x$LINE" = "x" ]; then
    echo "WARNING: patch line 2 of index.html not found, version/date NOT updated."
else
    LINE="vice-$VMAJOR.$VMINOR"
    if [ "$VBUILD" != "0" ]; then
        LINE="$LINE.$VBUILD"
    fi
    LINE="$LINE.tar.gz"
    LC_ALL=C $SED_I -e "s:vice-[0-9]\+\.[0-9]\+\.*[0-9]*\.tar\.gz:$LINE:g" $README
fi

LINE=`grep "vice-[0-9]\+\.[0-9]\+\.*[0-9]*\.tar\.gz" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 2 new: $LINE"
fi

#  <li>Download <a href="https://sourceforge.net/projects/vice-emu/files/releases/binaries/windows/GTK3VICE-3.6.1-win64.zip/download">VICE 3.6.1</a> (64bit GTK3)</li>

LINE=`grep "VICE-[0-9]\+\.[0-9]\+\.*[0-9]*-win" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 3 old: $LINE"
fi

if [ "x$LINE" = "x" ]; then
    echo "WARNING: patch line 3 of index.html not found, version/date NOT updated."
else
    LINE="VICE-$VMAJOR.$VMINOR"
    if [ "$VBUILD" != "0" ]; then
        LINE="$LINE.$VBUILD"
    fi
    LINE="$LINE-win"
    LC_ALL=C $SED_I -e "s:VICE-[0-9]\+\.[0-9]\+\.*[0-9]*-win:$LINE:g" $README
fi

LINE=`grep "VICE-[0-9]\+\.[0-9]\+\.*[0-9]*-win" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 3 new: $LINE"
fi

LINE=`grep "VICE [0-9]\+\.[0-9]\+\.*[0-9]*" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 4 old: $LINE"
fi

if [ "x$LINE" = "x" ]; then
    echo "WARNING: patch line 4 of index.html not found, version/date NOT updated."
else
    LINE="VICE $VMAJOR.$VMINOR"
    if [ "$VBUILD" != "0" ]; then
        LINE="$LINE.$VBUILD"
    fi
    LC_ALL=C $SED_I -e "s:VICE [0-9]\+\.[0-9]\+\.*[0-9]*:$LINE:g" $README
fi

LINE=`grep "VICE [0-9]\+\.[0-9]\+\.*[0-9]*" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 4 new: $LINE"
fi

LINE=`grep "[0-9]\+\.[0-9]\+\.*[0-9]*.dmg" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 5 old: $LINE"
fi

if [ "x$LINE" = "x" ]; then
    echo "WARNING: patch line 5 of index.html not found, version/date NOT updated."
else
    LINE="$VMAJOR.$VMINOR"
    if [ "$VBUILD" != "0" ]; then
        LINE="$LINE.$VBUILD"
    fi
    LINE="$LINE.dmg"
    LC_ALL=C $SED_I -e "s:[0-9]\+\.[0-9]\+\.*[0-9]*.dmg:$LINE:g" $README
fi

LINE=`grep "[0-9]\+\.[0-9]\+\.*[0-9]*.dmg" < $README`
if [ "x$VERBOSE" = "x1" ]; then
    echo "line 5 new: $LINE"
fi
