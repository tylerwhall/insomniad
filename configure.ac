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
AC_CONFIG_FILES([
        Makefile
])
AC_OUTPUT
