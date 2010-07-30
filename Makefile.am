
dist_m4_MACROS		= aclocal.m4 m4/libtool.m4 m4/lt~obsolete.m4 \
			  m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4

MAINTAINERCLEANFILES	= $(dist_m4_MACROS) configure depcomp ltmain.sh \
			  config.guess config.sub missing install-sh \
			  compile config.h.in config.h.in~ \
			  Makefile.in

SUBDIRS                 = utils lib daemon client man

ACLOCAL_AMFLAGS		= -I m4

EXTRA_DIST		= $(dist_m4_MACROS) cluvirt.spec

