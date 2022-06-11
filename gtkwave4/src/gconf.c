/*
 * Copyright (c) Tony Bybell 2012-2018.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <config.h>
#include "gconf.h"
#include "wavealloca.h"
#include "globals.h"

int wave_rpc_id = 0;

#ifdef WAVE_HAVE_GSETTINGS
static GSettings *gs = NULL;

static void remove_client(void)
{
if(gs)
	{
	g_object_unref(gs);
	gs = NULL;
	}
}

static char *parse_rpc_id(char *str, int *this_wave_rpc_id)
{
char *comma = strchr(str, ',');
char *str2 = comma ? (comma+1) : NULL;

*this_wave_rpc_id = atoi(str);

return(str2);
}

static void
user_function (GSettings *settings,
               gchar     *key,
               gpointer   user_data)
{
(void)user_data;

char *str = NULL;
char *str2 = NULL;
int this_wave_rpc_id = -1;

g_settings_get (settings, key, "s", &str);

if(!strcmp(key, "open"))
	{
	if((str)&&(str[0]))
        	{
		str2 = parse_rpc_id(str, &this_wave_rpc_id);
		if((this_wave_rpc_id == wave_rpc_id) && str2)
			{
	          	fprintf(stderr, "GTKWAVE | RPC Open: '%s'\n", str2);

	          	deal_with_rpc_open(str, NULL);
			g_settings_set(settings, "open", "s", "");
			}
        	}
	}
else
if(!strcmp(key, "quit"))
	{
	if((str)&&(str[0]))
	        {
		str2 = parse_rpc_id(str, &this_wave_rpc_id);
		if((this_wave_rpc_id == wave_rpc_id) && str2)
			{
	          	const char *rc = str2;
	          	int rcv = atoi(rc);
	          	fprintf(stderr, "GTKWAVE | RPC Quit: exit return code %d\n", rcv);
	          	g_settings_set(settings, "quit", "s", "");
	          	exit(rcv);
			}
        	}
	}
else
if(!strcmp(key, "reload"))
	{
	if((str)&&(str[0]))
        	{
		this_wave_rpc_id = atoi(str);
		if(this_wave_rpc_id == wave_rpc_id)
			{
	          	if(in_main_iteration()) goto bot;
	          	reload_into_new_context();
			g_settings_set(settings, "reload", "s", "");
			}
        	}
	}
else
if(!strcmp(key, "zoom-full"))
	{
	if((str)&&(str[0]))
	        {
		this_wave_rpc_id = atoi(str);
		if(this_wave_rpc_id == wave_rpc_id)
			{
	          	if(in_main_iteration()) goto bot;
	          	service_zoom_full(NULL, NULL);
			g_settings_set(settings, "zoom-full", "s", "");
			}
        	}
	}
else
if(!strcmp(key, "writesave"))
	{
	if((str)&&(str[0]))
        	{
		str2 = parse_rpc_id(str, &this_wave_rpc_id);
		if((this_wave_rpc_id == wave_rpc_id) && str2)
			{
	          	const char *fni = str2;
	          	if(fni && !in_main_iteration())
	                	{
	                  	int use_arg = strcmp(fni, "+"); /* plus filename uses default */
	                  	const char *fn = use_arg ? fni : GLOBALS->filesel_writesave;
	                  	if(fn)
	                        	{
	                        	FILE *wave;
	
	                        	if(!(wave=fopen(fn, "wb")))
	                                	{
	                                	fprintf(stderr, "GTKWAVE | RPC Writesave: error opening save file '%s' for writing.\n", fn);
	                                	perror("Why");
	                                	errno=0;
	                                	}
	                                	else
	                                	{
	                                	write_save_helper(fn, wave);
	                                	if(use_arg)
	                                        	{
	                                        	if(GLOBALS->filesel_writesave) { free_2(GLOBALS->filesel_writesave); }
	                                        	GLOBALS->filesel_writesave = strdup_2(fn);
	                                        	}
	                                	wave_gconf_client_set_string("/current/savefile", fn);
	                                	fclose(wave);
	                                	fprintf(stderr, "GTKWAVE | RPC Writesave: wrote save file '%s'.\n", GLOBALS->filesel_writesave);
	                                	}
	                        	}
	                	}
	
		  	g_settings_set(settings, "writesave", "s", "");
			}
        	}
	}
