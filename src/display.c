/*===========================================================================
 Copyright (c) 1998-2000, The Santa Cruz Operation 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 *Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 *Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 *Neither name of The Santa Cruz Operation nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE. 
 =========================================================================*/

/*	cscope - interactive C symbol cross-reference
 *
 *	display functions
 */

#include "global.h"
#ifdef CCS
#include "sgs.h"	/* ESG_PKG and ESG_REL */
#else
#include "version.h"	/* FILEVERSION and FIXVERSION */
#endif
#include <curses.h>	/* LINES, COLS */
#include <setjmp.h>	/* jmp_buf */
#include <stdarg.h>	/* va_list stuff */
#include <time.h>
#include <errno.h>      /* sys_errlist 18-Apr-2000 hops */

static char const rcsid[] = "$Id$";

int	booklen;		/* OGS book name display field length */
int	*displine;		/* screen line of displayed reference */
int	disprefs;		/* displayed references */
int	field;			/* input field */
int	filelen;		/* file name display field length */
int	fcnlen;			/* function name display field length */
int	mdisprefs;		/* maximum displayed references */
int	nextline;		/* next line to be shown */
FILE	*nonglobalrefs;		/* non-global references file */
int	numlen;			/* line number display field length */
int	topline = 1;		/* top line of page */
int	bottomline;		/* bottom line of page */
long	searchcount;		/* count of files searched */
int	subsystemlen;		/* OGS subsystem name display field length */
int	totallines;		/* total reference lines */
unsigned fldcolumn;		/* input field column */

static	int	fldline;		/* input field line */
static	jmp_buf	env;			/* setjmp/longjmp buffer */
static	int	lastdispline;		/* last displayed reference line */
static	char	lastmsg[MSGLEN + 1];	/* last message displayed */
static	char	helpstring[] = "Press the ? key for help";
static	char	selprompt[] = 
	"Select lines to change (press the ? key for help): ";

#if BSD	/* compiler bug workaround */
#define	Void	char *
#else
#define	Void	void
#endif

extern	BOOL	findcalledby(void);
extern	char	*findstring(void);
extern	Void	findcalling(void);
extern	Void	findallfcns(void);
extern	Void	finddef(void);
extern	Void	findfile(void);
extern	Void	findinclude(void);
extern	Void	findsymbol(void);

typedef char *(*FP)();	/* pointer to function returning a character pointer */

static	struct	{		/* text of input fields */
	char	*text1;
	char	*text2;
	FP	findfcn;
} fields[FIELDS + 1] = {	/* samuel has a search that is not part of the cscope display */
	"Find this", "C symbol",			(FP) findsymbol,
	"Find this", "global definition",		(FP) finddef,
	"Find", "functions called by this function",	(FP) findcalledby,
	"Find", "functions calling this function",	(FP) findcalling,
	"Find this", "text string",			findstring,
	"Change this", "text string",			findstring,
	"Find this", "egrep pattern",			findregexp,
	"Find this", "file",				(FP) findfile,
	"Find", "files #including this file",		(FP) findinclude,
	"Find all", "function definitions",		(FP) findallfcns,	/* samuel only */
};

/* initialize display parameters */

void
dispinit(void)
{
	/* calculate the maximum displayed reference lines */
	lastdispline = FLDLINE - 2;
	mdisprefs = lastdispline - REFLINE + 1;
	if (mdisprefs <= 0) {
		(void) fprintf(stderr, "%s: screen too small\n", argv0);
		myexit(1);
	}

	if (mouse == NO && mdisprefs > 9 && select_large == NO) {
		mdisprefs = 9;
	}
	else if (mouse == NO && mdisprefs > 45 && select_large == YES) {
		/* Limit is 45 select lines */
		mdisprefs = 45;
	}

	/* allocate the displayed line array */
	displine = (int *) mymalloc(mdisprefs * sizeof(int));
}
/* display a page of the references */

