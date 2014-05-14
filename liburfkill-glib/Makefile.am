NULL =

AM_CPPFLAGS = \
	$(GLIB_CFLAGS)						\
	$(GIO_CFLAGS)						\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/liburfkill-glib				\
	-DURF_COMPILATION					\
	-DG_LOG_DOMAIN=\"liburfkill-glib\"			\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"			\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

BUILT_SOURCES =							\
	$(NULL)

lib_LTLIBRARIES =						\
	liburfkill-glib.la

liburfkill_glib_includedir = $(includedir)/liburfkill-glib
liburfkill_glib_include_HEADERS =				\
	urfkill.h						\
	urf-version.h						\
	urf-enum.h						\
	urf-device.h						\
	urf-killswitch.h					\
	urf-client.h

liburfkill_glib_la_SOURCES =					\
	urf-device.c						\
	urf-killswitch.c					\
	urf-client.c						\
	$(BUILT_SOURCES)

liburfkill_glib_la_LIBADD =					\
	$(INTLLIBS)						\
	$(GLIB_LIBS)						\
	$(GIO_LIBS)

liburfkill_glib_la_LDFLAGS =					\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-export-dynamic						\
	-no-undefined						\
	-export-symbols-regex '^urf_.*'

liburfkill_glib_la_CFLAGS =					\
	$(WARNINGFLAGS_C)					\
	$(NULL)

EXTRA_DIST =							\
	urf-version.h.in					\
	$(NULL)

CLEANFILES = $(BUILT_SOURCES)

clean-local:
	rm -f *~

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_sources =						\
	$(liburfkill_glib_la_SOURCES)				\
	$(liburfkill_glib_include_HEADERS)

Urfkill-@GIR_VERSION@.gir: liburfkill-glib.la
Urfkill_@GIR_VERSION_AM@_gir_INCLUDES = GObject-2.0 Gio-2.0
Urfkill_@GIR_VERSION_AM@_gir_CFLAGS = $(AM_CPPFLAGS)
Urfkill_@GIR_VERSION_AM@_gir_LIBS = liburfkill-glib.la
Urfkill_@GIR_VERSION_AM@_gir_FILES = $(introspection_sources)
Urfkill_@GIR_VERSION_AM@_gir_SCANNERFLAGS = --identifier-prefix=Urf --warn-all
INTROSPECTION_GIRS += Urfkill-@GIR_VERSION@.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif#HAVE_INTROSPECTION

-include $(top_srcdir)/git.mk
