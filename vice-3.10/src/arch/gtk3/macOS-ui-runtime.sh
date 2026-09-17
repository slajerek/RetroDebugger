#
# macOS-runtime.sh - GTK specific runtime init
#
# Written by
#  David Hogan <david.q.hogan@gmail.com>
#
# Intended to be combined with common runtime init and embedded
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

# setup gtk environment
# See https://gitlab.gnome.org/GNOME/gtk-mac-bundler

export XDG_CONFIG_DIRS="$BUNDLE_ETC"/xdg
export XDG_DATA_DIRS="$BUNDLE_SHARE"
export GTK_DATA_PREFIX="$BUNDLE_RESOURCES"
export GTK_EXE_PREFIX="$BUNDLE_RESOURCES"
export GTK_PATH="$BUNDLE_RESOURCES"

export GDK_PIXBUF_MODULE_FILE="$BUNDLE_LIB/gdk-pixbuf-2.0/2.10.0/loaders.cache"
export GTK_IM_MODULE_FILE="$BUNDLE_LIB/gtk-3.0/3.0.0/immodules.cache"

export LC_ALL=en_US

if test -f "$BUNDLE_LIB/charset.alias"; then
    export CHARSETALIASDIR="$BUNDLE_LIB"
fi