else
if(!strcmp(key, "move-to-time"))
	{
	if((str)&&(str[0]))
    		{
		str2 = parse_rpc_id(str, &this_wave_rpc_id);
		if((this_wave_rpc_id == wave_rpc_id) && str2)
			{
	       		if(!in_main_iteration())
	               		{
	               		char *e_copy = GLOBALS->entrybox_text;
	               		GLOBALS->entrybox_text=strdup_2(str2);
	               		movetotime_cleanup(NULL, NULL);
	
	               		GLOBALS->entrybox_text = e_copy;
	               		}
	
	       		g_settings_set(settings, "move-to-time", "s", "");
			}
		}
	}
else
if(!strcmp(key, "zoom-size"))
	{
	if((str)&&(str[0]))
        	{
		str2 = parse_rpc_id(str, &this_wave_rpc_id);
		if((this_wave_rpc_id == wave_rpc_id) && str2)
			{
	          	if(!in_main_iteration())
	                	{
	                	char *e_copy = GLOBALS->entrybox_text;
	                	GLOBALS->entrybox_text=strdup_2(str2);
	
	                	zoomsize_cleanup(NULL, NULL);
	
	                	GLOBALS->entrybox_text = e_copy;
	                	}
	
			g_settings_set(settings, "zoom-size", "s", "");
			}
        	}
	}

bot:
if(str) g_free(str);
}

void wave_gconf_init(int argc, char **argv)
{
(void)argc;
(void)argv;

if(!gs)
	{
	gs = g_settings_new (WAVE_GSETTINGS_SCHEMA_ID);
	g_signal_connect (gs, "changed", G_CALLBACK (user_function), NULL);
	atexit(remove_client);
	}
}

gboolean wave_gconf_client_set_string(const gchar *key, const gchar *val)
{
if(key && gs)
        {
	const char *ks = strrchr(key, '/');
	char *k2 = NULL;
	if(ks) { ks = ks+1; } else { ks = key; }
	if(strchr(ks, '_'))
		{
		char *s;
		k2 = s = strdup_2(ks);
		while(*s) { if(*s=='_') *s='-'; s++; }
		}
	g_settings_set(gs, k2 ? k2 : ks, "s", val ? val : "");
	if(k2) free_2(k2);
        return(TRUE);
        }

return(FALSE);
}


static gchar *wave_gconf_client_get_string(const gchar *key)
{
if(key && gs)
        {
	const char *ks = strrchr(key, '/');
	char *k2 = NULL;
	char *str = NULL;
	if(ks) { ks = ks+1; } else { ks = key; }
	if(strchr(ks, '_'))
		{
		char *s;
		k2 = s = strdup_2(ks);
		while(*s) { if(*s=='_') *s='-'; s++; }
		}
	g_settings_get (gs, k2 ? k2 : ks, "s", &str);
	if(k2) free_2(k2);
        return(str);
        }

return(NULL);
}

void wave_gconf_restore(char **dumpfile, char **savefile, char **rcfile, char **wave_pwd, int *opt_vcd)
{
char *s;

if(dumpfile && savefile && rcfile && wave_pwd && opt_vcd)
        {
        if(*dumpfile) { free_2(*dumpfile); *dumpfile = NULL; }
        s = wave_gconf_client_get_string("/current/dumpfile");
        if(s) { if(s[0]) *dumpfile = strdup_2(s); g_free(s); }

        if(*savefile) { free_2(*savefile); *savefile = NULL; }
        s = wave_gconf_client_get_string("/current/savefile");
        if(s) { if(s[0]) *savefile = strdup_2(s); g_free(s); }

        if(*rcfile) { free_2(*rcfile); *rcfile = NULL; }
        s = wave_gconf_client_get_string("/current/rcfile");
        if(s) { if(s[0]) *rcfile = strdup_2(s); g_free(s); }

        if(*wave_pwd) { free_2(*wave_pwd); *wave_pwd = NULL; }
        s = wave_gconf_client_get_string("/current/pwd");
        if(s) { if(s[0]) *wave_pwd = strdup_2(s); g_free(s); }

        s = wave_gconf_client_get_string("/current/optimized-vcd");
        if(s) { if(s[0]) *opt_vcd = atoi(s); g_free(s); }
        }
}

#else

