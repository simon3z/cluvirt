
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2

sbin_PROGRAMS		= cluvirtd

cluvirtd_SOURCES	= cluster.c main.c status.c group.c socket.c
cluvirtd_CFLAGS		= $(xml_CFLAGS) $(virt_CFLAGS)
cluvirtd_LDFLAGS	= $(xml_LIBS) $(virt_LIBS)

