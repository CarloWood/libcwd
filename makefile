-include Makefile

maintainer-startup:
	aclocal; autoheader; autoconf; automake
	./configure --enable-maintainer-mode --prefix=/usr
