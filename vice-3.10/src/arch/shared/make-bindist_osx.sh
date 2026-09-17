#!/bin/bash
set -o errexit
set -o nounset

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
# Usage: make-bindist.sh <top_srcdir> <strip> <vice-version> <--enable-arch> <zip|nozip> <ui_type>
#                         $1           $2      $3             $4              $5         $6
#

WARN_CODE_SIGN=false

[ -z ${DEPS_PREFIX+set} ] && (>&2 echo "ERROR: Please set DEPS_PREFIX environment variable to something like /opt/local, /usr/local, or /opt/homebrew"; exit 1)

if [ -z ${CODE_SIGN_ID+set} ]; then
  # Sign as ad-hoc if no identity is provided
  CODE_SIGN_ID="-"
  WARN_CODE_SIGN=true
fi

if [ "$UI_TYPE" = "GTK3" ]; then
  if which gtk-update-icon-cache-3.0 > /dev/null; then
    # macports
    GTK_UPDATE_ICON_CACHE=gtk-update-icon-cache-3.0
  elif which gtk3-update-icon-cache >/dev/null; then
    GTK_UPDATE_ICON_CACHE=gtk3-update-icon-cache
  elif which gtk-update-icon-cache >/dev/null; then
    GTK_UPDATE_ICON_CACHE=gtk-update-icon-cache
  else
    >&2 echo "ERROR: Could not find gtk-update-icon-cache-3.0, gtk3-update-icon-cache, nor gtk-update-icon-cache."
    exit 1
  fi
fi

CPU_ARCH=$(uname -m | tr _ -)

echo "Generating macOS binary distribution."
echo "  UI type: $UI_TYPE"
echo " CPU arch: $CPU_ARCH"

# setup BUILD dir
BUILD_DIR=$(echo "vice-$CPU_ARCH-$UI_TYPE-$VICE_VERSION" | tr '[:upper:]' '[:lower:]')

if [[ ! -z "${GITHUB_REF_NAME:-}" ]]; then
  #GitHub Actions build
  BUILD_DIR="$BUILD_DIR-$GITHUB_REF_NAME"
else
  SVN_VERSION=$(svn info --show-item revision "$TOP_DIR" 2>/dev/null || true)
  if [[ ! -z "$SVN_VERSION" ]]; then
    BUILD_DIR="$BUILD_DIR-r$SVN_VERSION"
    # if the source is not clean, add '-uncommitted-changes' to the name
    if [[ ! -z "$(svn status -q "$TOP_DIR")" ]]; then
      BUILD_DIR="$BUILD_DIR-uncommitted-changes"
    fi
  fi
fi

if [ -d $BUILD_DIR ]; then
  rm -rf $BUILD_DIR
fi

mkdir $BUILD_DIR

# define emulators and command line tools
EMULATORS="xscpu64 x64dtv x64sc x128 xcbm2 xcbm5x0 xpet xplus4 xvic vsid"
TOOLS="c1541 tools/petcat/petcat tools/cartconv/cartconv"

# define data files for emulators
ROM_COMMON="DRIVES PRINTER"
ROM_x64=C64
ROM_xscpu64=SCPU64
ROM_x64sc=C64
ROM_x64dtv=C64DTV
ROM_x128=C128
ROM_xcbm2=CBM-II
ROM_xcbm5x0=CBM-II
ROM_xpet=PET
ROM_xplus4=PLUS4
ROM_xvic=VIC20
ROM_vsid=C64

# files to remove from ROM directory
if [ "$UI_TYPE" = "GTK3" ]; then
  ROM_REMOVE="sdl_*.v?m"
elif [ "$UI_TYPE" = "SDL2" ]; then
  ROM_REMOVE="gtk3_*.v?m"
fi

# define droppable file types
DROP_EXTENSIONS="x64 p64 g64 d64 d71 d81 t64 tap prg p00 crt reu"

# runtime scripts
MACOS_SCRIPTS=macOS-runtime-scripts.inc
LAUNCHER=../shared/macOS-launcher.sh
REDIRECT_LAUNCHER=../shared/macOS-redirect-launcher.applescript

copy_tree () {
  (cd "$1" && tar --exclude 'Makefile*' --exclude .svn -c -f - .) | (cd "$2" && tar xf -)
}

resolve_link () {
  local link=$1
  local link_target=$(readlink $link)

  echo "$(cd $(dirname $link) && cd $(dirname $link_target) && pwd -P)/$(basename $link_target)"
}

