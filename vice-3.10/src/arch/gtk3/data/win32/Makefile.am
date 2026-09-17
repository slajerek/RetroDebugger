if WINDOWS_COMPILE
if USE_GTK3UI

# Create Windows .ico files
#
# Currently the generation of the Windows .res files happens here, I tried to
# do this work in data/common/Makefile.am, so we can use the same files for
# SDL builds, but that didn't quite work out. Probably due to recursive make
# not handling dependencies outside its tree very well (/src vs /data).
#

all:
	(cd $(top_builddir)/data/common && $(MAKE) win32_res_files)

endif
endif
