
NULL =

dbusifdir = $(datadir)/dbus-1/interfaces
dbusif_DATA = \
	org.freedesktop.URfkill.xml		\
	org.freedesktop.URfkill.Device.xml	\
	org.freedesktop.URfkill.Killswitch.xml	\
	$(NULL)

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

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = urfkill-glib.pc

urfkillconfdir = $(sysconfdir)/urfkill
urfkillconf_DATA = urfkill.conf

DISTCLEANFILES =					\
	org.freedesktop.URfkill.conf			\
	org.freedesktop.URfkill.service			\
	$(NULL)

EXTRA_DIST =						\
	$(dbusif_DATA)					\
	$(service_in_files)				\
	$(dbusconf_in_files)				\
	urfkill-glib.pc.in				\
	urfkill.conf					\
	$(NULL)

clean-local :
	rm -f *~ $(service_DATA) $(dbusconf_DATA)

-include $(top_srcdir)/git.mk
