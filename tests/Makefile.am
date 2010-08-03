
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Wconversion -Wshadow -O2

TESTS_VALGRIND		= $(LIBTOOL) --mode=execute valgrind -q --error-exitcode=127 --leak-check=full

check-valgrind: $(check_PROGRAMS)
	$(MAKE) TESTS_ENVIRONMENT="$(TESTS_VALGRIND)" check

check_PROGRAMS		= check_domain 
TESTS			= $(check_PROGRAMS)

check_domain_SOURCES	= check_domain.c
check_domain_CPPFLAGS	= $(AM_CPPFLAGS) -I$(top_srcdir)/utils -I$(top_srcdir)/lib
check_domain_LDADD	= $(top_srcdir)/lib/libcluvirt.la

