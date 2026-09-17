# Makefile.am to build liblinenoise-ng.a for VICE

AM_CPPFLAGS = @VICE_CPPFLAGS@
AM_CFLAGS = @VICE_CFLAGS@
AM_CXXFLAGS = @VICE_CXXFLAGS@ -Wno-unused-variable
AM_LDFLAGS = @VICE_LDFLAGS@

noinst_LIBRARIES = liblinenoiseng.a

liblinenoiseng_a_SOURCES = \
			   ConvertUTF.cpp \
			   linenoise.cpp \
			   wcwidth.cpp

EXTRA_DIST = \
	     ConvertUTF.h \
	     linenoise.h \
	     wcwidth.h

