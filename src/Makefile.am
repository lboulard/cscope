## Process this file with automake to produce Makefile.in

bin_PROGRAMS = cscope

cscope_SOURCES = scanner.l egrep.y \
		alloc.c basename.c command.c compath.c constants.h \
		crossref.c dir.c display.c edit.c exec.c find.c \
		global.h help.c history.c input.c invlib.c invlib.h \
		library.h logdir.c lookup.c main.c mouse.c mygetenv.c \
		mypopen.c version.h vp.h vpaccess.c vpfopen.c \
		vpinit.c vpopen.c

LEXLIB = @LEXLIB@

cscope_LDADD =  $(CURSES_LIBS) $(LEXLIB)

INCLUDES = $(CURSES_INCLUDEDIR)

## Hack to clean the generated files and not include them in a "make dist".
BUILT_SOURCES = scanner.c egrep.c
CLEANFILES = scanner.c egrep.c
dist-hook:
	rm -f $(distdir)/scanner.c $(distdir)/egrep.c

EXTRA_DIST = emacs.e gmacs.ml
