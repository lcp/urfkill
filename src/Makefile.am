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

urf-daemon-glue.h: org.freedesktop.URfkill.xml Makefile.am
	dbus-binding-tool --prefix=urf_daemon --mode=glib-server --output=urf-daemon-glue.h org.freedesktop.URfkill.xml

libexec_PROGRAMS = urfkilld

dbusifdir = $(datadir)/dbus-1/interfaces
dbusif_DATA =							\
	org.freedesktop.URfkill.xml

urfkilld_SOURCES =						\
	egg-debug.c						\
	egg-debug.h						\
	urf-killswitch.h					\
	urf-killswitch.c					\
	urf-polkit.h						\
	urf-polkit.c						\
	urf-daemon.h						\
	urf-daemon.c						\
	urf-main.c						\
	$(BUILT_SOURCES)

urfkilld_CPPFLAGS =						\
	-I$(top_srcdir)/src					\
	-DG_LOG_DOMAIN=\"urf-daemon\"				\
	$(AM_CPPFLAGS)

urfkilld_LDADD =						\
	-lm							\
	$(GIO_LIBS)						\
	$(POLKIT_LIBS)						\
	$(DBUS_GLIB_LIBS)

servicedir       = $(datadir)/dbus-1/system-services
service_in_files = org.freedesktop.URfkill.service.in
service_DATA     = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	@sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_in_files = org.freedesktop.URfkill.conf.in
dbusconf_DATA = $(dbusconf_in_files:.conf.in=.conf)

$(dbusconf_DATA): $(dbusconf_in_files) Makefile
	cp $< $@

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST =							\
	org.freedesktop.URfkill.xml				\
	up-marshal.list						\
	$(service_in_files)					\
	$(dbusconf_in_files)

clean-local :
	rm -f *~ $(service_DATA) $(dbusconf_DATA)

