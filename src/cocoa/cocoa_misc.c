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
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:nspath]];
}

/*************************/
/* simplereq.c           */
/*************************/

static int gtk_simplereqbox_req_bridge_2(const char *title,
                                         const char *default_text,
                                         const char *oktext,
                                         const char *canceltext,
                                         int is_alert,
                                         int is_entry,
                                         const char *default_in_text_entry,
                                         char **out_text_entry,
                                         int width)
{
    NSAlert *alert = [[[NSAlert alloc] init] autorelease];
    int rc = 0;

    if (oktext) {
        [alert addButtonWithTitle:[NSString stringWithUTF8String:oktext]];

        if (canceltext) {
            [alert addButtonWithTitle:[NSString stringWithUTF8String:canceltext]];
        }
    }

    if (title) {
        [alert setMessageText:[NSString stringWithUTF8String:title]];
    }

    if (default_text) {
        [alert setInformativeText:[NSString stringWithUTF8String:default_text]];
    }

    if (is_alert) {
        [alert setAlertStyle:NSAlertStyleCritical];
    } else {
        [alert setAlertStyle:NSAlertStyleInformational];
    }

    NSTextField *input = nil;
    if (is_entry && default_in_text_entry && out_text_entry) {
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
    if (zIntResult == NSAlertFirstButtonReturn) {
        rc = 1;
        if (is_entry && default_in_text_entry && out_text_entry) {
            [input validateEditing];
            *out_text_entry = strdup([[input stringValue] UTF8String]);
        }
    } else if (zIntResult == NSAlertSecondButtonReturn) {
        rc = 2;
    }

    return (rc);
}

int gtk_simplereqbox_req_bridge(const char *title,
                                const char *default_text,
                                const char *oktext,
                                const char *canceltext,
                                int is_alert)
{
    return (gtk_simplereqbox_req_bridge_2(title,
                                          default_text,
                                          oktext,
                                          canceltext,
                                          is_alert,
                                          0,
                                          NULL,
                                          0,
                                          0));
}

#else

char *cocoa_misc_dummy_compilation_unit(void)
{
    return (NULL); /* dummy compilation unit */
}

#endif
