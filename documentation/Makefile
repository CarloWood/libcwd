include doxygen.config
INPUT_FILES=$(shell for i in $(INPUT); do if test -f $$i; then echo $$i; else for j in $(FILE_PATTERNS); do ls $$i/$$j; done; fi done)
EXAMPLE_FILES=$(shell find doxygen-examples -mindepth 1 -maxdepth 1 -type f -print)

.PHONY: reference-manual external styles tutorial

all: reference-manual tutorial

reference-manual: styles external
	rm -rf reference-manual
	doxygen doxygen.config
	mv reference-manual/preparation.html reference-manual/preparation.tmp
	cat reference-manual/preparation.tmp | \
	  sed -e 's/href="debug_8h\.html">debug\.h/href="#preparation_step2">debug.h/' \
	      -e 's/"sys\.h"/"<a class="code" href="#preparation_step2">sys.h<\/a>"/' \
	  > reference-manual/preparation.html
	rm reference-manual/preparation.tmp
	for i in `grep -l '<h2>Modules</h2>' reference-manual/*.html`; do \
	  mv $$i $$i.tmp; \
	  cat $$i.tmp | \
	  sed -e 's%<h2>Modules</h2>%<h2>Related</h2>%' > $$i; \
	  rm $$i.tmp; \
	done

reference-manual-clean:
	rm -rf reference-manual

external: external/INSTALL external/sys.h external/debug.h external/debug.cc external/libcwdrc

external-clean:
	rm -f external/INSTALL external/sys.h external/debug.h external/debug.cc external/libcwdrc

external/INSTALL: ../INSTALL
	cp ../INSTALL external/INSTALL

external/sys.h: ../example-project/sys.h
	cp ../example-project/sys.h external/sys.h

external/debug.h: ../example-project/debug.h
	cp ../example-project/debug.h external/debug.h

external/debug.cc: ../example-project/debug.cc
	cp ../example-project/debug.cc external/debug.cc

external/libcwdrc: ../libcwdrc
	cp ../libcwdrc external/libcwdrc

styles:
	$(MAKE) -C styles

tutorial:
	$(MAKE) -s -C tutorial

maintainer-clean: external-clean reference-manual-clean
	$(MAKE) -C styles maintainer-clean
	cd tutorial; rm -f examples5/debug.* examples5/test* tutorial/core examples7/test* tut5.m4 tut7.m4 *.html

