/*
 * Copyright (c) Tony Bybell 2012.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "cocoa_misc.h"

#ifdef WAVE_COCOA_GTK

#include "alert_sheet.m"
#include <sys/stat.h>

/*************************/
/* menu.c                */
/*************************/

void gtk_open_external_file(const char *fpath)
{
NSString *nspath = [NSString stringWithUTF8String:fpath];
[[NSWorkspace sharedWorkspace] openFile:nspath];
}


/*************************/
/* file.c                */
/*************************/

static char *gtk_open_file_req_bridge(const char *title, const char *fpath, const char *pattn)
{
        NSOpenPanel * zOpenPanel = [NSOpenPanel openPanel];
	[zOpenPanel setCanChooseDirectories:NO];
	[zOpenPanel setCanChooseFiles:YES];
	[zOpenPanel setAllowsMultipleSelection:NO];
	[zOpenPanel setTreatsFilePackagesAsDirectories:YES];

	NSString *nstitle = [NSString stringWithUTF8String:title];
	[zOpenPanel setTitle:nstitle];

        NSArray * zAryOfExtensions = nil;

	if(pattn)
		{
		const char *d = strchr(pattn, '.');
		if(d)
			{
			const char *pattn2 = d+1;
			if(!strcasecmp(pattn2, "sav") || !strcasecmp(pattn2, "gtkw"))
				{
			        zAryOfExtensions = [NSArray arrayWithObjects:@"gtkw", @"sav", nil];
				}
				else
				{
				NSString *s = [NSString stringWithUTF8String:pattn2];
			        zAryOfExtensions = [NSArray arrayWithObjects:s,nil];
				}
			}
		}

	NSString *p_path = nil;
	NSString *p_file = nil;

	if(fpath)
		{
		char *s_temp = realpath(fpath,NULL);

		if(s_temp)
			{
			struct stat sbuf;
			if(!stat(s_temp,&sbuf))
				{
				if(S_ISDIR(sbuf.st_mode))
					{
					p_path = [NSString stringWithUTF8String:s_temp];
					NSURL *URL = [NSURL URLWithString:p_path];
					[zOpenPanel setDirectoryURL:URL];
					}
					else
					{
					p_path = [[NSString stringWithUTF8String:s_temp] stringByDeletingLastPathComponent];
					p_file = [[NSString stringWithUTF8String:s_temp] lastPathComponent];
					NSURL *URL = [NSURL URLWithString:p_path];
					[zOpenPanel setDirectoryURL:URL];
					}
				}

			free(s_temp);
			}
		}	

        NSInteger zIntResult = [zOpenPanel runModalForDirectory:p_path
		file:p_file
		types:zAryOfExtensions];

        if (zIntResult == NSFileHandlingPanelCancelButton) {
                return(NULL);
        } 

        NSURL *zUrl = [zOpenPanel URL];
	NSString *us = [zUrl absoluteString];
	const char *cst = [us UTF8String];
	return(g_filename_from_uri(cst, NULL, NULL));
}

static char *gtk_save_file_req_bridge(const char *title, const char *fpath, const char *pattn)
{
        NSSavePanel * zSavePanel = [NSSavePanel savePanel];
	[zSavePanel setAllowsOtherFileTypes:YES];
	[zSavePanel setTreatsFilePackagesAsDirectories:YES];
	[zSavePanel setExtensionHidden:NO];

	NSString *nstitle = [NSString stringWithUTF8String:title];
	[zSavePanel setTitle:nstitle];

        NSArray * zAryOfExtensions = nil;

	if(pattn)
		{
		const char *d = strchr(pattn, '.');
		if(d)
			{
			const char *pattn2 = d+1;
			if(!strcasecmp(pattn2, "sav") || !strcasecmp(pattn2, "gtkw"))
				{
			        zAryOfExtensions = [NSArray arrayWithObjects:@"gtkw", @"sav", nil];
				}
				else
				{
				NSString *s = [NSString stringWithUTF8String:pattn2];
			        zAryOfExtensions = [NSArray arrayWithObjects:s,nil];
				}
			}
		}

        [zSavePanel setAllowedFileTypes:zAryOfExtensions];

	NSString *p_path = nil;
	NSString *p_file = nil;

	if(fpath)
		{
		char *s_temp = realpath(fpath,NULL);

		if(s_temp)
			{
			struct stat sbuf;
			if(!stat(s_temp,&sbuf))
				{
				if(S_ISDIR(sbuf.st_mode))
					{
					p_path = [NSString stringWithUTF8String:s_temp];
					NSURL *URL = [NSURL URLWithString:p_path];
					[zSavePanel setDirectoryURL:URL];
					}
					else
					{
					p_path = [[NSString stringWithUTF8String:s_temp] stringByDeletingLastPathComponent];
					p_file = [[NSString stringWithUTF8String:s_temp] lastPathComponent];
					NSURL *URL = [NSURL URLWithString:p_path];
					[zSavePanel setDirectoryURL:URL];
					[zSavePanel setNameFieldStringValue:p_file];
					}
				}

			free(s_temp);
			}
		}	


        NSInteger zIntResult = [zSavePanel runModal];

        if (zIntResult == NSFileHandlingPanelCancelButton) {
                return(NULL);
        } 
        
        NSURL *zUrl = [zSavePanel URL];
	NSString *us = [zUrl absoluteString];
	const char *cst = [us UTF8String];
	return(g_filename_from_uri(cst, NULL, NULL));
}


