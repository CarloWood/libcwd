-include Makefile

PACKAGE=$(shell pwd | sed -e 's%.*/%%g')

maintainer-startup:
	libtoolize --copy --force
ifeq ($(PACKAGE), libcw)
	aclocal -I ../libcwd
	ln -sf ../libcwd/maintMakefile.in
else
	aclocal
endif
	autoheader
	autoconf
	@(automakepath=`which automake`; \
	 prefix=`grep '^$$prefix = ' $$automakepath | sed -e 's/[^"]*"//' -e 's/".*//'`; \
	 tmp_am_dir=`grep '^$$am_dir = ' $$automakepath | sed -e 's/[^"]*"//' -e 's/".*//'`; \
	 eval "am_dir=\"$$tmp_am_dir\""; \
	 for i in install-sh missing mkinstalldirs; do echo "cp $$am_dir/$$i ."; cp $$am_dir/$$i .; done)
	automake
	@for i in config.guess config.sub ltmain.sh install-sh missing mkinstalldirs; do \
	  if test ! -f $$i; then echo "Warning: missing \"$$i\" in `pwd`"; fi; \
	done
