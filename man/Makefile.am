
dist_man8_MANS = \
	cluvirtd.8 \
	cluvirtadm.8

HELP2MAN = help2man
SUFFIXES = .x .8

cluvirtd.8:
	$(HELP2MAN) -s 8 -N -I cluvirtd.x -o $@ $(top_srcdir)/daemon/cluvirtd

cluvirtadm.8:
	$(HELP2MAN) -s 8 -N -I cluvirtadm.x -o $@ $(top_srcdir)/client/cluvirtadm