void
display(void)
{
	char	*subsystem;		/* OGS subsystem name */
	char	*book;			/* OGS book name */
	char	file[PATHLEN + 1];	/* file name */
	char	function[PATLEN + 1];	/* function name */
	char	linenum[NUMLEN + 1];	/* line number */
	int	screenline;		/* screen line number */
	int	width;			/* source line display width */
	int	i;
	char	*s;

	/* see if this is the initial display */
	erase();
	if (refsfound == NULL) {
#if CCS
		if (displayversion == YES) {
			printw("cscope %s", ESG_REL);
		}
		else {
			printw("cscope");
		}
#else
		printw("Cscope version %d%s", FILEVERSION, FIXVERSION);
#endif
		move(0, COLS - (int) sizeof(helpstring));
		addstr(helpstring);
	}
	/* if no references were found */
	else if (totallines == 0) {
		/* redisplay the last message */
		addstr(lastmsg);
	}
	else {	/* display the pattern */
		if (changing == YES) {
			printw("Change \"%s\" to \"%s\"", pattern, newpat);
		}
		else {
			printw("%c%s: %s", toupper(fields[field].text2[0]),
				fields[field].text2 + 1, pattern);
		}
		/* display the column headings */
		move(2, 2);
		if (ogs == YES && field != FILENAME) {
			printw("%-*s ", subsystemlen, "Subsystem");
			printw("%-*s ", booklen, "Book");
		}
		if (dispcomponents > 0) {
			if (select_large == YES) {
				printw(" %-*s ", filelen, "File");
			}
			else {
				printw("%-*s ", filelen, "File");
			}
		}
		if (field == SYMBOL || field == CALLEDBY || field == CALLING) {
			printw("%-*s ", fcnlen, "Function");
		}
		if (field != FILENAME) {
			addstr("Line");
		}
		addch('\n');

		/* if at end of file go back to beginning */
		if (nextline > totallines) {
			seekline(1);
		}
		/* calculate the source text column */
		if (select_large == YES) {
			width = COLS - numlen - 4;
		}
		else {
			width = COLS - numlen - 3;
		}
		if (ogs == YES) {
			width -= subsystemlen + booklen + 2;
		}
		if (dispcomponents > 0) {
			width -= filelen + 1;
		}
		if (field == SYMBOL || field == CALLEDBY || field == CALLING) {
			width -= fcnlen + 1;
		}
		/* until the max references have been displayed or 
		   there is no more room */
		topline = nextline;
		for (disprefs = 0, screenline = REFLINE;
		    disprefs < mdisprefs && screenline <= lastdispline;
		    ++disprefs, ++screenline) {
			
			/* read the reference line */
			if (fscanf(refsfound, "%s%s%s %[^\n]", file, function, 
			    linenum, yytext) < 4) {
				break;
			}
			++nextline;
			displine[disprefs] = screenline;
			
			/* if no mouse, display the selection number */
			if (mouse == YES) {
				addch(' ');
			}
			else {
				/* print numbers and then letters for
				   selections */
				if (disprefs < 9) {
					if (select_large == YES) {
						printw(" %d", disprefs + 1);
					}
					else {
						printw("%d", disprefs + 1);
					}
				}
				else if (select_large == YES) {
					if (disprefs < 19) {
						printw("0%d", disprefs - 9);
					}
					else {
						printw("0%c",
							disprefs - 19 + 'A');
					}
				}
			}
			/* display any change mark */
			if (changing == YES && 
			    change[topline + disprefs - 1] == YES) {
				addch('>');
			}
			else {
				addch(' ');
			}
			/* display the file name */
			if (field == FILENAME) {
				printw("%-*s ", filelen, file);
			}
			else {
				/* if OGS, display the subsystem and book names */
				if (ogs == YES) {
					ogsnames(file, &subsystem, &book);
					printw("%-*.*s ", subsystemlen, subsystemlen, subsystem);
					printw("%-*.*s ", booklen, booklen, book);
				}
				/* display the requested path components */
				if (dispcomponents > 0) {
					printw("%-*.*s ", filelen, filelen,
						pathcomponents(file, dispcomponents));
				}
			}
			/* display the function name */
			if (field == SYMBOL || field == CALLEDBY || field == CALLING) {
				printw("%-*.*s ", fcnlen, fcnlen, function);
			}
			if (field == FILENAME) {
				addch('\n');	/* go to next line */
				continue;
			}
			/* display the line number */
			printw("%*s ", numlen, linenum);

			/* there may be tabs in egrep output */
			while ((s = strchr(yytext, '\t')) != NULL) {
				*s = ' ';
			}
			/* display the source line */
			s = yytext;

			for (;;) {
				/* see if the source line will fit */
				if ((i = strlen(s)) > width) {
					
					/* find the nearest blank */
					for (i = width; s[i] != ' ' && i > 0; --i) {
						;
					}
					if (i == 0) {
						i = width;	/* no blank */
					}
				}
				/* print up to this point */
				printw("%.*s", i, s);
				s += i;
				
				/* if line didn't wrap around */
				if (i < width) {
					addch('\n');	/* go to next line */
				}
				/* skip blanks */
				while (*s == ' ') {
					++s;
				}
				/* see if there is more text */
				if (*s == '\0') {
					break;
				}
				/* if the source line is too long */
				if (++screenline > lastdispline) {

					/* if this is the first displayed line,
					   display what will fit on the screen */
					if (topline == nextline -1) {
						goto endrefs;
					}
					
					/* erase the reference */
					while (--screenline >= displine[disprefs]) {
						move(screenline, 0);
						clrtoeol();
					}
					++screenline;
					 
					/* go back to the beginning of this reference */
					--nextline;
					seekline(nextline);
					goto endrefs;
				}
				/* indent the continued source line */
				move(screenline, COLS - width);
			}

		}
	endrefs:
		/* position the cursor for the message */
		i = FLDLINE - 1;
		if (screenline < i) {
			addch('\n');
		}
		else {
			move(i, 0);
		}
		/* check for more references */
		i = totallines - nextline + 1;
		bottomline = nextline;
		if (i > 0) {
			s = "s";
			if (i == 1) {
				s = "";
			}
			printw("* %d more line%s - press the space bar to display more *", i, s);
		}
		/* if this is the last page of references */
		else if (topline > 1 && nextline > totallines) {
			addstr("* Press the space bar to display the first lines again *");
		}
	}
	/* display the input fields */
	move(FLDLINE, 0);
	for (i = 0; i < FIELDS; ++i) {
		printw("%s %s:\n", fields[i].text1, fields[i].text2);
	}
	/* display any prompt */
	if (changing == YES) {
		move(PRLINE, 0);
		addstr(selprompt);
	}
	drawscrollbar(topline, nextline);	/* display the scrollbar */
}

