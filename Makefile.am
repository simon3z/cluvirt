
EXTRA_DIST = cluvirt.spec

ACLOCAL_AMFLAGS	= -I m4

SUBDIRS = utils lib daemon client man tests

check-valgrind: all-recursive
	$(MAKE) -C tests $@

