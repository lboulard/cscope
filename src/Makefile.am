## Process this file with automake to produce Makefile.in

bin_PROGRAMS = cscope

cscope_SOURCES = $(LEXER_SOURCE).l egrep.y \
		alloc.c basename.c command.c compath.c constants.h \
		crossref.c dir.c display.c edit.c exec.c find.c \
		global.h help.c history.c input.c invlib.c invlib.h \
		library.h logdir.c lookup.c main.c mouse.c mygetenv.c \
		mypopen.c version.h vp.h vpaccess.c vpfopen.c \
		vpinit.c vpopen.c

LEXLIB = @LEXLIB@
LEXER_SOURCE = @LEXER_SOURCE@

cscope_LDADD =  $(CURSES_LIBS) $(LEXLIB)

INCLUDES = $(CURSES_INCLUDEDIR)

## Hack to clean the generated files and not include them in a "make dist".
BUILT_SOURCES = $(LEXER_SOURCE).c egrep.c
CLEANFILES = $(LEXER_SOURCE).c egrep.c
dist-hook:
	rm -f $(distdir)/$(LEXER_SOURCE).c $(distdir)/egrep.c

EXTRA_DIST = emacs.e gmacs.ml
