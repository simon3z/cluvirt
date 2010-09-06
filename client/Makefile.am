
bin_PROGRAMS = cluvirtadm
noinst_HEADERS = \
	member.h \
	interactive.h

cluvirtadm_SOURCES = \
	cluvirtadm.c \
	member.c \
	interactive.c
cluvirtadm_CPPFLAGS = \
	-I$(top_srcdir)/utils \
	-I$(top_srcdir)/lib
cluvirtadm_LDADD = \
	-lreadline \
	../utils/libutils.a \
	../lib/libcluvirt.la