/* set the cursor position for the field */
void
setfield(void)
{
	fldline = FLDLINE + field;
	fldcolumn = strlen(fields[field].text1) + strlen(fields[field].text2) + 3;
}

/* move to the current input field */

void
atfield(void)
{
	move(fldline, fldcolumn);
}

/* move to the changing lines prompt */

void
atchange(void)
{
	move(PRLINE, (int) sizeof(selprompt) - 1);
}

/* search for the symbol or text pattern */

/*ARGSUSED*/
SIGTYPE
jumpback(sig)
{
	longjmp(env, 1);
}

BOOL
search(void)
{
	char	*subsystem;		/* OGS subsystem name */
	char 	*book;			/* OGS book name */
	char	file[PATHLEN + 1];	/* file name */
	char	function[PATLEN + 1];	/* function name */
	char	linenum[NUMLEN + 1];	/* line number */
	char	*egreperror = NULL;	/* egrep error message */
	BOOL	funcexist = YES;		/* find "function" error */
	FINDINIT rc = NOERROR;		/* findinit return code */
	SIGTYPE	(*savesig)();		/* old value of signal */
	FP	f;			/* searching function */
	int	c, i;
	
	/* open the references found file for writing */
	if (writerefsfound() == NO) {
		return(NO);
	}
	/* find the pattern - stop on an interrupt */
	if (linemode == NO) {
		postmsg("Searching");
	}
	searchcount = 0;
	if (setjmp(env) == 0) {
		savesig = signal(SIGINT, jumpback);
		f = fields[field].findfcn;
		if (f == findregexp || f == findstring) {
			egreperror = (*f)(pattern);
		}
		else {
			if ((nonglobalrefs = myfopen(temp2, "w")) == NULL) {
				cannotopen(temp2);
				return(NO);
			}
			if ((rc = findinit()) == NOERROR) {
				(void) dbseek(0L); /* read the first block */
				if (f == (FP) findcalledby) funcexist = (BOOL)(*f)();
				else (*f)();
				findcleanup();

				/* append the non-global references */
				(void) freopen(temp2, "r", nonglobalrefs);
				while ((c = getc(nonglobalrefs)) != EOF) {
					(void) putc(c, refsfound);
				}
			}
			(void) fclose(nonglobalrefs);
		}
	}
	signal(SIGINT, savesig);

	/* rewind the cross-reference file */
	(void) lseek(symrefs, (long) 0, 0);
	
	/* reopen the references found file for reading */
	(void) freopen(temp1, "r", refsfound);
	nextline = 1;
	totallines = 0;
	
	/* see if it is empty */
	if ((c = getc(refsfound)) == EOF) {
		if (egreperror != NULL) {
			(void) sprintf(lastmsg, "Egrep %s in this pattern: %s", 
				egreperror, pattern);
		}
		else if (rc == NOTSYMBOL) {
			(void) sprintf(lastmsg, "This is not a C symbol: %s", 
				pattern);
		}
		else if (rc == REGCMPERROR) {
			(void) sprintf(lastmsg, "Error in this regcmp(3X) regular expression: %s", 
				pattern);
			
		}
		else if (funcexist == NO) {
			(void) sprintf(lastmsg, "Function definition does not exist: %s", 
				pattern);
		}
		else {
			(void) sprintf(lastmsg, "Could not find the %s: %s", 
				fields[field].text2, pattern);
		}
		return(NO);
	}
	/* put back the character read */
	(void) ungetc(c, refsfound);

	/* count the references found and find the length of the file,
	   function, and line number display fields */
	subsystemlen = 9;	/* strlen("Subsystem") */
	booklen = 4;		/* strlen("Book") */
	filelen = 4;		/* strlen("File") */
	fcnlen = 8;		/* strlen("Function") */
	numlen = 0;
	while (fscanf(refsfound, "%s%s%s", file, function, linenum) == 3) {
		if ((i = strlen(pathcomponents(file, dispcomponents))) > filelen) {
			filelen = i;
		}
		if (ogs == YES) {
			ogsnames(file, &subsystem, &book);
			if ((i = strlen(subsystem)) > subsystemlen) {
				subsystemlen = i;
			}
			if ((i = strlen(book)) > booklen) {
				booklen = i;
			}
		}
		if ((i = strlen(function)) > fcnlen) {
			fcnlen = i;
		}
		if ((i = strlen(linenum)) > numlen) {
			numlen = i;
		}
		/* skip the line text */
		while ((c = getc(refsfound)) != EOF && c != '\n') {
			;
		}
		++totallines;
	}
	rewind(refsfound);
	return(YES);
}

