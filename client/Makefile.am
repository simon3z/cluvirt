
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2 -Wconversion

bin_PROGRAMS		= cluvirtadm

cluvirtadm_SOURCES	= cluvirtadm.c member.c
cluvirtadm_CPPFLAGS	= -I$(top_srcdir)/utils -I$(top_srcdir)/lib $(xml_CFLAGS) $(virt_CFLAGS)
cluvirtadm_LDFLAGS	= $(xml_LIBS) $(virt_LIBS)
cluvirtadm_LDADD	= $(top_srcdir)/utils/libutils.a $(top_srcdir)/lib/libcluvirt.la

