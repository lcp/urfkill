profiledir = $(sysconfdir)/urfkill/profile

dist_profile_DATA = 10-asus-settings.xml 10-lenovo-settings.xml

check:
	for f in $(profile_DATA); do \
            echo -n "Validate XML in $$f : "; \
            xmllint --noout --dtdvalid $(top_srcdir)/profile/profile.dtd $(srcdir)/$$f 2> xmllint.error; \
            if test -s xmllint.error; \
              then \
                echo failed; \
                cat xmllint.error; \
                rm -f xmllint.error; \
                exit 1; \
            else \
                echo ok; \
                rm -f xmllint.error; \
            fi; \
        done;

clean-local :
	rm -f *~

-include $(top_srcdir)/git.mk
