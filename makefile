-include Makefile

maintainer-startup:
	if test -d /usr/share/libtool; then \
	  for i in config.guess config.sub ltconfig ltmain.sh; do cp /usr/share/libtool/$$i .; done; \
	fi
	aclocal; autoheader; autoconf; automake -a -c
	@for i in config.guess config.sub ltconfig ltmain.sh install-sh missing mkinstalldirs; do \
	  if test ! -e $$i; then echo "Warning: missing \"$$i\" in `pwd`"; fi; \
	done
