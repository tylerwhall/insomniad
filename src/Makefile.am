bin_PROGRAMS = insomniad
insomniad_SOURCES = src/insomniad.c

if HAVE_CHECK
TESTS = check_parse_wakeup_sources

check_PROGRAMS = check_parse_wakeup_sources
check_parse_wakeup_sources_SOURCES = src/check_parse_wakeup_sources.c
check_parse_wakeup_sources_CFLAGS = @CHECK_CFLAGS@ $(AM_CFLAGS)
check_parse_wakeup_sources_LDADD = @CHECK_LIBS@
endif
