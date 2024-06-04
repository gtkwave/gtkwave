/*
 * Copyright (c) Tony Bybell 1999-2013.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include <config.h>
#include <gtk/gtk.h>
#include "gtk23compat.h"
#include "menu.h"
#include "debug.h"
#include <string.h>
#include <stdlib.h>

void fileselbox(const char *title,
                char **filesel_path,
                GCallback ok_func,
                GCallback notok_func,
                const char *pattn,
                int is_writemode)
{
    int can_set_filename = 0;
    GtkFileChooserNative *pFileChooseNative;
    GtkWidget *pWindowMain;
    GtkFileFilter *filter;
    struct Global *old_globals = GLOBALS;
    /* fix problem where ungrab doesn't occur if button pressed + simultaneous accelerator key
     * occurs */
    if (GLOBALS->in_button_press_wavewindow_c_1) {
        XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }

    if (!*filesel_path) /* if no name specified, hijack loaded file name path */
    {
        if (GLOBALS->loaded_file_name) {
            char *can =
                realpath_2(GLOBALS->loaded_file_name,
                           NULL); /* prevents filechooser from blocking/asking where to save file */
            char *tname = strdup_2(can ? can : GLOBALS->loaded_file_name);
            char *delim = strrchr(tname, '/');
            if (!delim)
                delim = strrchr(tname, '\\');
            if (delim) {
                *(delim + 1) = 0; /* keep slash for gtk_file_chooser_set_filename vs
                                     gtk_file_chooser_set_current_folder test below */
                *filesel_path = tname;
            } else {
                free_2(tname);
            }

            free(can);
        }
    }

    if (GLOBALS->wave_script_args) {
        char *s = NULL;

        GLOBALS->fileselbox_text = filesel_path;
        GLOBALS->filesel_ok = 1;

        while ((!s) && (GLOBALS->wave_script_args->curr))
            s = wave_script_args_fgetmalloc_stripspaces(GLOBALS->wave_script_args);

        if (*GLOBALS->fileselbox_text)
            free_2(*GLOBALS->fileselbox_text);
        if (!s) {
            fprintf(stderr, "Null filename passed to fileselbox in script, exiting.\n");
            exit(255);
        }
        *GLOBALS->fileselbox_text = s;
        fprintf(stderr, "GTKWAVE | Filename '%s'\n", s);

        ok_func();
        return;
    }

    pWindowMain = GLOBALS->mainwindow;
    GLOBALS->fileselbox_text = filesel_path;
    GLOBALS->filesel_ok = 0;

    if (*GLOBALS->fileselbox_text && (!g_path_is_absolute(*GLOBALS->fileselbox_text))) {
        char *can = realpath_2(*GLOBALS->fileselbox_text, NULL);

        if (can) {
            if (*GLOBALS->fileselbox_text)
                free_2(*GLOBALS->fileselbox_text);
            *GLOBALS->fileselbox_text = (char *)malloc_2(strlen(can) + 1);
            strcpy(*GLOBALS->fileselbox_text, can);
            free(can);
            can_set_filename = 1;
        }
    } else {
        if (*GLOBALS->fileselbox_text) {
            can_set_filename = 1;
        }
    }

    if (is_writemode) {
        pFileChooseNative = gtk_file_chooser_native_new(title,
                                                        NULL,
                                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                                        XXX_GTK_STOCK_SAVE,
                                                        XXX_GTK_STOCK_CANCEL);

        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(pFileChooseNative), TRUE);
    } else {
        pFileChooseNative = gtk_file_chooser_native_new(title,
                                                        NULL,
                                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                                        XXX_GTK_STOCK_OPEN,
                                                        XXX_GTK_STOCK_CANCEL);
    }

    GLOBALS->pFileChoose = pFileChooseNative;

    if ((can_set_filename) && (*filesel_path)) {
        int flen = strlen(*filesel_path);
        if (((*filesel_path)[flen - 1] == '/') || ((*filesel_path)[flen - 1] == '\\')) {
            gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(pFileChooseNative), *filesel_path);
        } else {
            gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(pFileChooseNative), *filesel_path);
        }
    }

    if (pattn) {
        int is_gtkw = suffix_check(pattn, ".gtkw");
        int is_sav = suffix_check(pattn, ".sav");

        filter = gtk_file_filter_new();
        gtk_file_filter_add_pattern(filter, pattn);
        gtk_file_filter_set_name(filter, pattn);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(pFileChooseNative), filter);

        if (is_gtkw || is_sav) {
            const char *pattn2 = is_sav ? "*.gtkw" : "*.sav";

            filter = gtk_file_filter_new();
            gtk_file_filter_add_pattern(filter, pattn2);
            gtk_file_filter_set_name(filter, pattn2);
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(pFileChooseNative), filter);
        }

        if (strcmp(pattn, "*")) {
            filter = gtk_file_filter_new();
            gtk_file_filter_add_pattern(filter, "*");
            gtk_file_filter_set_name(filter, "*");
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(pFileChooseNative), filter);
        }
    }

    if (pWindowMain) {
        gtk_native_dialog_set_transient_for(GTK_NATIVE_DIALOG(pFileChooseNative),
                                            GTK_WINDOW(pWindowMain));
    }
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(pFileChooseNative), TRUE);
#ifdef MAC_INTEGRATION
    osx_menu_sensitivity(FALSE);
