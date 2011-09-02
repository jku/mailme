AC_PREREQ(2.53)
AC_INIT(mailme, 0.3.0)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(config.h)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_CANONICAL_HOST
AS_ALL_LINGUAS
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=mailme
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],
                   ["$GETTEXT_PACKAGE"],
                   [Gettext domain name])
AM_GLIB_GNU_GETTEXT

AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_CC_C_O
AC_PROG_LN_S

CFLAGS="$CFLAGS -Wall"

PKG_CHECK_MODULES(MAILME,
                  gobject-2.0
                  glib-2.0
                  telepathy-glib >= 0.14
                  dbus-glib-1
                  gio-2.0
                  libnotify)

AC_OUTPUT([
        Makefile
        src/Makefile
        po/Makefile.in
])
