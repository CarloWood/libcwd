-include Makefile

maintainer-startup:
	aclocal; autoheader; autoconf; automake
