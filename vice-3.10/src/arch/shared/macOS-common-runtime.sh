#
# macOS-runtime.sh - macOS common runtime init
#
# Written by
#  David Hogan <david.q.hogan@gmail.com>
#
# Intended to be combined with ui specific runtime init and embedded
# in launcher script via 'source'
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

BUNDLE_DIR="$(cd ../.. && pwd -P)"
BUNDLE_NAME="$(basename \"$BUNDLE_DIR\" .app)"
BUNDLE_CONTENTS="$BUNDLE_DIR/Contents"
BUNDLE_RESOURCES="$BUNDLE_CONTENTS/Resources"
BUNDLE_LIB="$BUNDLE_RESOURCES/lib"
BUNDLE_BIN="$BUNDLE_RESOURCES/bin"
BUNDLE_SHARE="$BUNDLE_RESOURCES/share"
BUNDLE_ETC="$BUNDLE_RESOURCES/etc"
