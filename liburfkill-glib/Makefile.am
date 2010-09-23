INCLUDES = \
	$(GLIB_CFLAGS)						\
	$(DBUS_GLIB_CFLAGS)					\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/liburfkill-glib				\
	-DUP_COMPILATION					\
	-DG_LOG_DOMAIN=\"liburfkill-glib\"			\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"			\
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

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

clean-local:
	rm -f *~

