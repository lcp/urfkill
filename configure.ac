dnl Process this file with autoconf to produce a configure script.

m4_define([urf_major_version], [0])
m4_define([urf_minor_version], [0])
m4_define([urf_micro_version], [1])
m4_define([urf_version],
          [urf_major_version.urf_minor_version.urf_micro_version])

AC_PREREQ(2.63)
AC_INIT([urfkill],[urf_version],[glin@novell.com],[urfkill])
AC_CONFIG_SRCDIR(src)
AM_INIT_AUTOMAKE([1.9 tar-ustar dist-bzip2])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# enable nice build output on automake1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# use this in urf-version.h
URF_MAJOR_VERSION=urf_major_version
URF_MINOR_VERSION=urf_minor_version
URF_MICRO_VERSION=urf_micro_version
AC_SUBST(VERSION)
AC_SUBST(URF_MAJOR_VERSION)
AC_SUBST(URF_MINOR_VERSION)
AC_SUBST(URF_MICRO_VERSION)

# libtool versioning - this applies to liburfkill-glib
#
# See http://sources.redhat.com/autobook/autobook/autobook_91.html#SEC91 for details
#
# increment;
# CURRENT	If the API or ABI interface has changed (reset REVISION to 0)
# REVISION	If the API and ABI remains the same, but bugs are fixed.
# AGE		Don't use.
LT_CURRENT=0
LT_REVISION=0
LT_AGE=0
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC

AC_HEADER_STDC
LT_INIT
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_SYS_LARGEFILE
AM_PROG_CC_C_O

# Taken from dbus
AC_ARG_ENABLE(ansi,             [  --enable-ansi           enable -ansi -pedantic gcc flags],enable_ansi=$enableval,enable_ansi=no)
AC_ARG_ENABLE(verbose-mode,     [  --enable-verbose-mode   support verbose debug mode],enable_verbose_mode=$enableval,enable_verbose_mode=$USE_MAINTAINER_MODE)
AC_ARG_ENABLE(man-pages,        [  --enable-man-pages      build manual pages],enable_man_pages=$enableval,enable_man_pages=yes)

if test "${enable_man_page}" != no; then
dnl
dnl Check for xsltproc
dnl
AC_PATH_PROG([XSLTPROC], [xsltproc])
  if test -z "$XSLTPROC"; then
    enable_man_pages=no
  fi
fi
AM_CONDITIONAL(MAN_PAGES_ENABLED, test x$enable_man_pages = xyes)

GTK_DOC_CHECK([1.9])

# udev rules go in /lib, not /usr/lib
if test "$prefix" = "/usr" -o "$prefix" = "/usr/local" ; then
  slashlibdir=/lib
else
  slashlibdir=$prefix/lib
fi
AC_SUBST(slashlibdir)

#### gcc warning flags

dnl ---------------------------------------------------------------------------
dnl - Extra verbose warning switches
dnl ---------------------------------------------------------------------------
CPPFLAGS="$CPPFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
CPPFLAGS="$CPPFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED"
CPPFLAGS="$CPPFLAGS -DGDK_MULTIHEAD_SAFE -DGTK_MULTIHEAD_SAFE"
CPPFLAGS="$CPPFLAGS -DGSEAL_ENABLE"

dnl ---------------------------------------------------------------------------
dnl - Use strict options (default enabled for devs, disabled in releases)
dnl ---------------------------------------------------------------------------
dnl if .git directory is present, considering we are working in the repository
if test -d ".git"; then
	default_strict=yes
else
	default_strict=no
fi

AC_ARG_ENABLE(strict, AS_HELP_STRING([--enable-strict],[Enable strict compilation options]), enable_strict=$enableval,
		enable_strict=$default_strict)
if test x$enable_strict != xno; then
	if test "$GCC" = "yes"; then
		WARNINGFLAGS_C="$WARNINGFLAGS_C -Werror"
	fi
fi

if test "$GCC" = "yes"; then
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wall"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wcast-align -Wno-uninitialized"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-declarations"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wredundant-decls"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wpointer-arith"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wcast-align"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wwrite-strings"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Winit-self"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wreturn-type"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wformat-nonliteral"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wformat-security"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-include-dirs"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-format-attribute"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wclobbered"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wempty-body"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wignored-qualifiers"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wsign-compare"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wtype-limits"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wuninitialized"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Waggregate-return"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wdeclaration-after-statement"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wshadow"
	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wno-strict-aliasing"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Winline"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Wmissing-parameter-type"
#	WARNINGFLAGS_C="$WARNINGFLAGS_C -Woverride-init"
else
	WARNINGFLAGS_C=""
fi
AC_SUBST(WARNINGFLAGS_C)

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.21.5])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.0])
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(DBUS_GLIB, [dbus-glib-1 >= 0.76])
AC_SUBST(DBUS_GLIB_CFLAGS)
AC_SUBST(DBUS_GLIB_LIBS)

PKG_CHECK_MODULES(POLKIT, \
		  polkit >= 0.9	\
		  polkit-dbus >= 0.9 \
		  polkit-grant >= 0.9)

PKG_CHECK_MODULES(GIO, [gio-2.0 >= 2.16.1])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

dnl ---------------------------------------------------------------------------
dnl - Build self tests
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(tests, AS_HELP_STRING([--enable-tests],[enable unit test code]),
	      enable_tests=$enableval,enable_tests=yes)
AM_CONDITIONAL(URF_BUILD_TESTS, test x$enable_tests = xyes)

if test "x$GCC" = "xyes"; then
  LDFLAGS="-Wl,--as-needed $LDFLAGS"
fi

IT_PROG_INTLTOOL([0.40.0])
GETTEXT_PACKAGE=urfkill
AC_SUBST([GETTEXT_PACKAGE])
AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[gettext domain])

AC_CONFIG_FILES([
Makefile
data/Makefile
data/urfkill-glib.pc
src/Makefile
liburfkill-glib/Makefile
liburfkill-glib/urf-version.h
policy/Makefile
po/Makefile.in
])
AC_OUTPUT

