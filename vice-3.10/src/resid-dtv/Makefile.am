## Process this file with automake to create Makefile.in

AR = @AR@

AM_CFLAGS = @VICE_CFLAGS@
AM_LDFLAGS = @VICE_LDFLAGS@

#AM_CXXFLAGS = @VICE_CXXFLAGS@

noinst_LIBRARIES = libresiddtv.a

libresiddtv_a_SOURCES = sid.cc voice.cc wave.cc envelope.cc filter.cc extfilt.cc version.cc

noinst_HEADERS = sid.h voice.h wave.h envelope.h filter.h extfilt.h bittrain.h residdtv-config.h

EXTRA_DIST = $(noinst_HEADERS)
