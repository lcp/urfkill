noinst_PROGRAMS = test-urfkill-client enumerate-devices device-write catch-signal inhibit-keycontrol monitor-killswitch killswitch-write

test_urfkill_client_SOURCES = test-urfkill-client.c
test_urfkill_client_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
test_urfkill_client_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

enumerate_devices_SOURCES = enumerate-devices.c
enumerate_devices_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
enumerate_devices_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

device_write_SOURCES = device-write.c
device_write_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
device_write_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

catch_signal_SOURCES = catch-signal.c
catch_signal_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
catch_signal_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

inhibit_keycontrol_SOURCES = inhibit-keycontrol.c
inhibit_keycontrol_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
inhibit_keycontrol_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

monitor_killswitch_SOURCES = monitor-killswitch.c
monitor_killswitch_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
monitor_killswitch_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

killswitch_write_SOURCES = killswitch-write.c
killswitch_write_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
killswitch_write_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la

-include $(top_srcdir)/git.mk