#endif

    /* check against old_globals is because of DnD context swapping so make response fail */

    if ((gtk_native_dialog_run(GTK_NATIVE_DIALOG(pFileChooseNative)) == GTK_RESPONSE_ACCEPT) &&
        (GLOBALS == old_globals) && (GLOBALS->fileselbox_text)) {
        const char *allocbuf;
        int alloclen;

        allocbuf = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(pFileChooseNative));
        if ((alloclen = strlen(allocbuf))) {
            int gtkw_test = 0;

            GLOBALS->filesel_ok = 1;

            if (*GLOBALS->fileselbox_text)
                free_2(*GLOBALS->fileselbox_text);
            *GLOBALS->fileselbox_text = (char *)malloc_2(alloclen + 1);
            strcpy(*GLOBALS->fileselbox_text, allocbuf);

            /* add missing suffix to write files */
            if (pattn && is_writemode) {
                char *s = *GLOBALS->fileselbox_text;
                char *s2;
                char *suffix = g_alloca(strlen(pattn) + 1);
                char *term;
                int attempt_suffix = 1;

                strcpy(suffix, pattn);
                while ((*suffix) && (*suffix != '.'))
                    suffix++;
                term = *suffix ? suffix + 1 : suffix;
                while ((*term) && (isalnum((int)(unsigned char)*term)))
                    term++;
                *term = 0;

                gtkw_test = suffix_check(pattn, ".gtkw") || suffix_check(pattn, ".sav");

                if (gtkw_test) {
                    attempt_suffix = (!suffix_check(s, ".gtkw")) && (!suffix_check(s, ".sav"));
                }

                if (attempt_suffix) {
                    if (strlen(s) > strlen(suffix)) {
                        if (strcmp(s + strlen(s) - strlen(suffix), suffix)) {
                            goto fix_suffix;
                        }
                    } else {
                    fix_suffix:
                        s2 = malloc_2(strlen(s) + strlen(suffix) + 1);
                        strcpy(s2, s);
                        strcat(s2, suffix);
                        free_2(s);
                        *GLOBALS->fileselbox_text = s2;
                    }
                }
            }

            if ((gtkw_test) && (*GLOBALS->fileselbox_text)) {
                GLOBALS->is_gtkw_save_file = suffix_check(*GLOBALS->fileselbox_text, ".gtkw");
            }
        }
        DEBUG(printf("Filesel OK %s\n", allocbuf));
#ifdef MAC_INTEGRATION
        osx_menu_sensitivity(TRUE);
#endif
        gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(pFileChooseNative));
        g_object_unref(pFileChooseNative);
        GLOBALS->pFileChoose = NULL; /* keeps DND from firing */
        gtkwave_main_iteration();
        ok_func();
    } else {
        DEBUG(printf("Filesel Entry Cancel\n"));
#ifdef MAC_INTEGRATION
        osx_menu_sensitivity(TRUE);
#endif
        gtk_native_dialog_destroy(GTK_NATIVE_DIALOG(pFileChooseNative));
        g_object_unref(pFileChooseNative);
        GLOBALS->pFileChoose = NULL; /* keeps DND from firing */
        gtkwave_main_iteration();
        if (notok_func)
            notok_func();
    }
}
