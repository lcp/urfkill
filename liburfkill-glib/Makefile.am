INCLUDES = \
	$(GLIB_CFLAGS)						\
	$(DBUS_GLIB_CFLAGS)					\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/liburfkill-glib				\
	-DUP_COMPILATION					\
	-DG_LOG_DOMAIN=\"liburfkill-glib\"			\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"			\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

lib_LTLIBRARIES =						\
	liburfkill-glib.la

liburfkill_glib_includedir = $(includedir)/liburfkill-glib
liburfkill_glib_include_HEADERS =				\
	urfkill.h						\
	urf-version.h						\
	urf-client.h

liburfkill_glib_la_SOURCES =					\
	urf-client.c

liburfkill_glib_la_LIBADD =					\
	$(INTLLIBS)						\
	$(GLIB_LIBS)						\
	$(DBUS_GLIB_LIBS)

liburfkill_glib_la_LDFLAGS =					\
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
	-export-dynamic						\
	-no-undefined						\
	-export-symbols-regex '^urf_.*'

liburfkill_glib_la_CFLAGS =					\
	$(WARNINGFLAGS_C)					\
	$(NULL)

EXTRA_DIST =							\
	urf-version.h.in

CLEANFILES = $(BUILT_SOURCES)

if HAVE_INTROSPECTION
introspection_sources = $(liburfkill_glib_la_SOURCES)

URfkillGlib-1.0.gir: liburfkill-glib.la
URfkillGlib_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
URfkillGlib_1_0_gir_CFLAGS = $(INCLUDES)
URfkillGlib_1_0_gir_LIBS = liburfkill-glib.la
URfkillGlib_1_0_gir_FILES = $(addprefix $(srcdir)/,$(introspection_sources))
INTROSPECTION_GIRS += URfkillGlib-1.0.gir

girdir = $(datadir)/gir-1.0
dist_gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(dist_gir_DATA) $(typelib_DATA)
endif

clean-local:
	rm -f *~

