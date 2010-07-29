
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2 -Wconversion

sbin_PROGRAMS		= cluvirtd
bin_PROGRAMS		= cluvirtadm
lib_LTLIBRARIES		= libcluvirt.la
include_HEADERS		= cluvirt.h
noinst_HEADERS		= utils.h

cluvirtd_SOURCES	= cluvirtd.c
cluvirtd_CPPFLAGS	= $(xml_CFLAGS) $(virt_CFLAGS)
cluvirtd_LDFLAGS	= $(xml_LIBS) $(virt_LIBS)
cluvirtd_LDADD		= libcluvirt.la

cluvirtadm_SOURCES	= cluvirtadm.c
cluvirtadm_CPPFLAGS	= $(xml_CFLAGS) $(virt_CFLAGS)
cluvirtadm_LDFLAGS	= $(xml_LIBS) $(virt_LIBS)
cluvirtadm_LDADD	= libcluvirt.la

libcluvirt_la_SOURCES	= status.c cluster.c member.c group.c utils.c
libcluvirt_la_CPPFLAGS	= $(xml_CFLAGS) $(virt_CFLAGS)
libcluvirt_la_LDFLAGS	= -version-info 0:1:0 $(xml_LIBS) $(virt_LIBS)

