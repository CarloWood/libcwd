TEMPLATES=main.css doxygen.css tutorial.css
BROWSERS=netscape4 netscape6 mozilla konqueror ie
OUTPUT=$(shell for b in $(BROWSERS); do for i in $(TEMPLATES); do echo $$b/$$i; done done)

all: $(OUTPUT)

%/main.css: main.css
	@MACRO=`echo $@ | sed -e 's%\([^/]*\)/.*%\1%' | tr a-z A-Z`; \
	cat main.css | gcc -D$$MACRO -E -c - | egrep -v '^(#|$$)' > $@

%/doxygen.css: doxygen.css
	@MACRO=`echo $@ | sed -e 's%\([^/]*\)/.*%\1%' | tr a-z A-Z`; \
	cat doxygen.css | gcc -D$$MACRO -E -c - | egrep -v '^(#|$$)' > $@

%/tutorial.css: tutorial.css
	@MACRO=`echo $@ | sed -e 's%\([^/]*\)/.*%\1%' | tr a-z A-Z`; \
	cat tutorial.css | gcc -D$$MACRO -E -c - | egrep -v '^(#|$$)' > $@

test.html: test.m4 ../definitions.m4
	m4 -I.. test.m4 > test.html