/* display search progress with default custom format */

void
progress(const char *fmt, ...)
{
	va_list	ap;
	static	long	start;
	long	now;
	char	msg[MSGLEN + 1];

	/* save the start time */
	if (searchcount == 0) {
		start = time(NULL);
	}
	/* display the progress every 3 seconds */
	else if ((now = time(NULL)) - start >= 3) {
		if (fmt == NULL) {	/* No arguments, print default msg */
			start = now;
			(void) sprintf(msg, "%ld of %d files searched", 
				searchcount, nsrcfiles);
			if (linemode == NO) postmsg(msg);
		} else {		/* Arguments, print custom message */
			va_start(ap, fmt);
			start = now;
			(void) vsnprintf(msg, MSGLEN + 1, fmt, ap);
			if (linemode == NO) postmsg(msg);
			va_end(ap);
		}
	}
	++searchcount;
}

/* print error message on system call failure */

void
myperror(char *text) 
{
	char	msg[MSGLEN + 1];	/* message */
	char	*s;

	s = "Unknown error";
#ifdef HAVE_STRERROR
        s = strerror(errno);
#else
	if (errno < sys_nerr) {
		s = (char *)sys_errlist[errno];
	}
#endif
	(void) sprintf(msg, "%s: %s", text, s);
	postmsg(msg);
}

