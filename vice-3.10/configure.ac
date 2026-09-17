dnl This file is part of VICE, the Versatile Commodore Emulator.
dnl See README for copyright notice.
dnl
dnl Process this file with GNU autoconf to produce a configure script.
dnl

dnl Set VICE version number.
dnl ONLY bump this on release, any revision in the version string indicates
dnl a development version. Don't add any strings to the version number (such
dnl as '-dev' or 'beta'), the '-r$SVNREV' revision string is the only thing we
dnl alllow after the version number.
m4_define(vice_version_major, 3)
m4_define(vice_version_minor, 10)
m4_define(vice_version_build, 0)

dnl Some people lose their shit if a version ends in .0
dnl also turns out some people lose it if it doesn't :)
m4_define(vice_version,
          ifelse(vice_version_build, 0,
                 ifelse(vice_version_label, ,
                        [vice_version_major.vice_version_minor],
                        [vice_version_major.vice_version_minor]),
                 ifelse(vice_version_label, ,
                        [vice_version_major.vice_version_minor.vice_version_build],
                        [vice_version_major.vice_version_minor.vice_version_build])))

dnl The major/minor stuff gets replaced in autogen.sh
AC_INIT([vice], [vice_version])
dnl We save the command line options here since AM_INIT_AUTOMAKE and other
dnl macros clobber $@:
cmdline_options="$@"
AC_DEFINE_UNQUOTED([CONFIGURE_FLAGS], ["$cmdline_options"],[Flags passed to configure])
AC_CONFIG_SRCDIR([src/maincpu.c])
AM_INIT_AUTOMAKE([-Wno-portability foreign])

dnl Make sure autotools doesn't add '-g -O2' by default when the user's CFLAGS
dnl or CXXFLAGS are empty
dnl This has to happen before any other AC_* calls, so leave it here please.
: ${CFLAGS=""}
: ${CXXFLAGS=""}
: ${OBJCFLAGS=""}

dnl Set VICE's compiler flags (can be overridden by the user)
VICE_CPPFLAGS="-I\$(top_srcdir)/src/arch/systemheaderoverride"
VICE_CFLAGS="-g -O3"
VICE_CXXFLAGS="-g -O3"
VICE_OBJCFLAGS="-g -O3"
VICE_LDFLAGS=""

dnl Avoid "ar: `u' modifier ignored since `D' is the default (see `U')" warning
dnl caused by a longstanding bug in libtool.
dnl See https://github.com/rsyslog/rsyslog/issues/1179 for some details.
: ${ARFLAGS="cr"}
: ${AR_FLAGS="cr"}
AC_SUBST(ARFLAGS)
AC_SUBST(AR_FLAGS)

AC_CONFIG_MACRO_DIR([m4])

dnl Make version number visible to generated files and source
VICE_VERSION_MAJOR=vice_version_major
VICE_VERSION_MINOR=vice_version_minor
VICE_VERSION_BUILD=vice_version_build
VICE_VERSION=vice_version
VERSION_RC="$VICE_VERSION_MAJOR,$VICE_VERSION_MINOR,$VICE_VERSION_BUILD,0"
VERSION_COMBINED=`expr $VICE_VERSION_MAJOR \* 65536 + $VICE_VERSION_MINOR \* 256 + $VICE_VERSION_BUILD`
AC_SUBST(VICE_VERSION_MAJOR)
AC_SUBST(VICE_VERSION_MINOR)
AC_SUBST(VICE_VERSION_BUILD)
AC_SUBST(VICE_VERSION)
AC_SUBST(VERSION_RC)
AC_SUBST(VERSION_COMBINED)


dnl Command-line options
dnl
dnl The whitespace (2 spaces) before --with[out]-option is significant, as is
dnl the whitespace before the option description, in order to align these
dnl properly with the auto-generated options of configure.ac
VICE_ARG_INIT()

dnl
dnl Sound options
dnl
VICE_ARG_WITH_LIST(alsa,                    [  --without-alsa          do not use the ALSA sound system])
VICE_ARG_WITH_LIST(fastsid,                 [  --with-fastsid          use the FastSID engine])
VICE_ARG_WITH_LIST(flac,                    [  --with-flac             use FLAC support])
VICE_ARG_WITH_LIST(mpg123,                  [  --with-mpg123           use MPG123 for MP3 decoding])
VICE_ARG_WITH_LIST(oss,                     [  --with-oss              use the OSS sound system])
VICE_ARG_WITH_LIST(pulse,                   [  --without-pulse         do not use PulseAudio sound system])
VICE_ARG_WITH_LIST(resid,                   [  --without-resid         do not use the reSID engine])
VICE_ARG_WITH_LIST(sdlsound,                [  --with-sdlsound         use SDL sound system])
VICE_ARG_WITH_LIST(sun,                     [  --without-sun           do not use the sun/netbsd sound system])
VICE_ARG_WITH_LIST(vorbis,                  [  --with-vorbis           use Ogg/Vorbis support])
VICE_ARG_WITH_LIST(lame,                    [  --with-lame             use LAME for MP3 encoding])
VICE_ARG_WITH_LIST(static-lame,             [  --with-static-lame      statically link LAME MP3])
VICE_ARG_WITH_LIST(portaudio,               [  --with-portaudio        use PortAudio for sound input])
VICE_ARG_ENABLE_LIST(catweasel,             [  --enable-catweasel      enables CatWeasel (PCI/Zorro/Clockport SID card) support])
VICE_ARG_ENABLE_LIST(hardsid,               [  --disable-hardsid       disables HardSID support])
VICE_ARG_ENABLE_LIST(midi,                  [  --enable-midi           enable MIDI support])
VICE_ARG_ENABLE_LIST(new8580filter,         [  --disable-new8580filter disable new 8580 filters])
VICE_ARG_ENABLE_LIST(parsid,                [  --enable-parsid         enables ParSID support])

dnl
dnl Graphics options
dnl
VICE_ARG_WITH_LIST(gif,                     [  --with-gif              use GIF screenshot system])
VICE_ARG_WITH_LIST(png,                     [  --without-png           do not use the PNG screenshot system])
dnl Only supported for SDL1
VICE_ARG_ENABLE_LIST(hwscale,               [  --disable-hwscale       disables GPU scaling of emulated screen (SDL1 UI only)])

dnl
dnl Misc build options
dnl
VICE_ARG_WITH_LIST(unzip-bin,               [  --with-unzip-bin        enables distribution of unzip.exe in the windows bindist])
VICE_ARG_WITH_LIST(libieee1284,             [  --with-libieee1284      use the libieee1284 parallel port library])
VICE_ARG_ENABLE_LIST(arch,                  [  --enable-arch[[=arch]]  enable architecture specific compilation [[default=yes]]], [], [enable_arch=yes])
VICE_ARG_ENABLE_LIST(cpuhistory,            [  --disable-cpuhistory    disable the 65xx cpu history feature])
VICE_ARG_ENABLE_LIST(ethernet,              [  --enable-ethernet       enables The Final Ethernet emulation])
VICE_ARG_ENABLE_LIST(ipv6,                  [  --disable-ipv6          disables the checking for IPv6 compatibility])
VICE_ARG_ENABLE_LIST(no-pic,                [  --enable-no-pic         enable the use of the no-pic switch [[default=yes]]])
VICE_ARG_ENABLE_LIST(realdevice,            [  --disable-realdevice    disables access to real peripheral devices (CBM4Linux/OpenCBM)])
VICE_ARG_ENABLE_LIST(rs232,                 [  --disable-rs232         disable RS232 support])
VICE_ARG_ENABLE_LIST(openmp,                [  --disable-openmp        disable OpenMP acceleration])

dnl
dnl Developer options
dnl
VICE_ARG_ENABLE_LIST(cmake,                 [  --enable-cmake          enable generation of CMakeLists.txt files [[default=no]]])
VICE_ARG_ENABLE_LIST(debug,                 [  --enable-debug          enable debug source options])
VICE_ARG_ENABLE_LIST(debug-gtk3ui,          [  --enable-debug-gtk3ui   enables debugging for the GTK3 UI])
VICE_ARG_ENABLE_LIST(debug-threads,         [  --enable-debug-threads  enable debug messages about the threading code [[default=no]]])
VICE_ARG_ENABLE_LIST(x64,                   [  --enable-x64            enable building of the old x64 emulator [[default=no]]])
VICE_ARG_ENABLE_LIST(x64-image,             [  --enable-x64-image      enable X64 image support [[default=no]]])
VICE_ARG_ENABLE_LIST(optimization,          [  --disable-optimization  disable code optimization by passing -O0 [[default=no]]])
VICE_ARG_ENABLE_LIST(extra-warnings,        [  --enable-extra-warnings enable anal warnings [[default=no]]])
VICE_ARG_ENABLE_LIST(io-simulation,         [  --enable-io-simulation  enable i/o simulation devices [[default=no]]])
VICE_ARG_ENABLE_LIST(experimental-devices,  [  --enable-experimental-devices    enable experimental device emulation [[default=no]]])

dnl Needed for WIC64
VICE_ARG_WITH_LIST(libcurl,                 [  --without-libcurl       disable libcurl support [[default=no]]])

dnl
dnl UI options
dnl
VICE_ARG_ENABLE_LIST(headlessui,            [  --enable-headlessui     enables building without a UI])
VICE_ARG_ENABLE_LIST(sdl1ui,                [  --enable-sdl1ui         enables SDL 1.2 UI support])
VICE_ARG_ENABLE_LIST(sdl2ui,                [  --enable-sdl2ui         enables SDL 2.x UI support])
VICE_ARG_ENABLE_LIST(gtk3ui,                [  --enable-gtk3ui         enables GTK3 UI support])

dnl If enabled, tries to install XDG .desktop files for running the binaries
dnl from a desktop menu. Only supported for Unix with the exception of MacOS.
dnl Gtk3 only.
VICE_ARG_ENABLE_LIST(desktop-files,         [  --enable-desktop-files  enable installation of XDG desktop files (GTK3 UI only)])
VICE_ARG_ENABLE_LIST(hidmgr,                [  --disable-hidmgr        disable IOHIDManager joystick support on Mac])

dnl
dnl Documentation options
dnl
VICE_ARG_ENABLE_LIST(html-docs,             [  --disable-html-docs     do not generate the HTML documentation [[default=no]]])
VICE_ARG_ENABLE_LIST(pdf-docs,              [  --enable-pdf-docs       generate the PDF documentation [[default=no]]])
VICE_ARG_ENABLE_LIST(platformdox,           [  --enable-platformdox    enable generating of documentation specific to the current host])



dnl
dnl Check for disabling compiler optimization with --disable-optimization
dnl
if test x"$enable_optimization" = "xno"; then
  VICE_CFLAGS="-g -O0"
  VICE_CXXFLAGS="-g -O0"
  VICE_OBJCFLAGS="-g -O0"
fi


AC_ARG_VAR(FW_DIR, [Location of support frameworks (macOS only)])


dnl check for conflicting options
VICE_ARG_LIST_AT_MOST_ONE([--enable-sdlui --enable-sdlui2 --enable-gtk3ui --enable-headlessui], [UI])
AS_IF([test x"$enable_gtk3ui" = "xno" -a x"$enable_sdl1ui" = "xno" -a x"$enable_sdl2ui" = "xno" -a x"$enable_headlessui" = "xno"],
      [AC_MSG_ERROR([You cannot disable all GUIs])])

dnl Try to find out which system we are on...
AC_CANONICAL_HOST

dnl Check for the existence of various programs that VICE and its component systems use

dnl Find compiler
AC_PROG_CC([cc gcc clang])

dnl Make sure the compiler supports C99 at a minimum
AC_PROG_CC_C99
AS_IF([test x"$ac_cv_prog_cc_c99" = "xno"],
      [AC_MSG_ERROR([A C99-compliant compiler is required.])])
dnl Force C99-compliance
dnl     Currently breaks some code depending on POSIX/BSD functions, for that
dnl     we'll have to pass -D_BSD_SOURCE or a similar define to the compiler,
dnl     before #including any header. So for now this is disabled.
dnl VICE_CFLAGS="$VICE_CFLAGS -std=c99"
AC_PROG_CPP
dnl C++ check, at least C++11
AC_PROG_CXX([c++ g++ clang++])
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory])
AC_PROG_CXXCPP
AC_PROG_OBJC([cc gcc clang])

dnl Try global compiler warnings
dnl
dnl Temporarily removed:
dnl     -Wextra (enables -Wsign-compare)
dnl     -Wswitch-default
dnl     -Wswitch-enum
dnl     -Wconversion
dnl     -Wredundant-decls
dnl     -Wnested-externs
dnl     -Wcast-qual
dnl     -Wcast-align
dnl     -Wsign-compare
dnl
dnl (Commenting these out in the following list makes configure barf)
dnl
dnl NOTE - before you optimise away the weird SUCCESS_CFLAGS thing, understand the following:
dnl        clang doesn't exit with an error for an unsupported warning flag by default.
dnl        clang has -Werror=x to promote a warning to an error, but gcc treats that as an error
dnl        clang and gcc both support -Werror to promote all warnings to errors
dnl        initialising and returning the value of arse removes the gcc warning that arse isn't used.
dnl        So, until a better solution is found, -Werror is added while testing -W flag support.
dnl
dnl Warnings added to C should also be added to C++ compiler list. Except these ones because novte:
dnl     -Wstrict-prototypes (g++ warning: "cc1plus: -Wstrict-prototypes is only valid of C/ObjC not C++" - but zero exit code)
dnl     -Wformat-signedness         (novte warnings - these could be addressed with source changes)
dnl
dnl C++ warnings implied by other settings but explicitly disabled:
dnl     -Waddress-of-packed-member  (novte warnings)
dnl     -Wchar-subscripts           (novte warnings)
dnl
dnl -Wunused-but-set-variable is enabled by -Wall in GCC >= 4.6 but doesn't
dnl exist in clang < 13.0. clang-tidy implements this check though, so our
dnl Github code analysis runner should find these issues pre-clang 13.
dnl
m4_foreach([flag],
           [[-W],
            [-Wall],
            [-Wextra],
            [-Wformat],
            [-Wformat-signedness],
            [-Wformat-pedantic],
            [-Wformat-security],
            [-Wshadow],
            [-Wpointer-arith],
            [-Wmissing-prototypes],
            [-Wstrict-prototypes],
            [-Wuninitialized],
            [-Wmissing-field-initializers],
            [-Wunreachable-code],
            [-Wmacro-redefined],
            [-Werror=implicit-function-declaration],
            [-Wfatal-errors],
            [-fdiagnostics-absolute-paths],
            [-Wshorten-64-to-32],
            [-Wdiscarded-qualifiers],
            [-Wno-unknown-pragmas],
            [-Wunused-but-set-variable]],
           [TEST_FLAG=flag
            VICE_CFLAG_REQUEST([$TEST_FLAG])
           ])

dnl Disable extra warnings if requested
dnl
dnl Enabled by default, disable with `--disable-extra-warnings`
if test x"$enable_extra_warnings" != "xyes"; then
    m4_foreach([flag],
               [[-Wno-sign-compare],
                [-Wno-ignored-qualifiers],
                [-Wno-unused-parameter]],
               [TEST_FLAG=flag
                VICE_CFLAG_REQUEST([$TEST_FLAG])
               ])
fi


dnl Check C++ flags
m4_foreach([flag],
           [[-Wall],
            [-Wformat],
            [-Wformat-pedantic],
            [-Wshadow],
            [-Wpointer-arith],
            [-Wuninitialized],
            [-Wunreachable-code],
            [-Wno-unused-parameter],
            [-Wmissing-prototypes],
            [-Werror=implicit-function-declaration],
            [-Wfatal-errors],
            [-fdiagnostics-absolute-paths],
            [-Wshorten-64-to-32]],
           [TEST_FLAG=flag
            VICE_CXXFLAG_REQUEST([$TEST_FLAG])
           ])

dnl VTE specific C++ flags
VTE_CXXFLAGS=""
m4_foreach([flag],
           [[-Wno-address-of-packed-member],
            [-Wno-char-subscripts],
            [-Wno-format-signedness],
            [-Wno-missing-prototypes],
            [-Wno-c99-designator],
            [-Wno-shorten-64-to-32],
            [-Wno-nonnull-compare]],
           [TEST_FLAG=flag
            VICE_CXXFLAG_REQUEST([$TEST_FLAG], [VTE_CXXFLAGS])
            ])
AC_SUBST(VTE_CXXFLAGS)

dnl Monitor specific flags (due to the code generated by yacc/lex)
MONITOR_CFLAGS=""
m4_foreach([flag],
           [[-Wno-unused-label],
            [-Wno-unreachable-code],
            [-Wno-misleading-indentation]],
           [TEST_FLAG=flag
            VICE_CFLAG_REQUEST([$TEST_FLAG], [MONITOR_CFLAGS])
            ])
AC_SUBST(MONITOR_CFLAGS)

AC_PROG_AWK
AS_IF([test x"$AWK" = "x:"], [AC_MSG_ERROR([Could not find awk!])])
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LEX
AS_IF([test x"$LEX" = "x:"], [AC_MSG_ERROR([Could not find either flex or lex!])])

AC_LIB_PROG_LD

AC_CHECK_PROG(AR, ar, ar, ar)
AC_CHECK_PROG(SVN, svn, svn, :)
AC_CHECK_PROG(TAR, tar, tar, :)
AC_CHECK_PROG(SVNVERSION, svnversion, svnversion, :)
AC_CHECK_PROGS(YACC, byacc yacc 'bison -y', [no])
AS_IF([test x"$YACC" = "xno"], [AC_MSG_ERROR([Could not find byacc, yacc or bison!])])

dnl Check for makeinfo, might only be required for AmigaOS since we don't
dnl generate info pages anymore ?
AC_CHECK_PROGS(MAKEINFO, makeinfo, no)
AS_IF([test x"$MAKEINFO" = "xno"],
      [AC_MSG_ERROR([makeinfo is missing, please install texinfo])])

dnl if --with-unzip-bin was given check for unzip and error out if not found
if test x"$with_unzip_bin" = "xyes"; then
  AC_PATH_PROGS(UNZIPBIN, unzip.exe, no)
  if test x"$UNZIPBIN" = "xno"; then
    AC_MSG_ERROR([unzip was requested (--with-unzip-bin), but unzip is missing])
  fi
else
  UNZIPBIN="no"
fi

dnl Check for dos2unix
AC_CHECK_PROGS(DOS2UNIX, dos2unix, no)
AS_IF([test x"$DOS2UNIX" = "xno"],
      [AC_MSG_ERROR([The dos2unix tool is missing, please install dos2unix])])

