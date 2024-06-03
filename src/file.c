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
/* #include <fnmatch.h> */
#ifdef MAC_INTEGRATION
#include <cocoa_misc.h>
#endif


void fileselbox_old(const char *title,
                    char **filesel_path,
                    GCallback ok_func,
                    GCallback notok_func,
                    const char *pattn,
                    int is_writemode)
{
#if defined __MINGW32__
    OPENFILENAME ofn;
    char szFile[260]; /* buffer for file name */
    char szPath[260]; /* buffer for path name */
    char lpstrFilter[260]; /* more than enough room for some patterns */
    BOOL rc;

    GLOBALS->fileselbox_text = filesel_path;
    GLOBALS->filesel_ok = 0;
    GLOBALS->cleanup_file_c_2 = ok_func;
    GLOBALS->bad_cleanup_file_c_1 = notok_func;

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.lpstrFilter = lpstrFilter;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = is_writemode ? (OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT)
                             : (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);

    if ((!pattn) || (!strcmp(pattn, "*"))) {
        sprintf(lpstrFilter, "%s%c%s%c", "All", 0, "*.*", 0);
        ofn.nFilterIndex = 0;
    } else {
        sprintf(lpstrFilter, "%s%c%s%c%s%c%s%c", pattn, 0, pattn, 0, "All", 0, "*.*", 0); /* cppcheck
                                                                                           */
        ofn.nFilterIndex = 0;
    }

    if (*filesel_path) {
        char *fsp = *filesel_path;
        int ch_idx = 0;
        char ch = 0;

        while (*fsp) {
            ch = *fsp;
            szFile[ch_idx++] = (ch != '/') ? ch : '\\';
            fsp++;
        }

        szFile[ch_idx] = 0;

        if ((ch == '/') || (ch == '\\')) {
            strcpy(szPath, szFile);
            szFile[0] = 0;
            ofn.lpstrInitialDir = szPath;
        }
    }

    rc = is_writemode ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn);

    if (rc == TRUE) {
        GLOBALS->filesel_ok = 1;
        if (*GLOBALS->fileselbox_text)
            free_2(*GLOBALS->fileselbox_text);
        if (!is_writemode) {
            *GLOBALS->fileselbox_text = (char *)strdup_2(szFile);
        } else {
            char *suf_str = NULL;
            int suf_add = 0;
            int szlen = 0;
            int suflen = 0;

            if (pattn) {
                suf_str = strstr(pattn, "*.");
                if (suf_str)
                    suf_str++;
            }

            if (suf_str) {
                szlen = strlen(szFile);
                suflen = strlen(suf_str);
                if (suflen > szlen) {
                    suf_add = 1;
                } else {
                    if (strcasecmp(szFile + (szlen - suflen), suf_str)) {
                        suf_add = 1;
                    }
                }
            }

            if (suf_str && suf_add) {
                *GLOBALS->fileselbox_text = (char *)malloc_2(szlen + suflen + 1);
                strcpy(*GLOBALS->fileselbox_text, szFile);
                strcpy(*GLOBALS->fileselbox_text + szlen, suf_str);
            } else {
                *GLOBALS->fileselbox_text = (char *)strdup_2(szFile);
            }
        }

        GLOBALS->cleanup_file_c_2();
    } else {
        if (GLOBALS->bad_cleanup_file_c_1)
            GLOBALS->bad_cleanup_file_c_1();
    }

#else
    (void)title;
    (void)filesel_path;
    (void)ok_func;
    (void)notok_func;
    (void)pattn;
    (void)is_writemode;

    fprintf(stderr, "fileselbox_old no longer supported, exiting.\n");
    exit(255);

#endif
}

