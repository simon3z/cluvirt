
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Wshadow -Wconversion -O2

sbin_PROGRAMS		= cluvirtd

cluvirtd_SOURCES	= cluvirtd.c cluster.c group.c
cluvirtd_CPPFLAGS	= $(AM_CPPFLAGS) -I$(top_srcdir)/utils -I$(top_srcdir)/lib
cluvirtd_LDADD		= $(top_srcdir)/utils/libutils.a $(top_srcdir)/lib/libcluvirt.la

