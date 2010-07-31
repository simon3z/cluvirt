
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2

bin_PROGRAMS		= cluvirtadm

cluvirtadm_SOURCES	= cluvirtadm.c member.c
cluvirtadm_CPPFLAGS	= $(AM_CPPFLAGS) -I$(top_srcdir)/utils -I$(top_srcdir)/lib
cluvirtadm_LDADD	= $(top_srcdir)/utils/libutils.a $(top_srcdir)/lib/libcluvirt.la

