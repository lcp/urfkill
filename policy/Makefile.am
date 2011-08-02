polkit_policydir = $(datadir)/polkit-1/actions
polkit_policy_DATA =					\
	org.freedesktop.urfkill.policy

# You will need a recent intltool or the patch from this bug
# http://bugzilla.gnome.org/show_bug.cgi?id=462312
@INTLTOOL_POLICY_RULE@

EXTRA_DIST =					\
	org.freedesktop.urfkill.policy.in

DISTCLEANFILES =				\
	org.freedesktop.urfkill.policy

clean-local :
	rm -f *~


-include $(top_srcdir)/git.mk