#ifdef WAVE_HAVE_GCONF

static GConfClient* client = NULL;

/************************************************************/

static void
open_callback(GConfClient* gclient,
                     guint cnxn_id,
                     GConfEntry *entry,
                     gpointer user_data)
{
(void)gclient;
(void)cnxn_id;
(void)user_data;

  if (gconf_entry_get_value (entry) == NULL)
    {
      /* value is unset */
    }
  else
    {
      if (gconf_entry_get_value (entry)->type == GCONF_VALUE_STRING)
        {
	  fprintf(stderr, "GTKWAVE | RPC Open: '%s'\n", gconf_value_get_string (gconf_entry_get_value (entry)) );

	  deal_with_rpc_open(gconf_value_get_string (gconf_entry_get_value (entry)), NULL);
	  gconf_entry_set_value(entry, NULL);
        }
      else
        {
          /* value is of wrong type */
        }
    }
}


static void
quit_callback(GConfClient* gclient,
                     guint cnxn_id,
                     GConfEntry *entry,
                     gpointer user_data)
{
(void)gclient;
(void)cnxn_id;
(void)user_data;

  if (gconf_entry_get_value (entry) == NULL)
    {
      /* value is unset */
    }
  else
    {
      if (gconf_entry_get_value (entry)->type == GCONF_VALUE_STRING)
        {
	  const char *rc = gconf_value_get_string (gconf_entry_get_value (entry));
	  int rcv = atoi(rc);
	  fprintf(stderr, "GTKWAVE | RPC Quit: exit return code %d\n", rcv);
	  gconf_entry_set_value(entry, NULL);
	  exit(rcv);
        }
      else
        {
          /* value is of wrong type */
        }
    }
}


static void
reload_callback(GConfClient* gclient,
                     guint cnxn_id,
                     GConfEntry *entry,
                     gpointer user_data)
{
(void)gclient;
(void)cnxn_id;
(void)user_data;

  if (gconf_entry_get_value (entry) == NULL)
    {
      /* value is unset */
    }
  else
    {
      if (gconf_entry_get_value (entry)->type == GCONF_VALUE_STRING)
        {
	  if(in_main_iteration()) return;
	  reload_into_new_context();
	  gconf_entry_set_value(entry, NULL);
        }
      else
        {
          /* value is of wrong type */
        }
    }
}


static void
zoomfull_callback(GConfClient* gclient,
                     guint cnxn_id,
                     GConfEntry *entry,
                     gpointer user_data)
{
(void)gclient;
(void)cnxn_id;
(void)user_data;

  if (gconf_entry_get_value (entry) == NULL)
    {
      /* value is unset */
    }
  else
    {
      if (gconf_entry_get_value (entry)->type == GCONF_VALUE_STRING)
        {
	  if(in_main_iteration()) return;
	  service_zoom_full(NULL, NULL);
	  gconf_entry_set_value(entry, NULL);
        }
      else
        {
          /* value is of wrong type */
        }
    }
}


static void
writesave_callback(GConfClient* gclient,
                     guint cnxn_id,
                     GConfEntry *entry,
                     gpointer user_data)
{
(void)gclient;
(void)cnxn_id;
(void)user_data;

  if (gconf_entry_get_value (entry) == NULL)
    {
      /* value is unset */
    }
  else
    {
      if (gconf_entry_get_value (entry)->type == GCONF_VALUE_STRING)
        {
	  const char *fni = gconf_value_get_string (gconf_entry_get_value (entry));
	  if(fni && !in_main_iteration())
		{
		  int use_arg = strcmp(fni, "+"); /* plus filename uses default */
		  const char *fn = use_arg ? fni : GLOBALS->filesel_writesave;
		  if(fn)
			{
		  	FILE *wave;

		  	if(!(wave=fopen(fn, "wb")))
		        	{
		        	fprintf(stderr, "GTKWAVE | RPC Writesave: error opening save file '%s' for writing.\n", fn);
		        	perror("Why");
		        	errno=0;
		        	}
		        	else
		        	{
		        	write_save_helper(fn, wave);
				if(use_arg)
					{
					if(GLOBALS->filesel_writesave) { free_2(GLOBALS->filesel_writesave); }
					GLOBALS->filesel_writesave = strdup_2(fn);
					}
		        	wave_gconf_client_set_string("/current/savefile", fn);
		        	fclose(wave);
		        	fprintf(stderr, "GTKWAVE | RPC Writesave: wrote save file '%s'.\n", GLOBALS->filesel_writesave);
		        	}
			}
		}

	  gconf_entry_set_value(entry, NULL);
        }
      else
        {
          /* value is of wrong type */
        }
    }
}


