# This makefile is not distributed, only the maintainer (you) will use it.
# You will need GNU make for the following line to work correctly.
-include Makefile

maintainer-startup:
	aclocal; autoheader; autoconf; automake -a
	./configure --enable-maintainer-mode --enable-debugging

maintainer-clean: maintainer-startup
	make maintainer-clean
