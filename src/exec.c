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
 *	process execution functions
 */

#include <unistd.h>
#include "global.h"
#include <stdarg.h>
#include <sys/wait.h>
#include <sys/types.h>      /* pid_t */
#include <curses.h>

static char const rcsid[] = "$Id$";

static	SIGTYPE	(*oldsigquit)();	/* old value of quit signal */
static	SIGTYPE	(*oldsighup)();		/* old value of hangup signal */
static	SIGTYPE	(*oldsigstp)();

static	int	join(pid_t p);
static	int	myexecvp(char *a, char **args);
static	pid_t	myfork(void);

/* execute forks and executes a program or shell script, waits for it to
 * finish, and returns its exit code.
 */

/*VARARGS1*/
int
execute(char *a, ...)	/* note: "exec" is already defined on u370 */
{
	va_list	ap;
	int	exitcode;
	char	*argv[BUFSIZ];
	pid_t	p;
	pid_t	myfork();

	/* fork and exec the program or shell script */
	endwin();	/* restore the terminal modes */
	mousecleanup();
	(void) fflush(stdout);
	va_start(ap, a);
	for (p = 0; (argv[p] = va_arg(ap, char *)) != 0; p++)
		;
	if ((p = myfork()) == 0) {
		(void) myexecvp(a, argv);	/* child */
	}
	else {
		exitcode = join(p);	/* parent */
	}
	/* the menu and scrollbar may be changed by the command executed */
#if UNIXPC || !TERMINFO
	nonl();
	cbreak();	/* endwin() turns off cbreak mode so restore it */
	noecho();
#endif
	mousemenu();
	drawscrollbar(topline, nextline);
	va_end(ap);
	return(exitcode);
}

/* myexecvp is an interface to the execvp system call to
 * modify argv[0] to reference the last component of its path-name.
 */

static int
myexecvp(char *a, char **args)
{
	int	i;
	char	msg[MSGLEN + 1];
	
	/* modify argv[0] to reference the last component of its path name */
	args[0] = basename(args[0]);

	/* execute the program or shell script */
	(void) execvp(a, args);	/* returns only on failure */
	(void) sprintf(msg, "\nCannot exec %s", a);
	perror(msg);		/* display the reason */
	askforreturn();		/* wait until the user sees the message */
	exit(1);		/* exit the child */
	/* NOTREACHED */
}

/* myfork acts like fork but also handles signals */

static pid_t
myfork(void)
{
	pid_t	p;		/* process number */

	p = fork();
	
	/* the parent ignores the interrupt, quit, and hangup signals */
	if (p > 0) {
		oldsigquit = signal(SIGQUIT, SIG_IGN);
		oldsighup = signal(SIGHUP, SIG_IGN);
		oldsigstp = signal(SIGTSTP, SIG_DFL);
	}
	/* so they can be used to stop the child */
	else if (p == 0) {
		(void) signal(SIGINT, SIG_DFL);
		(void) signal(SIGQUIT, SIG_DFL);
		(void) signal(SIGHUP, SIG_DFL);
		(void) signal(SIGTSTP, SIG_DFL);
	}
	/* check for fork failure */
	if (p == -1) {
		myperror("Cannot fork");
	}
	return p;
}

/* join is the compliment of fork */

static int
join(pid_t p) 
{
	int	status;  
	pid_t	w;

	/* wait for the correct child to exit */
	do {
		w = wait(&status);
	} while (p != -1 && w != p);

	/* restore signal handling */
	(void) signal(SIGQUIT, oldsigquit);
	(void) signal(SIGHUP, oldsighup);
	(void) signal(SIGTSTP, oldsigstp);

	/* return the child's exit code */
	return(status >> 8);
}