dnl Check for xa[65]
AC_CHECK_PROGS(XA, [xa xa65], no)
AS_IF([test x"$XA" = "xno"],
      [AC_MSG_ERROR([The xa assembler is missing, either install it via your
                  distribution or build and install it using the latest source
                  from https://www.floodgap.com/retrotech/xa/])])

if test x"$enable_pdf_docs" = xyes; then
  AC_CHECK_PROGS(TEXI2DVI, texi2dvi, no)
  if test x"$TEXI2DVI" = "xno"; then
    AC_MSG_ERROR([PDF docs were requested (--enable-pdf-docs), but texi2dvi is missing])
  fi
  AC_CHECK_PROGS(PDFTEX, pdftex, no)
  if test x"$PDFTEX" = "xno"; then
    AC_MSG_ERROR([PDF docs were requested (--enable-pdf-docs), but pdftex is missing])
  fi

fi

AC_CHECK_PROGS(PERL, perl)

if test x"${enable_html_docs:-yes}" != xno -a x"$PERL" = x; then
  AC_MSG_ERROR([Perl is required to generate the HTML documentation])]
fi

AM_CONDITIONAL([ENABLE_PDF_DOCS], [test x"$enable_pdf_docs" = xyes])
if test x"$enable_pdf_docs" = xyes; then
    VICE_PDF_FILE_NAME="vice.pdf"
else
    VICE_PDF_FILE_NAME=""
fi
AC_SUBST(VICE_PDF_FILE_NAME)

AM_CONDITIONAL([ENABLE_HTML_DOCS], [test x"${enable_html_docs:-yes}" != xno])
AM_CONDITIONAL([HAVE_PERL], [test x"$PERL" != x])

AC_CHECK_PROGS(ICONV, iconv, no)
AS_IF([test x"$ICONV" = x"no"], [AC_MSG_ERROR([iconv is missing])])
AC_CHECK_TOOL(RANLIB, ranlib, :)
AC_CHECK_TOOL(STRIP, strip, :)
AC_CHECK_TOOL(objdump, objdump, no)
AC_CHECK_TOOL(sdl_config, sdl-config, no)

dnl echo "Checking host_os for Windows: host_os = $host_os"
dnl Check for windres, required to link icons into windows binaries
if test x"$host_os" = "xmingw32"; then
    AC_CHECK_TOOL([WINDRES], [windres], [no])
    AS_IF([test x"$WINDRES" = "xno"],
           [AC_MSG_ERROR([Could not find required windres binary!])])
fi


AM_PROG_AS

dnl Other build environment checks
AC_SYS_LARGEFILE
AC_TYPE_SSIZE_T


AC_CONFIG_HEADERS([src/config.h])


is_unix=no
is_unix_x11=no
is_unix_macosx=no
is_unix_linux=no
is_win32=no
is_win32_gtk3=no
is_beos=no


DEBUG_SUPPORT="no "
DEBUG_THREADS_SUPPORT="no "
FEATURE_CPUMEMHISTORY_SUPPORT="no "
HAS_HIDMGR_SUPPORT="no "
HAS_USB_JOYSTICK_SUPPORT="no "
HAVE_AUDIO_UNIT_SUPPORT="no "
HAVE_CATWEASELMKIII_SUPPORT="no "
HAVE_DINPUT_SUPPORT="no "
HAVE_DYNLIB_SUPPORT_TOO="no "
HAVE_FASTSID_SUPPORT="no "
HAVE_FONTCONFIG_SUPPORT="no "
HAVE_GIF_SUPPORT="no "
HAVE_USBSID_SUPPORT="no "
HAVE_HARDSID_SUPPORT="no "
HAVE_HWSCALE_SUPPORT="no "
HAVE_IOSID_SUPPORT="no "
HAVE_IPV6_SUPPORT="no "
HAVE_LIBIEEE1284_SUPPORT="no "
HAVE_LIGHTPEN_SUPPORT="no "
HAVE_MIDI_SUPPORT="no "
HAVE_MOUSE_SUPPORT="no "
HAVE_NETWORK_SUPPORT="no "
HAVE_PARSID_SUPPORT="no "
HAVE_PCIUTILS_SUPPORT="no "
HAVE_PNG_SUPPORT="no "
HAVE_PORTSID_SUPPORT="no "
HAVE_RAWNET_SUPPORT="no "
HAVE_REALDEVICE_SUPPORT="no "
HAVE_RESID_SUPPORT="no "
HAVE_RS232DEV_SUPPORT="no "
HAVE_RS232NET_SUPPORT="no "
HAVE_OPENMP_SUPPORT="no "
HAVE_SDL_NUMJOYSTICKS_SUPPORT="no "
HAVE_SYS_AUDIOIO_H_SUPPORT="no "
HAVE_SYS_AUDIO_H_SUPPORT="no "
HAVE_ZLIB_SUPPORT="no "
LINUX_JOYSTICK_SUPPORT="no "
USE_ALSA_SUPPORT="no "
USE_COREAUDIO_SUPPORT="no "
USE_DMEDIA_SUPPORT="no "
USE_DXSOUND_SUPPORT="no "
USE_FLAC_SUPPORT="no "
USE_LAMEMP3_SUPPORT="no "
USE_MPG123_SUPPORT="no "
USE_OSS_SUPPORT="no "
USE_PORTAUDIO_SUPPORT="no "
USE_PULSE_SUPPORT="no "
USE_SANDBOX_MODE="no "
USE_SDL_AUDIO_SUPPORT="no "
USE_VORBIS_SUPPORT="no "
USE_NEW_8580_FILTER="no "
USE_DESKTOP_FILES="no "
HAVE_X64_IMAGE_SUPPORT="no "
HAVE_CAPABILITIES_SUPPORT="no "
HAVE_LIBCURL_SUPPORT="no "
HAVE_LINUX_EVDEV_SUPPORT="no "
HAVE_EXPERIMENTAL_DEVICES_SUPPORT="no "

dnl Why do we have this?
AM_CONDITIONAL(DUMMY_COMPILE, false)

dnl Set all FOO_COMPILE conditionals to false to simplify the code below

dnl BEOS_COMPILE means both 'classic' BeOS and Haiku
AM_CONDITIONAL(BEOS_COMPILE, false)
dnl HAIKU_COMPILE means specifically Haiku
AM_CONDITIONAL(HAIKU_COMPILE, false)
AM_CONDITIONAL(LINUX_COMPILE, false)
dnl BSD_COMPILE means Free, Net, Open and DragonFly BSD
AM_CONDITIONAL(BSD_COMPILE, false)
AM_CONDITIONAL(FREEBSD_COMPILE, false)
AM_CONDITIONAL(NETBSD_COMPILE, false)
AM_CONDITIONAL(OPENBSD_COMPILE, false)
AM_CONDITIONAL(DRAGONFLYBSD_COMPILE, false)


dnl Enable experimental emulation of certain devices
AS_IF([test x"$enable_experimental_devices" = "xyes"],
  [
    AC_DEFINE(HAVE_EXPERIMENTAL_DEVICES,,[Enable experimental devices emulation.])
    HAVE_EXPERIMENTAL_DEVICES_SUPPORT="yes"
  ])

AS_IF([test x"$enable_cpuhistory" != "xno"],
  [
    AC_DEFINE(FEATURE_CPUMEMHISTORY,,[Use the 65xx cpu history feature.])
    FEATURE_CPUMEMHISTORY_SUPPORT="yes"
  ])

dnl New 8580 filters: Changed on 2020-08-23 from default 'no' to default 'yes'.
dnl If we don't get any (valid) complaints, we should make this non-configurable.
AS_IF([test x"$enable_new8580filter" != "xno"],
  [
    AC_DEFINE(HAVE_NEW_8580_FILTER,,[Use the new 8580 filter])
    USE_NEW_8580_FILTER="yes"
  ])

AM_CONDITIONAL(VICE_QUIET, test x"$verbose" != "xyes" -a x"$enable_silent_rules" = "x")

user_cflags=$CFLAGS

dnl Make sure we have pkg-config >- 0.9.0
PKG_PROG_PKG_CONFIG
if test -z "$PKG_CONFIG"; then
  AC_MSG_ERROR([Could not locate pkg-config, please install pkg-config])
fi

SOUND_DRIVERS=""

dnl Check if a GUI order check needs to be done
AS_IF([test x"$enable_gtk3ui" != "xyes" -a x"$enable_sdl1ui" != "xyes" -a x"$enable_sdl2ui" != "xyes" -a x"$enable_headlessui" != "xyes"],
      [check_guis=yes])

dnl Check if gtk3 ui should be checked
if test x"$check_guis" = "xyes"; then
  if test x"$enable_gtk3ui" != "xno"; then
    check_gtk3=yes
  fi
else
  if test x"$enable_gtk3ui" = "xyes"; then
    check_gtk3=yes
  fi
fi


old_CFLAGS="$CFLAGS"
CFLAGS="$VICE_CFLAGS"
dnl Do these checks before any Gtk3 flags are added
AC_MSG_CHECKING(for time_t in time.h)
AC_CACHE_VAL(bu_cv_decl_time_t_time_h,
[AC_TRY_COMPILE([#include <time.h>], [time_t i;],
bu_cv_decl_time_t_time_h=yes, bu_cv_decl_time_t_time_h=no)])
AC_MSG_RESULT($bu_cv_decl_time_t_time_h)
if test $bu_cv_decl_time_t_time_h = yes; then
  AC_DEFINE([HAVE_TIME_T_IN_TIME_H],,
            [Is the type time_t defined in <time.h>?])
fi

AC_MSG_CHECKING(for time_t in sys/types.h)
AC_CACHE_VAL(bu_cv_decl_time_t_types_h,
[AC_TRY_COMPILE([#include <sys/types.h>], [time_t i;],
bu_cv_decl_time_t_types_h=yes, bu_cv_decl_time_t_types_h=no)])
AC_MSG_RESULT($bu_cv_decl_time_t_types_h)
if test $bu_cv_decl_time_t_types_h = yes; then
  AC_DEFINE([HAVE_TIME_T_IN_TYPES_H],,
            [Is the type time_t defined in <sys/types.h>?])
fi

if test $bu_cv_decl_time_t_time_h = yes; then
  AC_CHECK_SIZEOF([time_t],[],[#include <time.h>])
else
  AC_CHECK_SIZEOF([time_t],[],[#include <sys/types.h>])
fi

if test "x$ac_cv_sizeof_time_t" = "x4"; then
  AC_DEFINE([TIME_T_IS_32BIT],,[time_t is 32 bit])
elif test "x$ac_cv_sizeof_time_t" = "x8"; then
  AC_DEFINE([TIME_T_IS_64BIT],,[time_t is 64 bit])
else
  AC_MSG_ERROR([can not figure type of time_t])
fi


dnl Add runtime/linker paths for *BSD
CFLAGS="$old_CFLAGS"

case "$host_os" in
  freebsd*)
    AC_DEFINE(BSD_COMPILE,,[Are we compiling for *BSD?])
    AC_DEFINE(FREEBSD_COMPILE,,[Are we compiling for FreeBSD?])
    AM_CONDITIONAL(BSD_COMPILE, true)
    AM_CONDITIONAL(FREEBSD_COMPILE, true)
    CPPFLAGS="$CPPFLAGS -I/usr/local/include"
    fbsdrtp=`${CONFIG_SHELL-/bin/sh} "$srcdir/config.rpath" "$CC" "$GCC" "$VICE_LDFLAGS" "$LD" "$with_gnu_ld" "$host" "/usr/local/lib"`
    VICE_LDFLAGS="$VICE_LDFLAGS $fbsdrtp -L/usr/local/lib"
    ;;
  netbsd*)
    AC_DEFINE(BSD_COMPILE,,[Are we compiling for *BSD?])
    AC_DEFINE(NETBSD_COMPILE,,[Are we compiling for NetBSD?])
    AM_CONDITIONAL(BSD_COMPILE, true)
    AM_CONDITIONAL(NETBSD_COMPILE, true)
    CPPFLAGS="$CPPFLAGS -I/usr/pkg/include -I/usr/include"
    nbsdrtp=`${CONFIG_SHELL-/bin/sh} "$srcdir/config.rpath" "$CC" "$GCC" "$VICE_LDFLAGS" "$LD" "$with_gnu_ld" "$host" "/usr/pkg/lib"`
    VICE_LDFLAGS="$VICE_LDFLAGS $nbsdrtp -L/usr/pkg/lib"
    if test -d "/usr/X11R7/lib"; then
      CPPFLAGS="$CPPFLAGS -I/usr/X11R7/include"
      rtp=`${CONFIG_SHELL-/bin/sh} "$srcdir/config.rpath" "$CC" "$GCC" "$VICE_LDFLAGS" "$LD" "$with_gnu_ld" "$host" "/usr/X11R7/lib"`
      VICE_LDFLAGS="$VICE_LDFLAGS $rtp"
      LDFLAGS="$LDFLAGS -L/usr/X11R7/lib"
    else
      if test -d "/usr/X11R6/lib"; then
        CPPFLAGS="$CPPFLAGS -I/usr/X11R6/include"
        rtp=`${CONFIG_SHELL-/bin/sh} "$srcdir/config.rpath" "$CC" "$GCC" "$VICE_LDFLAGS" "$LD" "$with_gnu_ld" "$host" "/usr/X11R6/lib"`
        VICE_LDFLAGS="$VICE_LDFLAGS $rtp -L/usr/X11R6/lib"
        LDFLAGS="$LDFLAGS -L/usr/X11R6/lib"
      fi
    fi
    ;;
  openbsd*)
    AC_DEFINE(BSD_COMPILE,,[Are we compiling for *BSD?])
    AC_DEFINE(OPENBSD_COMPILE,,[Are we compiling for OpenBSD?])
    AM_CONDITIONAL(BSD_COMPILE, true)
    AM_CONDITIONAL(OPENBSD_COMPILE, true)
    CPPFLAGS="$CPPFLAGS -I/usr/local/include"
    obsdrtp=`${CONFIG_SHELL-/bin/sh} "$srcdir/config.rpath" "$CC" "$GCC" "$VICE_LDFLAGS" "$LD" "$with_gnu_ld" "$host" "/usr/local/lib"`
    VICE_LDFLAGS="$VICE_LDFLAGS $obsdrtp -L/usr/local/lib"
    ;;
  dragonfly*)
    AC_DEFINE(BSD_COMPILE,,[Are we compiling for *BSD?])
    AC_DEFINE(DRAGONFLYBSD_COMPILE,,[Are we compiling for DragonFly BSD?])
    AM_CONDITIONAL(BSD_COMPILE, true)
    AM_CONDITIONAL(DRAGONFLYBSD_COMPILE, true)
    CPPFLAGS="$CPPFLAGS -I/usr/local/include"
    dfbsdrtp=`${CONFIG_SHELL-/bin/sh} "$srcdir/config.rpath" "$CC" "$GCC" "$VICE_LDFLAGS" "$LD" "$with_gnu_ld" "$host" "/usr/local/lib"`
    VICE_LDFLAGS="$VICE_LDFLAGS $dfbsdrtp -L/usr/local/lib"
    ;;
  dnl Why is this in BSD section?
  cygwin*)
    VICE_CPPFLAGS="$VICE_CPPFLAGS -D_GNU_SOURCE=1"
    ;;
  dnl Again: why?
  mingw32*)
    is_win32=yes
    ;;
  *)
    ;;
esac


if test "x$CC" = "xgcc"; then
  GCC=yes
fi


if test x"$enable_arch" = "xno"; then
  march=""
  ENABLE_ARCH=no
else
  if test x"$enable_arch" = "xyes"; then
    AC_MSG_CHECKING([if the compiler supports -march=native])
    ORIG_CFLAGS="$VICE_CFLAGS"
    VICE_CFLAGS="$VICE_CFLAGS -march=native"
    AC_TRY_COMPILE([],
                   [int test;],
                   [ AC_MSG_RESULT(yes)
                     ENABLE_ARCH=yes
                     march="-march=native" ],
                   [ AC_MSG_RESULT(no)
                     ENABLE_ARCH=no
                     march="" ])
    VICE_CFLAGS="$ORIG_CFLAGS"
  else
    AC_MSG_CHECKING([if the compiler supports -march=$enable_arch])
    ORIG_CFLAGS="$VICE_CFLAGS"
    VICE_CFLAGS="$VICE_CFLAGS -march=$enable_arch"
    AC_TRY_COMPILE([],
                   [int test;],
                   [ AC_MSG_RESULT(yes)
                     ENABLE_ARCH=yes
                     march="-march=$enable_arch" ],
                   [ AC_MSG_RESULT(no)
                     ENABLE_ARCH=no
                     march="" ])
    VICE_CFLAGS="$ORIG_CFLAGS"
  fi
fi

AC_SUBST(ENABLE_ARCH)

dnl check if the compiler supports -posix
if test x"$posix_check" = "xyes"; then
  AC_MSG_CHECKING([if the compiler supports -posix])
  ORIG_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -posix"
  AC_TRY_COMPILE([],
                 [int test;],
                 [ AC_MSG_RESULT(yes)],
                 [ AC_MSG_RESULT(no)
                   CFLAGS=$ORIG_CFLAGS ])
fi

dnl check if ar truncates object names
if test x"$ar_check" != "xno"; then
  AC_MSG_CHECKING(if ar truncates object names)
  rm -f vicetest.c
  touch vicetest.c
  $CC -c vicetest.c -o some_long_object_name.o
  rm -f vicetest.c
  rm -f libvicetest.a
  $AR cru libvicetest.a some_long_object_name.o
  rm -f some_long_object_name.o
  $AR x libvicetest.a
  if test -f some_long_object_name.o; then
    AC_MSG_RESULT(no)
    rm -f some_long_object_name.o libvicetest.a
  else
    AC_MSG_RESULT(yes)
    rm -f some*.o libvicetest.a
    AC_MSG_ERROR([ar is too old, upgrade your ar])
  fi
fi


AM_CONDITIONAL(USE_SVN_REVISION,false)
AM_CONDITIONAL(USE_SVN_REVISION_OVERRIDE,false)

AC_ARG_VAR(SVN_REVISION_OVERRIDE,,[Subversion revision number to embed in build (without the 'r')])

if test x"$SVN_REVISION_OVERRIDE" != "x"; then
  AM_CONDITIONAL(USE_SVN_REVISION,true)
  AM_CONDITIONAL(USE_SVN_REVISION_OVERRIDE,true)
  AC_DEFINE(USE_SVN_REVISION,,[define when using the svn revision in addition to the version])
  AC_DEFINE(USE_SVN_REVISION_OVERRIDE,,[define when using an svn revision passed to configure via SVN_REVISION_OVERRIDE env var])
  dnl Make SVN revision override available in Makefiles so we can pass the revision
  dnl to make-bindist scripts:
  AC_SUBST([SVN_REVISION_OVERRIDE], [$SVN_REVISION_OVERRIDE])
else
  if test x"$SVN" != "x"; then
    svnrevision=`$SVN 2>dummy.tmp info $srcdir | grep Revision | cut -d " " -f 2`
    rm dummy.tmp
    if test x"$svnrevision" != "x"; then
      AC_DEFINE(USE_SVN_REVISION,,[define when using the svn revision in addition to the version])
      AM_CONDITIONAL(USE_SVN_REVISION,true)
    fi
  fi
fi

JOYSTICK_DRIVERS=""

dnl Hardware SID drivers (obsolete so disabled by default)
AM_CONDITIONAL(HAVE_CATWEASELMKIII, false)
AM_CONDITIONAL(HAVE_PARSID, false)

if test x"$is_win32" = "xyes" -o x"$is_win32_gtk3" = "xyes"; then

  dnl This is used by subsequent tests.
  ac_cv_prog_gcc=yes
  GCC=yes

  AC_CHECK_HEADERS([commctrl.h shlobj.h winioctl.h winsock.h], [], [], [#include <windows.h>])

  AC_DEFINE(HAVE_HTONL,,[Define to 1 if you have the htonl function.])
  AC_DEFINE(HAVE_HTONS,,[Define to 1 if you have the htons function.])
  AC_DEFINE(HAVE_NETWORK,,[Enable netplay support])
  HAVE_NETWORK_SUPPORT="yes"
  AC_DEFINE(HAVE_RS232NET,,[Enable RS232 network support])
  HAVE_RS232NET_SUPPORT="yes"
  if test x"$host_cpu" = "xx86_64" -o x"$host_cpu" = "xamd64"; then
    LIBS="$LIBS -lwinmm -lntdll -ld2d1 -ld3d11 -lavrt -lws2_32"
  else
    LIBS="$LIBS -lwinmm -lntdll -ld2d1 -ld3d11 -lavrt -lwsock32 -lmingwex -lmsvcrt"
  fi
  if test x"$enable_catweasel" = "xyes"; then
    AC_DEFINE(HAVE_CATWEASELMKIII,,[Support for Catweasel MKIII.])
    AM_CONDITIONAL(HAVE_CATWEASELMKIII, true)
    HAVE_CATWEASELMKIII_SUPPORT="yes"
  fi

  dnl check for usbsid required libraries and headers
  if test x"$enable_usbsid" != "xno" ; then
    PKG_PROG_PKG_CONFIG
    WIN32_USBSID_HEADER_PRESENT=yes
    AC_CHECK_HEADERS(libusb-1.0/libusb.h,,[WIN32_USBSID_HEADER_PRESENT=no],)
    if test x"$WIN32_USBSID_HEADER_PRESENT" = "xyes"; then
      AC_CHECK_LIB(usb-1.0, libusb_init,[LIBS="-lusb-1.0 $LIBS"; USBSID_LIB_PRESENT=yes],,)
      if test x"$PKG_CONFIG" != "x" -a x"$USBSID_LIB_PRESENT" = "xyes"; then
        PKG_CHECK_MODULES(LIBUSB, [libusb-1.0], HAVE_LIBUSB_SUPPORT="yes")
        AC_MSG_CHECKING([for pkgconfig libusb-1.0])
        if test x"$HAVE_LIBUSB_SUPPORT" = "xyes"; then
          VICE_CFLAGS="$VICE_CFLAGS $LIBUSB_CFLAGS";
          VICE_CPPFLAGS="$VICE_CPPFLAGS $LIBUSB_CFLAGS";
          AC_MSG_RESULT([yes])
        else
          AC_MSG_RESULT([no])
        fi
      fi
    fi
  fi

  dnl usbsid support
  AM_CONDITIONAL(HAVE_USBSID, false)
  AC_MSG_CHECKING([for USBSID-Pico support])
  if test x"$enable_usbsid" != "xno" ; then
    if test x"$HAVE_LIBUSB_SUPPORT" = "xyes"; then
      AC_DEFINE([HAVE_USBSID],,[Support for USBSID])
      HAVE_USBSID_SUPPORT="yes"
      AM_CONDITIONAL(HAVE_USBSID, true)
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no (LIBUSB not found)])
    fi
  else
    AC_MSG_RESULT([no (USBSID-Pico disabled)])
  fi

  if test x"$enable_hardsid" != "xno"; then
    AC_DEFINE(HAVE_HARDSID,,[Support for HardSID.])
    HAVE_HARDSID_SUPPORT="yes"
  fi
  if test x"$enable_parsid" = "xyes"; then
    AC_DEFINE(HAVE_PARSID,,[Support for ParSID.])
    AM_CONDITIONAL(HAVE_PARSID, true)
    HAVE_PARSID_SUPPORT="yes"

    if test x"$with_libieee1284" = "xyes"; then
      LIBIEEE1284_HEADERS_PRESENT=yes

      dnl Check for needed io headers
      AC_CHECK_HEADERS(ieee1284.h,,[LIBIEEE1284_HEADERS_PRESENT=no],)

      if test x"$LIBIEEE1284_HEADERS_PRESENT" = "xyes"; then
        AC_CHECK_LIB(ieee1284, ieee1284_find_ports,[LIBS="-lieee1284 $LIBS"; PARSID_SUPPORT=yes],,)
        if test x"$PARSID_SUPPORT" = "xyes"; then
          AC_DEFINE(HAVE_LIBIEEE1284,,[Define to 1 if you have the 'ieee1284' library (-lieee1284).])
        else
          AC_MSG_ERROR([libieee1284 is missing])
        fi
      else
        AC_MSG_ERROR([libieee1284 header is missing])
      fi
    fi
  fi
  if test x"$enable_midi" = "xyes"; then
    AC_DEFINE(HAVE_MIDI,,[Enable support for MIDI.])
    HAVE_MIDI_SUPPORT="yes"
  fi
  if test x"$enable_rs232" != "xno"; then
    AC_DEFINE(HAVE_RS232DEV,,[Enable RS232 device emulation.])
    HAVE_RS232DEV_SUPPORT="yes"
  fi

  dnl warnings_cxx="-Wall -W -Wno-unused-parameter -Wno-missing-field-initializers -Wno-sign-compare"
  dnl (Didn't we already check this?
  if test x"$nowarnings" != "xyes"; then
      warnings_cxx="-Wall -Wno-unused-parameter -Wno-sign-compare"
      VICE_CFLAGS="$VICE_CFLAGS $warnings_cxx"
      VICE_CXXFLAGS="$VICE_CXXFLAGS $warnings_cxx"
  fi

elif test x"$host_os" = "xbeos" -o x"$host_os" = "xhaiku"; then

  dnl Again: ingnored for now

  dnl This is used by subsequent tests.
  ac_cv_prog_gcc=yes
  GCC=yes

  is_beos=yes
  AC_DEFINE(BEOS_COMPILE,,[Are we compiling for either BeOS or Haiku?])
  AM_CONDITIONAL(BEOS_COMPILE, true)

  if test x"$host_os" = "xhaiku"; then
    AC_DEFINE(HAIKU_COMPILE,,[Are we compiling for Haiku?])
    AM_CONDITIONAL(HAIKU_COMPILE, true)
  fi

  if test x"$enable_catweasel" = "xyes"; then
    AC_DEFINE(HAVE_CATWEASELMKIII,,[Support for Catweasel MKIII.])
    AM_CONDITIONAL(HAVE_CATWEASELMKIII, true)
    HAVE_CATWEASELMKIII_SUPPORT="yes"
  fi

  if test x"$enable_hardsid" != "xno"; then
    AC_DEFINE(HAVE_HARDSID,,[Support for HardSID.])
    HAVE_HARDSID_SUPPORT="yes"
  fi

  if test x"$enable_parsid" = "xyes"; then
    AC_DEFINE(HAVE_PARSID,,[Support for ParSID.])
    AM_CONDITIONAL(HAVE_PARSID, true)
    HAVE_PARSID_SUPPORT="yes"

    if test x"$with_libieee1284" = "xyes"; then
      LIBIEEE1284_HEADERS_PRESENT=yes

      dnl Check for needed io headers
      AC_CHECK_HEADERS(ieee1284.h,,[LIBIEEE1284_HEADERS_PRESENT=no],)

      if test x"$LIBIEEE1284_HEADERS_PRESENT" = "xyes"; then
        AC_CHECK_LIB(ieee1284, ieee1284_find_ports,[LIBS="-lieee1284 $LIBS"; PARSID_SUPPORT=yes],,)
        if test x"$PARSID_SUPPORT" = "xyes"; then
          AC_DEFINE(HAVE_LIBIEEE1284,,[Define to 1 if you have the 'ieee1284' library (-lieee1284).])
        else
          AC_MSG_ERROR([libieee1284 is missing])
        fi
      else
        AC_MSG_ERROR([libieee1284 header is missing])
      fi
    fi
  fi

  dnl Haiku needs a few compiler flags to compile:
  dnl Probably can get rid off libstdc++, but who will ever notice?
  VICE_CFLAGS="$VICE_CFLAGS -static -static-libgcc -static-libstdc++"

else

dnl check for darwin first.
case "$host_os" in
darwin*)
  dnl check for needed CoreVideo header
  AC_CHECK_HEADERS(CoreVideo/CVHostTime.h)

  dnl check for needed CoreServices header
  AC_CHECK_HEADERS(CoreServices/CoreServices.h)

  dnl change the host_os to puredarwin to make the configure think it is
  dnl just another unix.
  if test x"$ac_cv_header_CoreServices_CoreServices_h" != "xyes"; then
    host_os=puredarwin;
    AC_DEFINE(DARWIN_COMPILE,,[Enable plain darwin compilation])
  fi
  ;;
*)
  ;;
esac

dnl Check for host os with version attached. Typically on UN*X like systems.
case "$host_os" in

dnl Mac OS X Host
darwin*)

  AC_PROG_OBJCPP

  is_unix=yes
  is_unix_macosx=yes
  is_unix_x11=yes

  dnl disable toolchain checking for macosx
  toolchain_check=no

  dnl The vice build system builds empty libs for things that are configured out,
  dnl then links all of them together. This causes ranlib to complain like:
  dnl
  dnl   file: libarch.a(joy-win32-dinput-handle.o) has no symbols
  dnl
  dnl This warning can be disabled by passing -no_warning_for_no_symbols to ranlib.
  dnl ar also calls ranlib, and there is no way to pass the argument.
  dnl
  dnl You can split an 'ar cru' call into separate ar and ranlib calls, but,
  dnl sometimes ranlib will invoke libtool which has its own warning we can't
  dnl change without major build system changes.
  dnl
  dnl Given all that it's simpler to just grep -v out the warnings we don't want,
  dnl which these scripts do.

  RANLIB="\$(top_srcdir)/src/arch/shared/macOS-wrap-ranlib.sh"
  AR="\$(top_srcdir)/src/arch/shared/macOS-wrap-ar.sh"

  dnl
  dnl Support targeting a specific macOS version
  dnl

  VICE_ARG_ENABLE_LIST(
    macos-minimum-version,
    AS_HELP_STRING(
      [--enable-macos-minimum-version=version],
      [compile for a specific macOS version such as 10.10 or 11.0])
    )
  if test x"$enable_macos_minimum_version" = "x"; then
    AC_MSG_CHECKING([default macOS minimum version (use --enable-macos-minimum-version to override)])
    enable_macos_minimum_version=`echo "int main(int argc, char** argv) { return 0; }" | cc -x c - ;
                                  otool -l a.out | awk '/LC_VERSION_MIN_MACOSX/ { f_old=1 }
                                                        /LC_BUILD_VERSION/ { f_new=1 }
                                                        f_old && /version/ { print $2; exit; }
                                                        f_new && /minos/ { print $2; exit; }' ;
                                  rm a.out`
    AC_MSG_RESULT($enable_macos_minimum_version)
  else
    AC_MSG_CHECKING([if the compiler supports -mmacosx-version-min=$enable_macos_minimum_version])
    ORIG_CFLAGS="$CFLAGS"
    CFLAGS="$VICE_CFLAGS -mmacosx-version-min=$enable_macos_minimum_version"
    AC_TRY_COMPILE([],
                  [int test;],
                  [ AC_MSG_RESULT(yes)
                    CFLAGS="$ORIG_CFLAGS"],
                  [ AC_MSG_RESULT(no)
                    AC_MSG_ERROR([Requested macOS minimum version $enable_macos_minimum_version not supported])
                    ])
  fi

  VICE_CFLAGS="$VICE_CFLAGS -mmacosx-version-min=$enable_macos_minimum_version"
  VICE_CXXFLAGS="$VICE_CXXFLAGS -mmacosx-version-min=$enable_macos_minimum_version"
  VICE_LDFLAGS="$VICE_LDFLAGS -mmacosx-version-min=$enable_macos_minimum_version"

  dnl
  dnl Yeah, OpenGL and other stuff is deprecated, we don't need to be warned about it.
  dnl Please don't remove this again whoever did last time :)
  dnl

  VICE_CFLAGS="$VICE_CFLAGS -Wno-deprecated-declarations"
  VICE_CXXFLAGS="$VICE_CXXFLAGS -Wno-deprecated-declarations"

  if test x"$enable_rs232" != "xno"; then
    AC_DEFINE(HAVE_RS232DEV,,[Enable RS232 device emulation.])
    HAVE_RS232DEV_SUPPORT="yes"
  fi

  AC_DEFINE(HAVE_MOUSE,,[Enable 1351 mouse support])
  HAVE_MOUSE_SUPPORT="yes"
  AC_DEFINE(HAVE_LIGHTPEN,,[Enable lightpen support])
  HAVE_LIGHTPEN_SUPPORT="yes"

  AC_DEFINE(MACOS_COMPILE,,[Enable MacOS-specific code.])

  dnl Set CPPFLAGS as well as VICE_CPPFLAGS for the macports / homebrew dep search paths.
  dnl If we don't, then various header / compile checks won't succeed.

  MAC_CCPFLAGS=""
  MAC_LDFLAGS="-headerpad_max_install_names"
  MAC_FRAMEWORKS="-framework CoreText -framework CoreServices -framework AppKit -framework CoreGraphics"

  dnl Macports headers and libraries

  AC_CHECK_FILE(/opt/local/include,         [ MAC_CCPFLAGS="$MAC_CCPFLAGS -isystem/opt/local/include" ])
  AC_CHECK_FILE(/opt/local/include/libomp,  [ MAC_CCPFLAGS="$MAC_CCPFLAGS -isystem/opt/local/include/libomp" ])
  AC_CHECK_FILE(/opt/local/lib,             [ MAC_LDFLAGS="$MAC_LDFLAGS -L/opt/local/lib -Wl,-rpath,/opt/local/lib" ])
  AC_CHECK_FILE(/opt/local/lib/libomp,      [ MAC_LDFLAGS="$MAC_LDFLAGS -L/opt/local/lib/libomp -Wl,-rpath,/opt/local/lib/libomp" ])

  dnl Homebrew headers and libraries

  if test "`uname -m`" = "arm64"; then
    AC_CHECK_FILE(/opt/homebrew/include,         [ MAC_CCPFLAGS="$MAC_CCPFLAGS -isystem/opt/homebrew/include" ])
    AC_CHECK_FILE(/opt/homebrew/include/libomp,  [ MAC_CCPFLAGS="$MAC_CCPFLAGS -isystem/opt/homebrew/include/libomp" ])
    AC_CHECK_FILE(/opt/homebrew/lib,             [ MAC_LDFLAGS="$MAC_LDFLAGS -L/opt/homebrew/lib -Wl,-rpath,/opt/homebrew/lib" ])
    AC_CHECK_FILE(/opt/homebrew/lib/libomp,      [ MAC_LDFLAGS="$MAC_LDFLAGS -L/opt/homebrew/lib/libomp -Wl,-rpath,/opt/homebrew/lib/libomp" ])
  else
    AC_CHECK_FILE(/usr/local/include,         [ MAC_CCPFLAGS="$MAC_CCPFLAGS -isystem/usr/local/include" ])
    AC_CHECK_FILE(/usr/local/include/libomp,  [ MAC_CCPFLAGS="$MAC_CCPFLAGS -isystem/usr/local/include/libomp" ])
    AC_CHECK_FILE(/usr/local/lib,             [ MAC_LDFLAGS="$MAC_LDFLAGS -L/usr/local/lib -Wl,-rpath,/usr/local/lib" ])
    AC_CHECK_FILE(/usr/local/lib/libomp,      [ MAC_LDFLAGS="$MAC_LDFLAGS -L/usr/local/lib/libomp -Wl,-rpath,/usr/local/lib/libomp" ])
  fi
  
  CPPFLAGS="$CPPFLAGS $MAC_CCPFLAGS"
  VICE_CPPFLAGS="$VICE_CPPFLAGS $MAC_CCPFLAGS"
  LDFLAGS="$LDFLAGS $MAC_LDFLAGS"
  VICE_LDFLAGS="$VICE_LDFLAGS $MAC_LDFLAGS"
  LIBS="$LIBS $MAC_FRAMEWORKS"
  ;;

