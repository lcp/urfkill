NULL =

if MAN_PAGES_ENABLED

man_MANS = 				\
	urfkill.7			\
	urfkilld.8			\
	$(NULL)

%.7 %.8 : %.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

endif # MAN_PAGES_ENABLED

MAINTAINERCLEANFILES =			\
	urfkill.7			\
	urfkilld.8			\
	$(NULL)

EXTRA_DIST= 				\
	urfkill.xml			\
	urfkilld.xml			\
	$(NULLL)

clean-local:
	rm -f *~ *.[78]

-include $(top_srcdir)/git.mk
