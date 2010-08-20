
EXTRA_DIST = cluvirt.spec

ACLOCAL_AMFLAGS	= -I m4

SUBDIRS = utils lib daemon client man tests

.PHONY: check-valgrind rpmbuild

check-valgrind: all-recursive
	$(MAKE) -C tests $@

rpm_builddir = $(abs_builddir)/rpmbuild
RPMBUILD = rpmbuild --define '_topdir $(rpm_builddir)'

rpmbuild: dist-gzip
	$(MKDIR_P) $(rpm_builddir)
	$(RPMBUILD) -ta $(distdir).tar.gz