dnl All other UN*X systems
*)
  is_unix=yes
  is_unix_x11=yes

  case "$host_os" in
  linux*)
    AC_DEFINE(LINUX_COMPILE,,[Are we compiling for Linux?])
    AM_CONDITIONAL(LINUX_COMPILE, true)
    is_unix_linux=yes
    ;;
  esac

  AC_DEFINE(HAVE_MOUSE,,[Enable 1351 mouse support])
  HAVE_MOUSE_SUPPORT="yes"
  AC_DEFINE(HAVE_LIGHTPEN,,[Enable lightpen support])
  HAVE_LIGHTPEN_SUPPORT="yes"
  if test x"$enable_midi" = "xyes"; then
    AC_DEFINE(HAVE_MIDI,,[Enable support for MIDI.])
    HAVE_MIDI_SUPPORT="yes"
  fi
  if test x"$enable_rs232" != "xno"; then
    AC_DEFINE(HAVE_RS232DEV,,[Enable RS232 device emulation.])
    HAVE_RS232DEV_SUPPORT="yes"
  fi
  ;;
esac
fi

dnl check for debug build
if test x"$enable_debug" = "xyes"; then
  have_backtrace_symbols="no"
  DEBUGBUILD=1
  AM_CONDITIONAL(HAVE_DEBUG, true)
  STRIP=":"
  DEBUG_SUPPORT="yes"
  AC_CHECK_HEADERS(execinfo.h,,)

  dnl PCBSD/FreeBSD/Haiku needs -lexecinfo for debug builds
  AC_CHECK_FUNC(backtrace_symbols,
                have_backtrace_symbols="yes",
                have_backtrace_symbols="no")

  if test x"$have_backtrace_symbols" != "xyes"; then
    AC_CHECK_LIB(execinfo,
                 backtrace_symbols,
                 [ LIBS="$LIBS -lexecinfo"
                   have_backtrace_symbols="yes" ],
                 AC_MSG_WARN("Could not find backtrace_symbols in either libc or libexecinfo. Either install libexecinfo, or disable debugging. Please note that --enable-debug is only available for Linux, *BSD, Solaris and OSX."))

  fi

  if test x"$have_backtrace_symbols" = "xyes"; then
    AC_DEFINE(HAVE_BT_SYMBOLS,,[Use backtrace facility of libC or libexecinfo])
    dnl Required for gcc to get proper backtraces
    VICE_LDFLAGS="$VICE_LDFLAGS -rdynamic"
  fi

else
  dnl Disable asserts
  VICE_CPPFLAGS="$VICE_CPPFLAGS -DNDEBUG"
  DEBUGBUILD=0
  AM_CONDITIONAL(HAVE_DEBUG, false)
  dnl Remove super annoying horrible terminal window on Windows
  dnl (but not when using the headless 'UI')
  if test x"$is_win32" = "xyes" -a x"$enable_headlessui" != "xyes"; then
    VICE_CPPFLAGS="$VICE_CPPFLAGS -mwindows"
    VICE_CFLAGS="$VICE_CFLAGS -mwindows"
    VICE_CXXFLAGS="$VICE_CXXFLAGS -mwindows"
  fi
fi

AC_SUBST(DEBUGBUILD)