extract_rpaths () {
  local thing="$1"

  LOADER_PATH=$(dirname "$thing")
  # >&2 echo "Loader path for $thing: $LOADER_PATH"

  RPATHS=$(
    otool -l "$thing" \
    | grep -A 2 LC_RPATH \
    | grep path \
    | sed 's/.*path //' \
    | sed 's/ (offset.*//' \
    | sed "s,@loader_path/,$LOADER_PATH/,"
  )

  # >&2 echo "RPATHs for $thing:"
  # >&2 echo "$RPATHS"

  echo "$RPATHS"
}

find_thing_libs () {
  local thing="$1"

  otool -L "$thing" \
    | grep -v "$thing" \
    | egrep -v "^\s+/usr/lib/" \
    | egrep -v "^\s+/System/Library/" \
    | awk '{print $1}'

  true
}

copy_thing_libs_internal () {
  local thing="$1"

  shift
  local exe_rpaths="$@"

  THING_LIBS=$(find_thing_libs "$thing")
  # >&2 echo "*** $thing libs:"
  # >&2 echo "$THING_LIBS"

  # copy this lib's libs. Some resolution of @rpath and @loader_path is needed for some libs.
  RPATHS=$(extract_rpaths "$thing")

  # >&2 echo "Combined lib + exe RPATHs for $thing:"
  # >&2 echo -e "$RPATHS\n$exe_rpaths"

  for lib in $THING_LIBS; do
    resolved_lib="$lib"
    if [[ "$resolved_lib" == @rpath/* ]]; then
      # >&2 echo "Considering @rpath lib: $resolved_lib"
      for rpath in $RPATHS $exe_rpaths; do
        # >&2 echo "  considering rpath: $rpath"
        candidate_lib="$rpath/${resolved_lib#@rpath/}"

        if [[ "$candidate_lib" == @loader_path/* ]]; then
          candidate_lib="$LOADER_PATH/${candidate_lib#@loader_path/}"
        fi

        if [ -e "$candidate_lib" ]; then
          # >&2 echo "  candidate lib exists: $candidate_lib"
          resolved_lib="$candidate_lib"
          break
        fi
      done

    elif [[ "$lib" == @loader_path/* ]]; then
      candidate_lib="$LOADER_PATH/${lib#@loader_path/}"
      if [ -e "$candidate_lib" ]; then
        resolved_lib="$candidate_lib"
      fi
    fi

    # >&2 echo "Found lib to copy: $lib"
    # >&2 echo "  resolved to: $resolved_lib"

    if [ ! -e "$resolved_lib" ]; then
      >&2 echo "  ERROR: resolved lib $resolved_lib does not exist!"
      exit 1
    fi

    basename_lib=$(basename "$resolved_lib")
    if [ -e "$APP_LIB/$basename_lib" ]; then
      # >&2 echo "  already copied, skipping."
      continue
    fi

    # >&2 echo "  copying to bundle."
    cp "$resolved_lib" "$APP_LIB/"
    chmod 644 "$APP_LIB/$basename_lib"

    # Recursively copy this lib's libs
    copy_thing_libs_internal "$resolved_lib" "$exe_rpaths"
  done
}

copy_thing_libs () {
  local thing="$1"

  copy_thing_libs_internal "$thing" "$(extract_rpaths "$thing")"
}

# --- create bundle ------------------------------------------------------------

BUNDLE="VICE"
APP_NAME=$BUILD_DIR/$BUNDLE.app
APP_CONTENTS=$APP_NAME/Contents
APP_MACOS=$APP_CONTENTS/MacOS
APP_RESOURCES=$APP_CONTENTS/Resources
APP_ETC=$APP_RESOURCES/etc
APP_SHARE=$APP_RESOURCES/share
APP_COMMON=$APP_SHARE/vice/common
APP_HOTKEYS=$APP_SHARE/vice/hotkeys
APP_GLSL=$APP_SHARE/vice/GLSL
APP_ICONS=$APP_SHARE/vice/icons
APP_ROMS=$APP_SHARE/vice
APP_DOCS=$APP_SHARE/vice/doc
APP_BIN=$APP_RESOURCES/bin
APP_LIB=$APP_RESOURCES/lib

make_app_bundle() {
  local app_name="$1"
  local app_launcher="$2"
  local app_path="$BUILD_DIR/$app_name.app"
  local contents="$app_path/Contents"
  local macos="$contents/MacOS"
  local resources="$contents/Resources"
  local info_plist="$contents/Info.plist"
  local bundle_id="org.viceteam.$app_name"
  local icon_name="VICE.icns"
  local min_macos="${MIN_MACOS_VER:-12.0}"  # override with MIN_MACOS_VER if you want
  local app_exe_filename="$app_name"

  rm -rf "$app_path"

  # 2) Install launcher
  if [[ "$app_launcher" == *.applescript ]]; then
    osacompile -o "$app_path" "$app_launcher"
    app_exe_filename=droplet

    # Install emu specific icon
    if [[ "$app_name" == "x64sc" ]]; then
      src_icon_name="x64"
    elif [[ "$app_name" == "xcbm5x0" ]]; then
      src_icon_name="xcbm2"
    else
      src_icon_name="$app_name"
    fi

    rm "$resources/droplet.icns"
    mkdir -p "$resources/VICE.iconset"
    cp "$TOP_DIR/data/common/vice-${src_icon_name}_256.png" "$resources/VICE.iconset/icon_256x256.png"
    iconutil -c icns "$resources/VICE.iconset" -o "$resources/$icon_name"
    rm -rf "$resources/VICE.iconset"
  elif [[ "$app_launcher" == *.sh ]]; then
    mkdir -p "$macos"

    # A native wrapper is needed for macOS to not think it needs Rosetta
    cp src/arch/shared/macOS-launcher "$macos/$app_name"

    # Move shell script to Resources to avoid code signing issues
    mkdir -p "$resources"
    cp "$app_launcher" "$resources/${app_name}.sh"
    chmod +x "$resources/${app_name}.sh"

    # Install icon
    cp "$RUN_PATH/Resources/VICE.icns" "$resources/$icon_name"
  else
    >&2 echo "ERROR: Unknown launcher type: $app_launcher"
    exit 1
  fi

  # 3) Minimal Info.plist
  /usr/bin/env cat > "$info_plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleDevelopmentRegion</key>    <string>en</string>
  <key>CFBundleName</key>                 <string>${app_name}</string>
  <key>CFBundleDisplayName</key>          <string>${app_name}</string>
  <key>CFBundleIdentifier</key>           <string>${bundle_id}</string>
  <key>CFBundleVersion</key>              <string>${VICE_VERSION}</string>
  <key>CFBundleShortVersionString</key>   <string>${VICE_VERSION}</string>
  <key>CFBundlePackageType</key>          <string>APPL</string>
  <key>CFBundleExecutable</key>           <string>${app_exe_filename}</string>
  <key>CFBundleIconFile</key>             <string>${icon_name}</string>
  <key>CFBundlePackageType</key>          <string>APPL</string>
  <key>LSMinimumSystemVersion</key>       <string>${min_macos}</string>
  <key>NSHighResolutionCapable</key>      <true/>
  <key>NSAppTransportSecurity</key>
	<dict>
		<key>NSAllowsArbitraryLoads</key>
		<true/>
	</dict>
	<key>NSHumanReadableCopyright</key>
	<string>© 2025 The VICE Team</string>
  <key>CFBundleDocumentTypes</key>
  <array>
    <dict>
      <key>CFBundleTypeName</key><string>VICE Files</string>
      <key>CFBundleTypeRole</key><string>Viewer</string>
      <key>CFBundleTypeExtensions</key>
      <array>
$(for ext in $DROP_EXTENSIONS; do printf '        <string>%s</string>\n' "$ext"; done)
      </array>
    </dict>
  </array>
</dict>
</plist>
PLIST

  # Strip extended attrs from script (reduces Gatekeeper false positives when copying from net fs)
  xattr -cr "$app_path" || true
}

echo "  bundling $BUNDLE.app: "
echo -n "    "

make_app_bundle $BUNDLE $RUN_PATH/$LAUNCHER

echo -n "[dirs] "
mkdir -p $APP_ETC
mkdir -p $APP_SHARE
mkdir -p $APP_COMMON
mkdir -p $APP_HOTKEYS
mkdir -p $APP_ICONS
mkdir -p $APP_ROMS
mkdir -p $APP_DOCS
mkdir -p $APP_BIN
mkdir -p $APP_LIB


# --- copy roms and data into bundle -------------------------------------------

echo -n "[common ROMs] "
for rom in $ROM_COMMON ; do
  if [ ! -d $TOP_DIR/data/$rom ]; then
    echo "ERROR: missing ROM: $TOP_DIR/data/$rom"
    exit 1
  fi
  if [ ! -d "$APP_ROMS/$rom" ]; then
      mkdir "$APP_ROMS/$rom"
  fi
  copy_tree "$TOP_DIR/data/$rom" "$APP_ROMS/$rom"
  (cd "$APP_ROMS/$rom" && eval "rm -f $ROM_REMOVE")
done

# copy manual into bundle
if [ -f doc/vice.pdf ]; then
  echo -n "[manual] "
  cp doc/vice.pdf "$APP_DOCS"
fi

# any config files from /etc?
if [ -d etc ]; then
  mkdir -p $APP_ETC
  echo -n "[etc"
  (cd etc && tar cf - *) | (cd "$APP_ETC" && tar xf -)
  echo -n "] "
fi

echo


# --- embed emu binaries -------------------------------------------------------

if [ "$BUNDLE" = "VICE" ]; then
  BINARIES="$EMULATORS"
else
  BINARIES="$BUNDLE"
fi

for emu in $BINARIES ; do
  echo -n "     embedding $emu: "

  # copy binary
  echo -n "[binary] "
  if [ ! -e src/$emu ]; then
    echo "ERROR: missing binary: src/$emu"
    exit 1
  fi
  cp src/$emu $APP_BIN/$emu

  # strip binary
  if [ x"$STRIP" = "xstrip" ]; then
    echo -n "[strip] "
    /usr/bin/strip $APP_BIN/$emu
  fi

  # copy any needed "local" libs
  copy_thing_libs "$APP_BIN/$emu"

  # copy emulator ROM
  eval "ROM=\${ROM_$emu}"
  echo -n "[ROM=$ROM] "
  if [ ! -d $TOP_DIR/data/$ROM ]; then
    echo "ERROR: missing ROM: $TOP_DIR/data/$ROM"
    exit 1
  fi
  if [ ! -d "$APP_ROMS/$ROM" ]; then
      mkdir "$APP_ROMS/$ROM"
  fi
  copy_tree "$TOP_DIR/data/$ROM" "$APP_ROMS/$ROM"
  (cd $APP_ROMS/$ROM && eval "rm -f $ROM_REMOVE")

  echo -n "[app] "
  make_app_bundle $emu $RUN_PATH/$REDIRECT_LAUNCHER

  # ready
  echo
done


# --- emu runtime depenencies --------------------------------------------------

cp "$TOP_DIR/src/arch/shared/macOS-common-runtime.sh" "$APP_BIN/common-runtime.sh"

# Can use dtrace in a terminal and run x64sc.app to find runtime libs that aren't directly linked:
# sudo dtrace -n 'syscall::*stat*:entry /execname=="x64sc"/ { printf("%s", copyinstr(arg0)); }'
# sudo dtrace -n 'syscall::*open*:entry /execname=="x64sc"/ { printf("%s", copyinstr(arg0)); }'

if [ "$UI_TYPE" = "SDL2" ]; then
  cp "$TOP_DIR/src/arch/sdl/macOS-ui-runtime.sh" "$APP_BIN/ui-runtime.sh"
elif [ "$UI_TYPE" = "GTK3" ]; then
  cp "$TOP_DIR/src/arch/gtk3/macOS-ui-runtime.sh" "$APP_BIN/ui-runtime.sh"

  # Gtk runtime stuff
  cp -r $DEPS_PREFIX/lib/gdk-pixbuf-2.0 $APP_LIB
  cp -r $DEPS_PREFIX/lib/gtk-3.0 $APP_LIB
  cp -r $DEPS_PREFIX/etc/gtk-3.0 $APP_ETC
  cp -r $DEPS_PREFIX/share/glib-2.0 $APP_SHARE

  # Get rid of any compiled python that came with glib share
  find $APP_SHARE -name '*.pyc' -exec rm {} \;

  import_scalable_gtk_icons() {
    local in_icons="$DEPS_PREFIX/share/icons/$1"
    local out_icons="$APP_SHARE/icons/$1"

    mkdir "$out_icons"
    # Rewrite the index.theme file to exclude large non-vector assets
    awk '
      BEGIN {
        directories_found = 0
      }
      /^Directories=/ {
        directories_found = 1
        printf "Directories="
        system("echo " $0 " | tr \"=,\" \"\n\" | grep ^scalable | tr \"\n\" \",\" ")
        printf "\n"
        next
      }
      {
        if (!directories_found) {
          print
          next
        }
      }
      /^\[/ {
        eat = 0
        if ($0 !~ /^\[scalable\//) {
          eat = 1
        }
      }
      {
        if (!eat) {
          print
        }
      }' < "$in_icons/index.theme" > "$out_icons/index.theme"
    cp -r "$in_icons/scalable" "$out_icons/"

    # create the icon-theme.cache file
    $GTK_UPDATE_ICON_CACHE "$out_icons/"
  }

  mkdir $APP_SHARE/icons
  import_scalable_gtk_icons Adwaita

  # hicolor is small, just copy it. It doesn't have any scalable assets.
  cp -r $DEPS_PREFIX/share/icons/hicolor "$APP_SHARE/icons/"
  $GTK_UPDATE_ICON_CACHE "$APP_SHARE/icons/hicolor"

  export GTK_EXE_PREFIX="$APP_RESOURCES"
  export GTK_PATH="$APP_LIB/gtk-3.0"
  export DYLD_LIBRARY_PATH="$APP_LIB"

  # Keep only the necessary GTK schemas
  TMPDIR=$(mktemp -d)
  mv "$APP_SHARE/glib-2.0/schemas/org.gtk.Settings."*.xml $TMPDIR/
  find "$APP_SHARE"/glib-2.0/schemas/ -name "*.xml" -exec rm {} \;
  mv $TMPDIR/* "$APP_SHARE/glib-2.0/schemas/"
  rmdir $TMPDIR
  glib-compile-schemas "$APP_SHARE/glib-2.0/schemas"
fi

# .so libs need their libs too
for lib in `find $APP_LIB -name '*.so'`; do
  copy_thing_libs "$lib"
done

# Some libs are loaded at runtime
if grep -q "^#define HAVE_EXTERNAL_LAME " "src/config.h"; then
  cp $DEPS_PREFIX/lib/libmp3lame.dylib "$APP_LIB/"
  copy_thing_libs $DEPS_PREFIX/lib/libmp3lame.dylib
fi

if [ "$UI_TYPE" = "GTK3" ]; then
  # GDK-Pixbuf loaders.cache
  # Discover loader .so files from the bundle and feed them to the query tool:
  gdk-pixbuf-query-loaders $(find "$APP_LIB/gdk-pixbuf-2.0/2.10.0/loaders" -name '*.so') \
     > "$APP_LIB/gdk-pixbuf-2.0/2.10.0/loaders.cache"
  sed -i '' -e "s,$(pwd)/$APP_LIB/,,g" "$APP_LIB/gdk-pixbuf-2.0/2.10.0/loaders.cache"

  # GTK3 IM modules cache
  # keep only im-quartz.so and replace the cache
  find "$APP_LIB/gtk-3.0/3.0.0/immodules/" -name '*.so' ! -name 'im-quartz.so' -exec rm -f {} \;
  echo '"gtk-3.0/3.0.0/immodules/im-quartz.so"' > "$APP_LIB/gtk-3.0/3.0.0/immodules.cache"
  echo '"quartz" "Mac OS X Quartz" "gtk30" "" "ja:ko:zh:*"' >> "$APP_LIB/gtk-3.0/3.0.0/immodules.cache"
fi


# --- copy tools ---------------------------------------------------------------

BIN_DIR=$BUILD_DIR/bin
mkdir $BIN_DIR

for tool_file in $TOOLS ; do
  tool_name=$(basename $tool_file)
  echo -n "  copying tool $tool_name: "

  # copy binary
  echo -n "[binary] "
  if [ ! -e src/$tool_file ]; then
    echo "ERROR: missing binary: src/$tool_file"
    exit 1
  fi
  cp src/$tool_file $APP_BIN/$tool_name

  # strip binary
  if [ x"$STRIP" = "xstrip" ]; then
    echo -n "[strip] "
    /usr/bin/strip $APP_BIN/$tool_name
  fi

  # copy any needed "local" libs
  copy_thing_libs "$APP_BIN/$tool_name"

  # ready
  echo
done


# --- deduplicate libs ---------------------------------------------------------

echo "Deduplicating libs"

relative_path() {
  python3 -c 'import os,sys; print(os.path.relpath(sys.argv[1], os.path.dirname(sys.argv[2])))' "$1" "$2"
}

for lib in $(find $APP_LIB -name '*.dylib' | sort -V -r); do
  if [ -L "$lib" ]; then
    continue
  fi

  for potential_duplicate in $(find $APP_LIB -type f -name '*.dylib' | sort -V -r); do
    if [ "$potential_duplicate" = "$lib" ]; then
      continue
    fi

    if cmp -s "$potential_duplicate" "$lib"; then
      echo "Replacing $(basename $potential_duplicate) with symlink to $(relative_path "$lib" "$potential_duplicate")"
      chmod u+w "$potential_duplicate"
      rm "$potential_duplicate"
      ln -s "$(relative_path "$lib" "$potential_duplicate")" "$potential_duplicate"
    fi
  done
done


# --- update linking for distribution ------------------------------------------

relink () {
  local thing=$1
  local thing_basename=`basename $lib`

  #
  # Any link to a lib in $DEPS_PREFIX is updated to @rpath/$lib
  #

  THING_LIBS=$(find_thing_libs "$thing")

  for thing_lib in $THING_LIBS; do
    install_name_tool -change $thing_lib "@rpath/$(basename $thing_lib)" $thing \
      2> >(grep -v "invalidate the code signature")
  done

  #
  # Update the shared library identification name - ignores non shared libraries
  # so we can blindly call it.
  #

  install_name_tool -id @rpath/$thing_basename $thing \
    2> >(grep -v "invalidate the code signature")

  #
  # If any existing rpaths exist, remove them.
  # Then an an rpath to our bundled lib folder.
  #

  set +o pipefail
  THING_RPATHS=$(otool -l $thing | grep -A2 LC_RPATH | grep path | sed -E 's/^[[:space:]]+path[[:space:]]+(.*)[[:space:]]+\(offset.*$/\1/')
  set -o pipefail

  for rpath in $THING_RPATHS; do
    install_name_tool -delete_rpath $rpath $thing \
      2> >(grep -v "invalidate the code signature")
  done

  install_name_tool -add_rpath @executable_path/../lib $thing \
    2> >(grep -v "invalidate the code signature")
}

echo "Relinking libs and binaries to relative bundle paths"

for lib in $(find $APP_LIB -type f -name '*.dylib' -or -name '*.so'); do
  relink $lib
  chmod 444 $lib
done

for bin in $BINARIES $TOOLS; do
  relink $APP_BIN/$(basename $bin)
  chmod 555 $APP_BIN/$(basename $bin)
done

# --- create general command line launchers ------------------------------------

echo "  creating command line launchers"
# TODO replace these with a small C program that we can code sign.
for emu in $EMULATORS $TOOLS; do
  emu=$(basename $emu)
  cat << "HEREDOC" | sed 's/^    //' > "$BIN_DIR/$emu"
    #!/bin/bash
    export VICE_INITIAL_CWD="$(pwd)"
    export PROGRAM="$(basename "$0")"
    "$(dirname "$0")/../VICE.app/Contents/MacOS/VICE" "$@"
HEREDOC
  chmod +x "$BIN_DIR/$emu"
done

cat << "HEREDOC" | sed 's/^  //' > "$BUILD_DIR/Terminal-README.txt"
  The launchers in bin are intended to be invoked from a terminal, and make
  visible the log output that is hidden when lanching the .app versions.

  From macOS 10.15 (Catalina) onwards, double clicking these in Finder
  will not initially work due to not being able to codesign a shell script.
  But after trying, you can open the 'Security & Privacy' section of System
  Preferences and there will be an 'Open Anyway' button that allows it.
HEREDOC


# --- copy docs ----------------------------------------------------------------

echo "  copying documents"

mkdir $BUILD_DIR/doc
cp README $BUILD_DIR/doc/README.txt
if [ -f doc/vice.pdf ]; then
  cp doc/vice.pdf $BUILD_DIR/doc/
fi

# Readme-GTK3.txt is pointless but the others have content
if [ "$UI_TYPE" != "GTK3" ]; then
  cp "$TOP_DIR/doc/readmes/Readme-$UI_TYPE.txt" $BUILD_DIR/doc/
fi

# --- copy fonts ---------------------------------------------------------------

echo "  copying fonts"
for FONT in $(ls -1 $TOP_DIR/data/common/*.ttf) ; do
  cp "$FONT" "$APP_COMMON/"
done

if [ "$UI_TYPE" = "GTK3" ]; then
  # --- copy vice.gresource ---
  echo "  copying vice.gresource"
  cp data/common/vice.gresource "$APP_COMMON/"

  # --- copy GLSL shaders ---
  mkdir -p "$APP_GLSL"
  for shader in $(find "$TOP_DIR/data/GLSL/" -type f -name '*.vert' -or -name '*.frag'); do
    cp "$shader" "$APP_GLSL/"
  done
fi

# --- copy hotkeys files ---
cp "$TOP_DIR/data/hotkeys/"*.vhk "$APP_HOTKEYS/"


# --- code signing (for Apple notarisation) ------------------------------------

code_sign_file () {
  if [ "$CODE_SIGN_ID" = "-" ]; then
    # Ad-hoc signing
    codesign --force --sign - -f "$1"
  else
    # Signing with provided identity. Can intermittently fail due to server allocated timestamps.
    local codesign_succeeded=false
    for in in $(seq 1 4)
    do
      if codesign -s "$CODE_SIGN_ID" --timestamp --options runtime -f "$1"
      then
        codesign_succeeded=true
        break
      else
        >&2 echo "Codesign failed, will retry"
        sleep 1
      fi
    done

    if ! $codesign_succeeded
    then
      >&2 echo "Codesign failed, giving up"
      false
    fi
  fi
}

if [ -n "$CODE_SIGN_ID" ]; then
  echo "  code signing"

  for lib in $(find $APP_LIB -type f -name '*.dylib' -or -name '*.so'); do
    code_sign_file $lib
  done

  for bin in $BINARIES $TOOLS ; do
    code_sign_file $APP_BIN/$(basename $bin)
  done

  for app in $(find $BUILD_DIR -type d -name '*.app'); do
    code_sign_file $app
  done
fi


# --- make dmg? ----------------------------------------------------------------

if [ x"$ZIP" = "xnozip" ]; then
  echo "ready. created dist directory: $BUILD_DIR"
  du -sh $BUILD_DIR
else
  du -sh $BUILD_DIR

  # image name
  BUILD_IMG=$BUILD_DIR.dmg
  BUILD_TMP_IMG=$BUILD_DIR.tmp.dmg
  DMG_DIR=.dmg.tmp

  # The dmg will contain a single draggable folder and a symlink to /Applications
  rm -rf $DMG_DIR
  mkdir $DMG_DIR
  ln -s /Applications $DMG_DIR/
  mv $BUILD_DIR $DMG_DIR

  # Create the image and format it
  echo "  creating DMG"
  hdiutil create -fs HFS+ -srcfolder $DMG_DIR $BUILD_TMP_IMG -volname $BUILD_DIR -ov -quiet

  mv $DMG_DIR/$BUILD_DIR .
  rm -rf $DMG_DIR

  # Compress the image
  echo "  compressing DMG"
  hdiutil convert $BUILD_TMP_IMG -format UDZO -o $BUILD_IMG -ov -quiet
  rm -f $BUILD_TMP_IMG

  # Sign the final DMG
  if [ -n "$CODE_SIGN_ID" ]; then
    echo "  signing DMG"
    code_sign_file $BUILD_IMG
  fi

  echo "ready. created dist file: $BUILD_IMG"
  du -sh $BUILD_IMG
  md5 -q $BUILD_IMG
fi

# --- show warnings ------------------------------------------------------------

if $WARN_CODE_SIGN; then
  cat <<HEREDOC | sed 's/^ *//'

    ****
    CODE_SIGN_ID environment variable not set, using ad-hoc signature. User will
    need to override Gatekeeper to run the apps.

    Run 'security find-identity -v -p codesigning' to list available identities,
    and set CODE_SIGN_ID to something like "Developer ID Application: <NAME> (<ID>)".
    ****

HEREDOC
fi

if test x"$ENABLEARCH" = "xyes"; then
  cat <<HEREDOC | sed 's/^ *//'

    ****
    Warning: binaries are optimized for your system and might not run on a different system,
    use --enable-arch=no to avoid this.
    ****

HEREDOC
fi

if [ ! -e doc/vice.pdf ]; then
  cat <<HEREDOC | sed 's/^ *//'

    ****
    Warning: This binary distribution does not include vice.pdf as it was not built.
    ****

HEREDOC
fi