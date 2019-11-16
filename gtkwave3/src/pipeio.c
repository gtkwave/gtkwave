/*
 * Copyright (c) Tony Bybell 2005-2010
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include "pipeio.h"

#if defined _MSC_VER || defined __MINGW32__

static void cleanup_p(struct pipe_ctx *p)
{
if(p->g_hChildStd_IN_Rd) CloseHandle(p->g_hChildStd_IN_Rd);
if(p->g_hChildStd_IN_Wr) CloseHandle(p->g_hChildStd_IN_Wr);
if(p->g_hChildStd_OUT_Rd) CloseHandle(p->g_hChildStd_OUT_Rd);
if(p->g_hChildStd_OUT_Wr) CloseHandle(p->g_hChildStd_OUT_Wr);

free_2(p);
}

struct pipe_ctx *pipeio_create(char *execappname, char *args)
{
SECURITY_ATTRIBUTES saAttr;
STARTUPINFO siStartInfo;
BOOL bSuccess = FALSE;
TCHAR *szCmdline;

struct pipe_ctx *p = calloc_2(1, sizeof(struct pipe_ctx));

saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); /*  Set the bInheritHandle flag so pipe handles are inherited */
saAttr.bInheritHandle = TRUE;
saAttr.lpSecurityDescriptor = NULL;

if (!CreatePipe(&p->g_hChildStd_OUT_Rd, &p->g_hChildStd_OUT_Wr, &saAttr, 0)) { cleanup_p(p); return(NULL); }
if (!SetHandleInformation(p->g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) { cleanup_p(p); return(NULL); }
if (!CreatePipe(&p->g_hChildStd_IN_Rd, &p->g_hChildStd_IN_Wr, &saAttr, 0)) { cleanup_p(p); return(NULL); }
if (!SetHandleInformation(p->g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) { cleanup_p(p); return(NULL); }

memset(&siStartInfo, 0, sizeof(STARTUPINFO));
siStartInfo.cb = sizeof(STARTUPINFO);
/* siStartInfo.hStdError = p->g_hChildStd_OUT_Wr; (not sure how to redirect, for example GetStdHandle(STD_ERROR_HANDLE) */
siStartInfo.hStdOutput = p->g_hChildStd_OUT_Wr;
siStartInfo.hStdInput = p->g_hChildStd_IN_Rd;
siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

if (strlen(args) == 0)
	{
	szCmdline = strdup_2(execappname);
	}
	else
	{
	szCmdline = malloc_2(strlen(execappname) + 1 + strlen(args) + 1);
	sprintf(szCmdline, "%s %s", execappname, args);
	}

bSuccess = CreateProcess(NULL,
      szCmdline,     /* command line */
      NULL,          /* process security attributes */
      NULL,          /* primary thread security attributes */
      TRUE,          /* handles are inherited */
      0,             /* creation flags */
      NULL,          /* use parent's environment */
      NULL,          /* use parent's current directory */
      &siStartInfo,  /* STARTUPINFO pointer */
      &p->piProcInfo);  /* receives PROCESS_INFORMATION */

free_2(szCmdline);

if(!bSuccess)
	{
	cleanup_p(p); return(NULL);
	}
	else
	{
	/* CloseHandle(p->piProcInfo.hProcess); */
	/* CloseHandle(p->piProcInfo.hThread); */
	}

return(p);
}

void pipeio_destroy(struct pipe_ctx *p)
{
CloseHandle(p->g_hChildStd_IN_Rd);
CloseHandle(p->g_hChildStd_IN_Wr);
CloseHandle(p->g_hChildStd_OUT_Rd);
CloseHandle(p->g_hChildStd_OUT_Wr);
TerminateProcess(p->piProcInfo.hProcess, 0);

free_2(p);
}

#else
#include <sys/wait.h>

struct pipe_ctx *pipeio_create(char *execappname, char *arg)
{
int rc1, rc2;
pid_t pid, wave_pid;
int filedes_w[2];
int filedes_r[2];
struct pipe_ctx *p;
int mystat;

FILE *fsin=NULL, *fsout = NULL;

rc1 = pipe(filedes_r);
if(rc1) return(NULL);

rc2 = pipe(filedes_w);
if(rc2) { close(filedes_r[0]); close(filedes_r[1]); return(NULL); }

wave_pid = getpid();

if((pid=fork()))
	{
	fsout = fdopen(filedes_w[1], "wb");
	fsin = fdopen(filedes_r[0], "rb");
	close(filedes_w[0]);
	close(filedes_r[1]);
	}
	else
	{
	dup2(filedes_w[0], 0);
	dup2(filedes_r[1], 1);

	close(filedes_w[1]);
	close(filedes_r[0]);

#ifdef _AIX
	/* NOTE: doesn't handle ctrl-c or killing, but I don't want to mess with this right now for AIX */
	if ((!arg)||(strlen(arg) == 0)) {
	  execl(execappname, execappname, NULL);
	} else {
	  execl(execappname, execappname, arg, NULL);
	}
	exit(0);
#else
	if((pid=fork()))	/* monitor process */
		{
		do 	{
			sleep(1);
			} while(wave_pid == getppid()); /* inherited by init yet? */

		kill(pid, SIGKILL);
		waitpid(pid, &mystat, 0);

		exit(0);
		}
		else		/* actual helper */
		{
		  if (strlen(arg) == 0) {
		    execl(execappname, execappname, NULL);
		  } else {
		    execl(execappname, execappname, arg, NULL);
		  }
		exit(0);
		}
#endif
	}

p = malloc_2(sizeof(struct pipe_ctx));
p->pid = pid;
p->sin = fsin;
p->sout = fsout;
p->fd0 = filedes_r[0]; /* for potential select() ops */
p->fd1 = filedes_w[1]; /* ditto */

return(p);
}


void pipeio_destroy(struct pipe_ctx *p)
{
/* #ifdef _AIX */
int mystat;
kill(p->pid, SIGKILL);
waitpid(p->pid, &mystat, 0);
/* #endif */

fclose(p->sout);
fclose(p->sin);
free_2(p);
}

#endif

