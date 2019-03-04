/*
 * This file was generated by the mkinit program.
 */

#include "mystring.h"
#include "shell.h"
#include "init.h"
#include "eval.h"
#include "expand.h"
#include <stdio.h>
#include "input.h"
#include "error.h"
#include "parser.h"
#include "redir.h"
#include "trap.h"
#include "output.h"
#include "memalloc.h"
#include "tab.h"
#include <linenoise/linenoise.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "cd.h"
#include "var.h"



#undef  ATABSIZE
#define ATABSIZE 39
#undef  ARITH_MAX_PREC
#define ARITH_MAX_PREC 8
#undef  CD_PHYSICAL
#define CD_PHYSICAL 1
#undef  CD_PRINT
#define CD_PRINT 2
#undef  CMDTABLESIZE
#define CMDTABLESIZE 31		/* should be prime */
#undef  ARB
#define ARB 1			/* actual size determined at run time */
#undef  RMESCAPE_ALLOC
#define RMESCAPE_ALLOC	0x1	/* Allocate a new string */
#undef  RMESCAPE_GLOB
#define RMESCAPE_GLOB	0x2	/* Add backslashes for glob */
#undef  RMESCAPE_GROW
#define RMESCAPE_GROW	0x8	/* Grow strings instead of stalloc */
#undef  RMESCAPE_HEAP
#define RMESCAPE_HEAP	0x10	/* Malloc strings instead of stalloc */
#undef  QUOTES_ESC
#define QUOTES_ESC	(EXP_FULL | EXP_CASE | EXP_QPAT)
#undef  QUOTES_KEEPNUL
#define QUOTES_KEEPNUL	EXP_TILDE
#undef  EOF_NLEFT
#define EOF_NLEFT -99		/* value of parsenleft when EOF pushed back */
#undef  IBUFSIZ
#define IBUFSIZ (BUFSIZ + 1)
#undef  CUR_DELETE
#define CUR_DELETE 2
#undef  CUR_RUNNING
#define CUR_RUNNING 1
#undef  CUR_STOPPED
#define CUR_STOPPED 0
#undef  DOWAIT_NORMAL
#define DOWAIT_NORMAL 0
#undef  DOWAIT_BLOCK
#define DOWAIT_BLOCK 1
#undef  DOWAIT_WAITCMD
#define DOWAIT_WAITCMD 2
#undef  PROFILE
#define PROFILE 0
#undef  MINSIZE
#define MINSIZE SHELL_ALIGN(504)
#undef  DEFINE_OPTIONS
#define DEFINE_OPTIONS
#undef  FAKEEOFMARK
#define FAKEEOFMARK (char *)1
#undef  REALLY_CLOSED
#define REALLY_CLOSED -3	/* fd that was closed and still is */
#undef  EMPTY
#define EMPTY -2		/* marks an unused slot in redirtab */
#undef  CLOSED
#define CLOSED -1		/* fd opened for redir needs to be closed */
#undef  S_DFL
#define S_DFL 1			/* default signal handling (SIG_DFL) */
#undef  S_CATCH
#define S_CATCH 2		/* signal is caught */
#undef  S_IGN
#define S_IGN 3			/* signal is ignored (SIG_IGN) */
#undef  S_HARD_IGN
#define S_HARD_IGN 4		/* signal is ignored permenantly */
#undef  S_RESET
#define S_RESET 5		/* temporary - to reset a hard ignored sig */
#undef  OUTBUFSIZ
#define OUTBUFSIZ BUFSIZ
#undef  MEM_OUT
#define MEM_OUT -3		/* output to dynamically allocated memory */
#undef  SKIP1
#define SKIP1	"#-+ 0"
#undef  SKIP2
#define SKIP2	"*0123456789"
#undef  isalnum
#define isalnum _isalnum
#undef  iscntrl
#define iscntrl _iscntrl
#undef  islower
#define islower _islower
#undef  isspace
#define isspace _isspace
#undef  isalpha
#define isalpha _isalpha
#undef  isdigit
#define isdigit _isdigit
#undef  isprint
#define isprint _isprint
#undef  isupper
#define isupper _isupper
#undef  isblank
#define isblank _isblank
#undef  isgraph
#define isgraph _isgraph
#undef  ispunct
#define ispunct _ispunct
#undef  isxdigit
#define isxdigit _isxdigit
#undef  VTABSIZE
#define VTABSIZE 39



extern int loopnest;		/* current loop nesting level */

extern struct parsefile basepf;	/* top level input file */
extern char basebuf[IBUFSIZ];	/* buffer for top level input file */

struct redirtab {
	struct redirtab *next;
	int renamed[10];
};

extern struct redirtab *redirlist;

extern struct localvar_list *localvar_stack;
extern char defoptindvar[];
extern char **environ;



/*
 * Initialization code.
 */

void
init() {

      /* from input.c: */
      {
	      basepf.nextc = basepf.buf = basebuf;
	      basepf.linno = 1;
      }

      /* from trap.c: */
      {
	      sigmode[SIGCHLD - 1] = S_DFL;
	      setsignal(SIGCHLD);
      }

      /* from output.c: */
      {
#ifdef USE_GLIBC_STDIO
	      initstreams();
#endif
      }

      /* from tab.c: */
      {
	  linenoiseSetCompletionCallback(tab_complete);
      }

      /* from var.c: */
      {
	      char **envp;
	      static char ppid[32] = "PPID=";
	      const char *p;
	      struct stat st1, st2;

	      initvar();
	      for (envp = environ ; *envp ; envp++) {
		      p = endofname(*envp);
		      if (p != *envp && *p == '=') {
			      setvareq(*envp, VEXPORT|VTEXTFIXED);
		      }
	      }

	      setvareq(defoptindvar, VTEXTFIXED);

	      fmtstr(ppid + 5, sizeof(ppid) - 5, "%ld", (long) getppid());
	      setvareq(ppid, VTEXTFIXED);

	      p = lookupvar("PWD");
	      if (p)
		      if (*p != '/' || stat(p, &st1) || stat(".", &st2) ||
			  st1.st_dev != st2.st_dev || st1.st_ino != st2.st_ino)
			      p = 0;
	      setpwd(p, 0);
      }
}



/*
 * This routine is called when an error or an interrupt occurs in an
 * interactive shell and control is returned to the main command loop.
 */

void
reset() {

      /* from eval.c: */
      {
	      evalskip = 0;
	      loopnest = 0;
	      if (savestatus >= 0) {
		      exitstatus = savestatus;
		      savestatus = -1;
	      }
      }

      /* from expand.c: */
      {
	      ifsfree();
      }

      /* from input.c: */
      {
	      /* clear input buffer */
	      basepf.lleft = basepf.nleft = 0;
	      popallfiles();
      }

      /* from redir.c: */
      {
	      /*
	       * Discard all saved file descriptors.
	       */
	      unwindredir(0);
      }

      /* from output.c: */
      {
#ifdef notyet
	      out1 = &output;
	      out2 = &errout;
#ifdef USE_GLIBC_STDIO
	      if (memout.stream != NULL)
		      __closememout();
#endif
	      if (memout.buf != NULL) {
		      ckfree(memout.buf);
		      memout.buf = NULL;
	      }
#endif
      }

      /* from var.c: */
      {
	      unwindlocalvars(0);
      }
}
