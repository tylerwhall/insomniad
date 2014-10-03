AC_PREREQ(2.60)
AC_INIT([insomniad],
        [1],
        [tylerwhall@gmail.com],
        [insomniad])
AC_CONFIG_SRCDIR([src/parse_wakeup_sources.c])
AM_INIT_AUTOMAKE([
        foreign
        1.11
        subdir-objects
        -Wall
        -Wno-portability
        silent-rules
        tar-pax
        no-dist-gzip
        dist-xz])
AM_SILENT_RULES([yes])
AC_PROG_CC
PKG_CHECK_MODULES([CHECK], [check], [HAVE_CHECK=true], [HAVE_CHECK=false])
AM_CONDITIONAL([HAVE_CHECK], [test x$HAVE_CHECK == xtrue])

AC_SEARCH_LIBS([clock_gettime], [rt])

dnl Check for systemd
AC_ARG_WITH([systemdsystemunitdir],
             AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
                             [], [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)]
)
if test "$with_systemdsystemunitdir" != ""; then
    AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
fi
AM_CONDITIONAL(HAVE_SYSTEMD, test "$with_systemdsystemunitdir" != "")
test "x$exec_prefix" = "xNONE" && exec_prefix=$ac_default_prefix
BINDIR=`eval echo $bindir`
AC_SUBST([BINDIR])

AC_CONFIG_FILES([
        Makefile
        src/insomniad.service
])
AC_OUTPUT
