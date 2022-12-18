/*
 * Copyright (c) Tony Bybell 2006-2009.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __MINGW32__
#include <windows.h>
#undef MINGW_USE_XID
#endif
#include "wave_locale.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "debug.h"

#ifdef WAVE_USE_GTK2
static int use_embedded = 1;
#endif

static int plug_removed(GtkWidget *widget, gpointer data)
{
(void)widget;
(void)data;

static int cnt = 2;

fprintf(stderr, "GtkPlug removed\n");
cnt--;

if(cnt==0)
	{
	fprintf(stderr, "No GtkPlugs left, exiting.\n");
	gtk_exit(0);
	}

return(FALSE);   /* TRUE would keep xsocket open */
}

int quit_callback (GtkWidget *widget, gpointer data)
{
(void)widget;

fprintf(stderr,"%s\n", (char *)data);
gtk_exit(0);

return(FALSE);
}


int main(int argc, char **argv)
{
struct gtkwave_dual_ipc_t *dual_ctx;
char buf[257], buf2[257];
int shmid;
GtkWidget *mainwindow;
int i;
int split_point = -1;
#ifdef __MINGW32__
char mapName[65];
HANDLE hMapFile;
#endif

#if !defined(__MINGW32__) && !defined(_MSC_VER)
GtkWidget *xsocket[2] = { NULL, NULL };
GtkWidget *main_vbox, *vpan;
#endif

WAVE_LOCALE_FIX

if(!gtk_init_check(&argc, &argv))
        {
        printf("Could not initialize GTK!  Is DISPLAY env var/xhost set?\n\n");
	exit(255);
        }

#ifdef __CYGWIN__
fprintf(stderr, "TWINWAVE| If the viewer crashes with a Bad system call error,\n");
fprintf(stderr, "TWINWAVE| make sure that Cygserver is enabled.\n");
#endif

for(i=0;i<argc;i++)
	{
	if(!strcmp(argv[i], "+"))
		{
		split_point = i;
		break;
		}

	if(!strcmp(argv[i], "++"))
		{
		split_point = i;
#ifdef WAVE_USE_GTK2
		use_embedded = 0;
#endif
		break;
		}
	}

if(split_point < 0)
	{
	printf("Usage:\n------\n%s arglist1 separator arglist2\n\n"
		"The '+' between argument lists splits and creates one window.\n"
		"The '++' between argument lists splits and creates two windows.\n"
		"\n", argv[0]);
	exit(255);
	}

mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
gtk_window_set_title(GTK_WINDOW(mainwindow), "TwinWave Initializing");
#ifndef MAC_INTEGRATION
gtk_widget_set_usize(GTK_WIDGET(mainwindow), 820, 800);
#else
gtk_widget_set_usize(GTK_WIDGET(mainwindow), 400,32); /* quartz doesn't retarget into mainwindow */
#endif
gtk_widget_show(mainwindow);

gtk_signal_connect(GTK_OBJECT(mainwindow), "destroy", GTK_SIGNAL_FUNC(quit_callback), "WM destroy");

#if !defined(__MINGW32__) && !defined(_MSC_VER)
xsocket[0] = gtk_socket_new ();
xsocket[1] = gtk_socket_new ();
gtk_widget_show (xsocket[0]);
gtk_widget_show (xsocket[1]);

#ifdef WAVE_USE_GTK2
gtk_signal_connect(GTK_OBJECT(xsocket[0]), "plug-removed", GTK_SIGNAL_FUNC(plug_removed), NULL);
#endif

main_vbox = gtk_vbox_new(FALSE, 5);
gtk_container_border_width(GTK_CONTAINER(main_vbox), 1);
gtk_container_add(GTK_CONTAINER(mainwindow), main_vbox);
gtk_widget_show(main_vbox);

vpan = gtk_vpaned_new ();
gtk_widget_show (vpan);
gtk_box_pack_start (GTK_BOX (main_vbox), vpan, TRUE, TRUE, 1);

gtk_paned_pack1 (GTK_PANED (vpan), xsocket[0], TRUE, FALSE);

#ifdef WAVE_USE_GTK2
gtk_signal_connect(GTK_OBJECT(xsocket[1]), "plug-removed", GTK_SIGNAL_FUNC(plug_removed), NULL);
#endif

gtk_paned_pack2 (GTK_PANED (vpan), xsocket[1], TRUE, FALSE);
#endif

#ifdef __MINGW32__
shmid = getpid();
sprintf(mapName, "twinwave%d", shmid);
hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 2 * sizeof(struct gtkwave_dual_ipc_t), mapName);
if(hMapFile != NULL)
        {
        dual_ctx = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 2 * sizeof(struct gtkwave_dual_ipc_t));

        if(dual_ctx)
               	{
               	memset(dual_ctx, 0, 2 * sizeof(struct gtkwave_dual_ipc_t));
               	memcpy(&dual_ctx[0].matchword, DUAL_MATCHWORD, 4);
               	memcpy(&dual_ctx[1].matchword, DUAL_MATCHWORD, 4);

		/* child 0 */
				{
				int idx;
				int n_items = split_point + 5;
				int slen;
				char **arglist = calloc(n_items, sizeof(char *));
				char *mylist;
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				BOOL rc;

				memset(&si, 0, sizeof(STARTUPINFO));
				memset(&pi, 0, sizeof(PROCESS_INFORMATION));

				sprintf(buf, "0+%08X", shmid);
#ifdef MINGW_USE_XID
				sprintf(buf2, "%x", gtk_socket_get_id (GTK_SOCKET(xsocket[0])));
#else
				sprintf(buf2, "%x", 0);
#endif

				arglist[0] = "gtkwave.exe";
				arglist[1] = "-D";
				arglist[2] = buf;
				arglist[3] = "-X";
				arglist[4] = buf2;

				for(i=1;i<split_point;i++)
					{
					arglist[i+4] = argv[i];
					}

				arglist[i+4] = NULL;

				slen = 1;
				for(idx=0;idx<i+4;idx++)
					{
					slen += strlen(arglist[idx]);
					slen++;
					}
				mylist = calloc(1, slen);
				for(idx=0;idx<i+4;idx++)
					{
					strcat(mylist, arglist[idx]);
					strcat(mylist, " ");
					}

				si.cb = sizeof(si);

				rc = CreateProcess(
					arglist[0],
					mylist,
					NULL,
					NULL,
					FALSE,
					0,
					NULL,
					NULL,
					&si,
					&pi);

				if(!rc)
					{
					fprintf(stderr, "Child 0 failed '%s' '%s'\n", arglist[0], mylist);
					exit(255);
					}

				free(mylist);
				free(arglist);
				}

		/* child 1 */
				{
				int idx;
				int n_items = argc - split_point + 5;
				int slen;
				char **arglist = calloc(n_items, sizeof(char *));
				char *mylist;
				STARTUPINFO si;
				PROCESS_INFORMATION pi;
				BOOL rc;

				memset(&si, 0, sizeof(STARTUPINFO));
				memset(&pi, 0, sizeof(PROCESS_INFORMATION));

				sprintf(buf, "1+%08X", shmid);
#ifdef MINGW_USE_XID
				sprintf(buf2, "%x", gtk_socket_get_id (GTK_SOCKET(xsocket[1])));
#else
				sprintf(buf2, "%x", 0);
#endif

				arglist[0] = "gtkwave.exe";
				arglist[1] = "-D";
				arglist[2] = buf;
				arglist[3] = "-X";
				arglist[4] = buf2;

				for(i=split_point+1;i<argc;i++)
					{
					arglist[i-split_point+4] = argv[i];
					}

				i-=split_point;
				arglist[i+4] = NULL;

				slen = 1;
				for(idx=0;idx<i+4;idx++)
					{
					slen += strlen(arglist[idx]);
					slen++;
					}
				mylist = calloc(1, slen);
				for(idx=0;idx<i+4;idx++)
					{
					strcat(mylist, arglist[idx]);
					strcat(mylist, " ");
					}

				si.cb = sizeof(si);

				rc = CreateProcess(
					arglist[0],
					mylist,
					NULL,
					NULL,
					FALSE,
					0,
					NULL,
					NULL,
					&si,
					&pi);

				if(!rc)
					{
					fprintf(stderr, "Child 1 failed '%s' '%s'\n", arglist[0], mylist);
					exit(255);
					}

				free(mylist);
				free(arglist);
				}


		for(;;)
			{
			Sleep(1000 / 5);
			while (gtk_events_pending()) gtk_main_iteration();

			if((!dual_ctx[0].viewer_is_initialized)&&(dual_ctx[1].viewer_is_initialized))
				{
				gtk_window_set_title(GTK_WINDOW(mainwindow), "TwinWave Waiting on Viewer #1");
				}
			else
			if((dual_ctx[0].viewer_is_initialized)&&(!dual_ctx[1].viewer_is_initialized))
				{
				gtk_window_set_title(GTK_WINDOW(mainwindow), "TwinWave Waiting on Viewer #2");
				}
			else
			if((dual_ctx[0].viewer_is_initialized)&&(dual_ctx[1].viewer_is_initialized))
				{
				gtk_window_set_title(GTK_WINDOW(mainwindow), "TwinWave");
				break;
				}
			}
#ifdef MINGW_USE_XID
		gtk_main();
#endif
		}
	}
