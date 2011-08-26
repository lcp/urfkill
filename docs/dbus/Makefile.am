NULL=

all : org.freedesktop.URfkill.ref.xml org.freedesktop.URfkill.Device.ref.xml org.freedesktop.URfkill.Killswitch.ref.xml

org.freedesktop.URfkill.ref.xml : $(top_srcdir)/data/org.freedesktop.URfkill.xml $(top_srcdir)/docs/dbus/spec-to-docbook.xsl
	echo "<?xml version=\"1.0\"?>""<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\" \"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\">" > $@
	$(XSLTPROC) $(top_srcdir)/docs/dbus/spec-to-docbook.xsl $< | tail -n +2 >> $@

org.freedesktop.URfkill.Device.ref.xml : $(top_srcdir)/data/org.freedesktop.URfkill.Device.xml $(top_srcdir)/docs/dbus/spec-to-docbook.xsl
	echo "<?xml version=\"1.0\"?>""<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\" \"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\">" > $@
	$(XSLTPROC) $(top_srcdir)/docs/dbus/spec-to-docbook.xsl $< | tail -n +2 >> $@

org.freedesktop.URfkill.Killswitch.ref.xml : $(top_srcdir)/data/org.freedesktop.URfkill.Killswitch.xml $(top_srcdir)/docs/dbus/spec-to-docbook.xsl
	echo "<?xml version=\"1.0\"?>""<!DOCTYPE refentry PUBLIC \"-//OASIS//DTD DocBook XML V4.1.2//EN\" \"http://www.oasis-open.org/docbook/xml/4.1.2/docbookx.dtd\">" > $@
	$(XSLTPROC) $(top_srcdir)/docs/dbus/spec-to-docbook.xsl $< | tail -n +2 >> $@

MAINTAINERCLEANFILES =					\
	org.freedesktop.URfkill.Killswitch.ref.xml	\
	org.freedesktop.URfkill.Device.ref.xml		\
	org.freedesktop.URfkill.ref.xml			\
	$(NULL)

EXTRA_DIST = spec-to-docbook.xsl dbus-introspect-docs.dtd

clean-local :
	rm -f *~ *.ref.xml

-include $(top_srcdir)/git.mk
