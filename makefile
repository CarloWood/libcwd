-include Makefile

maintainer-startup:
	for i in config.guess config.sub ltconfig ltmain.sh; do cp /usr/share/libtool/$$i .; done
	aclocal; autoheader; autoconf; automake -a -c
