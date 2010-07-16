
dist_man8_MANS		= cluvirtd.8 cluvirtadm.8

MAINTAINERCLEANFILES	= Makefile.in $(dist_man8_MANS)

EXTRA_DIST		= $(dist_man8_MANS)

HELP2MAN		= help2man
SUFFIXES		= .x .8

.x.8:
	$(HELP2MAN) -s 8 -I $< -o $@ $(top_srcdir)/src/$*

