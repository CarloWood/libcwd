-include Makefile

maintainer-startup:
	(lt_dir=`which libtool | sed -e 's%/bin/%/share/%'`; \
	 for i in config.guess config.sub ltmain.sh; do cp $$lt_dir/$$i .; done)
	aclocal
	autoheader
	autoconf
	automake -a -c || \
	(automakepath=`which automake`; \
	 prefix=`grep '^$$prefix = ' $$automakepath | sed -e 's/[^"]*"//' -e 's/".*//'`; \
	 tmp_am_dir=`grep '^$$am_dir = ' $$automakepath | sed -e 's/[^"]*"//' -e 's/".*//'`; \
	 eval "am_dir=\"$$tmp_am_dir\""; \
	 for i in install-sh missing mkinstalldirs; do cp $$am_dir/$$i .; done; \
	 automake)
	@for i in config.guess config.sub ltmain.sh install-sh missing mkinstalldirs; do \
	  if test ! -e $$i; then echo "Warning: missing \"$$i\" in `pwd`"; fi; \
	done
