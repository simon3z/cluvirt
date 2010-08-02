
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Wconversion -Wshadow -O2

lib_LTLIBRARIES		= libcluvirt.la
include_HEADERS		= cluvirt.h

libcluvirt_la_SOURCES	= libcluvirt.c status.c
libcluvirt_la_CPPFLAGS	= $(AM_CPPFLAGS) -I$(top_srcdir)/utils
libcluvirt_la_LDFLAGS	= -version-info 0:1:0

