
VALGRIND = \
	$(LIBTOOL) --mode=execute \
	valgrind -q --error-exitcode=127 --leak-check=full

check-valgrind: $(check_PROGRAMS)
	$(MAKE) TESTS_ENVIRONMENT="$(VALGRIND)" check

check_PROGRAMS = check_domain check_clnode check_handle
TESTS = $(check_PROGRAMS)

noinst_HEADERS = chkutils.h

AM_CPPFLAGS = \
	-I$(top_srcdir)/utils \
	-I$(top_srcdir)/lib

check_domain_SOURCES = check_domain.c
check_domain_LDADD = $(top_srcdir)/lib/libcluvirt.la

check_clnode_SOURCES = check_clnode.c
check_clnode_LDADD = $(top_srcdir)/lib/libcluvirt.la

check_handle_SOURCES = check_handle.c
check_handle_LDADD = $(top_srcdir)/lib/libcluvirt.la