static void
move_to_time_callback(GConfClient* gclient,
                     guint cnxn_id,
                     GConfEntry *entry,
                     gpointer user_data)
{
(void)gclient;
(void)cnxn_id;
(void)user_data;

  if (gconf_entry_get_value (entry) == NULL)
    {
      /* value is unset */
    }
  else
    {
      if (gconf_entry_get_value (entry)->type == GCONF_VALUE_STRING)
        {
	  const char *str = gconf_value_get_string (gconf_entry_get_value (entry));
	  if(str && !in_main_iteration())
		{
		char *e_copy = GLOBALS->entrybox_text;
	        GLOBALS->entrybox_text=strdup_2(str);

		movetotime_cleanup(NULL, NULL);

		GLOBALS->entrybox_text = e_copy;
		}

	  gconf_entry_set_value(entry, NULL);
        }
      else
        {
          /* value is of wrong type */
        }
    }
}


static void
zoom_size_callback(GConfClient* gclient,
                     guint cnxn_id,
                     GConfEntry *entry,
                     gpointer user_data)
{
(void)gclient;
(void)cnxn_id;
(void)user_data;

  if (gconf_entry_get_value (entry) == NULL)
    {
      /* value is unset */
    }
  else
    {
      if (gconf_entry_get_value (entry)->type == GCONF_VALUE_STRING)
        {
	  const char *str = gconf_value_get_string (gconf_entry_get_value (entry));
	  if(str && !in_main_iteration())
		{
		char *e_copy = GLOBALS->entrybox_text;
	        GLOBALS->entrybox_text=strdup_2(str);

		zoomsize_cleanup(NULL, NULL);

		GLOBALS->entrybox_text = e_copy;
		}

	  gconf_entry_set_value(entry, NULL);
        }
      else
        {
          /* value is of wrong type */
        }
    }
}

/************************************************************/

static void remove_client(void)
{
if(client)
	{
	g_object_unref(client);
	client = NULL;
	}
}


void wave_gconf_init(int argc, char **argv)
{
if(!client)
	{
	char *ks = wave_alloca(WAVE_GCONF_DIR_LEN + 32 + 32 + 1);
	int len = sprintf(ks, WAVE_GCONF_DIR"/%d", wave_rpc_id);

	gconf_init(argc, argv, NULL);
	client = gconf_client_get_default();
	atexit(remove_client);

	gconf_client_add_dir(client,
		ks,
	        GCONF_CLIENT_PRELOAD_NONE,
	        NULL);

	strcpy(ks + len, "/open");
	gconf_client_notify_add(client, ks,
	                          open_callback,
	                          NULL, /* user data */
	                          NULL, NULL);

	strcpy(ks + len, "/quit");
	gconf_client_notify_add(client, ks,
	                          quit_callback,
	                          NULL, /* user data */
	                          NULL, NULL);

	strcpy(ks + len, "/writesave");
	gconf_client_notify_add(client, ks,
	                          writesave_callback,
	                          NULL, /* user data */
	                          NULL, NULL);

	strcpy(ks + len, "/reload");
	gconf_client_notify_add(client, ks,
	                          reload_callback,
	                          NULL, /* user data */
	                          NULL, NULL);

	strcpy(ks + len, "/zoom_full");
	gconf_client_notify_add(client, ks,
	                          zoomfull_callback,
	                          NULL, /* user data */
	                          NULL, NULL);

	strcpy(ks + len, "/move_to_time");
	gconf_client_notify_add(client, ks,
	                          move_to_time_callback,
	                          NULL, /* user data */
	                          NULL, NULL);

	strcpy(ks + len, "/zoom_size");
	gconf_client_notify_add(client, ks,
	                          zoom_size_callback,
	                          NULL, /* user data */
	                          NULL, NULL);
	}
}


