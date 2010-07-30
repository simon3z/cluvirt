
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2 -Wconversion

sbin_PROGRAMS		= cluvirtd

cluvirtd_SOURCES	= cluvirtd.c cluster.c group.c
cluvirtd_CPPFLAGS	= -I$(top_srcdir)/utils -I$(top_srcdir)/lib $(xml_CFLAGS) $(virt_CFLAGS)
cluvirtd_LDFLAGS	= $(xml_LIBS) $(virt_LIBS)
cluvirtd_LDADD		= $(top_srcdir)/utils/libutils.a $(top_srcdir)/lib/libcluvirt.la

