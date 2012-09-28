## Process this file with automake to produce Makefile.in

NULL =

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
	-I$(top_srcdir)						\
	$(GIO_CFLAGS)						\
	$(POLKIT_CFLAGS)					\
	$(XML_CFLAGS)						\
	$(GLIB_CFLAGS)


libexec_PROGRAMS = urfkilld

urfkilld_SOURCES =						\
	urf-arbitrator.h					\
	urf-arbitrator.c					\
	urf-device.h						\
	urf-device.c						\
	urf-killswitch.h					\
	urf-killswitch.c					\
	urf-input.h						\
	urf-input.c						\
	urf-config.h						\
	urf-config.c						\
	urf-polkit.h						\
	urf-polkit.c						\
	urf-utils.h						\
	urf-utils.c						\
	urf-consolekit.h					\
	urf-consolekit.c					\
	urf-seat.h						\
	urf-seat.c						\
	urf-daemon.h						\
	urf-daemon.c						\
	urf-main.c						\
	$(NULL)

urfkilld_CPPFLAGS =						\
	-I$(top_srcdir)/src					\
	-DG_LOG_DOMAIN=\"URfkill\"				\
	$(WARNINGFLAGS_C)					\
	$(AM_CPPFLAGS)

urfkilld_LDADD =						\
	-lm							\
	$(LIBUDEV_LIBS)						\
	$(GIO_LIBS)						\
	$(POLKIT_LIBS)						\
	$(XML_LIBS)

CLEANFILES = $(BUILT_SOURCES)

clean-local :
	rm -f *~


-include $(top_srcdir)/git.mk
