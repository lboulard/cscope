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
 *	terminal input functions
 */

#include "global.h"
#include <curses.h>
#include <setjmp.h>	/* jmp_buf */

static char const rcsid[] = "$Id$";

static	jmp_buf	env;		/* setjmp/longjmp buffer */
static	int	prevchar;	/* previous, ungotten character */

/* catch the interrupt signal */

/*ARGSUSED*/
SIGTYPE
catchint(sig)
{
	(void) signal(SIGINT, catchint);
	longjmp(env, 1);
}

/* unget a character */

void
myungetch(int c)
{
	prevchar = c;
}

/* get a character from the terminal */

int
mygetch(void)
{
	SIGTYPE	(*savesig)();		/* old value of signal */
	int	c;

	/* change an interrupt signal to a break key character */
	if (setjmp(env) == 0) {
		savesig = signal(SIGINT, catchint);
		refresh();	/* update the display */
		mousereinit();	/* curses can change the menu number */
		if(prevchar) {
			c = prevchar;
			prevchar = 0;
		}
		else
			c = getch();	/* get a character from the terminal */
	}
	else {	/* longjmp to here from signal handler */
		c = KEY_BREAK;
	}
	(void) signal(SIGINT, savesig);
	return(c);
}

/* get a line from the terminal in non-canonical mode */

int
getline(char s[], unsigned size, int firstchar, BOOL iscaseless)
{
	int	c, i = 0;
	int	j;

	/* if a character already has been typed */
	if (firstchar != '\0') {
		if(iscaseless == YES) {
			firstchar = tolower(firstchar);
		}
		addch(firstchar);	/* display it */
		s[i++] = firstchar;	/* save it */
	}
	/* until the end of the line is reached */
	while ((c = mygetch()) != '\r' && c != '\n' && c != KEY_ENTER) {
		if (c == erasechar() || c == KEY_BACKSPACE || c == DEL) {
			/* erase */
			if (i > 0) {
				addstr("\b \b");
				--i;
			}
		}
		else if (c == killchar() || c == KEY_BREAK) {	/* kill */
			for (j = 0; j < i; ++j) {
				addch('\b');
			}
			for (j = 0; j < i; ++j) {
				addch(' ');
			}
			for (j = 0; j < i; ++j) {
				addch('\b');
			}
			i = 0;
		}
		else if (isprint(c) || c == '\t') {		/* printable */
			if(iscaseless == YES) {
				c = tolower(c);
			}
			/* if it will fit on the line */
			if (i < size) {
				addch(c);	/* display it */
				s[i++] = c;	/* save it */
			}
		}
#if UNIXPC
		else if (unixpcmouse == YES && c == ESC) {	/* mouse */
			(void) getmouseaction(ESC);	/* ignore it */
		}
#endif
		else if (mouse == YES && c == ctrl('X')) {
			(void) getmouseaction(ctrl('X'));	/* ignore it */
		}
		else if (c == EOF) {				/* end-of-file */
			break;
		}
		/* return on an empty line to allow a command to be entered */
		if (firstchar != '\0' && i == 0) {
			break;
		}
	}
	s[i] = '\0';
	return(i);
}

/* ask user to enter a character after reading the message */

void
askforchar(void)
{
	addstr("Type any character to continue: ");
	(void) mygetch();
}

/* ask user to press the RETURN key after reading the message */

void
askforreturn(void)
{
	(void) fprintf(stderr, "Press the RETURN key to continue: ");
	(void) getchar();
}

/* expand the ~ and $ shell meta characters in a path */

void
shellpath(char *out, int limit, char *in) 
{
	char	*lastchar;
	char	*s, *v;

	/* skip leading white space */
	while (isspace(*in)) {
		++in;
	}
	lastchar = out + limit - 1;

	/* a tilde (~) by itself represents $HOME; followed by a name it
	   represents the $LOGDIR of that login name */
	if (*in == '~') {
		*out++ = *in++;	/* copy the ~ because it may not be expanded */

		/* get the login name */
		s = out;
		while (s < lastchar && *in != '/' && *in != '\0' && !isspace(*in)) {
			*s++ = *in++;
		}
		*s = '\0';

		/* if the login name is null, then use $HOME */
		if (*out == '\0') {
			v = (char *) getenv("HOME");
		}
		else {	/* get the home directory of the login name */
			v = logdir(out);
		}
		/* copy the directory name */
		if (v != NULL) {
			(void) strcpy(out - 1, v);
			out += strlen(v) - 1;
		}
		else {	/* login not found, so ~ must be part of the file name */
			out += strlen(out);
		}
	}
	/* get the rest of the path */
	while (out < lastchar && *in != '\0' && !isspace(*in)) {

		/* look for an environment variable */
		if (*in == '$') {
			*out++ = *in++;	/* copy the $ because it may not be expanded */

			/* get the variable name */
			s = out;
			while (s < lastchar && *in != '/' && *in != '\0' &&
			    !isspace(*in)) {
				*s++ = *in++;
			}
			*s = '\0';
	
			/* get its value */
			if ((v = (char *) getenv(out)) != NULL) {
				(void) strcpy(out - 1, v);
				out += strlen(v) - 1;
			}
			else {	/* var not found, so $ must be part of the file name */
				out += strlen(out);
			}
		}
		else {	/* ordinary character */
			*out++ = *in++;
		}
	}
	*out = '\0';
}