#else
shmid = shmget(0, 2 * sizeof(struct gtkwave_dual_ipc_t), IPC_CREAT | 0600 );
if(shmid >=0)
	{
        struct shmid_ds ds;

        dual_ctx = shmat(shmid, NULL, 0);
        if(dual_ctx)
               	{
               	memset(dual_ctx, 0, 2 * sizeof(struct gtkwave_dual_ipc_t));
               	memcpy(&dual_ctx[0].matchword, DUAL_MATCHWORD, 4);
               	memcpy(&dual_ctx[1].matchword, DUAL_MATCHWORD, 4);

#ifdef __linux__
              	shmctl(shmid, IPC_RMID, &ds); /* mark for destroy */
#endif

		if(fork())
			{
			if(fork())
				{
				struct timeval tv;

				for(;;)
					{
			                tv.tv_sec = 0;
		                        tv.tv_usec = 1000000 / 5;
	        	                select(0, NULL, NULL, NULL, &tv);

					while (gtk_events_pending()) gtk_main_iteration();

					if((!dual_ctx[0].viewer_is_initialized)&&(dual_ctx[1].viewer_is_initialized))
						{
						gtk_window_set_title(GTK_WINDOW(mainwindow), "TwinWave Waiting on Viewer #1");
						}
					else
					if((dual_ctx[0].viewer_is_initialized)&&(!dual_ctx[1].viewer_is_initialized))
						{
						gtk_window_set_title(GTK_WINDOW(mainwindow), "TwinWave Waiting on Viewer #2");
						}
					else
					if((dual_ctx[0].viewer_is_initialized)&&(dual_ctx[1].viewer_is_initialized))
						{
						gtk_window_set_title(GTK_WINDOW(mainwindow), "TwinWave");
						break;
						}
					}

#ifndef __linux__
				while (gtk_events_pending()) gtk_main_iteration();
				sleep(2);
		               	shmctl(shmid, IPC_RMID, &ds); /* mark for destroy */
#endif
#ifdef WAVE_USE_GTK2
				if(use_embedded)
					{
					gtk_main();
					}
#endif
				}
				else
				{
				int n_items = split_point + 5;
				char **arglist = calloc(n_items, sizeof(char *));

				sprintf(buf, "0+%08X", shmid);
#ifdef WAVE_USE_GTK2
				if(use_embedded)
					{
					sprintf(buf2, "%x", gtk_socket_get_id (GTK_SOCKET(xsocket[0])));
					}
					else
#endif
					{
					sprintf(buf2, "%x", 0);
					}

				arglist[0] = "gtkwave";
				arglist[1] = "-D";
				arglist[2] = buf;
				arglist[3] = "-X";
				arglist[4] = buf2;

				for(i=1;i<split_point;i++)
					{
					arglist[i+4] = argv[i];
					}

				arglist[i+4] = NULL;

				execvp(arglist[0], arglist);

				fprintf(stderr, "Child failed\n");
				exit(255);
				}
			}
			else
			{
			int n_items = argc - split_point + 5;
			char **arglist = calloc(n_items, sizeof(char *));

			sprintf(buf, "1+%08X", shmid);
#ifdef WAVE_USE_GTK2
			if(use_embedded)
				{
				sprintf(buf2, "%x", gtk_socket_get_id (GTK_SOCKET(xsocket[1])));
				}
				else
#endif
				{
				sprintf(buf2, "%x", 0);
				}

			arglist[0] = "gtkwave";
			arglist[1] = "-D";
			arglist[2] = buf;
			arglist[3] = "-X";
			arglist[4] = buf2;

			for(i=split_point+1;i<argc;i++)
				{
				arglist[i-split_point+4] = argv[i];
				}
			i-=split_point;
			arglist[i+4] = NULL;

			execvp(arglist[0], arglist);

			fprintf(stderr, "Child failed\n");
			exit(255);
			}
		}
	}
#endif

return(0);
}
