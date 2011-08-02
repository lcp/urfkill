noinst_PROGRAMS = test-urfkill-client enumerate-devices catch-signal inhibit-keycontrol

test_urfkill_client_SOURCES = test-urfkill-client.c
test_urfkill_client_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
test_urfkill_client_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

enumerate_devices_SOURCES = enumerate-devices.c
enumerate_devices_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
enumerate_devices_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

catch_signal_SOURCES = catch-signal.c
catch_signal_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
catch_signal_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

inhibit_keycontrol_SOURCES = inhibit-keycontrol.c
inhibit_keycontrol_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
inhibit_keycontrol_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

-include $(top_srcdir)/git.mk