void fileselbox(const char *title,
                char **filesel_path,
                GCallback ok_func,
                GCallback notok_func,
                const char *pattn,
                int is_writemode)
{
#ifndef MAC_INTEGRATION
    int can_set_filename = 0;
    GtkFileChooserNative *pFileChooseNative;
    GtkWidget *pWindowMain;
    GtkFileFilter *filter;
    struct Global *old_globals = GLOBALS;
#endif

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

#ifdef MAC_INTEGRATION

    GLOBALS->fileselbox_text = filesel_path;
    GLOBALS->filesel_ok = 0;
    char *rc = gtk_file_req_bridge(title, *filesel_path, pattn, is_writemode);
    if (rc) {
        GLOBALS->filesel_ok = 1;

        if (*GLOBALS->fileselbox_text)
            free_2(*GLOBALS->fileselbox_text);
        *GLOBALS->fileselbox_text = strdup_2(rc);
        g_free(rc);
        if (ok_func)
            ok_func();
    } else {
        if (notok_func)
            notok_func();
    }
    return;

#else

#if defined __MINGW32__

    fileselbox_old(title, filesel_path, ok_func, notok_func, pattn, is_writemode);
    return;

#else

    pWindowMain = GLOBALS->mainwindow;
    GLOBALS->fileselbox_text = filesel_path;
    GLOBALS->filesel_ok = 0;

    if (*GLOBALS->fileselbox_text && (!g_path_is_absolute(*GLOBALS->fileselbox_text))) {
#if defined __USE_BSD || defined __USE_XOPEN_EXTENDED || defined __CYGWIN__ || defined HAVE_REALPATH
        char *can = realpath_2(*GLOBALS->fileselbox_text, NULL);

        if (can) {
            if (*GLOBALS->fileselbox_text)
                free_2(*GLOBALS->fileselbox_text);
            *GLOBALS->fileselbox_text = (char *)malloc_2(strlen(can) + 1);
            strcpy(*GLOBALS->fileselbox_text, can);
            free(can);
            can_set_filename = 1;
        }
#else
#if __GNUC__
#warning Absolute file path warnings might be issued by the file chooser dialogue on this system!
#endif
#endif
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
                                                  XXX_GTK_STOCK_CANCEL
                                                  );

        gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(pFileChooseNative), TRUE);
    } else {
        pFileChooseNative = gtk_file_chooser_native_new(title,
                                                  NULL,
                                                  GTK_FILE_CHOOSER_ACTION_OPEN,
                                                  XXX_GTK_STOCK_OPEN,
                                                  XXX_GTK_STOCK_CANCEL
                                                  );
    }

    //GLOBALS->pFileChoose = pFileChoose;

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
        gtk_native_dialog_set_transient_for(GTK_NATIVE_DIALOG(pFileChooseNative), GTK_WINDOW(pWindowMain));
    }
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(pFileChooseNative),TRUE);
    //wave_gtk_grab_add(pFileChoose);

    /* check against old_globals is because of DnD context swapping so make response fail */
// 这里处理了一种奇怪的情况，当用户在拖放的同时打开了文件选择器并且释放了拖放，则文件选择器的任何操作不会生效
    if ((gtk_native_dialog_run(GTK_NATIVE_DIALOG(pFileChooseNative)) == GTK_RESPONSE_ACCEPT) &&
        (GLOBALS == old_globals) && (GLOBALS->fileselbox_text)) {
        const char *allocbuf;
        int alloclen;
//获取选中的文件名并计算其长度。如果长度有效（大于零），继续处理。
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

        (printf("Filesel OK %s\n", allocbuf));
        //wave_gtk_grab_remove(pFileChoose);
        //gtk_widget_destroy(pFileChoose);
        GLOBALS->pFileChoose = NULL; /* keeps DND from firing */

        gtkwave_main_iteration();
        ok_func();
    } else {
        (printf("Filesel Entry Cancel\n"));
        //wave_gtk_grab_remove(pFileChoose);
        //gtk_widget_destroy(pFileChoose);
        GLOBALS->pFileChoose = NULL; /* keeps DND from firing */

        gtkwave_main_iteration();
        if (GLOBALS->bad_cleanup_file_c_1)
            notok_func();
    }

#endif

#endif
}