have_openmp="no"
if test x"$enable_openmp" != "xno"; then
  dnl
  dnl Check for native compiler support for OpenMP.
  dnl
  dnl It's probably there on Windows and Linux, probably not on macOS.
  dnl

  TEST_OPENMP_C=" int thread_count = 0;
                  #pragma omp parallel reduction(+:thread_count)
                  thread_count += 1;
                  if (thread_count > 1)
                    return 0;
                  return 1;"

  AC_MSG_CHECKING([for native OpenMP])
  ORIG_CFLAGS="$CFLAGS"
  CFLAGS="$CFLAGS -fopenmp"
  AC_TRY_LINK([ #include <omp.h> ],
              [ $TEST_OPENMP_C ],
              [ AC_MSG_RESULT(yes)
                have_openmp="yes"
                VICE_CFLAGS="$VICE_CFLAGS -fopenmp"
                VICE_CXXFLAGS="$VICE_CXXFLAGS -fopenmp"
                VICE_LDFLAGS="$VICE_LDFLAGS -fopenmp" ],
              [ AC_MSG_RESULT(no) ])
  CFLAGS=$ORIG_CFLAGS

  if test x"$have_openmp" = "xno"; then
    if test x"$is_unix_macosx" = "xyes"; then
      dnl
      dnl Xcode ships clang without OpenMP, but it can be installed with macports / brew.
      dnl

      AC_MSG_CHECKING([for MacPorts / Homebrew OpenMP])
      ORIG_CFLAGS="$CFLAGS"
      CFLAGS="$CFLAGS -Xpreprocessor -fopenmp -lomp"
      AC_TRY_LINK([ #include <omp.h> ],
                  [ $TEST_OPENMP_C ],
                  [ AC_MSG_RESULT(yes)
                    have_openmp="yes"
                    VICE_CFLAGS="$VICE_CFLAGS -Xpreprocessor -fopenmp"
                    VICE_CXXFLAGS="$VICE_CXXFLAGS -Xpreprocessor -fopenmp"
                    LIBS="$LIBS -lomp"
                    ],
                  [ AC_MSG_RESULT(no)
                    CFLAGS=$ORIG_CFLAGS ])
      CFLAGS=$ORIG_CFLAGS

      if test x"$have_openmp" = "xno"; then
        echo "warning: OpenMP acceleration enabled, but not available (port/brew install libomp)"
      fi
    else
      echo "warning: OpenMP acceleration enabled, but not available"
    fi
  fi
fi

dnl Check for threading code debugging
AM_CONDITIONAL(HAVE_DEBUG_THREADS, test x"$enable_debug_threads" = "xyes")
if test x"$enable_debug_threads" = x"yes"; then
  DEBUG_THREADS_SUPPORT="yes"
  AC_DEFINE(HAVE_DEBUG_THREADS,,[Enable threading debug support])
fi


dnl Check and setup GTK3 compilation, only fail if the gtk3 ui was requested
if test x"$check_gtk3" = "xyes"; then

  PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= 3.22], [have_gtk3_module=yes], [have_gtk3_module=no; AC_MSG_RESULT([no])])
  PKG_CHECK_MODULES(GLIB, [glib-2.0], [have_glib2_module=yes], [have_glib2_module=no; AC_MSG_RESULT([no])])

  dnl Extra tools required for Gtk builds
  dnl These programs live in libglib2.0-dev-bin on Debian

  dnl error out if gtk3 support is missing and gtk3 ui was requested
  if test x"$have_gtk3_module" = "xno" -o x"$have_glib2_module" = "xno"; then
    if test x"$enable_gtk3ui" = "xyes"; then
      AC_MSG_ERROR([GTK3 UI requested, but GTK3 support packages are missing])
    fi
  fi

  dnl Used to compile the vice.gresource.xml file
  AC_CHECK_PROG(GLIB_COMPILE_RESOURCES, [glib-compile-resources], [glib-compile-resources], [no])
  AS_IF([test x"$GLIB_COMPILE_RESOURCES" = "xno"],
        [AC_MSG_ERROR([glib-compile-resources is missing, please install glib development binaries])])
  dnl Used by VTE for signal marshalling
  AC_CHECK_PROG(GLIB_GENMARSHAL, [glib-genmarshal], [glib-genmarshal], [no])
  AS_IF([test x"$GLIB_GENMARSHAL" = "xno"],
        [AC_MSG_ERROR([glib-genmarshal is missing, please install glib development binaries])])

  dnl only continue to test if gtk3 support was present
  if test x"$have_gtk3_module" = "xyes" -a x"$have_glib2_module" = "xyes"; then

    VICE_CFLAGS="$VICE_CFLAGS $GLIB_CFLAGS $GTK_CFLAGS"
    VICE_CXXFLAGS="$VICE_CXXFLAGS $GLIB_CFLAGS $GTK_CFLAGS"
    LIBS="$LIBS $GTK_LIBS $GLIB_LIBS"

    if test x"$is_unix_macosx" = "xyes"; then
      dnl GTK3-FIXME: put OSX specific checks here
      :
    elif test x"$is_win32" = "xyes"; then
      dnl GTK3-FIXME: put windows specific checks here
      :
    else
      dnl GTK3-FIXME: put *nix specific checks here
      :
    fi

    dnl enforce OpenGL/GLEW usability unless explicitly disabled
    if test x"$is_unix_macosx" = "xyes"; then
      AC_MSG_CHECKING(whether we can use the OpenGL Framework)
      old_LIBS="$LIBS"
      LIBS="$LIBS -framework OpenGL"
      AC_TRY_LINK([#define GL_GLEXT_PROTOTYPES
                   #include <OpenGL/gl.h>],
                   [glUniform4f(0,1,2,3,4);],
                   [AC_MSG_RESULT(yes);
                    have_opengl_lib=yes],
                   [AC_MSG_RESULT(no);
                    LIBS="$old_LIBS"])
      if test x"$have_opengl_lib" != "xyes"; then
        AC_MSG_ERROR([Cannot find the OpenGL framework. Something is seriously wrong with your build environment.])
      fi
      AC_SEARCH_LIBS(glewInit, [GLEW glew32], [have_glew=yes], [have_glew=no])
      if test x"$have_glew" != "xyes"; then
        AC_MSG_ERROR([Cannot find the OpenGL Extension Wrangler Library, GLEW. (port/brew install glew)])
      fi
    else
      dnl Find where the GL functions live on this system
      old_clags="$CFLAGS"
      CFLAGS="$VICE_CFLAGS"
      PKG_CHECK_MODULES(GLEW, [glew], have_pkg_glew="yes")
      if test x"$have_pkg_glew" = "xyes"; then
          LIBS="$LIBS $GLEW_LIBS"
      fi
      AC_SEARCH_LIBS(glViewport, [GL opengl32], [have_opengl_lib=yes], [have_opengl_lib=no])
      AC_SEARCH_LIBS(glewInit, [GLEW glew32], [have_glew=yes], [have_glew=no])

      if test x"$have_opengl_lib" != "xyes" -o x"$have_glew" != "xyes"; then
        AC_MSG_CHECKING(whether the opengl32 and glew32 libraries work anyway)
        old_LIBS="$LIBS"
        LIBS="$LIBS -lopengl32 -lglew32"
        AC_TRY_LINK([#include <GL/glew.h>],
                    [glewInit();],
                    [AC_MSG_RESULT(yes);
                     have_opengl_lib=yes;
                     have_glew=yes],
                    [AC_MSG_RESULT(no);
                     LIBS="$old_LIBS"])
      fi

      CFLAGS="$old_CFLAGS"

      if test x"$have_opengl_lib" != "xyes" -o x"$have_glew" != "xyes"; then
        AC_MSG_ERROR([Could not find OpenGL and GLEW libraries, aborting.])
      fi
    fi

    AC_DEFINE(USE_GTK3UI,,[Use GTK3 UI.])
    if test x"$enable_debug_gtk3ui" = "xyes"; then
      AC_DEFINE(HAVE_DEBUG_GTK3UI,,[Add GTK3 UI debugging code.])
    fi

    dnl Platform canonical names for Win32-GTK3 have names like "i686-w64-mingw32"
    case "$host_os" in
      *mingw32)
          host_os=mingw32-gtk3
          is_win32_gtk3=yes

          dnl Needed for cross-building win32, -static makes some CoInitialize
          dnl check fail, so I had to leave that out. This also works on msys2.
          LIBS="$LIBS -static-libgcc"

    esac

    dnl Don't check for fontconfig on MacOS or Windows, we don't use fontconfig
    dnl directly on those platforms
    if test x"$is_unix_macosx" != "xyes" -a x"$is_win32" != "xyes"; then
      dnl check for fontconfig
      PKG_CHECK_MODULES([FONTCONFIG], [fontconfig >= 2.0.0],
          [HAVE_FONTCONFIG_SUPPORT=yes; LIBS="$LIBS -lfontconfig"],
          AC_MSG_ERROR([failed to find fontconfig]))
      AC_DEFINE(HAVE_FONTCONFIG,,[Use fontconfig for custom fonts.])
    fi

    dnl now that the GTK3 UI support has been detected, define enable_gtk3ui and undefine the others to prevent checks
    enable_gtk3ui=yes
    enable_sdl1ui=no
    enable_sdl2ui=no
    enable_headlessui=no
    found_gui=gtk3


    dnl Gtk3 always has HW scaling
    AC_DEFINE(HAVE_HWSCALE,,[Enable arbitrary window scaling])
    HAVE_HWSCALE_SUPPORT="yes"
  fi
fi

dnl Check if sdl2 ui should be checked
if test x"$check_guis" = "xyes"; then
  if test x"$enable_sdl2ui" != "xno"; then
    check_sdl2=yes
  fi
else
  if test x"$enable_sdl2ui" = "xyes"; then
    check_sdl2=yes
  fi
fi
if test x"$with_sdlsound" = "xyes" -a x"$enable_sdl1ui" != "xyes"; then
  check_sdl2=yes
  if test x"$enable_headlessui" = "xyes" -o x"$enable_gtk3ui" = "xyes"; then
    check_sdlaudio_only=yes
    enable_sdl1ui=no
    enable_sdl2ui=no
  fi
fi

dnl Check and setup SDL2 compilation, only fail if the sdl2 ui was requested
if test x"$check_sdl2" = "xyes"; then
  user_cflags="$CFLAGS"
  user_cxxflags="$CXXFLAGS"
  backup_CFLAGS="$VICE_CFLAGS"
  backup_CXXFLAGS="$VICE_CXXFLAGS"
  old_libs=$LIBS

  found_sdl2=no

  PKG_CHECK_MODULES(SDL2, sdl2, [
    use_sdl_prefix=no
    found_sdl2=yes
    VICE_CFLAGS="$VICE_CFLAGS $SDL2_CFLAGS"
    VICE_CXXFLAGS="$VICE_CXXFLAGS $SDL2_CFLAGS"
    LIBS="$LIBS $SDL2_LIBS"
  ],[
    dnl fail if SDL2 ui was requested, otherwise restore flags and continue
    if test x"$enable_sdl2ui" = "xyes"; then
      AC_MSG_ERROR([SDL 2.x UI requested, but SDL 2.x library is missing.])
    fi
  ])

  dnl only continue if sdl2 was found
  if test x"$found_sdl2" = "xyes"; then

    dnl check for the 2 cases where audio needs to be checked
    check_sdl2audio=no
    if test x"$enable_sdl2ui" = "xyes" -o x"$with_sdlsound" = "xyes"; then
      check_sdl2audio=yes
    fi

    user_CFLAGS="$CFLAGS"
    user_CXXFLAGS="$CXXFLAGS"
    CFLAGS="$VICE_CFLAGS"
    CXXFLAGS="$VICE_CXXFLAGS"

    dnl only do if audio needs to be checked
    if test x"$check_sdl2audio" = "xyes"; then
      AC_MSG_CHECKING(for SDL audio support)
      AC_TRY_LINK([#include <SDL_audio.h>],
                  [SDL_OpenAudio(NULL, NULL)],
                  [AC_MSG_RESULT(yes);
                    USE_SDL_AUDIO_SUPPORT="yes";
                    AC_DEFINE(USE_SDL_AUDIO,,[Enable SDL sound support.])],
                  [AC_MSG_RESULT(no);])

      dnl fail if --with-sdlsound was given
      if test x"$USE_SDL_AUDIO_SUPPORT" != "xyes"; then
        if test x"$with_sdlsound" = "xyes"; then
          AC_MSG_ERROR([SDL sound support missing])
        fi
      else
        SOUND_DRIVERS="$SOUND_DRIVERS soundsdl.o"
      fi
    fi

    dnl SDL2 now definitely works, so set the last few settings.
    dnl SDL2 is much more consistent than SDL1 so none of these
    dnl actually require tests.
    if test x"$enable_sdl2ui" != "xno"; then
      AC_DEFINE(USE_SDL2UI,,[Enable SDL 2.x UI support.])
      AC_DEFINE(HAVE_HWSCALE,,[Enable arbitrary window scaling])
      HAVE_HWSCALE_SUPPORT="yes"

      AC_CHECK_FUNCS(SDL_NumJoysticks)

      dnl now that the SDL 2.0 UI support has been detected, define
      dnl enable_sdl2ui and undefine the others to prevent checks

      if test x"$check_sdlaudio_only" != "xyes"; then
        enable_gtk3ui=no
        enable_sdl1ui=no
        enable_sdl2ui=yes
        enable_headlessui=no
        found_gui=sdl2
      fi

      dnl Require SDL2_image
      PKG_CHECK_MODULES(SDL2_IMAGE,
                        [SDL2_image >= 2.0.0],
                        [have_sdl2_image=yes],
                        [have_sdl2_image=no; AC_MSG_RESULT([no])])
      if test x"$have_sdl2_image" = "xyes"; then
        VICE_CFLAGS="$VICE_CFLAGS $SDL2_IMAGE_CFLAGS"
        VICE_CXXFLAGS="$VICE_CXXFLAGS $SDL2_IMAGE_CFLAGS"
        LIBS="$LIBS $SDL2_IMAGE_LIBS"
      else
        AC_MSG_ERROR([SDL2_image-dev is required.])
      fi
    fi
    # LIBS="$LIBS -static-libstdc++"

    CFLAGS="$user_CFLAGS"
    CXXFLAGS="$user_CXXFLAGS"
  fi
fi

dnl Check if sdl ui should be checked
if test x"$check_guis" = "xyes"; then
  if test x"$enable_sdl1ui" != "xno"; then
    check_sdl1=yes
  fi
else
  if test x"$enable_sdl1ui" = "xyes"; then
    check_sdl1=yes
  fi
fi
if test x"$with_sdlsound" = "xyes"; then
  check_sdl1=yes
fi


dnl Check and setup SDL 1.2 compilation.
if test x"$check_sdl1" = "xyes"; then

  dnl check for the 2 cases where audio needs to be checked
  check_sdlaudio=no
  if test x"$ac_cv_lib_SDL2_SDL_OpenAudio" != "xyes"; then
    if test x"$enable_sdl1ui" = "xno"; then
      if test x"$with_sdlsound" = "xyes"; then
        check_sdlaudio=yes
      fi
    else
      check_sdlaudio=yes
    fi
  fi

  dnl fail if SDL ui was requested
  if test x"$sdl_config" = "xno" -a x"$enable_sdl1ui" = "xyes"; then
    AC_MSG_ERROR([SDL UI requested, but sdl-config is missing])
  fi

  dnl only continue if sdl-config was found
  if test x"$sdl_config" != "xno"; then
    user_cflags="$CFLAGS"
    user_cxxflags="$CXXFLAGS"
    old_libs=$LIBS

    CFLAGS="$VICE_CFLAGS `$sdl_config --cflags`"
    CXXFLAGS="$VICE_CXXFLAGS `$sdl_config --cflags`"
    LIBS="$LIBS `$sdl_config --libs`"

    dnl only do if audio needs to be checked
    if test x"$check_sdlaudio" = "xyes"; then
      AC_CHECK_HEADERS(SDL/SDL_audio.h,[AC_CHECK_LIB(SDL, SDL_OpenAudio,
                       [use_sdl_prefix=yes;
                        USE_SDL_AUDIO_SUPPORT="yes";
                        AC_DEFINE(USE_SDL_PREFIX,,[Enable SDL prefix for header inclusion.])
                        AC_DEFINE(USE_SDL_AUDIO,,[Enable SDL sound support.])],
                       [CFLAGS="$user_cflags"; CXXFLAGS="$user_cxxflags"; LIBS=$old_libs; break],$EXTRA_SDL_LIBS)],
                       [CFLAGS="$user_cflags"; CXXFLAGS="$old_cxxflags"; LIBS=$old_libs; break])

      if test x"$ac_cv_lib_SDL_SDL_OpenAudio" != "xyes"; then
        unset ac_cv_lib_SDL_SDL_OpenAudio
        AC_CHECK_HEADERS(SDL_audio.h,[AC_CHECK_LIB(SDL, SDL_OpenAudio,
                         [use_sdl_prefix=no;
                          USE_SDL_AUDIO_SUPPORT="yes";
                          AC_DEFINE(USE_SDL_AUDIO,,[Enable SDL sound support.])],
                         [CFLAGS="$user_cflags"; CXXFLAGS="$user_cxxflags"; LIBS=$old_libs; break],$EXTRA_SDL_LIBS)],
                         [CFLAGS="$old_cflags"; CXXFLAGS="$old_cxxflags"; LIBS=$old_libs; break])
      fi

      dnl fail if --with-sdlsound was given
      if test x"$ac_cv_lib_SDL_SDL_OpenAudio" != "xyes"; then
        if test x"$with_sdlsound" = "xyes"; then
          AC_MSG_ERROR([SDL sound support missing])
        fi
      else
        SOUND_DRIVERS="$SOUND_DRIVERS soundsdl.o"
      fi
    fi

    dnl only check if SDL 1.2 ui is needed
    if test x"$enable_sdl1ui" != "xno"; then
      AC_CHECK_HEADERS(SDL/SDL.h,[AC_CHECK_LIB(SDL, SDL_SetVideoMode,
                         [use_sdl_prefix=yes;
                          AC_DEFINE(USE_SDL_PREFIX,,[Enable SDL prefix for header inclusion.])
                          AC_DEFINE(USE_SDLUI,,[Enable SDL 1.2 UI support.])],
                         [CFLAGS="$user_cflags"; CXXFLAGS="$user_cxxflags"; LIBS=$old_libs; break],$EXTRA_SDL_LIBS)],
                         [CFLAGS="$user_cflags"; CXXFLAGS="$user_cxxflags"; LIBS=$old_libs; break])

        if test x"$ac_cv_lib_SDL_SDL_SetVideoMode" != "xyes"; then
          unset ac_cv_lib_SDL_SDL_SetVideoMode
          AC_CHECK_HEADERS(SDL.h,[AC_CHECK_LIB(SDL, SDL_SetVideoMode,
                           [use_sdl_prefix=no;
                            AC_DEFINE(USE_SDLUI,,[Enable SDL 1.2 UI support.])],
                           [CFLAGS=$old_cflags; CXXFLAGS=$old_cxxflags; LIBS=$old_libs; break],$EXTRA_SDL_LIBS)],
                           [CFLAGS=$old_cflags; CXXFLAGS=$old_cxxflags; LIBS=$old_libs; break])
        fi

        dnl fail if SDL 1.2 ui was requested
        if test x"$ac_cv_lib_SDL_SDL_SetVideoMode" != "xyes" -a x"$enable_sdl1ui" = "xyes"; then
          AC_MSG_ERROR([SDL 1.2 headers and/or libraries missing])
        fi

      dnl only continue if SDL headers/libs are present
      if test x"$ac_cv_lib_SDL_SDL_SetVideoMode" = "xyes"; then
        AC_CHECK_FUNCS(SDL_NumJoysticks)

        if test x"$enable_hwscale" != "xno"; then
          if test x"$is_unix_macosx" = "xyes" -o x"$is_win32" = "xyes"; then
            AC_MSG_CHECKING(whether we can use the OpenGL Framework)
            old_LIBS="$LIBS"
            if test x"$is_unix_macosx" = "xyes"; then
              LIBS="$LIBS -framework OpenGL"
            else
              LIBS="$LIBS -lopengl32"
            fi
            if test x"$use_sdl_prefix" = "xyes"; then
              AC_TRY_LINK([#include <SDL/SDL_opengl.h>],
                          [glViewport(1,2,3,4)],
                          [AC_MSG_RESULT(yes);
                           have_opengl_lib=yes],
                          [AC_MSG_RESULT(no);
                           LIBS="$old_LIBS"])
            else
              AC_TRY_LINK([#include <SDL_opengl.h>],
                          [glViewport(1,2,3,4)],
                          [AC_MSG_RESULT(yes);
                           have_opengl_lib=yes],
                          [AC_MSG_RESULT(no);
                           LIBS="$old_LIBS"])
            fi
          else
            AC_SEARCH_LIBS(glViewport, [GL], [have_opengl_lib=yes], [have_opengl_lib=no])
          fi

          if test x"$have_opengl_lib" != "xyes"; then
            AC_MSG_ERROR(The SDL 1.2 UI requires OpenGL for hardware scaling. Install OpenGL development headers or set --disable-hwscale to disable scaling.)
          fi
        fi

        if test x"$enable_hwscale" != "xno"; then
          AC_DEFINE(HAVE_HWSCALE,,[Enable arbitrary window scaling])
          HAVE_HWSCALE_SUPPORT="yes"
        fi

        dnl check for the SDL_main.h header
        if test x"$use_sdl_prefix" = "xyes"; then
          AC_CHECK_HEADERS(SDL/SDL_main.h)
        else
          AC_CHECK_HEADERS(SDL_main.h)
        fi

        AC_MSG_CHECKING(whether we can use the SDLmain library)
        old_LIBS="$LIBS"
        LIBS="$LIBS -lSDLmain -lSDL $EXTRA_SDL_LIBS"
        if test x"$use_sdl_prefix" = "xyes"; then
          AC_TRY_LINK([#include <SDL/SDL_main.h>
                       int main(int argc, char *argv[])
                       { return 0; }
                       #undef main
                       #define main something_else],
                      [return 0;],
                      [AC_MSG_RESULT(yes);
                       have_sdlmain=yes],
                      [AC_MSG_RESULT(no);
                       LIBS="$old_LIBS"])
        else
          AC_TRY_LINK([#include <SDL_main.h>
                       int main(int argc, char *argv[])
                       { return 0; }
                       #undef main
                       #define main something_else],
                      [return 0;],
                      [AC_MSG_RESULT(yes);
                       have_sdlmain=yes],
                      [AC_MSG_RESULT(no);
                       LIBS="$old_LIBS"])
        fi

        if test x"$have_sdlmain" != "xyes"; then
          AC_MSG_CHECKING(whether we can use the SDLmain library when adding -lmingw32)
          old_LIBS="$LIBS"
          LIBS="$LIBS -lmingw32 -lSDLmain -lSDL"
          if test x"$use_sdl_prefix" = "xyes"; then
            AC_TRY_LINK([#include <SDL/SDL_main.h>
                         int main(int argc, char *argv[])
                         { return 0; }
                         #undef main
                         #define main something_else],
                        [return 0;],
                        [AC_MSG_RESULT(yes);
                         have_sdlmain=yes],
                        [AC_MSG_RESULT(no);
                         LIBS="$old_LIBS"])
          else
            AC_TRY_LINK([#include <SDL_main.h>
                         int main(int argc, char *argv[])
                         { return 0; }
                         #undef main
                         #define main something_else],
                        [return 0;],
                        [AC_MSG_RESULT(yes);
                         have_sdlmain=yes],
                        [AC_MSG_RESULT(no);
                         LIBS="$old_LIBS"])
          fi
        fi

        if test x"$have_sdlmain" = "xyes"; then
          AC_DEFINE(HAVE_SDLMAIN,,[Enable SDLmain replacement])
        fi

        dnl Require SDL_image
        dnl The version number I sucked out of my thumb, on my Debian the
        dnl version is 1.2.12
        dnl
        PKG_CHECK_MODULES(SDL_IMAGE,
                          [SDL_image >= 1.2.0],
                          [have_sdl_image=yes],
                          [have_sdl_image=no; AC_MSG_RESULT([no])])
        if test x"$have_sdl_image" = "xyes"; then
          LIBS="$LIBS -lSDL_image"
        else
          AC_MSG_ERROR([SDL_image1.2-dev is required.])
        fi

        dnl now that the SDL1 UI support has been detected, define enable_sdl1ui and undefine the others to prevent checks
        if test x"$check_sdlaudio_only" != "xyes"; then
          enable_gtk3ui=no
          enable_sdl1ui=yes
          enable_sdl2ui=no
          enable_headlessui=no
          found_gui=sdl1
        fi
      fi
    fi
  fi
fi

if test x"$enable_headlessui" = "xyes"; then
  AC_DEFINE(USE_HEADLESSUI,,[Enable Headless UI support.])
  enable_gtk3ui=no
  enable_sdl1ui=no
  enable_sdl2ui=no
  found_gui=headless
fi

dnl check if any gui support was found
if test x"$found_gui" = "x"; then
  AC_MSG_ERROR([No GUI support found])
fi


dnl Check if --enable-debug-gtk3ui was requested but the UI isn't gtk3
if test x"$enable_debug_gtk3ui" = "xyes" -a x"$enable_gtk3ui" != "xyes"; then
    AC_MSG_ERROR([Requested Gtk3 UI debugging, but the requested UI isn't Gtk3.])
fi


dnl Check if --disable-hwscale was requested for the Gtk3 UI
if test x"$enable_gtk3ui" = "xyes" -a x"$enable_hwscale" = "xno"; then
  AC_MSG_ERROR([The option `--disable-hwscale` is illegal for the Gtk3 UI. Please remove the option and make sure the OpenGL development headers are installed.])
fi


dnl Check if --disable-hwscale was requested for the Gtk3 UI
if test x"$enable_sdl2ui" = "xyes" -a x"$enable_hwscale" = "xno"; then
  AC_MSG_ERROR([The option `--disable-hwscale` is illegal for the SDL2 UI.])
fi



dnl check if sdl sound was requested but not found
if test x"$with_sdlsound" = "xyes"; then
  if test x"$USE_SDL_AUDIO_SUPPORT" != "xyes"; then
    AC_MSG_ERROR([No SDL sound support found])
  fi
fi

dnl are we putting emu into a background thread (requires UI code support)
if test x"$enable_gtk3ui" = "xyes"; then
  AC_DEFINE(USE_VICE_THREAD,,[UI and emu each on different threads])
  VICE_CFLAGS="$VICE_CFLAGS -pthread"
  VICE_CXXFLAGS="$VICE_CXXFLAGS -pthread"
  VICE_LDFLAGS="$VICE_LDFLAGS -pthread"
  show_multithreaded="yes"

  if test x"$is_unix_x11" = x"yes" -a x"$is_unix_macosx" != x"yes"; then
    dnl xlib is used for adding a native child window for opengl rendering
    LIBS="$LIBS -lX11"
  fi

else
  show_multithreaded="no"
fi

if test x"$is_win32" = "xyes" -a x"$enable_sdl1ui" != "xyes" -a x"$enable_sdl2ui" != "xyes" -a x"$enable_headlessui" != "xyes"; then
  dinput_header_no_lib="no"

  AC_CHECK_HEADER(dinput.h,
              [ HAVE_DINPUT_SUPPORT="yes";
                JOYSTICK_DRIVERS="$JOYSTICK_DRIVERS joystick_win32_directinput.o";
                AC_DEFINE(HAVE_DINPUT,,[Use DirectInput joystick driver]) ]
              [ AC_MSG_CHECKING(for -ldinput8) ]
              [SAVELIBS="$LIBS"] [ LIBS="-ldinput8 $LIBS" ]
              [ AC_LINK_IFELSE([
                               AC_LANG_PROGRAM([
                               #define DIRECTINPUT_VERSION 0x800
                               #include <dinput.h>
                               ],[return DirectInput8Create(0,0,0,0,0);])
                               ],
                            [ AC_DEFINE(HAVE_DINPUT_LIB, [], [dinput8.lib or libdinput8.a are present]) ]
                            [ AC_MSG_RESULT(yes) ],
                            [ dinput_header_no_lib="yes" ]
                            [ LIBS="$SAVELIBS" ]
                            [ AC_MSG_RESULT(no) ]
              )]
     )
fi


dnl Check/setup XDG desktop files support
dnl
dnl XDG .desktop files are only supported for the Gtk3 UI on Unix (excl. MacOS)
dnl
AM_CONDITIONAL(INSTALL_DESKTOP_FILES, false)
if test x"$enable_desktop_files" = "xyes"; then
  dnl Check Gtk3 UI
  if test x"$enable_gtk3ui" != "xyes"; then
    AC_MSG_ERROR([XDG desktop files requested but UI is not Gtk3.])
  fi
  dnl Check OS (Unix)
  if test x"$is_unix_x11" != "xyes"; then
    AC_MSG_ERROR([XDG desktop files requested but OS is not Unix.])
  fi
  dnl Check OS (not MacOS)
  if test x"$is_unix_macos" = "xyes"; then
    AC_MSG_ERROR([XDG desktop files requested but OS is MacOS.])
  fi
  dnl Check for desktop-file-install
  AC_CHECK_PROG(DESKTOP_FILE_INSTALL, desktop-file-install, desktop_file_install, no)
  if test x"$DESKTOP_FILE_INSTALL" = "xno"; then
    AC_MSG_ERROR([XDG desktop files requested but required tool desktop-file-install is missing. Please install desktop-file-utils.])
  fi
  dnl Check for xdg-icon-resource
  AC_CHECK_PROG(XDG_ICON_RESOURCE, xdg-icon-resource, xdg-icon-resource, no)
  if test x"$XDG_ICON_RESOURCE" = "xno"; then
    AC_MSG_ERROR([XDG desktop files requested but required tool xdg-icon-resource is missing. Please install xdg-utils.])
  fi
  dnl Check for update-desktop-database
  AC_CHECK_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database, update-desktop-database, no)
  if test x"$UPDATE_DESKTOP_DATABASE" = "xno"; then
    AC_MSG_ERROR([XDG desktop files requested but required tool update-desktop-database is missing. Please install desktop-file-utils.])
  fi

  dnl We got the UI + OS + tools:
  USE_DESKTOP_FILES="yes"
  AC_DEFINE(INSTALL_DESKTOP_FILES, , [Install XDG desktop files and icons])
  AM_CONDITIONAL(INSTALL_DESKTOP_FILES, true)
fi


dnl Check for the icotool binary to generate Windows .ico files
AC_CHECK_PROG(ICOTOOL, icotool, icotool, no)
if test x"$is_win32" = "xyes"; then
  if test x"$ICOTOOL" = "xno"; then
    AC_MSG_ERROR([Building for Windows, but icotool is missing. Please install icoutils."])
  fi
fi


dnl Extension for executable files in this system
AC_EXEEXT

AC_CHECK_LIB(iconv, libiconv_open,,)

dnl Check if --param inline-unit-growth=60 can be used.
old_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS -Werror --param inline-unit-growth=60"
AC_MSG_CHECKING([if the compiler accepts --param inline-unit-growth=60])
AC_TRY_COMPILE([],
               [int test;],
               [ AC_MSG_RESULT(yes)
                 INLINE_UNIT_GROWTH="--param inline-unit-growth=60"
                ],
               [ AC_MSG_RESULT(no)
                INLINE_UNIT_GROWTH=""
                ])
CFLAGS=$old_CFLAGS
AC_SUBST(INLINE_UNIT_GROWTH)

dnl Check if --param max-inline-insns-single=600 can be used.
old_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS -Werror --param max-inline-insns-single=600"
AC_MSG_CHECKING([if the compiler accepts --param max-inline-insns-single=600])
AC_TRY_COMPILE([],
               [int test;],
               [ AC_MSG_RESULT(yes)
                 MAX_INLINE_INSN_SINGLE="--param max-inline-insns-single=600"
                ],
               [ AC_MSG_RESULT(no)
                MAX_INLINE_INSN_SINGLE=""
                ])
CFLAGS=$old_CFLAGS
AC_SUBST(MAX_INLINE_INSN_SINGLE)


dnl Check for SID engines
dnl
dnl FastSID is disabled by default, so we check against "yes", ReSID is enabled
dnl by default so we check against "no" ($with_resid being empty means the
dnl default, ie "yes")
dnl
dnl Remove this check to allow building without a SID engine
if test x"$with_fastsid" != "xyes" -a x"$with_resid" = "xno"; then
  AC_MSG_ERROR([Neither FastSID nor ReSID sound engine enabled, please enable at least one.])
fi


dnl FastSID
AM_CONDITIONAL(HAVE_FASTSID, test x"$with_fastsid" = "xyes")
if test x"$with_fastsid" = "xyes"; then
  HAVE_FASTSID_SUPPORT="yes"
  AC_DEFINE(HAVE_FASTSID,,[This version provides FastSID support.])
fi

dnl Set output drivers to none
GFXOUTPUT_DRIVERS=""

RESID_DIR=
RESID_LIBS=
RESID_INCLUDES=
RESID_DEP=

RESID_DTV_DIR=
RESID_DTV_LIBS=
RESID_DTV_INCLUDES=
RESID_DTV_DEP=

dnl Set linker to C++ compiler/linker: even SDL builds use C++ code (linenoise-ng)
LINKCC='$(CXX)'

dnl disable reSID for certain platforms

RESIDSUB=""
RESIDDTVSUB=""

if test x"$with_resid" = "xno"; then
  dnl Do not attempt to configure reSID.
  dnl NOTE: no_recursion is set because directories specified with
  dnl AC_CONFIG_SUBDIRS are always recursed into regardless of where the
  dnl macro is used.
  dnl Another workaround must be found if other self-contained packages go
  dnl into VICE.
  
  dnl A workaround for an autoconf bug when not using/checking the C++
  dnl compiler, we'll define am__fastdepCXX_TRUE as #
  am__fastdepCXX_TRUE=#
  am__fastdepCXX_FALSE=
else
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_PROG_CXX([g++ clang++ cc])

  dnl disable resid when no C++ compiler is found
  if test x"$CXX" != "x" ; then
    LINKCC='$(CXX)'

    dnl Set CXXFLAGS. Use -fno-exceptions for G++ if supported.
    if test "$ac_test_CXXFLAGS" != set; then
      if test "$GXX" = yes; then
        old_CXXFLAGS=$CXXFLAGS
        CXXFLAGS="$CXXFLAGS -fno-exceptions"
        AC_MSG_CHECKING([whether the C++ compiler ($CXX $CXXFLAGS $LDFLAGS) works])
        AC_TRY_COMPILE([],
                       [ int test; ],
                       [ AC_MSG_RESULT(yes) ],
                       [ AC_MSG_RESULT(no)
                         CXXFLAGS=$old_CXXFLAGS
                         ])
      fi
    fi
    AC_PROG_CXXCPP
  fi

  if test x"$with_resid" = "xyes" -o x"$with_resid" = "x"; then
    AC_DEFINE(HAVE_RESID,,[This version provides ReSID support.])
    HAVE_RESID_SUPPORT="yes"
    AX_SUBDIRS_CONFIGURE([src/resid], [--disable-option-checking])
    RESID_DIR=resid
    RESID_LIBS="\$(top_builddir)/src/resid/libresid.a"
    RESID_INCLUDES="-I\$(top_builddir)/src/resid"
    RESID_DEP=libresid
    RESIDSUB=resid

    AC_DEFINE(HAVE_RESID_DTV,,[This version provides ReSID-DTV support.])
    AX_SUBDIRS_CONFIGURE([src/resid-dtv], [--disable-option-checking])
    RESID_DTV_DIR=resid-dtv
    RESID_DTV_LIBS="\$(top_builddir)/src/resid-dtv/libresiddtv.a"
    RESID_DTV_INCLUDES="-I\$(top_builddir)/src/resid-dtv"
    RESID_DTV_DEP=libresiddtv
    RESIDDTVSUB=resid-dtv
  fi
  AC_LANG_RESTORE
fi

AC_SUBST(RESIDSUB)
AC_SUBST(RESIDDTVSUB)

if test x"$RESIDSUB" = "xresid"; then
  AM_CONDITIONAL(RESID_DIR_USED, true)
else
  AM_CONDITIONAL(RESID_DIR_USED, false)
fi

AC_SUBST(LINKCC)

AM_CONDITIONAL(HAVE_RESID, test x"$with_resid" != "xno")

AC_SUBST(RESID_DIR)
AC_SUBST(RESID_LIBS)
AC_SUBST(RESID_INCLUDES)
AC_SUBST(RESID_DEP)

AM_CONDITIONAL(HAVE_RESID_DTV, test x"$with_resid" != "xno")
AC_SUBST(RESID_DTV_DIR)
AC_SUBST(RESID_DTV_LIBS)
AC_SUBST(RESID_DTV_INCLUDES)
AC_SUBST(RESID_DTV_DEP)

dnl Check for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADERS(sys/types.h)

AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T



dnl Define the type sizes. (No, it doesn't)
if [[ x"$ac_cv_prog_cc_cross" != "xyes" ]]; then
  dnl This appears to define `WORDS_BIGENDIAN`. Not sure if it gets AC_SUBST'ed
  AC_C_BIGENDIAN
else
  dnl if cross-compiling, we have to do this by hand
  dnl
  dnl This is bollocks, cross-compiling doesn't suddenly make the assumption
  dnl it's compiling for what looks like a 32-bit target. Any code that requires
  dnl a certain width of a type uses types from <stdint.h>, or at least it should,
  dnl this is VICE after all. --cpx
  echo "warning: assuming sizeof(unsigned short) == 2,"
  echo "                  sizeof(unsigned int) == 4,"
  echo "                  sizeof(unsigned long) == 4"
fi

if test x"$cross_compiling" = "xyes"; then
  AM_CONDITIONAL(CROSS, true)
  CROSS=true
else
  AM_CONDITIONAL(CROSS, false)
  CROSS=false
fi
AC_SUBST(CROSS)

AC_CHECK_SIZEOF(unsigned short, 2)
AC_CHECK_SIZEOF(unsigned int, 4)
AC_CHECK_SIZEOF(unsigned long, 4)


dnl Check for header files.
AC_HEADER_DIRENT
AC_CHECK_HEADERS(direct.h errno.h fcntl.h limits.h regex.h unistd.h strings.h \
sys/dirent.h sys/stat.h inttypes.h libgen.h sys/ioctl.h \
dir.h io.h process.h signal.h alloca.h wchar.h stdint.h sys/time.h)


AC_CHECK_HEADER(regexp.h,,,
                [#define    INIT        register char *sp = instring;
                 #define    GETC()      (*sp++)
                 #define    PEEKC()     (*sp)
                 #define    UNGETC(c)   sp
                 #define    RETURN(ptr) return NULL;
                 #define    ERROR(val)  _RegExpError(val)
])
AC_DECL_SYS_SIGLIST


dnl Check for capabilities support.
dnl
dnl Currently only makes sense on Linux/BSD for rawnet support.
AC_CHECK_HEADER(sys/capability.h,
  AC_CHECK_LIB(cap,cap_init,
                [ LIBS="$LIBS -lcap";
                  AC_DEFINE(HAVE_CAPABILITIES,,
                            [Support for POSIX capabilities.])
                  HAVE_CAPABILITIES_SUPPORT="yes"
                ]
  )
)


dnl ----- Dynamic Lib Loading Support -----
dynlib_support=no
DYNLIB_LIBS=
dnl check for dlopen support on unix and beos/haiku systems
if test x"$is_unix" = "xyes" -o x"$is_beos" = "xyes"; then
  AC_CHECK_HEADER(dlfcn.h,,)
  if test x"$ac_cv_header_dlfcn_h" = "xyes" ; then
    AC_CHECK_FUNC(dlopen,
                 [ DYNLIB_LIBS="";
                   AC_DEFINE(HAVE_DYNLIB_SUPPORT,,
                             [Support for dynamic library loading.])
                   dynlib_support=yes;
                   HAVE_DYNLIB_SUPPORT_TOO="yes"
                 ])
    if test x"$dynlib_support" != "xyes" ; then
      AC_CHECK_LIB(dl, dlopen,
                 [ DYNLIB_LIBS="-ldl";
                   AC_DEFINE(HAVE_DYNLIB_SUPPORT,,
                             [Support for dynamic library loading.])
                   dynlib_support=yes;
                   HAVE_DYNLIB_SUPPORT_TOO="yes";
                   LIBS="-ldl $LIBS"
                 ],,)
    fi
  fi
elif test x"$is_win32" = "xyes" ; then
  AC_DEFINE(HAVE_DYNLIB_SUPPORT,,[Support for dynamic library loading.])
  HAVE_DYNLIB_SUPPORT_TOO="yes"
fi
AC_SUBST(DYNLIB_LIBS)

dnl ----- Joystick support -----
JOY_LIBS=
AM_CONDITIONAL(HAVE_LINUX_EVDEV, false)
if test x"$is_unix_x11" = "xyes" -a x"$enable_sdl1ui" != "xyes" -a x"$enable_sdl2ui" != "xyes" -a x"$enable_headlessui" != "xyes"; then
  dnl Linux evdev joystick driver
  if test x"$is_unix_linux" = "xyes"; then
    PKG_CHECK_MODULES([libevdev],
                      [libevdev],
                      [AC_DEFINE(HAVE_LINUX_EVDEV,,[Enable support for Linux evdev joysticks.])
                       JOYSTICK_DRIVERS="$JOYSTICK_DRIVERS joystick_linux_evdev.o"
                       HAVE_LINUX_EVDEV_SUPPORT="yes"],
                      [AC_MSG_ERROR([Please install libevdev headers.])])
    VICE_CFLAGS="$VICE_CFLAGS $libevdev_CFLAGS"
    VICE_CXXFLAGS="$VICE_CXXFLAGS $libevdev_CXXFLAGS"
    LIBS="$LIBS $libevdev_LIBS"
    AM_CONDITIONAL(HAVE_LINUX_EVDEV, true)
  fi

  dnl NetBSD/FreeBSD USB joystick support
  usbhid_header=no
  usbhid_lib=no
  usb_lib=no
  AC_CHECK_LIB(usbhid, hid_get_report_desc, usbhid_lib=yes)
  if test x"$usbhid_lib" = "xyes" ; then
    AC_CHECK_HEADER(usbhid.h,
                    [AC_DEFINE(HAVE_USBHID_H,1,
                      [Define to 1 if you have the <usbhid.h> header file.])
                    usb_header=yes])
    AC_CHECK_HEADER(libusbhid.h,
                    [AC_DEFINE(HAVE_LIBUSBHID_H,1,
                      [Define to 1 if you have the <libusbhid.h> header file.])
                    usb_header=yes])

    if test x"$usb_header" = "xyes" ; then
      AC_DEFINE(HAS_USB_JOYSTICK,,[Enable emulation for USB joysticks.])
      JOY_LIBS="-lusbhid"
      HAS_USB_JOYSTICK_SUPPORT="yes"
    fi
  else
    AC_CHECK_LIB(usb, hid_get_report_desc, usb_lib=yes)
    if test x"$usb_lib" = "xyes"; then
      AC_CHECK_HEADER(usb.h,
                      [AC_DEFINE(HAVE_USB_H,1,
                        [Define to 1 if you have the <usb.h> header file.])
                      usb_header=yes])
      AC_CHECK_HEADER(libusb.h,
                      [AC_DEFINE(HAVE_LIBUSB_H,1,
                        [Define to 1 if you have the <libusb.h> header file.])
                      usb_header=yes])
      if test x"$usb_header" = "xyes" ; then
        AC_DEFINE(HAS_USB_JOYSTICK,,[Enable emulation for USB joysticks.])
        JOY_LIBS="-lusb"
        HAS_USB_JOYSTICK_SUPPORT="yes"
      fi
    fi
  fi
  if test x"$HAS_USB_JOYSTICK_SUPPORT" = "xyes"; then
    JOYSTICK_DRIVERS="$JOYSTICK_DRIVERS joystick_bsd.o";
  fi
fi

if test x"$is_unix_macosx" = "xyes" -a x"$enable_sdl1ui" != "xyes" -a x"$enable_sdl2ui" != "xyes" -a x"$enable_headlessui" != "xyes"; then
  dnl --- Mac joystick support ---

  dnl check for HID Manager
  if test x"$enable_hidmgr" != "xno" ; then
    AC_CHECK_HEADER(IOKit/hid/IOHIDManager.h,,)
    if test x"$ac_cv_header_IOKit_hid_IOHIDManager_h" = "xyes" ; then
      AC_MSG_CHECKING(whether we can link the (mac) IOHIDManager)
      old_LIBS="${LIBS}"
      LIBS="$LIBS -framework IOKit -framework CoreServices"
      AC_TRY_LINK([#include <IOKit/hid/IOHIDManager.h>],
                  [IOHIDManagerCreate(NULL,0)],
                  [AC_MSG_RESULT(yes);
                   JOY_LIBS="-framework IOKit -framework CoreServices";
                   HAS_HIDMGR_SUPPORT="yes";
                   JOYSTICK_DRIVERS="$JOYSTICK_DRIVERS joystick_osx.o";
                   ],
                  [AC_MSG_RESULT(no);
                   LIBS=${old_LIBS}])
      LIBS="${old_LIBS}"
    fi
  fi
fi
AC_SUBST(JOY_LIBS)

dnl Check for math library
AC_CHECK_HEADERS(math.h)
AC_CHECK_LIB(m, sqrt,,,$LIBS)


dnl ----- ZLib -----
ZLIB_LIBS=

AC_CHECK_HEADER(zlib.h,,)
if test x"$ac_cv_header_zlib_h" = "xyes" ; then
  ac_cv_lib_z_zlibVersion=yes
  AC_CHECK_LIB(z, zlibVersion,
               [ ZLIB_LIBS="-lz";
                 HAVE_ZLIB_SUPPORT="yes";
                 AC_DEFINE(HAVE_ZLIB,,
                 [Can we use the ZLIB compression library?]) ],,)
else
  AC_MSG_ERROR([Zlib header not found, please install zlib.])
fi
AC_SUBST(ZLIB_LIBS)

dnl --- Curl / WIC64 ---
dnl We need at least version 7.77.1 for `CURLSSLOPT_NATIVE_CA`
dnl
dnl 2023-11-18: check relaxed to accept any version, the wic64 code uses an
dnl #ifdef to check for `CURLSSLOPT_NATIVE_CA` to see if the option can be
dnl enabled. See https://sourceforge.net/p/vice-emu/bugs/1964/
if test x"$with_libcurl" != "xno"; then
  PKG_CHECK_MODULES(libcurl, [libcurl], [have_libcurl=yes], [have_libcurl=no])
  if test x"$have_libcurl" = "xno"; then
    AC_MSG_ERROR([libcurl support requested, but libcurl was not found.])
  fi
  HAVE_LIBCURL_SUPPORT="yes"
  VICE_CFLAGS="$VICE_CFLAGS $libcurl_CFLAGS"
  VICE_LDFLAGS="$VICE_LDFLAGS $libcurl_LDFLAGS"
  LIBS="$LIBS $libcurl_LIBS"
  AC_DEFINE(HAVE_LIBCURL, , [libcurl support])
fi

dnl ----- Netplay Support -----
NETPLAY_LIBS=
old_LIBS="$LIBS"
if test x"$is_unix_x11" = "xyes" -o x"$is_unix_macosx" = "xyes"; then

  dnl Check for needed network headers
  UNIX_NETWORK_HEADERS_PRESENT=yes
  AC_CHECK_HEADERS(sys/types.h unistd.h sys/socket.h sys/time.h sys/select.h netinet/in.h arpa/inet.h netdb.h,,
                   [UNIX_NETWORK_HEADERS_PRESENT=no],)

  if test x"$UNIX_NETWORK_HEADERS_PRESENT" = "xyes"; then

    dnl Check for possible extra needed network libraries
    AC_CHECK_LIB(intl, dgettext,[ NETPLAY_LIBS="-lintl $NETPLAY_LIBS";
                                  LIBS="-lintl $LIBS"],,)
    AC_CHECK_LIB(nsl, gethostbyname,[ NETPLAY_LIBS="-lnsl $NETPLAY_LIBS";
                                      LIBS="-lnsl $LIBS"],,)
    AC_CHECK_LIB(socket, gethostbyname,[ NETPLAY_LIBS="-lsocket $NETPLAY_LIBS";
                                         LIBS="-lsocket $LIBS"],,)
    AC_CHECK_LIB(bsd, gethostbyname,[ NETPLAY_LIBS="-lbsd $NETPLAY_LIBS";
                                      LIBS="-lbsd $LIBS"],,)
    AC_CHECK_LIB(net, gethostbyname,[ NETPLAY_LIBS="-lnet $NETPLAY_LIBS";
                                      LIBS="-lnet $LIBS"],,)
    AC_CHECK_LIB(inet, gethostbyname,[ NETPLAY_LIBS="-linet $NETPLAY_LIBS";
                                       LIBS="-linet $LIBS"],,)

    dnl Check for needed functions
    UNIX_NETWORK_FUNCS_PRESENT=yes

    dnl Check for netinet/tcp.h
    AC_CHECK_HEADERS([netinet/tcp.h])

    AC_CHECK_FUNCS(socket send bind listen gethostbyname connect recv accept,,
                   [UNIX_NETWORK_FUNCS_PRESENT=no],)

    if test x"$UNIX_NETWORK_FUNCS_PRESENT" = "xyes"; then
      AC_DEFINE(HAVE_NETWORK,,[Enable netplay support])
      HAVE_NETWORK_SUPPORT="yes"
      AC_DEFINE(HAVE_RS232NET,,[Enable RS232 network support])
      HAVE_RS232NET_SUPPORT="yes"
      AC_CHECK_FUNCS(htons htonl)
      AC_CHECK_FUNCS(getdtablesize getrlimit)
      AC_MSG_CHECKING([For socklen_t])
      AC_TRY_COMPILE([#include <sys/types.h>
                      #include <sys/socket.h>],
                     [socklen_t foo; foo = 1235;],
                     [AC_DEFINE(HAVE_SOCKLEN_T,,[Define if the socklen_t type is present.])
                      AC_MSG_RESULT(yes)],
                     [AC_MSG_RESULT(no)])

      AC_MSG_CHECKING([for in_addr_t])
      AC_TRY_COMPILE([#include <sys/types.h>
                      #include <stdlib.h>
                      #include <stddef.h>
                      #include <netinet/in.h>], [in_addr_t foo;],
                      [AC_DEFINE(HAVE_IN_ADDR_T,,[Define if the in_addr_t type is present.])
                       AC_MSG_RESULT(yes)],
                      [AC_MSG_RESULT(no)])

      AC_MSG_CHECKING([for Unix domain sockets])
      AC_TRY_COMPILE([#include <sys/socket.h>
                      #include <sys/un.h>], [
                      static struct sockaddr_un addr;
                      socket(AF_UNIX, SOCK_STREAM, 0);],
                      [AC_DEFINE(HAVE_UNIX_DOMAIN_SOCKETS,,[Define if Unix domain sockets are supported])
                      AC_MSG_RESULT(yes)],
                      AC_MSG_RESULT(no))

    fi
  fi
elif test x"$is_beos" = "xyes"; then

  dnl Check for needed network headers
  BEOS_NETWORK_HEADERS_PRESENT=yes
  AC_CHECK_HEADERS(sys/socket.h netdb.h ByteOrder.h,,
                   [BEOS_NETWORK_HEADERS_PRESENT=no],)

  if test x"$BEOS_NETWORK_HEADERS_PRESENT" = "xyes"; then

    dnl Check for possible extra needed network libraries

    AC_CHECK_LIB(network, gethostbyname,[ NETPLAY_LIBS="-lnetwork $NETPLAY_LIBS";
                                      LIBS="-lnetwork $LIBS"],,)

    AC_CHECK_LIB(net, gethostbyname,[ NETPLAY_LIBS="-lnet -lnetapi $NETPLAY_LIBS";
                                      LIBS="-lnet -lnetapi $LIBS"],,)

    dnl Check for needed functions

    BEOS_NETWORK_FUNCS_PRESENT=yes

    AC_CHECK_FUNCS(socket send bind listen gethostbyname connect recv accept,,
                   [BEOS_NETWORK_FUNCS_PRESENT=no],)

    if test x"$BEOS_NETWORK_FUNCS_PRESENT" = "xyes"; then
      AC_DEFINE(HAVE_NETWORK,,[Enable netplay support])
      HAVE_NETWORK_SUPPORT="yes"
      AC_DEFINE(HAVE_RS232NET,,[Enable RS232 network support])
      HAVE_RS232NET_SUPPORT="yes"
      AC_CHECK_FUNCS(getdtablesize getrlimit)
    fi
  fi
fi

dnl Check for availability of IPV6
if test x"$is_unix_x11" = "xyes" -o x"$is_unix_macosx" = "xyes" ; then
  if test x"$UNIX_NETWORK_FUNCS_PRESENT" = "xyes"; then
    AC_MSG_CHECKING([if IPV6 should be enabled])
    if test x"$enable_ipv6" != "xno"; then
      have_ipv6=no
      AC_TRY_COMPILE([
        #include <sys/socket.h>
        #include <sys/types.h>
        #include <netinet/in.h>], [
        struct sockaddr_storage ss;
        struct in6_addr in6;
        socket(AF_INET6, SOCK_STREAM, 0);
        in6 = in6addr_any;
        ],
        have_ipv6=yes,
        have_ipv6=no
      )
      AC_MSG_RESULT($have_ipv6)

      if test $have_ipv6 = yes; then
        have_broken_ss_family=no

        dnl on some platforms, the structure sockaddr doesn't have a
        dnl ss_family, but __ss_family. If we find no ss_family then we
        dnl check for __ss_family, and if found define NEED_PREFIXED_SS_FAMILY.
        AC_MSG_CHECKING([struct sockaddr::ss_family])
        AC_TRY_COMPILE([
          #include <sys/socket.h>
          #include <sys/types.h>], [
          struct sockaddr_storage ss ;
          ss.ss_family = 0 ;
          ],
          have_ss_family=yes,
          have_ss_family=no
        )
        AC_MSG_RESULT($have_ss_family)
        if test x"$have_ss_family" = "xno" ; then
          AC_MSG_CHECKING([broken struct sockaddr::ss_family])
          AC_TRY_COMPILE([
            #include <sys/socket.h>
            #include <sys/types.h>], [
            struct sockaddr_storage ss ;
            ss.__ss_family = 0 ;
            ],
            have_broken_ss_family=yes,
            have_broken_ss_family=no
          )
          AC_MSG_RESULT($have_broken_ss_family)
          if test x$have_broken_ss_family = xyes ; then
            AC_DEFINE(NEED_PREFIXED_SS_FAMILY, [],
            [Whether struct sockaddr::__ss_family exists])
            AC_DEFINE(ss_family, __ss_family,
            [ss_family is not defined here, use __ss_family instead])
          else
            AC_MSG_WARN(ss_family and __ss_family not found)
          fi
        fi

        have_gethostbyname2=no
        AC_CHECK_FUNC(gethostbyname2, have_gethostbyname2=yes)
        if test $have_gethostbyname2 = yes; then
          AC_DEFINE([HAVE_GETHOSTBYNAME2], [], [Define if gethostbyname2 can be used])
          AC_DEFINE([HAVE_IPV6], [], [Define if ipv6 can be used])
          HAVE_IPV6_SUPPORT="yes"
        else
          have_getipnodebyname=no
          AC_CHECK_FUNC(getipnodebyname, have_getipnodebyname=yes)
          if test $have_getipnodebyname = yes; then
            AC_DEFINE([HAVE_GETIPNODEBYNAME], [], [Define if getipnodebyname can be used])
            AC_DEFINE([HAVE_IPV6], [], [Define if ipv6 can be used])
            HAVE_IPV6_SUPPORT="yes"
          fi
        fi
      fi
    else
      AC_MSG_RESULT("no")
    fi
  fi
fi
LIBS="$old_LIBS"
AC_SUBST(NETPLAY_LIBS)

dnl check for glibc 2.20+
linux_glibc_220_and_up=yes
AC_MSG_CHECKING(if we are using glibc-2.20 and up)
AC_TRY_LINK([#ifdef __linux
             #  include <ctype.h>
             #  ifdef __GLIBC__
             #    if (__GLIBC__>=2)
             #      if ((__GLIBC__==2 && __GLIBC_MINOR__>=20) || __GLIBC__>=3)
             #        error GLIBC2.20+
             #      endif
             #    endif
             #  endif
             #endif],
             [int a=0],
             linux_glibc_220_and_up=no)
AC_MSG_RESULT($linux_glibc_220_and_up)

dnl ----- Ethernet -----
TFE_LIBS=""
if test x"$enable_ethernet" = "xyes"; then
  have_tuntap="no"
  if test x"$is_win32" = "xyes"; then
    dnl We don't set TFE_LIBS here since we dynload the DLLs required on Windows
    dnl TODO: Maybe use AC_SEARCH_LIBS to support the npcap SDK headers?
    AC_CHECK_LIB(pcap, pcap_inject,[
      AC_DEFINE(HAVE_PCAP,,[Support for PCAP library.])
      AC_DEFINE(HAVE_RAWNET,,[Support for CS8900A ethernet controller.])
      HAVE_RAWNET_SUPPORT="yes";
      AC_DEFINE(HAVE_PCAP_INJECT,,[A libpcap version with pcap_inject is available])
    ],,$LIBS)
    AC_CHECK_LIB(pcap, pcap_sendpacket,[
      AC_DEFINE(HAVE_PCAP,,[Support for PCAP library.])
      AC_DEFINE(HAVE_RAWNET,,[Support for CS8900A ethernet controller.])
      HAVE_RAWNET_SUPPORT="yes";
      AC_DEFINE(HAVE_PCAP_SENDPACKET,,[A libpcap version with pcap_sendpacket is available])
    ],,$LIBS)
    if test x"$HAVE_RAWNET_SUPPORT" != "xyes"; then
      AC_MSG_ERROR([Ethernet support requested via --enable-ethernet, but the pcap headers were not found.])
    fi
  else
    dnl libpcap.a on Unix
    AC_CHECK_LIB(nsl, gethostbyname,[ LIBS="-lnsl $LIBS" ],,)
    AC_CHECK_LIB(socket, gethostbyname,[ LIBS="-lsocket $LIBS" ],,)
    AC_CHECK_LIB(pcap, pcap_inject,[
      AC_DEFINE(HAVE_PCAP,,[Support for PCAP library.])
      AC_DEFINE(HAVE_RAWNET,,[Support for CS8900A ethernet controller.])
      HAVE_RAWNET_SUPPORT="yes";
      AC_DEFINE(HAVE_PCAP_INJECT,,[A libpcap version with pcap_inject is available])
      TFE_LIBS="-lpcap"
    ],,$LIBS)
    AC_CHECK_LIB(pcap, pcap_sendpacket,[
      AC_DEFINE(HAVE_PCAP,,[Support for PCAP library.])
      AC_DEFINE(HAVE_RAWNET,,[Support for CS8900A ethernet controller.])
      HAVE_RAWNET_SUPPORT="yes";
      AC_DEFINE(HAVE_PCAP_SENDPACKET,,[A libpcap version with pcap_sendpacket is available])
      TFE_LIBS="-lpcap"
    ],,$LIBS)
    dnl libpcap.a
    dnl libnet.a
    AC_CHECK_LIB(pcap, pcap_open_live,[
      AC_CHECK_LIB(net, libnet_init,[
        AC_DEFINE(HAVE_PCAP,,[Support for PCAP library.])
        AC_DEFINE(HAVE_RAWNET,,[Support for CS8900A ethernet controller.])
        HAVE_RAWNET_SUPPORT="yes";
        AC_DEFINE(HAVE_LIBNET,,[Support for libnet library])
        TFE_LIBS="-lpcap `libnet-config --libs`"
        libnet_cflags="`libnet-config --defines`"
    ],,$LIBS)],,$LIBS)
    dnl libpcap.a
    dnl libnet.a
    AC_CHECK_LIB(pcap, pcap_open_live,[
      AC_CHECK_LIB(net, libnet_write_link_layer,[
        AC_DEFINE(HAVE_PCAP,,[Support for PCAP library.])
        AC_DEFINE(HAVE_RAWNET,,[Support for CS8900A ethernet controller.])
        HAVE_RAWNET_SUPPORT="yes";
        AC_DEFINE(HAVE_LIBNET,,[Support for libnet library])
        TFE_LIBS="-lpcap `libnet-config --libs`"
        libnet_cflags="`libnet-config --defines`"
    ],,$LIBS)],,$LIBS)
    if test x"$libnet_cflags" != "x"; then
      if test x"$linux_glibc_220_and_up" = "xyes"; then
        new_libnet_cflags=""
        for i in $libnet_cflags
        do
          if test x"$i" = "x-D_BSD_SOURCE"; then
            new_libnet_cflags="$new_libnet_cflags -D_DEFAULT_SOURCE"
          else
            new_libnet_cflags="$new_libnet_cflags $i"
          fi
        done
        CFLAGS="$CFLAGS $new_libnet_cflags"
      else
        CFLAGS="$CFLAGS $libnet_cflags"
      fi
    fi
    dnl check for tuntap
    AC_CHECK_HEADER(linux/if_tun.h,
                    [AC_DEFINE(HAVE_TUNTAP,,
                     [TUN/TAP support using <linux/if_tun.h>])
                     AC_DEFINE(HAVE_RAWNET,,[Support for CS8900A ethernet controller.])
                     have_tuntap=yes
                     HAVE_RAWNET_SUPPORT="yes"])
  fi
  if test x"$TFE_LIBS" = "x" -a x"$is_win32" = "xno" -a x"$is_win32_gtk3" = "xno" -a x"$have_tuntap" = "xno"; then
    AC_MSG_ERROR([Needed pcap library not found, please install libpcap-dev or equivalent])
  fi
fi
AC_SUBST(TFE_LIBS)

dnl ----- Sound Drivers -----
if test x"$is_unix" = "xyes" -a x"$is_unix_macosx" = "xno"; then
  if test x"$with_pulse" != "xno"; then
    AC_CHECK_HEADERS(pulse/simple.h,[AC_CHECK_LIB(pulse-simple, pa_simple_new,
                     [SOUND_DRIVERS="$SOUND_DRIVERS soundpulse.o";
                      SOUND_LIBS="$SOUND_LIBS -lpulse-simple -lpulse";
                      USE_PULSE_SUPPORT="yes";
                      AC_DEFINE(USE_PULSE,,[Enable pulseaudio support.])],,$SOUND_LIBS)])
    if test x"$with_pulse" != "xno" -a x"$USE_PULSE_SUPPORT" != "xyes"; then
      AC_MSG_ERROR([pulse audio support missing])
    fi
  fi

  dnl ALSA support checks
  if test x"$with_alsa" != "xno"; then
    AC_CHECK_HEADERS(alsa/asoundlib.h,[AC_CHECK_LIB(asound, snd_pcm_open,
                     [SOUND_DRIVERS="$SOUND_DRIVERS soundalsa.o";
                      SOUND_LIBS="$SOUND_LIBS -lasound";
                      USE_ALSA_SUPPORT="yes";
                      AC_DEFINE(USE_ALSA,,[Enable alsa support.])],,$SOUND_LIBS)])
    AC_CHECK_LIB(asound, snd_pcm_avail,
                 [AC_DEFINE(HAVE_SND_PCM_AVAIL,,[Use more accurate buffer fill reporting])])
    if test x"$with_alsa" != "xno" -a x"$USE_ALSA_SUPPORT" != "xyes"; then
      AC_MSG_ERROR([alsa support is missing])
    fi
  fi


  dnl OSS support checks
  dnl
  dnl For some reason 'sounduss.o' is actually correct. you'd expect soundoss.o
  dnl but that fails to compile
  dnl
  if test x"$with_oss" = "xyes"; then
    AC_CHECK_HEADERS(linux/soundcard.h machine/soundcard.h sys/soundcard.h soundcard.h,
                     [USE_OSS_SUPPORT="yes";
                      AC_DEFINE(USE_OSS,,[Enable oss support.])])
    AC_CHECK_LIB(ossaudio, _oss_ioctl,,,$SOUND_LIBS)
    if test x"$USE_OSS_SUPPORT" != "xyes"; then
      AC_MSG_ERROR([OSS support missing])
    fi
  fi

  dnl <sys/audioio.h> for sun / netbsd audio
  if test x"$with_sun" != "xno" -a x"$host_os" != "openbsd"; then
    AC_CHECK_HEADERS(sys/audioio.h,
                    [SOUND_DRIVERS="$SOUND_DRIVERS soundsun.o"; break])
  fi

fi


dnl --- Lame ---

if test x"$with_lame" = "xyes" ; then

  old_CFLAGS="$CFLAGS"
  CFLAGS="$VICE_CFLAGS"

  AC_CHECK_HEADER(lame/lame.h,,)

  dnl the header is available!
  if test x"$ac_cv_header_lame_lame_h" = "xyes" ; then
    AC_MSG_CHECKING([for lame lib linking])
    if test x"$dynlib_support" = "xyes" -a x"$with_static_lame" != "xyes" ; then
      dnl dynlib linking
      SOUND_DRIVERS="$SOUND_DRIVERS soundmp3.o lamelib.o"
      AC_DEFINE(USE_LAMEMP3,,[Use lame for MP3 encoding.])
      USE_LAMEMP3_SUPPORT="yes"
      AC_DEFINE(HAVE_EXTERNAL_LAME,,[External linking for lame libs])
      AC_MSG_RESULT([dynlib])
    else
      dnl try static linking
      AC_MSG_RESULT([trying static])
      AC_CHECK_LIB(mp3lame, lame_encode_buffer,[ADD_MPGDECODER=0],,)
      if test "$ac_cv_lib_mp3lame_lame_encode_buffer" = "no"; then
        unset ac_cv_lib_mp3lame_lame_encode_buffer
        AC_CHECK_LIB(mp3lame, lame_encode_buffer,[ADD_MPGDECODER=1],,[-lmpgdecoder])
      fi
      if test "$ac_cv_lib_mp3lame_lame_encode_buffer" != "no"; then
        SOUND_DRIVERS="$SOUND_DRIVERS soundmp3.o";
        SOUND_LIBS="$SOUND_LIBS -lmp3lame";
        if test "$ADD_MPGDECODER" = "1" ; then
          SOUND_LIBS="$SOUND_LIBS -lmpgdecoder";
        fi
        AC_DEFINE(USE_LAMEMP3,,[Use lame for MP3 encoding.])
        USE_LAMEMP3_SUPPORT="yes"
        AC_DEFINE(HAVE_STATIC_LAME,,[Static linking for lame libs])
      fi
      if test x"$USE_LAMEMP3_SUPPORT" != "xyes" -a x"$with_lame" = "xyes"; then
        AC_MSG_ERROR([lame mp3 support is missing])
      fi
    fi
  else
    AC_MSG_ERROR([lame mp3 support is missing])
  fi

  CFLAGS="$old_CFLAGS"
fi

AC_PATH_PROGS(FCCACHE, fc-cache, false)
if test x"$FCCACHE" = "xfalse"; then
  AM_CONDITIONAL(HAVE_FC_CACHE, false)
else
  AM_CONDITIONAL(HAVE_FC_CACHE, true)
fi

dsound_header_no_lib="no"

if test x"$is_win32" = "xyes" -o x"$is_win32_gtk3" = "xyes"; then
    if test x"$enable_sdl1ui" != "xyes" -a x"$enable_sdl2ui" != "xyes"; then
        SOUND_DRIVERS="$SOUND_DRIVERS soundwmm.o"
        AC_MSG_CHECKING(whether we can use directx sound)
        AC_CHECK_HEADER(dsound.h,
              [ AC_DEFINE(USE_DXSOUND,,[Enable directx sound support.])
                USE_DXSOUND_SUPPORT="yes" ]
              [ SOUND_DRIVERS="$SOUND_DRIVERS sounddx.o" ]
              [ AC_MSG_CHECKING(for -ldsound) ]
              [ old_LIBS="$LIBS" ]
              [ LIBS="$LIBS -ldsound" ]
              [ AC_LINK_IFELSE([
                               AC_LANG_PROGRAM(
                                               [#define DIRECTSOUND_VERSION 0x500
                                                #include <dsound.h>],
                                               [LPDIRECTSOUNDBUFFER buffer=NULL;
                                                IDirectSoundBuffer_Restore(buffer);]
                                              )
                               ],
                            [ AC_DEFINE(HAVE_DSOUND_LIB, [], [dsound.lib or libdsound.a are present]) ]
                            [ AC_MSG_RESULT(yes) ]
                            [ SOUND_LIBS="$SOUND_LIBS -ldsound" ],
                            [ AC_MSG_RESULT(no) ]
                            [ dsound_header_no_lib="yes" ]
              )]
              [ LIBS="$old_LIBS" ]
        )

        if test x"$dsound_header_no_lib" = "xyes" -o x"$dinput_header_no_lib" = "xyes"; then
              AC_MSG_CHECKING(if CoInitialize can be linked)
              LIBS="$LIBS -lole32"
              AC_LINK_IFELSE([
                               AC_LANG_PROGRAM(
                                               [#include <objbase.h>],
                                               [CoInitialize(NULL);]
                                              )
                               ],
                               [ AC_MSG_RESULT(yes) ],
                               [ AC_MSG_ERROR(no) ]
                              )
        fi
    else
        SOUND_DRIVERS="$SOUND_DRIVERS soundwmm.o"
    fi
fi

dnl Do not remove (yet), for some reason these things are required even for
dnl SDL2 on Haiku.
if test x"$is_beos" = "xyes"; then
    SOUND_DRIVERS="$SOUND_DRIVERS soundbeos.o soundbsp.o"
fi


dnl Check for Ogg/Vorbis support
if test x"$with_vorbis" != "xyes"; then
    VORBIS_PRESENT=no
else
    VORBIS_PRESENT=yes
    AC_CHECK_HEADERS(vorbis/vorbisfile.h,,[VORBIS_PRESENT=no],)

    if test x"$VORBIS_PRESENT" = "xyes"; then
        AC_CHECK_LIB(ogg, ogg_stream_flush,, [VORBIS_PRESENT=no])
    fi

    if test x"$VORBIS_PRESENT" = "xyes"; then
        AC_CHECK_LIB(vorbis, vorbis_info_clear,, [VORBIS_PRESENT=no])
    fi

    if test x"$VORBIS_PRESENT" = "xyes"; then
        AC_CHECK_LIB(vorbisfile, ov_open_callbacks,, [VORBIS_PRESENT=no])
    fi

    if test x"$VORBIS_PRESENT" = "xyes"; then
        AC_CHECK_LIB(vorbisenc, vorbis_encode_init_vbr,, [VORBIS_PRESENT=no])
    fi

    if test x"$with_vorbis" = "xyes" -a x"$VORBIS_PRESENT" = "xno"; then
        AC_MSG_ERROR([Ogg/Vorbis requested but missing.])
    fi
    dnl Where does soundvorbis.o come from?
    SOUND_DRIVERS="$SOUND_DRIVERS soundvorbis.o";
    LIBS="$LIBS -lvorbisfile -logg -lvorbis -lvorbisenc"
    USE_VORBIS_SUPPORT="yes"
    AC_DEFINE(USE_VORBIS,,[Enable ogg/vorbis support.])
fi


dnl Check FLAC support
if test x"$with_flac" != "xyes"; then
    FLAC_PRESENT=no
else
    FLAC_PRESENT=yes

    AC_CHECK_HEADERS(FLAC/stream_decoder.h,,[FLAC_PRESENT=no],)

    if test x"$FLAC_PRESENT" = "xyes"; then
        AC_CHECK_LIB(FLAC, FLAC__stream_decoder_get_channels,, [FLAC_PRESENT=no])
    fi

    if test x"$FLAC_PRESENT" = "xno"; then
        unset ac_cv_lib_FLAC_FLAC__stream_decoder_get_channels
        dnl Windows-specific check:
        AC_CHECK_LIB(FLAC, FLAC__stream_decoder_get_channels,, [FLAC_PRESENT=no], [-lwinmm])
        if test x"$FLAC_PRESENT" = "xyes"; then
            LIBS="$LIBS -lwinmm"
        fi
    fi

    if test x"$FLAC_PRESENT" = "xno"; then
        AC_MSG_ERROR([FLAC support requested, but missing.])
    fi
    LIBS="$LIBS -lFLAC"
    SOUND_DRIVERS="$SOUND_DRIVERS soundflac.o"
    USE_FLAC_SUPPORT="yes"
    AC_DEFINE(USE_FLAC,,[Enable FLAC support.])

fi


if test x"$is_unix_macosx" = "xyes"; then

  dnl --- check for CoreAudio ---
  AC_CHECK_HEADER(CoreAudio/CoreAudio.h,,)
  if test x"$ac_cv_header_CoreAudio_CoreAudio_h" = "xyes" ; then
    AC_CHECK_HEADER(AudioToolbox/AudioToolbox.h,,)
    if test x"$ac_cv_header_AudioToolbox_AudioToolbox_h" = "xyes" ; then
      AC_MSG_CHECKING(whether we can link the CoreAudio framework)
      old_LIBS="${LIBS}"
      LIBS="${LIBS} -framework CoreAudio -framework AudioToolbox"
      AC_TRY_LINK([#include <CoreAudio/CoreAudio.h>],
                  [AudioDeviceStart(0,0);],
                  [AC_MSG_RESULT(yes);
                   SOUND_DRIVERS="$SOUND_DRIVERS soundcoreaudio.o";
                   SOUND_LIBS="$SOUND_LIBS -framework CoreAudio -framework AudioToolbox";
                   USE_COREAUDIO_SUPPORT="yes";
                   AC_DEFINE(USE_COREAUDIO,,[Enable CoreAudio support.])],
                  [AC_MSG_RESULT(no)])
      LIBS=${old_LIBS}

      AC_CHECK_HEADER(AudioUnit/AudioUnit.h,,)
      if test x"$ac_cv_header_AudioUnit_AudioUnit_h" = "xyes" ; then
        AC_MSG_CHECKING(whether we can link the AudioUnit framework)
        old_LIBS="${LIBS}"
        LIBS="${LIBS} -framework AudioUnit"
        AC_TRY_LINK([#include <AudioUnit/AudioUnit.h>],
                    [AudioOutputUnitStart(NULL);],
                    [AC_MSG_RESULT(yes);
                     SOUND_LIBS="$SOUND_LIBS -framework AudioUnit";
                     HAVE_AUDIO_UNIT_SUPPORT="yes";
                     AC_DEFINE(HAVE_AUDIO_UNIT,,[Enable AudioUnit support.])],
                    [AC_MSG_RESULT(no)])
        LIBS=${old_LIBS}
      fi
    fi
  fi

  dnl --- check for CoreMIDI ---
  if test x"$enable_midi" = "xyes"; then
    AC_MSG_CHECKING(whether we can link the CoreMIDI framework)
    old_LIBS="${LIBS}"
    LIBS="$LIBS -framework CoreMidi -framework CoreServices"
    AC_TRY_LINK([#include <CoreMIDI/MIDIServices.h>],
                [MIDIGetNumberOfDevices()],
                [AC_MSG_RESULT(yes);
                 AC_DEFINE(HAVE_MIDI,,[Enable MIDI emulation.])
                 HAVE_MIDI_SUPPORT="yes"],
               [AC_MSG_RESULT(no);
                 LIBS=${old_LIBS}])
  fi
  if test x"$enable_midi" = "xyes" -a x"$HAVE_MIDI_SUPPORT" != "xyes"; then
    AC_MSG_ERROR([MIDI support missing])
  fi
fi

AC_SUBST(SOUND_DRIVERS)
AC_SUBST(SOUND_LIBS)

dnl Check for ParSID/USBSID/HardSID/CW support
if test x"$is_unix" = "xyes"; then

  dnl Check cpu first
  case "$host_cpu" in
  i*86 | x86_64* | amd64*)
      ;;
  *)
      enable_parsid=no
      enable_hardsid=no
      enable_catweasel=no
      ;;
  esac

  if test x"$enable_parsid" = "xyes" -o x"$enable_hardsid" != "xno" -o x"$enable_catweasel" = "xyes"; then

    if test x"$with_libieee1284" = "xyes"; then
      LIBIEEE1284_HEADERS_PRESENT=yes
    else
      LIBIEEE1284_HEADERS_PRESENT=no
    fi
    LINUX_HARDSID_HEADER_PRESENT=yes
    LINUX_PARPORT_HEADERS_PRESENT=yes
    CATWEASEL_HEADER_PRESENT=yes
    BSD_SET_IOPERM_HEADERS_PRESENT=yes
    PARSID_SUPPORT=no
    IOSID_SUPPORT=no
    PORTSID_SUPPORT=no
    PCIUTILS_HEADER_PRESENT=yes
    PCIUTILS_LIB_PRESENT=no

    dnl Check for needed io headers
    if test x"$with_libieee1284" = "xyes"; then
      AC_CHECK_HEADERS(ieee1284.h,,[LIBIEEE1284_HEADERS_PRESENT=no],)
    fi
    AC_CHECK_HEADERS(linux/hardsid.h,,[LINUX_HARDSID_HEADER_PRESENT=no],)
    AC_CHECK_HEADERS(linux/ppdev.h linux/parport.h,,[LINUX_PARPORT_HEADERS_PRESENT=no],)
    AC_CHECK_HEADERS(dev/ppbus/ppi.h dev/ppbus/ppbconf.h,,[FREEBSD_PARPORT_HEADERS_PRESENT=no],)
    AC_CHECK_HEADERS(cwsid.h,,[CATWEASEL_HEADER_PRESENT=no],)
    AC_CHECK_HEADERS(machine/sysarch.h,,[BSD_SET_IOPERM_HEADERS_PRESENT=no],)
    AC_CHECK_HEADERS(unistd.h sys/io.h machine/pio.h machine/cpufunc.h)
    AC_CHECK_HEADERS(pci/pci.h,,[PCIUTILS_HEADER_PRESENT=no],)

    if test x"$PCIUTILS_HEADER_PRESENT" = "xyes"; then
      AC_CHECK_LIB(pciutils, pci_fill_info,[LIBS="-lpciutils $LIBS"; PCIUTILS_LIB_PRESENT=yes],,)
      if test x"$PCIUTILS_LIB_PRESENT" = "xno"; then
        AC_CHECK_LIB(pci, pci_fill_info,[LIBS="-lpci $LIBS"; PCIUTILS_LIB_PRESENT=yes],,)
      fi
      if test x"$PCIUTILS_LIB_PRESENT" != "xno"; then
        AC_DEFINE(HAVE_LIBPCI,,[Define to 1 if you have the 'pciutils' library.])
        HAVE_PCIUTILS_SUPPORT=yes
      fi
    fi

    if test x"$LIBIEEE1284_HEADERS_PRESENT" = "xyes"; then
      AC_CHECK_LIB(ieee1284, ieee1284_find_ports,[LIBS="-lieee1284 $LIBS"; PARSID_SUPPORT=yes],,)
      if test x"$PARSID_SUPPORT" = "xyes"; then
        AC_DEFINE(HAVE_LIBIEEE1284,
                  [Define to 1 if you have the 'ieee1284' library (-lieee1284).])
        HAVE_LIBIEEE1284_SUPPORT="yes"
      else
        AC_MSG_ERROR([libieee1284 is missing])
      fi
    else
      if test x"$with_libieee1284" = "xyes"; then
        AC_MSG_ERROR([libieee1284 header is missing])
      fi
    fi

    if test x"$PARSID_SUPPORT" != "xno" -a x"$enable_hardsid" = "xno"; then
      IOSID_SUPPORT=yes
      HAVE_IOSID_SUPPORT=yes
    fi

    if test x"$LINUX_PARPORT_HEADERS_PRESENT" = "xyes"; then
      AC_DEFINE(HAVE_LINUX_PARPORT_HEADERS,,[Support for Linux par port device file.])
      PORTSID_SUPPORT=yes
      HAVE_PORTSID_SUPPORT=yes
    fi

    if test x"$FREEBSD_PARPORT_HEADERS_PRESENT" = "xyes"; then
      AC_DEFINE(HAVE_FREEBSD_PARPORT_HEADERS,,[Support for FreeBSD par port device file.])
      PORTSID_SUPPORT=yes
      HAVE_PORTSID_SUPPORT=yes
    fi

    if test x"$IOSID_SUPPORT" = "xno"; then
      if test x"$BSD_SET_IOPERM_HEADERS_PRESENT" = "xyes"; then
        AC_CHECK_LIB(amd64, amd64_set_ioperm,[LIBS="-lamd64 $LIBS"; IOSID_SUPPORT=yes],,)
        if test x"$IOSID_SUPPORT" = "xyes"; then
          AC_DEFINE(HAVE_LIBAMD64,,
                    [Define to 1 if you have the 'amd64' library (-lamd64).])
        fi
      fi
    fi

    if test x"$IOSID_SUPPORT" = "xno"; then
      if test x"$BSD_SET_IOPERM_HEADERS_PRESENT" = "xyes"; then
        AC_CHECK_LIB(i386, i386_get_ioperm,[LIBS="-li386 $LIBS"],,)
        AC_CHECK_FUNCS(i386_set_ioperm,[IOSID_SUPPORT=yes],)
      fi
    fi

    if test x"$IOSID_SUPPORT" = "xno"; then
      AC_CHECK_FUNCS(ioperm,[IOSID_SUPPORT=yes],)
      AC_CHECK_FUNCS(inb outb outb_p inb_p inbv outbv)
    fi

    if test x"$IOSID_SUPPORT" = "xyes"; then
      if test x"$enable_parsid" = "xyno"; then
        AC_DEFINE(HAVE_PARSID,,[Support for ParSID.])
        AM_CONDITIONAL(HAVE_PARSID, true)
        HAVE_PARSID_SUPPORT="yes"
      fi
      if test x"$enable_catweasel" = "xyes"; then
        AC_DEFINE(HAVE_CATWEASELMKIII,,[Support for Catweasel MKIII.])
        AM_CONDITIONAL(HAVE_CATWEASELMKIII, true)
        HAVE_CATWEASELMKIII_SUPPORT="yes"
      fi
      if test x"$enable_hardsid" != "xno"; then
        AC_DEFINE(HAVE_HARDSID,,[Support for HardSID.])
        HAVE_HARDSID_SUPPORT="yes"
      fi
    else
      if test x"$enable_hardsid" != "xno" -a x"$LINUX_HARDSID_HEADER_PRESENT" = "xyes"; then
        AC_DEFINE(HAVE_HARDSID,,[Support for HardSID.])
        HAVE_HARDSID_SUPPORT="yes"
      fi
      if test x"$enable_catweasel" = "xyes" -a x"$CATWEASEL_HEADER_PRESENT" = "xyes"; then
        AC_DEFINE(HAVE_CATWEASELMKIII,,[Support for Catweasel MKIII.])
        AM_CONDITIONAL(HAVE_CATWEASELMKIII, true)
        HAVE_CATWEASELMKIII_SUPPORT="yes"
      fi
    fi

    if test x"$PORTSID_SUPPORT" = "xyes"; then
      if test x"$enable_parsid" = "xyes" -a x"HAVE_PARSID_SUPPORT" != "xyes"; then
        AC_DEFINE(HAVE_PARSID,,[Support for ParSID.])
        AM_CONDITIONAL(HAVE_PARSID, true)
        AC_DEFINE(HAVE_PORTSID,,[Support for file device based access to ParSID.])
        HAVE_PARSID_SUPPORT="yes"
      fi
    fi

    if test x"$enable_catweasel" = "xyes" -a x"$HAVE_CATWEASELMKIII_SUPPORT" != "xyes"; then
      AC_MSG_ERROR([CatWeaselMKIII support is missing])
    fi

    if test x"$enable_hardsid" = "xyes" -a x"$HAVE_HARDSID_SUPPORT" != "xyes"; then
      AC_MSG_ERROR([HardSID support is missing])
    fi

  fi

  dnl usbsid required libraries and headers
  if test x"$enable_usbsid" != "xno" ; then
    PKG_PROG_PKG_CONFIG
    UNIX_USBSID_HEADER_PRESENT=yes
    AC_CHECK_HEADERS(libusb-1.0/libusb.h,,[UNIX_USBSID_HEADER_PRESENT=no],)
    if test x"$UNIX_USBSID_HEADER_PRESENT" = "xyes"; then
      AC_CHECK_LIB(usb-1.0, libusb_init,[LIBS="-lusb-1.0 $LIBS"; USBSID_LIB_PRESENT=yes],,)
      if test x"$PKG_CONFIG" != "x" -a x"$USBSID_LIB_PRESENT" = "xyes"; then
        PKG_CHECK_MODULES(LIBUSB, [libusb-1.0], HAVE_LIBUSB_SUPPORT="yes")
        AC_MSG_CHECKING([for pkgconfig libusb-1.0])
        if test x"$HAVE_LIBUSB_SUPPORT" = "xyes"; then
          VICE_CFLAGS="$VICE_CFLAGS $LIBUSB_CFLAGS";
          VICE_CPPFLAGS="$VICE_CPPFLAGS $LIBUSB_CFLAGS";
          AC_MSG_RESULT([yes])
        else
          AC_MSG_RESULT([no])
        fi
      fi
    fi
  fi

  dnl usbsid support
  AM_CONDITIONAL(HAVE_USBSID, false)
  AC_MSG_CHECKING([for USBSID-Pico support])
  if test x"$enable_usbsid" != "xno" ; then
    if test x"$HAVE_LIBUSB_SUPPORT" = "xyes"; then
      AC_DEFINE([HAVE_USBSID],,[Support for USBSID])
      HAVE_USBSID_SUPPORT="yes"
      AM_CONDITIONAL(HAVE_USBSID, true)
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no (LIBUSB not found!!)])
    fi
  else
    AC_MSG_RESULT([no (USBSID-Pico disabled)])
  fi

fi

AC_TYPE_SIGNAL
AC_FUNC_VFORK
AC_CHECK_TYPES(u_short)
AC_CHECK_TYPES(socklen_t, [], [], [[#include <sys/socket.h>]])
dnl POSIX provides off_t by including <stdio.h>, but on Windows we have to
dnl include <sys/types.h>
AC_CHECK_TYPES(off_t)
AC_CHECK_TYPES(off_t,
               [AC_DEFINE(HAVE_OFF_T_IN_SYS_TYPES,,[Include sys/types.h for off_t])],
               [],
               [[#include <sys/types.h>]])

dnl some platforms have some of the functions in libbsd,
dnl so we check it out first.
AC_CHECK_LIB(bsd,gettimeofday,,,$LIBS)

dnl some platforms have some of the functions in libposix,
dnl so we check it out second.
AC_CHECK_LIB(posix,gettimeofday,,,$LIBS)

AC_CHECK_FUNCS(gettimeofday memmove atexit strerror strcasecmp strncasecmp dirname mkstemp swab getcwd getpwuid random rewinddir strtok strtok_r strtoul snprintf vsnprintf ltoa ultoa stpcpy strlcpy strlwr strrev fseeko ftello _fseeki64 _ftelli64)
AC_CHECK_FUNCS(strdup, [have_strdup_func=yes], [have_strdup_func=no])

if test x"$have_strdup_func" = "xno"; then
  AC_MSG_CHECKING(whether strdup is defined as a macro)
  AC_TRY_LINK([#include <string.h>],
               [char *something = strdup("something")],
               [AC_MSG_RESULT(yes); have_strdup_func=yes],
               [AC_MSG_RESULT(no); have_strdup_func=no])

  if test x"$have_strdup_func" = "xyes"; then
    AC_DEFINE(HAVE_STRDUP,,[Define to 1 if you have the 'strdup' function.])
  fi
fi

AC_SUBST(LIBS)


AC_CHECK_FUNC(nanosleep,
              [ AC_DEFINE(HAVE_NANOSLEEP,,[Use nanosleep instead of usleep]) ])

dnl Check time.h.

dnl AC_HEADER_TIME
dnl AC_STRUCT_TM

dnl Configure graphics output drivers
old_LIBS="$LIBS"
GFXOUTPUT_LIBS=

dnl check for png support
if test x"$with_png" != "xno" ; then
  dnl Check for the PNG library.
  AC_CHECK_HEADER(png.h,,)
  if test x"$ac_cv_header_png_h" = "xyes" ; then
    unset ac_cv_lib_png_png_sig_cmp
    AC_CHECK_LIB(png, png_sig_cmp,
                 [ GFXOUTPUT_LIBS="-lpng $GFXOUTPUT_LIBS";
                   GFXOUTPUT_DRIVERS="$GFXOUTPUT_DRIVERS pngdrv.o";
                   HAVE_PNG_SUPPORT="yes";
                   AC_DEFINE(HAVE_PNG,,[Can we use the PNG library?]) ],,)
    if test "$ac_cv_lib_png_png_sig_cmp" = "no"; then
      unset ac_cv_lib_png_png_sig_cmp
      AC_CHECK_LIB(png, png_sig_cmp,
                   [ GFXOUTPUT_LIBS="-lpng -lz $GFXOUTPUT_LIBS";
                     GFXOUTPUT_DRIVERS="$GFXOUTPUT_DRIVERS pngdrv.o";
                     HAVE_PNG_SUPPORT="yes";
                     AC_DEFINE(HAVE_PNG,,
                               [Can we use the PNG library?]) ],,"-lz")
    fi
  fi

  if test x"$HAVE_PNG_SUPPORT" != "xyes" -a x"$cross_compiling" != "xyes"; then
    PKG_PROG_PKG_CONFIG
    if test x"$PKG_CONFIG" != "x"; then
      AC_MSG_CHECKING([if pkgconfig libpng is present])
      PKG_CHECK_MODULES(PNG, [libpng], HAVE_PNG_SUPPORT="yes")
      if test x"$HAVE_PNG_SUPPORT" = "xyes"; then
          GFXOUTPUT_LIBS="$PNG_LIBS $GFXOUTPUT_LIBS"
          GFXOUTPUT_DRIVERS="$GFXOUTPUT_DRIVERS pngdrv.o";
          VICE_CFLAGS="$VICE_CFLAGS $PNG_CFLAGS";
          AC_DEFINE(HAVE_PNG,,[Can we use the PNG library?])
          AC_MSG_RESULT([yes])
      else
          AC_MSG_RESULT([no])
      fi
    fi
  fi

  if test x"$HAVE_PNG_SUPPORT" != "xyes"; then
    AC_MSG_ERROR([PNG support is missing])
  fi
fi

dnl Check for the GIF or UNGIF library.
if test x"$with_gif" = "xyes"; then
  AC_CHECK_HEADER(gif_lib.h,,)
  if test x"$ac_cv_header_gif_lib_h" = "xyes" ; then
    AC_CHECK_LIB(ungif, EGifPutLine, [
                 GFXOUTPUT_LIBS="-lungif $GFXOUTPUT_LIBS";
                 GFXOUTPUT_DRIVERS="$GFXOUTPUT_DRIVERS gifdrv.o";
                 HAVE_GIF_SUPPORT="yes";
                 AC_DEFINE(HAVE_GIF,,[Can we use the GIF or UNGIF library?]) ],,)
    if test "$ac_cv_lib_ungif_EGifPutLine" = "no"; then
      AC_CHECK_LIB(gif, EGifPutLine, [
                   GFXOUTPUT_LIBS="-lgif $GFXOUTPUT_LIBS";
                   GFXOUTPUT_DRIVERS="$GFXOUTPUT_DRIVERS gifdrv.o";
                   HAVE_GIF_SUPPORT="yes";
                   AC_DEFINE(HAVE_GIF,,[Can we use the GIF or UNGIF library?]) ],,)
    fi
 fi
 if test x"$HAVE_GIF_SUPPORT" != "xyes"; then
   AC_MSG_ERROR([GIF support not present])
 fi
fi


LIBS="$old_LIBS"
AC_SUBST(GFXOUTPUT_DRIVERS)
AC_SUBST(GFXOUTPUT_LIBS)

AC_SUBST(JOYSTICK_DRIVERS)

dnl PortAudio support checks
if test x"$with_portaudio" = "xyes"; then
  AC_CHECK_HEADERS(portaudio.h, PA_HEADERS="yes", PA_HEADERS="no")

  if test x"$PA_HEADERS" = "xyes"; then
    if test x"$is_win32" = "xyes" -o x"$is_win32_gtk3" = "xyes"; then
      AC_CHECK_LIB(portaudio, Pa_Initialize,
                   [LIBS="$LIBS -lportaudio -lwinmm";
                    USE_PORTAUDIO_SUPPORT="yes";
                    AC_DEFINE(USE_PORTAUDIO,,[Enable portaudio sampling support.])],,-lwinmm)
    else
      AC_CHECK_LIB(portaudio, Pa_Initialize,
                 [LIBS="$LIBS -lportaudio";
                  USE_PORTAUDIO_SUPPORT="yes";
                  AC_DEFINE(USE_PORTAUDIO,,[Enable portaudio sampling support.])],,)
    fi
  fi

  if test x"$with_portaudio" = "xyes" -a x"$USE_PORTAUDIO_SUPPORT" != "xyes"; then
    AC_MSG_ERROR([PortAudio support requested (--with-portaudio), but PortAudio headers are missing])
  fi
fi

dnl Check MP3 decoding support
if test x"$with_mpg123" = "xyes"; then
    AC_CHECK_HEADERS(mpg123.h,[AC_CHECK_LIB(mpg123, mpg123_init,
                     [LIBS="$LIBS -lmpg123";
                     USE_MPG123_SUPPORT="yes";
                     AC_DEFINE(USE_MPG123,,[Enable mpg123 mp3 decoding support.])],,)])
    if test x"$USE_MPG123_SUPPORT" != "xyes"; then
        AC_MSG_ERROR([MPG123 support requested but missing])
    fi
    if test x"$is_win32" = "xyes"; then
        AC_DEFINE(HAVE_GLOB_H,,)
    else
        AC_CHECK_HEADERS(glob.h,,)
    fi
fi

dnl Check for libraries and header files
if test x"$is_beos" = "xyes"; then

  dnl On BEOS, we already know what we want.

  LIBS="$LIBS -lbe -ltracker -ldevice -lmedia -lgame"
  dnl AC_SUBST(LDFLAGS)
fi

dnl CBM4Linux/OpenCBM
AC_MSG_CHECKING([for OpenCBM support])
if test x"$enable_realdevice" != "xno"; then
  if test x"$is_win32" = "xyes"; then
    AC_DEFINE(HAVE_REALDEVICE,,[Support for OpenCBM (former CBM4Linux).])
    HAVE_REALDEVICE_SUPPORT="yes"
    AM_CONDITIONAL(HAVE_REALDEVICE, true)
    AC_MSG_RESULT([yes])
  else
    if test x"$dynlib_support" = "xyes"; then
      if test x"$is_beos" != "xyes"; then
        AC_DEFINE(HAVE_REALDEVICE,,[Support for OpenCBM (former CBM4Linux).])
        HAVE_REALDEVICE_SUPPORT="yes"
        AM_CONDITIONAL(HAVE_REALDEVICE, true)
        AC_MSG_RESULT([yes])
      else
        AM_CONDITIONAL(HAVE_REALDEVICE, false)
        AC_MSG_RESULT([no (not supported on current host)])
      fi
    else
      AM_CONDITIONAL(HAVE_REALDEVICE, false)
      AC_MSG_RESULT([no (dynamic lib support missing)])
    fi
  fi
else
  AM_CONDITIONAL(HAVE_REALDEVICE, false)
  AC_MSG_RESULT([no (realdevice disabled)])
fi

AC_SUBST(VICE_CPPFLAGS)
AC_SUBST(VICE_CFLAGS)
AC_SUBST(VICE_CXXFLAGS)
AC_SUBST(VICE_LDFLAGS)

if test x"$is_unix_macosx" = "xyes"; then
  dnl For now, OBJCFLAGS is the same as CFLAGS
  VICE_OBJCFLAGS="$VICE_CFLAGS $OBJCFLAGS"
  AC_SUBST(VICE_OBJCFLAGS)
fi

AC_SUBST(vsid_LDFLAGS)
AC_SUBST(x64_LDFLAGS)
AC_SUBST(x128_LDFLAGS)
AC_SUBST(xscpu64_LDFLAGS)
AC_SUBST(xvic_LDFLAGS)
AC_SUBST(xpet_LDFLAGS)
AC_SUBST(xplus4_LDFLAGS)
AC_SUBST(xcbm2_LDFLAGS)


dnl Do we need to build x64?
if test x"$enable_x64" = "xyes"; then
  X64_INCLUDED="yes"
  AM_CONDITIONAL(SUPPORT_X64, true)
else
  AM_CONDITIONAL(SUPPORT_X64, false)
  X64_INCLUDED="no"
fi


dnl Setup the system-specific object files.
AM_CONDITIONAL(USE_SDLUI, false)
AM_CONDITIONAL(USE_SDL2UI, false)
if test x"$enable_sdl1ui" = "xyes" -o x"$enable_sdl2ui" = "xyes"; then
  ARCH_DIR="\$(top_builddir)/src/arch/sdl"
  ARCH_SRC_DIR="\$(top_srcdir)/src/arch/sdl"
  ARCH_LIBS="$ARCH_DIR/libarch.a"
  ARCH_INCLUDES="-I$ARCH_SRC_DIR -I\$(top_srcdir)/src/arch/shared"
  AC_DEFINE(HAVE_MOUSE,,[Enable 1351 mouse support])
  HAVE_MOUSE_SUPPORT="yes"
  AC_DEFINE(HAVE_LIGHTPEN,,[Enable lightpen support])
  HAVE_LIGHTPEN_SUPPORT="yes"

  if test x"$enable_sdl2ui" = "xyes"; then
    AM_CONDITIONAL(USE_SDL2UI, true)
    SDLCONFIG="$sdl2_config"
  else
    AM_CONDITIONAL(USE_SDLUI, true)
    SDLCONFIG="$sdl_config"
  fi
  AC_SUBST(SDLCONFIG)
fi

SDL_EXTRA_LIBS=""

AM_CONDITIONAL(MAKE_INSTALL, true)
AM_CONDITIONAL(MAKE_BINDIST, false)


dnl
dnl Set up X64 image support
dnl
if test x"$enable_x64_image" = "xyes"; then
  HAVE_X64_IMAGE_SUPPORT="yes"
  AM_CONDITIONAL(HAVE_X64_IMAGE, true)
  AC_DEFINE(HAVE_X64_IMAGE,,[Support X64 images])
else
  AM_CONDITIONAL(HAVE_X64_IMAGE, false)
fi


if test x"$enable_gtk3ui" = "xyes"; then
  dnl Use C++ compiler/linker since novte uses C++
  LINKCC='$(CXX)'
  ARCH_DIR="\$(top_builddir)/src/arch/gtk3"
  ARCH_SRC_DIR="\$(top_srcdir)/src/arch/gtk3"
  ARCH_LIBS="$ARCH_DIR/libarch.a $ARCH_DIR/widgets/libwidgets.a $ARCH_DIR/widgets/base/libbasewidgets.a $ARCH_DIR/novte/libnovte.a $ARCH_DIR/joystickdrv/libjoystickdrv.a"
  if test x"$is_win32" = "xyes" -o x"$is_win32_gtk3" = "xyes"; then
    ARCH_INCLUDES="-I$ARCH_SRC_DIR -I\$(top_srcdir)/src/arch/shared -I\$(top_srcdir)/src/arch/mingw32-pcap/wpcap"
    AM_CONDITIONAL(MAKE_INSTALL, false)
    AM_CONDITIONAL(MAKE_BINDIST, true)
  else
    ARCH_INCLUDES="-I$ARCH_SRC_DIR -I\$(top_srcdir)/src/arch/shared -I\$(top_srcdir)/src/arch"
  fi
  AC_DEFINE(HAVE_MOUSE,,[Enable 1351 mouse support])
  HAVE_MOUSE_SUPPORT="yes"
  AC_DEFINE(HAVE_LIGHTPEN,,[Enable lightpen support])
  HAVE_LIGHTPEN_SUPPORT="yes"

  AM_CONDITIONAL(USE_GTK3UI, true)

else
  AM_CONDITIONAL(USE_GTK3UI, false)
fi


# WINDRES_LIB is used to link some Windows resource file to Windows binaries
# It's not longer used by Gtk3, that uses its own per-emu files, so right now it's
# only used for SDL. But the Gtk3 Windows .res files should also be used for SDL
# on Windows, which means moving around some autotools stuff.
#
# For now though, let's keep this intact. -- compyx
#
if test x"$enable_sdl" = "xyes" -o x"$enable_sdl2ui" = "xyes"; then
  WINDRES_LIB="$ARCH_DIR/icon.res"
else
  WINDRES_LIB=""
fi


if test x"$enable_headlessui" = "xyes"; then
  ARCH_DIR="\$(top_builddir)/src/arch/headless"
  ARCH_SRC_DIR="\$(top_srcdir)/src/arch/headless"
  ARCH_LIBS="$ARCH_DIR/libarch.a"
  if test x"$is_win32" = "xyes"; then
    ARCH_INCLUDES="-I$ARCH_SRC_DIR -I\$(top_srcdir)/src/arch/shared -I\$(top_srcdir)/src/arch/mingw32-pcap/wpcap"
    AM_CONDITIONAL(MAKE_INSTALL, false)
    AM_CONDITIONAL(MAKE_BINDIST, true)
  else
    ARCH_INCLUDES="-I$ARCH_SRC_DIR -I\$(top_srcdir)/src/arch/shared -I\$(top_srcdir)/src/arch"
  fi
  AC_DEFINE(HAVE_MOUSE,,[Enable 1351 mouse support])
  HAVE_MOUSE_SUPPORT="yes"
  AC_DEFINE(HAVE_LIGHTPEN,,[Enable lightpen support])
  HAVE_LIGHTPEN_SUPPORT="yes"

  AM_CONDITIONAL(USE_HEADLESSUI, true)
else
  AM_CONDITIONAL(USE_HEADLESSUI, false)
fi

if test x"$is_unix" = "xyes"; then

  if test x"$is_win32_gtk3" = "xyes"; then
    AC_DEFINE(WINDOWS_COMPILE,,[Are we compiling for windows?])
    LIBS="$LIBS -lntdll"
  else
    AC_DEFINE(UNIX_COMPILE,,[Are we compiling for unix?])
  fi

  AM_CONDITIONAL(UNIX_COMPILE, true)
  if test x"$is_win32_gtk3" = "xyes"; then
     AM_CONDITIONAL(WINDOWS_COMPILE, true)
  else
     AM_CONDITIONAL(WINDOWS_COMPILE, false)
  fi

  if test x"$is_unix_macosx" = "xyes" ; then
    AM_CONDITIONAL(MACOS_COMPILE, true)
    dnl macOS supports both make install and make bindist
    AM_CONDITIONAL(MAKE_BINDIST, true)
  else
    AM_CONDITIONAL(MACOS_COMPILE, false)
  fi


elif test x"$is_win32" = "xyes"; then
  AC_DEFINE(WINDOWS_COMPILE,,[Are we compiling for win32?])

  if test x"$enable_sdl1ui" = "xyes" -o x"$enable_sdl2ui" = "xyes"; then
    if test x"$host_cpu" = "xx86_64" -o x"$host_cpu" = "xamd64"; then
      SDL_EXTRA_LIBS="$UI_LIBS -lwinmm -lws2_32"
    else
      SDL_EXTRA_LIBS="$UI_LIBS -lwinmm -lwsock32"
    fi
  fi

  AM_CONDITIONAL(WINDOWS_COMPILE, true)
  AM_CONDITIONAL(UNIX_COMPILE, false)
  AM_CONDITIONAL(MACOS_COMPILE, false)
  AM_CONDITIONAL(MAKE_INSTALL, false)
  AM_CONDITIONAL(MAKE_BINDIST, true)
elif test x"$is_beos" = "xyes"; then
  AM_CONDITIONAL(UNIX_COMPILE, false)
  AM_CONDITIONAL(WINDOWS_COMPILE, false)
  AM_CONDITIONAL(MACOS_COMPILE, false)
  AM_CONDITIONAL(MAKE_INSTALL, false)
  AM_CONDITIONAL(MAKE_BINDIST, true)
else
  AC_MSG_WARN([No architecture defined!])
fi

dnl Check wether we have gcc
if test x"$GCC" = "xyes" ; then
  AC_DEFINE(USE_GCC,, [Define when using gcc] )
fi


AC_SUBST(ARCH_INCLUDES)
AC_SUBST(ARCH_LIBS)
AC_SUBST(ARCH_DIR)
AC_SUBST(ARCH_SRC_DIR)
AC_SUBST(ARCH_MAKE_BINDIST_DIR)
AC_SUBST(ARCH_EXTRA_SUBDIR)
AC_SUBST(UI_LIBS)
AC_SUBST(SDL_EXTRA_LIBS)
AC_SUBST(SOUND_MIDAS_OBJ)
AC_SUBST(SOUND_SDL_OBJ)
AC_SUBST(WINDRES_LIB)

PLATFORM_DOX_FLAGS=""
if test x"$enable_platformdox" = "xyes"; then
  if test x"$enable_sdl1ui" = "xyes" -o x"$enable_sdl2ui" = "xyes"; then
    PLATFORM_DOX_FLAGS="platformsdl"
  fi
  if test x"$enable_gtk3ui" = "xyes"; then
    PLATFORM_DOX_FLAGS="platformgtk3"
  fi
fi
if test x"$PLATFORM_DOX_FLAGS" = "x"; then
  PLATFORM_DOX_FLAGS="platformall"
fi
AC_SUBST(PLATFORM_DOX_FLAGS)

if test "$prefix" = NONE && test "$exec_prefix" = NONE; then
  PREFIX="$ac_default_prefix"
elif test "$prefix" = NONE; then
  PREFIX="$exec_prefix"
else
  PREFIX="$prefix"
fi

dnl Set up directories for the various OS'es
AC_DEFINE_UNQUOTED(PREFIX, "$PREFIX", [Where do we want to install the executable?])

dnl Exceptions for Windows and perhaps MacOS need to be made in the code since
dnl those are 'portable' (meaning any data is relative to the binary)

dnl VICEDIR="$datadir/$PACKAGE"
dnl VICE_ROOTDIR="$PREFIX"
dnl VICE_BINDIR="$bindir"
dnl VICE_DATADIR="$datadir/$PACKAGE"
dnl VICE_DOCDIR="$docdir"

dnl Set directories used by archdep
dnl
dnl Some of these are probably not required, we can either set some dirs to be
dnl relative, or use code to determine the location of files with bindists (ie
dnl Windows and MacOS)
dnl
dnl That will obviously require some target checks, the current stuff works
dnl targets that use `make install`, so at least Linux and BSD and perhaps
dnl MacOS when doing the 'Unix thing'.
dnl
dnl -- compyx, 2020-02-08
dnl
dnl AC_SUBST(VICEDIR, [$datadir/$PACKAGE])
dnl AC_SUBST(VICE_ROOTDIR, [$PREFIX])
dnl AC_SUBST(VICE_BINDIR, [$bindir])
AC_SUBST(VICE_DATADIR, [$datadir/$PACKAGE])
AC_SUBST(VICE_DOCDIR, [$docdir])


dnl Export --program-prefix and --program-suffix for the .desktop files
if test x"$program_prefix" = "xNONE"; then
    AC_SUBST(PROGRAM_PREFIX, "")
    #AC_MSG_ERROR([No prefix!])
else
    AC_SUBST(PROGRAM_PREFIX, [$program_prefix])
fi

if test x"$program_suffix" = "xNONE"; then
    AC_SUBST(PROGRAM_SUFFIX, "")
else
    AC_SUBST(PROGRAM_SUFFIX, [$program_suffix])
fi

AC_OUTPUT([Makefile
           build/Makefile
           build/beos/Makefile
           build/macOS/Makefile
           data/Makefile
           data/C128/Makefile
           data/C64/Makefile
           data/C64DTV/Makefile
           data/CBM-II/Makefile
           data/DRIVES/Makefile
           data/PET/Makefile
           data/PLUS4/Makefile
           data/SCPU64/Makefile
           data/PRINTER/Makefile
           data/VIC20/Makefile
           data/common/Makefile
           data/GLSL/Makefile
           data/hotkeys/Makefile
           doc/Makefile
           doc/building/Makefile
           doc/html/Makefile
           doc/readmes/Makefile
           doc/vim/Makefile
           doc/vim/ftdetect/Makefile
           doc/vim/syntax/Makefile
           src/Makefile
           src/debug.h
           src/arch/Makefile
           src/arch/gtk3/Makefile
           src/arch/gtk3/data/Makefile
           src/arch/gtk3/data/macos/Makefile
           src/arch/gtk3/data/win32/Makefile
           src/arch/gtk3/joystickdrv/Makefile
           src/arch/gtk3/novte/Makefile
           src/arch/gtk3/widgets/Makefile
           src/arch/gtk3/widgets/base/Makefile
           src/arch/sdl/Makefile
           src/arch/headless/Makefile
           src/arch/shared/Makefile
           src/arch/shared/hotkeys/Makefile
           src/arch/shared/hwsiddrv/Makefile
           src/arch/shared/mididrv/Makefile
           src/arch/shared/socketdrv/Makefile
           src/arch/shared/sounddrv/Makefile
           src/buildtools/Makefile
           src/c128/Makefile
           src/c128/cart/Makefile
           src/c64/Makefile
           src/c64/cart/Makefile
           src/c64dtv/Makefile
           src/scpu64/Makefile
           src/cbm2/Makefile
           src/cbm2/cart/Makefile
           src/core/Makefile
           src/core/rtc/Makefile
           src/crtc/Makefile
           src/datasette/Makefile
           src/diag/Makefile
           src/diskimage/Makefile
           src/drive/Makefile
           src/drive/iec/Makefile
           src/drive/iec/c64exp/Makefile
           src/drive/iec/plus4exp/Makefile
           src/drive/iec128dcr/Makefile
           src/drive/iecieee/Makefile
           src/drive/ieee/Makefile
           src/drive/tcbm/Makefile
           src/fileio/Makefile
           src/fsdevice/Makefile
           src/gfxoutputdrv/Makefile
           src/hvsc/Makefile
           src/iecbus/Makefile
           src/imagecontents/Makefile
           src/joyport/Makefile
           src/lib/Makefile
           src/lib/p64/Makefile
           src/lib/linenoise-ng/Makefile
           src/lib/libusbsiddrv/Makefile
           src/lib/libzmbv/Makefile
           src/lib/md5/Makefile
           src/monitor/Makefile
           src/parallel/Makefile
           src/pet/Makefile
           src/plus4/Makefile
           src/plus4/cart/Makefile
           src/printerdrv/Makefile
           src/raster/Makefile
           src/rs232drv/Makefile
           src/samplerdrv/Makefile
           src/serial/Makefile
           src/sid/Makefile
           src/tape/Makefile
           src/tapeport/Makefile
           src/tools/Makefile
           src/tools/cartconv/Makefile
           src/tools/petcat/Makefile
           src/userport/Makefile
           src/vdc/Makefile
           src/vdrive/Makefile
           src/vic20/Makefile
           src/vic20/cart/Makefile
           src/vicii/Makefile
           src/viciisc/Makefile
           src/video/Makefile
           src/version.h
           src/vice-version.sh
           ])

if test x"$enable_cmake" = x"yes" ; then
  "$srcdir/cmake-bootstrap.sh"
fi

dnl Local Variables:
dnl mode: autoconf
dnl compile-command: "autoconf"
dnl End:

dnl construct show_arch
if test "$is_beos" != "xyes" -a x"$is_unix_macosx" != "xyes" -a x"$is_win32" != "xyes" -a x"$is_win32_gtk3" != "xyes"; then
    show_arch="Unix"
    real_arch="Unix"
else
    if test x"$is_beos" = "xyes"; then
        show_arch="BeOS"
        real_arch="BeOS"
    fi
    if test x"$is_unix_macosx" = "xyes"; then
        show_arch="macOS $enable_macos_minimum_version"
        real_arch="OSX"
    fi
    if test x"$is_win32" = "xyes" -o x"$is_win32_gtk3" = "xyes"; then
        if test x"$enable_gtk3ui" = "xyes"; then
            show_arch="Win32-GTK3 (treated as Unix)"
            real_arch="Unix"
        fi
    fi
fi



dnl construct show_gui
show_gui="Unknown"
if test x"$enable_sdl1ui" = "xyes"; then
    show_gui="SDL 1.2"
fi

if test x"$enable_sdl2ui" = "xyes"; then
    show_gui="SDL 2.x"
fi

if test x"$enable_gtk3ui" = "xyes"; then
    if test x"$enable_debug_gtk3ui" = "xyes"; then
        show_gui="GTK3 (debug)"
    else
        show_gui="GTK3"
    fi
fi

if test x"$enable_headlessui" = "xyes"; then
    show_gui="Headless"
fi

if test "x$ac_cv_header_sys_audio_h" = xyes; then
    HAVE_SYS_AUDIO_H_SUPPORT="yes"
fi

if test "x$ac_cv_header_sys_audioio_h" = xyes; then
    HAVE_SYS_AUDIOIO_H_SUPPORT="yes"
fi

if test "x$ac_cv_func_SDL_NumJoysticks" = xyes; then
    HAVE_SDL_NUMJOYSTICKS_SUPPORT="yes"
fi


echo "----------------------------------------------------------------------"
echo ""
echo "configure summary:"
echo ""

echo "Platform canonical : $ac_cv_host"
echo "Architecture       : $show_arch"
echo "GUI                : $show_gui"
echo "Multithreaded UI   : $show_multithreaded"
echo "OpenMP             : $have_openmp"
if test x"$program_prefix" = "xNONE"; then
    echo "Program prefix     : (none) (--program-prefix)"
else
    echo "Program prefix     : $program_prefix (--program-prefix)"
fi
if test x"$program_suffix" = "xNONE"; then
    echo "Program suffix     : (none) (--program-suffix)"
else
    echo "Program suffix     : $program_suffix (--program-suffix)"
fi

echo ""
echo "SCREEN/UI"
echo "---------"
if test x"$real_arch" != "xBeOS"; then
    echo "Hardware scaling support       : $HAVE_HWSCALE_SUPPORT"
fi

echo ""
echo "SOUND"
echo "-----"
echo "FastSID support              : $HAVE_FASTSID_SUPPORT (--with/without-fastsid)"
echo "ReSID support                : $HAVE_RESID_SUPPORT (--with/without-resid)"
echo "New 8580 filter support      : $USE_NEW_8580_FILTER (--enable/disable-new8580filter)"
echo "PortAudio sound input support: $USE_PORTAUDIO_SUPPORT (--with/without-portaudio)"

if test x"$real_arch" = "xUnix"; then
    echo "OSS sound support            : $USE_OSS_SUPPORT (--with/without-oss)"
    echo "ALSA sound support           : $USE_ALSA_SUPPORT (--with/without-alsa)"
    echo "Pulseaudio sound support     : $USE_PULSE_SUPPORT (--with/without-pulse)"
fi

if test x"$real_arch" = "xOSX"; then
    echo "CoreAudio sound support      : $USE_COREAUDIO_SUPPORT"
    echo "AudioUnit sound support      : $HAVE_AUDIO_UNIT_SUPPORT"
fi

if test x"$real_arch" = "xWin32" -o x"$is_win32_gtk3" = "xyes"; then
    echo "DirectX sound support        : $USE_DXSOUND_SUPPORT"
fi

if test x"$real_arch" != "xDOS"; then
    echo "SDL sound support            : $USE_SDL_AUDIO_SUPPORT (--with/without-sdlsound)"
fi

if test x"$real_arch" = "xUnix"; then
    echo "NetBSD/Solaris sound support : $HAVE_SYS_AUDIOIO_H_SUPPORT"
fi

if test x"$real_arch" = "xUnix" -o x"$real_arch" = "xOSX" -o x"$real_arch" = "xWin32"; then
    echo "MIDI support                 : $HAVE_MIDI_SUPPORT (--enable/disable-midi)"
    echo "USBSID-Pico support          : $HAVE_USBSID_SUPPORT"
fi

if test x"$real_arch" != "xOSX" -a x"$real_arch" != "xBeOS"; then
    echo "Catweasel MK3 support        : $HAVE_CATWEASELMKIII_SUPPORT (--enable/disable-catweasel)"
    echo "HardSID support              : $HAVE_HARDSID_SUPPORT (--enable/disable-hardsid)"
fi

if test x"$real_arch" = "xUnix" -o x"$real_arch" = "xDOS" -o x"$real_arch" = "xWin32"; then
    echo "ParSID support               : $HAVE_PARSID_SUPPORT (--enable/disable-parsid)"
    echo "direct I/O access support    : $HAVE_IOSID_SUPPORT"
    echo "lpt port access              : $HAVE_PORTSID_SUPPORT"
fi

echo "PCI utils support            : $HAVE_PCIUTILS_SUPPORT"

echo "MP3 encoding support         : $USE_LAMEMP3_SUPPORT (--with/without-lame)"
echo "MP3 decoding support         : $USE_MPG123_SUPPORT (--with/without-mpg123)"
echo "FLAC en/de-coding support    : $USE_FLAC_SUPPORT (--with/without-flac)"
echo "Vorbis en/de-coding support  : $USE_VORBIS_SUPPORT (--with/without-vorbis)"

echo ""
echo "SCREENSHOTS"
echo "-----------"

echo "GIF encoding support : $HAVE_GIF_SUPPORT (--with/without-gif)"
echo "PNG encoding support : $HAVE_PNG_SUPPORT (--with/without-png)"

echo ""
echo "INPUT"
echo "-----"

echo "Mouse support               : $HAVE_MOUSE_SUPPORT"
echo "Lightpen support            : $HAVE_LIGHTPEN_SUPPORT"


if test x"$real_arch" = "xUnix"; then
    if test x"$show_gui" != "xSDL" -a x"$show_gui" != "xSDL2"; then
        echo "Linux style joystick support: $LINUX_JOYSTICK_SUPPORT"
        echo "USB joystick support        : $HAS_USB_JOYSTICK_SUPPORT"
    fi
fi

if test x"$real_arch" = "xOSX"; then
    if test x"$show_gui" != "xSDL" -a x"$show_gui" != "xSDL2"; then
        echo "Mac joystick support         : $MAC_JOYSTICK_SUPPORT"
        echo "IOHIDManager joystick support: $HAS_HIDMGR_SUPPORT (--enable/disable-hidmgr)"
    fi
fi

if test x"$real_arch" = "xWin32" -o x"$is_win32_gtk3" = "xyes"; then
    if test x"$show_gui" != "xSDL" -a x"$show_gui" != "xSDL2"; then
        echo "DirectInput joystick support: $HAVE_DINPUT_SUPPORT"
    fi
fi

if test x"$show_gui" = "xSDL" -o x"$show_gui" = "xSDL2"; then
    echo "SDL joystick support        : $HAVE_SDL_NUMJOYSTICKS_SUPPORT"
fi

echo ""
echo "MODEM/NETWORK"
echo "-------------"

if test x"$real_arch" = "xUnix" -o x"$real_arch" = "xWin32"; then
    echo "RS232 device support             : $HAVE_RS232DEV_SUPPORT (--enable/disable-rs232)"
fi

echo "Network support                  : $HAVE_NETWORK_SUPPORT"
echo "RS232 network support            : $HAVE_RS232NET_SUPPORT"
echo "IPv6 network support             : $HAVE_IPV6_SUPPORT (--enable/disable-ipv6)"
echo "Network capture/injection support: $HAVE_RAWNET_SUPPORT (--enable/disable-ethernet)"
echo "libcurl support                  : $HAVE_LIBCURL_SUPPORT (--with/without-libcurl)"

echo ""
echo "DRIVE"
echo "-----"

if test x"$real_arch" = "xUnix" -o x"$real_arch" = "xWin32"; then
  echo "Real device (OpenCBM support)    : $HAVE_REALDEVICE_SUPPORT (--enable/disable-realdevice)"
fi
echo "X64 image support                : $HAVE_X64_IMAGE_SUPPORT (--enable/disable--x64-image)"

dnl echo ""
dnl echo "MONITOR"
dnl echo "-------"
dnl echo "Native Gtk3 monitor support: $HAVE_GTK3_MONITOR_SUPPORT"

echo ""
echo "LIBS"
echo "----"

if test x"$real_arch" = "xUnix" -o x"$real_arch" = "xOSX" -o x"$real_arch" = "xWin32"; then
    echo "Dynamic linking support           : $HAVE_DYNLIB_SUPPORT_TOO"
fi

if test x"$real_arch" = "xUnix"; then
    echo "POSIX 1003.1e capabilities support: $HAVE_CAPABILITIES_SUPPORT"
fi

if test x"$real_arch" = "xUnix" -o x"$real_arch" = "xWin32"; then
    echo "libieee1284 support               : $HAVE_LIBIEEE1284_SUPPORT (--with/--wihout-libieee1284)"
fi


echo ""
echo "DOCUMENTATION"
echo "-------------"
if test x"$MAKEINFO" = "x:"; then
    echo "Generate Info/text: no (install texinfo)"
else
    echo "Generate Info/text: yes"
fi
if test x"$enable_pdf_docs" = xyes; then
    echo "Generate PDF      : yes"
else
    echo "Generate PDF      : no"
fi
if test x"${enable_html_docs:-yes}" != xno; then
    echo "Generate HTML     : yes"
else
    echo "Generate HTML     : no"
fi

echo ""
echo "MISC"
echo "----"

echo "65xx CPU history support      : $FEATURE_CPUMEMHISTORY_SUPPORT (--enable/disable-cpuhistory)"
echo "Debug support                 : $DEBUG_SUPPORT (--enable/disable-debug)"
echo "Threading debug support       : $DEBUG_THREADS_SUPPORT (--enable/disable-debug-threads"
echo "Build old x64 emulator        : $X64_INCLUDED (--enable/--disable-x64)"
echo "Install XDG .desktop files    : $USE_DESKTOP_FILES"
echo "icotool for Windows found     : $ICOTOOL"
echo "Experimental devices emulation: $HAVE_EXPERIMENTAL_DEVICES_SUPPORT (--enable-experimental-devices)"

echo ""
echo "User CPPFLAGS:  $CPPFLAGS"
echo "VICE_CPPFLAGS:  $VICE_CPPFLAGS"
echo "User CFLAGS:    $CFLAGS"
echo "VICE CFLAGS:    $VICE_CFLAGS"
echo "User CXXFLAGS:  $CXXFLAGS"
echo "VICE CXXFLAGS:  $VICE_CXXFLAGS"
if test x"$is_unix_macosx" = "xyes"; then
  echo "User OBJCFLAGS: $OBJCFLAGS"
  echo "VICE OBJCFLAGS: $VICE_OBJCFLAGS"
fi
echo "VTE_CXXFLAGS:   $VTE_CXXFLAGS"
echo "MONITOR_CFLAGS: $MONITOR_CFLAGS"
echo "User LDFLAGS:   $LDFLAGS"
echo "VICE LDFLAGS:   $VICE_LDFLAGS"
echo "----------------------------------------------------------------------"

VICE_ARG_LIST_CHECK($cmdline_options, $enable_option_checking)
