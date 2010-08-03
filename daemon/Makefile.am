
sbin_PROGRAMS = cluvirtd
noinst_HEADERS = \
	cluster.h \
	domain.h \
	group.h

cluvirtd_SOURCES = \
	cluvirtd.c \
	cluster.c \
	group.c \
	domain.c
cluvirtd_CPPFLAGS = \
	-I$(top_srcdir)/utils \
	-I$(top_srcdir)/lib \
        $(xml_CFLAGS) \
	$(virt_CFLAGS)
cluvirtd_LDADD = \
	$(top_srcdir)/utils/libutils.a \
	$(top_srcdir)/lib/libcluvirt.la \
	$(xml_LIBS) \
	$(virt_LIBS)

