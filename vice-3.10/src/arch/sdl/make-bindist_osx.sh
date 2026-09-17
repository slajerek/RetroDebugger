#!/bin/bash

#
# make-bindist.sh - make binary distribution for the Mac OSX port
#
# Written by
#  Christian Vogelgsang <chris@vogelgsang.org>
#  David Hogan <david.q.hogan@gmail.com>
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
# Usage: make-bindist.sh <top_srcdir> <strip> <vice-version> <--enable-arch> <zip|nozip> <SDL-version>
#                         $1           $2      $3             $4              $5          $6
#

RUN_PATH=`dirname $0`

TOP_DIR=$(realpath "$1")
STRIP=$2
VICE_VERSION=$3
ENABLEARCH=$4
ZIP=$5
SDLVERSION=$6

# define UI type
if test x"$SDLVERSION" = "x2"; then
  UI_TYPE=SDL2
else
  echo -e "****\nWARNING: SDL version 1 is not tested!\n****"
  UI_TYPE=SDL
fi

source "$RUN_PATH/../shared/make-bindist_osx.sh"