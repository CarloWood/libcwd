include doxygen.config
INPUT_FILES=$(shell for i in $(INPUT); do if test -f $$i; then echo $$i; else for j in $(FILE_PATTERNS); do ls $$i/$$j; done; fi done)
EXAMPLE_FILES=$(shell find doxygen-examples -mindepth 1 -maxdepth 1 -type f -print)

.PHONY: html external styles tutorial

all: html tutorial

html: styles external
	rm -rf html
	doxygen doxygen.config
	mv html/preparation.html html/preparation.tmp
	cat html/preparation.tmp | \
	  sed -e 's/href="debug_8h\.html">debug\.h/href="#preparation_step2">debug.h/' \
	      -e 's/"sys\.h"/"<a class="code" href="#preparation_step2">sys.h<\/a>"/' \
	  > html/preparation.html
	rm html/preparation.tmp


external: external/INSTALL external/sys.h external/debug.h external/debug.cc

external/INSTALL: ../INSTALL
	cp ../INSTALL external/INSTALL

external/sys.h: ../example-project/sys.h
	cp ../example-project/sys.h external/sys.h

external/debug.h: ../example-project/debug.h
	cp ../example-project/debug.h external/debug.h

external/debug.cc: ../example-project/debug.cc
	cp ../example-project/debug.cc external/debug.cc

styles:
	$(MAKE) -C styles

tutorial:
	$(MAKE) -s -C tutorial

