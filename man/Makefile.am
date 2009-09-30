
man_MANS		= cluvirtd.8 cluvirtadm.8

MAINTAINERCLEANFILES	= Makefile.in


HELP2MAN=help2man

cluvirtd.8:
	$(HELP2MAN) -s 8 \
	    -I cluvirtd.x -N -o $@ $(top_srcdir)/cluvirtd/cluvirtd

cluvirtadm.8:
	$(HELP2MAN) -s 8 \
	    -I cluvirtadm.x -N -o $@ $(top_srcdir)/cluvirtadm/cluvirtadm

