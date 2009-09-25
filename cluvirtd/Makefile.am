
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2 # -Wconversion

sbin_PROGRAMS		= cluvirtd

cluvirtd_SOURCES	= main.c
cluvirtd_CPPFLAGS	= -I$(top_srcdir)/libcluvirt
cluvirtd_LDADD		= $(top_builddir)/libcluvirt/libcluvirt.a
cluvirtd_LDFLAGS	= $(xml_LIBS) $(virt_LIBS)

