
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2 # -Wconversion

sbin_PROGRAMS		= cluvirtd

cluvirtd_SOURCES	= main.c status.c cluster.c
cluvirtd_CFLAGS		= $(xml_CFLAGS) $(virt_CFLAGS)
cluvirtd_LDFLAGS	= $(xml_LIBS) $(virt_LIBS)