/* postmsg clears the message line and prints the message */

/* VARARGS */
void
postmsg(char *msg) 
{
	if (linemode == YES || incurses == NO) {
		(void) printf("%s\n", msg);
	}
	else {
		move(MSGLINE, 0);
		clrtoeol();
		addstr(msg);
		refresh();
	}
	(void) strncpy(lastmsg, msg, sizeof(lastmsg) - 1);
}

/* clearmsg2 clears the second message line */

void
clearmsg2(void)
{
	if (linemode == NO) {
		move(MSGLINE + 1, 0);
		clrtoeol();
	}
}

/* postmsg2 clears the second message line and prints the message */

void
postmsg2(char *msg) 
{
	if (linemode == YES) {
		(void) printf("%s\n", msg);
	}
	else {
		clearmsg2();
		addstr(msg);
	}
}
/* position references found file at specified line */

void
seekline(int line) 
{
	int	c;

	/* verify that there is a references found file */
	if (refsfound == NULL) {
		return;
	}
	/* go to the beginning of the file */
	rewind(refsfound);
	
	/* find the requested line */
	nextline = 1;
	while (nextline < line && (c = getc(refsfound)) != EOF) {
		if (c == '\n') {
			nextline++;
		}
	}
}

/* get the OGS subsystem and book names */

void
ogsnames(char *file, char **subsystem, char **book)
{
	static	char	buf[PATHLEN + 1];
	char	*s, *slash;

	*subsystem = *book = "";
	(void) strcpy(buf,file);
	s = buf;
	if (*s == '/') {
		++s;
	}
	while ((slash = strchr(s, '/')) != NULL) {
		*slash = '\0';
		if ((int)strlen(s) >= 3 && strncmp(slash - 3, ".ss", 3) == 0) {
			*subsystem = s;
			s = slash + 1;
			if ((slash = strchr(s, '/')) != NULL) {
				*book = s;
				*slash = '\0';
			}
			break;
		}
		s = slash + 1;
	}
}

/* get the requested path components */

char *
pathcomponents(char *path, int components)
{
	int	i;
	char	*s;
	
	s = path + strlen(path) - 1;
	for (i = 0; i < components; ++i) {
		while (s > path && *--s != '/') {
			;
		}
	}
	if (s > path && *s == '/') {
		++s;
	}
	return(s);
}

/* open the references found file for writing */

BOOL
writerefsfound(void)
{
	if (refsfound == NULL) {
		if ((refsfound = myfopen(temp1, "w")) == NULL) {
			cannotopen(temp1);
			return(NO);
		}
	}
	else if (freopen(temp1, "w", refsfound) == NULL) {
		postmsg("Cannot reopen temporary file");
		return(NO);
	}
	return(YES);
}