char *gtk_file_req_bridge(const char *title, const char *fpath, const char *pattn, int is_writemode)
{
char *rc;

if(is_writemode)
	{
	rc = gtk_save_file_req_bridge(title, fpath, pattn);
	}	
	else
	{
	rc = gtk_open_file_req_bridge(title, fpath, pattn);
	}

return(rc);
}


/*************************/
/* simplereq.c / entry.c */
/*************************/

static int gtk_simplereqbox_req_bridge_2(char *title, char *default_text, char *oktext, char *canceltext, 
	int is_alert, int is_entry, char *default_in_text_entry, char **out_text_entry, int width)
{
NSAlert *alert = [[[NSAlert alloc] init] autorelease];
int rc = 0;

if(oktext)
	{
	[alert addButtonWithTitle: [NSString stringWithUTF8String:oktext]];

	if(canceltext)
		{
		[alert addButtonWithTitle: [NSString stringWithUTF8String:canceltext]];
		}
	}

if(title)
	{
	[alert setMessageText: [NSString stringWithUTF8String:title]];
	}

if(default_text)
	{
	[alert setInformativeText: [NSString stringWithUTF8String:default_text]];
	}

if(is_alert)
	{
	[alert setAlertStyle:NSCriticalAlertStyle];
	}
	else
	{
	[alert setAlertStyle:NSInformationalAlertStyle];
	}

NSTextField *input = nil;
if(is_entry && default_in_text_entry && out_text_entry)
	{
	input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, width, 24)];
	[input setSelectable:YES];
	[input setEditable:YES];
	[input setImportsGraphics:NO];
	[[alert window] makeFirstResponder:input];
	[input setStringValue:[NSString stringWithUTF8String:default_in_text_entry]];
	[input selectText:input];
	[alert setAccessoryView:input];
	}

NSInteger zIntResult = [alert runModalSheet];
if(zIntResult == NSAlertFirstButtonReturn)
	{
	rc = 1;
	if(is_entry && default_in_text_entry && out_text_entry)
		{
		[input validateEditing];
		*out_text_entry = strdup([[input stringValue] UTF8String]);
		}
	}
else
if(zIntResult == NSAlertSecondButtonReturn)
	{
	rc = 2;
	}

return(rc);
}


int gtk_simplereqbox_req_bridge(char *title, char *default_text, char *oktext, char *canceltext, int is_alert)
{
return(gtk_simplereqbox_req_bridge_2(title, default_text, oktext, canceltext, is_alert, 0, NULL, 0, 0));
}


int entrybox_req_bridge(char *title, int width, char *dflt_text, char *comment, int maxch, char **out_text_entry)
{
int rc = gtk_simplereqbox_req_bridge_2(title, comment, "OK", "Cancel", 
	0, 1, dflt_text, out_text_entry, width);

if((rc == 1)&&(*out_text_entry))
	{
	int len = strlen(*out_text_entry);
	if(len > maxch)
		{
		char *s2 = calloc(1, maxch+1);
		memcpy(s2, *out_text_entry, maxch);
		free(*out_text_entry);
		*out_text_entry = s2;
		}
	}

return(rc);
}


#else

char *cocoa_misc_dummy_compilation_unit(void)
{
return(NULL); /* dummy compilation unit */
}

#endif
