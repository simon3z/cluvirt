
sbin_PROGRAMS = cluvirtd
noinst_HEADERS = \
	domain.h \
	group.h

cluvirtd_SOURCES = \
	cluvirtd.c \
	domain.c \
	group.c
cluvirtd_CPPFLAGS = \
	-I$(top_srcdir)/utils \
	-I$(top_srcdir)/lib \
	$(xml_CFLAGS) \
	$(virt_CFLAGS)
cluvirtd_LDADD = \
	../utils/libutils.a \
	../lib/libcluvirt.la \
	$(xml_LIBS) \
	$(virt_LIBS)

