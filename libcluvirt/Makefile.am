
MAINTAINERCLEANFILES	= Makefile.in

AM_CPPFLAGS		= -Wall -Werror -Wshadow -O2 # -Wconversion

noinst_LIBRARIES	= libcluvirt.a

libcluvirt_a_SOURCES	= cluster.c status.c 
libcluvirt_a_CFLAGS	= $(xml_CFLAGS) $(virt_CFLAGS)