gboolean wave_gconf_client_set_string(const gchar *key, const gchar *val)
{
if(key && client)
	{
	char *ks = wave_alloca(WAVE_GCONF_DIR_LEN + 32 + strlen(key) + 1);
	sprintf(ks, WAVE_GCONF_DIR"/%d%s", wave_rpc_id, key);

	return(gconf_client_set_string(client, ks, val ? val : "", NULL));
	}

return(FALSE);
}


static gchar *wave_gconf_client_get_string(const gchar *key)
{
if(key && client)
	{
	char *ks = wave_alloca(WAVE_GCONF_DIR_LEN + 32 + strlen(key) + 1);
	sprintf(ks, WAVE_GCONF_DIR"/%d%s", wave_rpc_id, key);

	return(gconf_client_get_string(client, ks, NULL));
	}

return(NULL);
}


void wave_gconf_restore(char **dumpfile, char **savefile, char **rcfile, char **wave_pwd, int *opt_vcd)
{
char *s;

if(dumpfile && savefile && rcfile && wave_pwd && opt_vcd)
	{
	if(*dumpfile) { free_2(*dumpfile); *dumpfile = NULL; }
	s = wave_gconf_client_get_string("/current/dumpfile");
	if(s) { if(s[0]) *dumpfile = strdup_2(s); g_free(s); }

	if(*savefile) { free_2(*savefile); *savefile = NULL; }
	s = wave_gconf_client_get_string("/current/savefile");
	if(s) { if(s[0]) *savefile = strdup_2(s); g_free(s); }

	if(*rcfile) { free_2(*rcfile); *rcfile = NULL; }
	s = wave_gconf_client_get_string("/current/rcfile");
	if(s) { if(s[0]) *rcfile = strdup_2(s); g_free(s); }

	if(*wave_pwd) { free_2(*wave_pwd); *wave_pwd = NULL; }
	s = wave_gconf_client_get_string("/current/pwd");
	if(s) { if(s[0]) *wave_pwd = strdup_2(s); g_free(s); }

	s = wave_gconf_client_get_string("/current/optimized_vcd");
	if(s) { if(s[0]) *opt_vcd = atoi(s); g_free(s); }
	}
}

#else

void wave_gconf_init(int argc, char **argv)
{
(void)argc;
(void)argv;
}

gboolean wave_gconf_client_set_string(const gchar *key, const gchar *val)
{
(void)key;
(void)val;

return(FALSE);
}

void wave_gconf_restore(char **dumpfile, char **savefile, char **rcfile, char **wave_pwd, int *opt_vcd)
{
(void)dumpfile;
(void)savefile;
(void)rcfile;
(void)wave_pwd;
(void)opt_vcd;

/* nothing */
}

#endif

#endif 

/*

Examples of RPC manipulation (gconf):

gconftool-2 --dump /com.geda.gtkwave
gconftool-2 --dump /com.geda.gtkwave --recursive-unset

gconftool-2 --type string --set /com.geda.gtkwave/0/open /pub/systema_packed.fst
gconftool-2 --type string --set /com.geda.gtkwave/0/open `pwd`/`basename -- des.gtkw`

gconftool-2 --type string --set /com.geda.gtkwave/0/writesave /tmp/this.gtkw
gconftool-2 --type string --set /com.geda.gtkwave/0/writesave +

gconftool-2 --type string --set /com.geda.gtkwave/0/quit 0
gconftool-2 --type string --set /com.geda.gtkwave/0/reload 0

gconftool-2 --type string --set /com.geda.gtkwave/0/zoom_full 0
gconftool-2 --type string --set /com.geda.gtkwave/0/zoom_size -- -4.6
gconftool-2 --type string --set /com.geda.gtkwave/0/move_to_time 123ns
gconftool-2 --type string --set /com.geda.gtkwave/0/move_to_time A


Examples of RPC manipulation (gsettings).
First number is RPC ID:

gsettings set com.geda.gtkwave open 0,/pub/systema_packed.fst

gsettings set com.geda.gtkwave writesave 0,/tmp/this.gtkw
gsettings set com.geda.gtkwave writesave 0,+

gsettings set com.geda.gtkwave quit 0,0
gsettings set com.geda.gtkwave reload 0

gsettings set com.geda.gtkwave zoom-full 0
gsettings set com.geda.gtkwave zoom-size 0,-4.6
gsettings set com.geda.gtkwave move-to-time 0,123ns
gsettings set com.geda.gtkwave move-to-time 0,A

*/
