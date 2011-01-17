## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir)/src -I$(top_srcdir)/src		\
	-DPACKAGE_LIBEXEC_DIR=\""$(libexecdir)"\"		\
	-DPACKAGE_SYSCONF_DIR=\""$(sysconfdir)"\"		\
	-DPACKAGE_DATA_DIR=\""$(datadir)"\"			\
	-DPACKAGE_BIN_DIR=\""$(bindir)"\"			\
	-DPACKAGE_LOCALSTATE_DIR=\""$(localstatedir)"\"		\
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"			\
	-DPACKAGE_LIB_DIR=\""$(libdir)"\"			\
	-D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT			\
	-DG_UDEV_API_IS_SUBJECT_TO_CHANGE			\
	-DUP_COMPILATION					\
	-DUP_DISABLE_DEPRECATED					\
	-I$(top_srcdir)						\
	$(GIO_CFLAGS)						\
	$(DBUS_GLIB_CFLAGS)					\
	$(POLKIT_CFLAGS)					\
	$(GLIB_CFLAGS)


BUILT_SOURCES =							\
	urf-daemon-glue.h					\
	urf-marshal.h						\
	urf-marshal.c

urf-marshal.h: urf-marshal.list
	glib-genmarshal $< --prefix=urf_marshal --header > $@

urf-marshal.c: urf-marshal.list
	echo "#include \"urf-marshal.h\"" > $@ && glib-genmarshal $< --prefix=urf_marshal --body >> $@

urf-daemon-glue.h: $(top_srcdir)/data/org.freedesktop.URfkill.xml Makefile.am
	dbus-binding-tool --prefix=urf_daemon --mode=glib-server --output=urf-daemon-glue.h \
	$(top_srcdir)/data/org.freedesktop.URfkill.xml

libexec_PROGRAMS = urfkilld

urfkilld_SOURCES =						\
	urf-killswitch.h					\
	urf-killswitch.c					\
	urf-input.h						\
	urf-input.c						\
	urf-polkit.h						\
	urf-polkit.c						\
	urf-utils.h						\
	urf-utils.c						\
	urf-daemon.h						\
	urf-daemon.c						\
	urf-main.c						\
	$(BUILT_SOURCES)

urfkilld_CPPFLAGS =						\
	-I$(top_srcdir)/src					\
	-DG_LOG_DOMAIN=\"URfkill\"				\
	$(AM_CPPFLAGS)

urfkilld_LDADD =						\
	-lm							\
	$(LIBUDEV_LIBS)						\
	$(GIO_LIBS)						\
	$(POLKIT_LIBS)						\
	$(DBUS_GLIB_LIBS)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =							\
	up-marshal.list	

clean-local :
	rm -f *~

