
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2

bin_PROGRAMS		= cluvirtadm

cluvirtadm_SOURCES	= main.c member.c
cluvirtadm_CPPFLAGS	= -I$(top_srcdir)/libcluvirt
cluvirtadm_LDADD	= $(top_builddir)/libcluvirt/libcluvirt.a
cluvirtadm_LDFLAGS	= $(xml_LIBS) $(virt_LIBS)
