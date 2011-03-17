noinst_PROGRAMS = test-urfkill-client

test_urfkill_client_SOURCES = test-urfkill-client.c
test_urfkill_client_CFLAGS = -I$(top_srcdir)/liburfkill-glib $(GLIB_CFLAGS) $(DBUS_GLIB_CFLAGS)
test_urfkill_client_LDADD = $(GLIB_LIBS) $(DBUS_GLIB_LIBS) ../liburfkill-glib/liburfkill-glib.la
