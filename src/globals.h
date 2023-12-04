/*
 * Copyright (c) Kermin Elliott Fleming 2007-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>

#if defined __MINGW32__
#include <windows.h>
#include <io.h>
#endif

#define XXX_GDK_DRAWABLE(x) x

#include <jrb.h>

#include "analyzer.h"
#include "bsearch.h"
#include "busy.h"
#include "clipping.h"
#include "color.h"
#include "currenttime.h"
#include "debug.h"
#include "fgetdynamic.h"
#include "fonts.h"
#include "fstapi.h"
#include "gconf.h"
#include "ghw.h"
#include "gtk23compat.h"
#include "main.h"
#include "memory.h"
#include "menu.h"
#include "pipeio.h"
#include "pixmaps.h"
#include "print.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "rc.h"
#include "regex_wave.h"
#include "savefile.h"
#include "strace.h"
#include "symbol.h"
#include "tcl_helper.h"
#include "translate.h"
#include "tree.h"
#include "vcd.h"
#include "vcd_saver.h"
#include "vlist.h"
#include "version.h"
#include "extload.h"
#include "fst.h"

#ifdef _WAVE_HAVE_JUDY
#include <Judy.h>
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
#ifdef GDK_WINDOWING_X11
#ifndef X_H
#include <X11/X.h>
#endif
#endif
#endif

struct Global
{
    GwProject *project;
    GwStems *stems;

    /*
     * analyzer.c
     */
    TraceFlagsType default_flags; /* from analyzer.c 5 */
    unsigned int default_fpshift;
    Times tims; /* from analyzer.c 6 */
    Traces traces; /* from analyzer.c 7 */
    int hier_max_level; /* from analyzer.c 8 */
    int hier_max_level_shadow; /* from analyzer.c */
    GwTime timestart_from_savefile;
    char timestart_from_savefile_valid;
    int group_depth;
    char hier_ignore_escapes;

    /*
     * baseconvert.c
     */
    char color_active_in_filter; /* from baseconvert.c 9 */

    /*
     * bsearch.c
     */
    GwTime shift_timebase; /* from bsearch.c 10 */
    GwTime shift_timebase_default_for_add; /* from bsearch.c 11 */
    GwTime max_compare_time_tc_bsearch_c_1; /* from bsearch.c 12 */
    GwTime *max_compare_pos_tc_bsearch_c_1; /* from bsearch.c 13 */
    GwTime max_compare_time_bsearch_c_1; /* from bsearch.c 14 */
    GwHistEnt *max_compare_pos_bsearch_c_1; /* from bsearch.c 15 */
    GwHistEnt **max_compare_index; /* from bsearch.c 16 */
    GwTime vmax_compare_time_bsearch_c_1; /* from bsearch.c 17 */
    GwVectorEnt *vmax_compare_pos_bsearch_c_1; /* from bsearch.c 18 */
    GwVectorEnt **vmax_compare_index; /* from bsearch.c 19 */
    int maxlen_trunc; /* from bsearch.c 20 */
    char *maxlen_trunc_pos_bsearch_c_1; /* from bsearch.c 21 */
    char *trunc_asciibase_bsearch_c_1; /* from bsearch.c 22 */

    /*
     * busy.c
     */
    GdkCursor *busycursor_busy_c_1; /* from busy.c 23 */
    int busy_busy_c_1; /* from busy.c 24 */

    /*
     * color.c
     */
    char keep_xz_colors;
    GwColorTheme *color_theme;

    /*
     * currenttime.c
     */
    GwTime global_time_offset;
    char is_vcd; /* from currenttime.c 56 */
    char partial_vcd; /* from currenttime.c 57 */
    char use_maxtime_display; /* from currenttime.c 58 */
    char use_frequency_delta; /* from currenttime.c 59 */
    GwTime cached_currenttimeval_currenttime_c_1; /* from currenttime.c 62 */
    GwTime max_time; /* from currenttime.c 64 */
    GwTime min_time; /* from currenttime.c 65 */
    char display_grid; /* from currenttime.c 66 */
    char fullscreen;
    char show_toolbar;
    GtkWidget *time_box;
    GwTime time_scale; /* from currenttime.c 67 */
    char time_dimension; /* from currenttime.c 68 */
    char scale_to_time_dimension; /* from currenttime.c */
    GwTime time_trunc_val_currenttime_c_1; /* from currenttime.c 77 */
    char use_full_precision; /* from currenttime.c 78 */

    /*
     * debug.c
     */
    void **alloc2_chain; /* from debug.c */
    int outstanding; /* from debug.c */
    const char *atoi_cont_ptr; /* from debug.c 79 */
    char disable_tooltips; /* from debug.c 80 */

    /*
     * entry.c
     */
    char *entrybox_text; /* from entry.c 83 */

    /* extload.c */
    unsigned int extload_ffr_import_count; /* from extload.c */
    void *extload_ffr_ctx; /* from extload.c */
    FILE *extload; /* from extload.c */
    unsigned int *extload_idcodes; /* from extload.c */
    int *extload_inv_idcodes; /* from extload.c */
#if !defined __MINGW32__
    time_t extload_lastmod; /* from extload.c */
    char extload_already_errored; /* from extload.c */
#endif
    char **extload_namecache;
    int *extload_namecache_max;
    int *extload_namecache_lens;
    int *extload_namecache_patched;
    struct symbol *extload_sym_block;
    GwNode *extload_node_block;
    void *extload_xc;
    struct symbol *extload_prevsymroot;
    struct symbol *extload_prevsym;
    GwTree **extload_npar;
    int extload_i;
    int extload_hlen;
    unsigned char extload_vt_prev;
    unsigned char extload_vd_prev;
    int f_name_build_buf_len;
    char *f_name_build_buf;
    unsigned int extload_max_tree;
    unsigned int extload_curr_tree;

    /*
     * fetchbuttons.c
     */
    GwTime fetchwindow; /* from fetchbuttons.c 85 */

    /*
     * fgetdynamic.c
     */
    int fgetmalloc_len; /* from fgetdynamic.c 86 */

    /*
     * file.c
     */
    GtkWidget *pFileChoose;
    char *pFileChooseFilterName;
    GPatternSpec *pPatternSpec;
    GtkWidget *fs_file_c_1; /* from file.c 87 */
    char **fileselbox_text; /* from file.c 88 */
    char filesel_ok; /* from file.c 89 */
    void (*cleanup_file_c_2)(void); /* from file.c 90 */
    void (*bad_cleanup_file_c_1)(void); /* from file.c 91 */

    /*
     * fonts.c
     */
    char *fontname_signals; /* from fonts.c 92 */
    char *fontname_waves; /* from fonts.c 93 */
    GdkScreen *fonts_screen;
    PangoContext *fonts_context;
    PangoLayout *fonts_layout;
    char use_pango_fonts;

    /*
     * fst.c
     */
    FstFile *fst_file;
    char nonimplicit_direction_encountered;
    char supplemental_datatypes_encountered;
    char supplemental_vartypes_encountered;
    char is_vhdl_component_format;
#ifdef _WAVE_HAVE_JUDY
    Pvoid_t *xl_enum_filter;
#else
    struct xl_tree_node **xl_enum_filter;
#endif
    int num_xl_enum_filter;
    JRB enum_nptrs_jrb;

    /*
     * ghw.c
     */
    char is_ghw; /* from ghw.c 102 */

    /*
     * globals.c
     */
    struct Global **
        *dead_context; /* for deallocating tabbed contexts later (when no race conditions exist) */
    struct Global **gtk_context_bridge_ptr; /* from globals.c, migrates to reloaded contexts to link
                                               buttons to ctx */

    /*
     * hierpack.c
     */
    unsigned char *hp_buf;
    size_t *hp_offs;
    size_t hp_prev;
    size_t hp_buf_siz;
    unsigned char *fmem_buf;
    size_t fmem_buf_siz;
    size_t fmem_buf_offs;
    size_t fmem_uncompressed_siz;
    char disable_auto_comphier;

    /*
     * logfile.c
     */
    void **logfiles;
    char *fontname_logfile; /* from logfile.c 137 */
    GtkTextIter iter_logfile_c_2; /* from logfile.c 139 */
    GtkTextTag *bold_tag_logfile_c_2; /* from logfile.c 140 */
    GtkTextTag *mono_tag_logfile_c_1; /* from logfile.c 141 */
    GtkTextTag *size_tag_logfile_c_1; /* from logfile.c 142 */

    /*
     * lx2.c
     */
    unsigned char is_lx2; /* from lx2.c 143 */

    /*
     * main.c
     */
    char is_gtkw_save_file;
    gboolean dumpfile_is_modified;
    GtkWidget *missing_file_toolbar;
    char *argvlist;
#ifdef HAVE_LIBTCL
    Tcl_Interp *interp;
#endif
    char *repscript_name;
    unsigned int repscript_period;
    char *tcl_init_cmd;
    char tcl_running;
    char block_xy_update;
    char *winname;
    unsigned int num_notebook_pages;
    unsigned int num_notebook_pages_cumulative;
    unsigned char context_tabposition;
    unsigned int this_context_page;
    unsigned char second_page_created;
    struct Global ***contexts;
    GtkWidget *notebook;
    char *loaded_file_name;
    char *unoptimized_vcd_file_name;
    char *skip_start;
    char *skip_end;
    enum FileType loaded_file_type;
    char is_optimized_stdin_vcd;
    char *whoami; /* from main.c 201 */
    struct logfile_chain *logfile; /* from main.c 202 */
    char *stems_name; /* from main.c 203 */
    int stems_type; /* from main.c 204 */
    char *aet_name; /* from main.c 205 */
    struct gtkwave_annotate_ipc_t *anno_ctx; /* from main.c 206 */
    struct gtkwave_dual_ipc_t *dual_ctx; /* from main.c 207 */
    int dual_id; /* from main.c 208 */
    unsigned int dual_attach_id_main_c_1; /* from main.c 209 */
    int dual_race_lock; /* from main.c 210 */
    GtkWidget *mainwindow; /* from main.c 211 */
    GtkWidget *signalwindow; /* from main.c 212 */
    GtkWidget *wavewindow; /* from main.c 213 */
    GtkWidget *toppanedwindow; /* from main.c 214 */
    GtkWidget *panedwindow;
    gint toppanedwindow_size_cache;
    gint panedwindow_size_cache;
    gint vpanedwindow_size_cache;
    GtkWidget *sstpane; /* from main.c 215 */
    GtkWidget *expanderwindow; /* from main.c 216 */
    char disable_window_manager; /* from main.c 217 */
    char disable_empty_gui; /* from main.c */
    char paned_pack_semantics; /* from main.c 218 */
    char zoom_was_explicitly_set; /* from main.c 219 */
    int initial_window_x; /* from main.c 220 */
    int initial_window_y; /* from main.c 221 */
    int initial_window_width; /* from main.c 222 */
    int initial_window_height; /* from main.c 223 */
    int xy_ignore_main_c_1; /* from main.c 224 */
    int optimize_vcd; /* from main.c 225 */
    int num_cpus; /* from main.c 226 */
    int initial_window_xpos; /* from main.c 227 */
    int initial_window_ypos; /* from main.c 228 */
    int initial_window_set_valid; /* from main.c 229 */
    int initial_window_xpos_set; /* from main.c 230 */
    int initial_window_ypos_set; /* from main.c 231 */
    int initial_window_get_valid; /* from main.c 232 */
    int initial_window_xpos_get; /* from main.c 233 */
    int initial_window_ypos_get; /* from main.c 234 */
    int xpos_delta; /* from main.c 235 */
    int ypos_delta; /* from main.c 236 */
    int sst_expanded; /* from main.c 240 */
#if GTK_CHECK_VERSION(3, 0, 0)
#ifdef GDK_WINDOWING_X11
    Window socket_xid; /* from main.c 241 */
#else
    unsigned long socket_xid; /* for windows compiles */
#endif
#else
    GdkNativeWindow socket_xid; /* from main.c 241 */
#endif
    int disable_menus; /* from main.c 242 */
    char *ftext_main_main_c_1; /* from main.c 243 */
    char dbl_mant_dig_override; /* from main.c */

    /*
     * menu.c
     */
    char *cutcopylist; /* from menu.c */
    char enable_fast_exit; /* from menu.c 253 */
    char quiet_checkmenu;
    struct wave_script_args *wave_script_args; /* from tcl_helper.c */
    char ignore_savefile_pane_pos;
    char ignore_savefile_pos; /* from menu.c 255 */
    char ignore_savefile_size; /* from menu.c 256 */
    char *regexp_string_menu_c_1; /* from menu.c 259 */
    GwTrace *trace_to_alias_menu_c_1; /* from menu.c 260 */
    GwTrace *showchangeall_menu_c_1; /* from menu.c 261 */
    char *filesel_newviewer_menu_c_1; /* from menu.c 262 */
    char *filesel_logfile_menu_c_1; /* from menu.c 263 */
    char *filesel_scriptfile_menu; /* from menu.c */
    char *filesel_writesave; /* from menu.c 264 */
    char *filesel_imagegrab; /* from menu.c */
    char save_success_menu_c_1; /* from menu.c 265 */
    char *filesel_vcd_writesave; /* from menu.c 266 */
    char *filesel_tim_writesave; /* from menu.c */
    int lock_menu_c_1; /* from menu.c 268 */
    int lock_menu_c_2; /* from menu.c 269 */
    char *buf_menu_c_1; /* from menu.c 270 */
    GtkWidget *signal_popup_menu; /* from menu.c */
    GtkWidget *sst_signal_popup_menu; /* from menu.c */

    /*
     * mouseover.c
     */
    char disable_mouseover; /* from mouseover.c 271 */
    char clipboard_mouseover; /* from mouseover.c */
    GtkWidget *mouseover_mouseover_c_1; /* from mouseover.c 272 */

    /*
     * pagebuttons.c
     */
    double page_divisor; /* from pagebuttons.c 279 */

    /*
     * pixmaps.c
     */
    GwHierarchyIcons *hierarchy_icons;

    /*
     * print.c
     */
    int inch_print_c_1; /* from print.c 316 */
    double ps_chwidth_print_c_1; /* from print.c 317 */
    double ybound_print_c_1; /* from print.c 318 */
    int pr_signal_fill_width_print_c_1; /* from print.c 319 */
    int ps_nummaxchars_print_c_1; /* from print.c 320 */
    char ps_fullpage; /* from print.c 321 */
    int ps_maxveclen; /* from print.c 322 */
    int liney_max; /* from print.c 323 */

    /*
     * ptranslate.c
     */
    int current_translate_proc; /* from ptranslate.c 326 */
    int current_filter_ptranslate_c_1; /* from ptranslate.c 327 */
    int num_proc_filters; /* from ptranslate.c 328 */
    char **procsel_filter; /* from ptranslate.c 329 */
    struct pipe_ctx **proc_filter; /* from ptranslate.c 330 */
    int is_active_ptranslate_c_2; /* from ptranslate.c 331 */
    char *fcurr_ptranslate_c_1; /* from ptranslate.c 332 */
    GtkWidget *window_ptranslate_c_5; /* from ptranslate.c 333 */
    GtkListStore *sig_store_ptranslate;
    GtkTreeSelection *sig_selection_ptranslate;

    /*
     * rc.c
     */
    int rc_line_no; /* from rc.c 336 */
    int possibly_use_rc_defaults; /* from rc.c 337 */
    char *editor_name; /* from rc.c */
    gboolean editor_run_in_terminal; /* from rc.c */

    /*
     * regex.c
     */
    regex_t *preg_regex_c_1; /* from regex.c 339 */
    int *regex_ok_regex_c_1; /* from regex.c 340 */

/*
 * renderopt.c
 */
#ifdef WAVE_GTK_UNIX_PRINT
    GtkPrintSettings *gprs;
    GtkPageSetup *gps;
    char *gp_tfn;
#endif
    char is_active_renderopt_c_3; /* from renderopt.c 341 */
    GtkWidget *window_renderopt_c_6; /* from renderopt.c 342 */
    char *filesel_print_pdf_renderopt_c_1; /* from renderopt.c */
    char *filesel_print_ps_renderopt_c_1; /* from renderopt.c 343 */
    char *filesel_print_mif_renderopt_c_1; /* from renderopt.c 344 */
    char target_mutex_renderopt_c_1[4]; /* from renderopt.c 346 */
    char page_mutex_renderopt_c_1[5]; /* from renderopt.c 348 */
    char render_mutex_renderopt_c_1[3]; /* from renderopt.c 350 */
    int page_size_type_renderopt_c_1; /* from renderopt.c 351 */

    /*
     * savefile.c
     */
    char *sfn;
    char *lcname;

    /*
     * search.c
     */
    GtkWidget *menuitem_search[5]; /* from search.c */
    GtkWidget *window1_search_c_2; /* from search.c 359 */
    GtkWidget *entry_a_search_c_1; /* from search.c 360 */
    char *entrybox_text_local_search_c_2; /* from search.c 361 */
    void (*cleanup_e_search_c_2)(void); /* from search.c 362 */
    SearchProgressData *pdata; /* from search.c 363 */
    int is_active_search_c_4; /* from search.c 364 */
    char is_insert_running_search_c_1; /* from search.c 365 */
    char is_replace_running_search_c_1; /* from search.c 366 */
    char is_append_running_search_c_1; /* from search.c 367 */
    char is_searching_running_search_c_1; /* from search.c 368 */
    char regex_mutex_search_c_1[5]; /* from search.c 371 */
    int regex_which_search_c_1; /* from search.c 372 */
    GtkWidget *window_search_c_7; /* from search.c 373 */
    GtkWidget *entry_search_c_3; /* from search.c 374 */
    char *searchbox_text_search_c_1; /* from search.c 377 */
    char bundle_direction_search_c_2; /* from search.c 378 */
    void (*cleanup_search_c_5)(void); /* from search.c 379 */
    int num_rows_search_c_2; /* from search.c 380 */
    int selected_rows_search_c_2; /* from search.c 381 */
    GtkListStore *sig_store_search;
    GtkTreeSelection *sig_selection_search;
    GtkWidget *sig_view_search;

    /*
     * signalwindow.c
     */
    GtkWidget *signalarea; /* from signalwindow.c 396 */
    struct font_engine_font_t *signalfont; /* from signalwindow.c 397 */
    int max_signal_name_pixel_width; /* from signalwindow.c 399 */
    int signal_pixmap_width; /* from signalwindow.c 400 */
    int fontheight; /* from signalwindow.c 404 */
    char dnd_state; /* from signalwindow.c 405 */
    gint cached_mouseover_x; /* from signalwindow.c */
    gint cached_mouseover_y; /* from signalwindow.c */
    gint mouseover_counter; /* from signalwindow.c */
    unsigned button2_debounce_flag : 1;
    int dragzoom_threshold;

    /*
     * simplereq.c
     */
    GtkWidget *window_simplereq_c_9; /* from simplereq.c 413 */
    void (*cleanup)(GtkWidget *, void *); /* from simplereq.c 414 */

    /*
     * splash.c
     */
    char splash_is_loading;
    char splash_fix_win_title;
    char splash_disable; /* from splash.c 415 */
    GtkWidget *splash_splash_c_1; /* from splash.c 419 */
    GtkWidget *darea_splash_c_1; /* from splash.c 420 */
    GTimer *gt_splash_c_1; /* from splash.c 421 */
    int timeout_tag; /* from splash.c 422 */
    int load_complete_splash_c_1; /* from splash.c 423 */
    int cnt_splash_c_1; /* from splash.c 424 */
    int prev_bar_x_splash_c_1; /* from splash.c 425 */
    GdkPixbuf *wave_splash_pixbuf;

    /*
     * status.c
     */
    GtkWidget *text_status_c_2; /* from status.c 426 */
    GtkTextIter iter_status_c_3; /* from status.c 428 */
    GtkTextTag *bold_tag_status_c_3; /* from status.c 429 */

    /*
     * strace.c
     */
    struct strace_ctx_t *strace_ctx; /* moved to strace.h */
    struct strace_ctx_t strace_windows[WAVE_NUM_STRACE_WINDOWS];
    int strace_current_window;
    int strace_repeat_count;

/*
 * symbol.c
 */
#ifdef _WAVE_HAVE_JUDY
    Pvoid_t sym_judy; /* from symbol.c */
    Pvoid_t s_selected; /* from symbol.c */
#endif
    struct symbol **sym_hash; /* from symbol.c 453 */
    struct symbol **facs; /* from symbol.c 454 */
    char facs_are_sorted; /* from symbol.c 455 */
    char facs_have_symbols_state_machine; /* from symbol.c */
    int numfacs; /* from symbol.c 456 */
    int regions; /* from symbol.c 457 */
    int longestname; /* from symbol.c 458 */
    int hashcache; /* from symbol.c 461 */

    /*
     * tcl_commands.c
     */
    char *previous_braced_tcl_string;

    /*
     * tcl_helper.c
     */
    char in_tcl_callback;
    char tcl_menu_toggle_item;

    /*
     * timeentry.c
     */
    GtkWidget *from_entry; /* from timeentry.c 462 */
    GtkWidget *to_entry; /* from timeentry.c 463 */

    /*
     * translate.c
     */
    int current_translate_file; /* from translate.c 464 */
    int current_filter_translate_c_2; /* from translate.c 465 */
    int num_file_filters; /* from translate.c 466 */
    char **filesel_filter; /* from translate.c 467 */
    struct xl_tree_node **xl_file_filter; /* from translate.c 468 */
    int is_active_translate_c_5; /* from translate.c 469 */
    char *fcurr_translate_c_2; /* from translate.c 470 */
    GtkWidget *window_translate_c_11; /* from translate.c 471 */
    GtkListStore *sig_store_translate;
    GtkTreeSelection *sig_selection_translate;

/*
 * tree.c
 */
#ifdef _WAVE_HAVE_JUDY
    Pvoid_t sym_tree;
    Pvoid_t sym_tree_addresses;
#endif
    GwTree *treeroot; /* from tree.c 473 */
    GwTree *mod_tree_parent; /* from tree.c */
    char *module_tree_c_1; /* from tree.c 474 */
    int module_len_tree_c_1; /* from tree.c 475 */
    GwTree *terminals_tchain_tree_c_1; /* from tree.c 476 */
    char hier_delimeter; /* from tree.c 477 */
    char hier_was_explicitly_set; /* from tree.c 478 */
    char alt_hier_delimeter; /* from tree.c 479 */
    struct symbol **facs2_tree_c_1; /* from tree.c 481 */
    int facs2_pos_tree_c_1; /* from tree.c 482 */
    unsigned char *talloc_pool_base;
    size_t talloc_idx;
    char *sst_exclude_filename;
    uint64_t exclhiermask;
    JRB exclcompname;
    JRB exclinstname;

/*
 * tree_component.c
 */
#ifdef _WAVE_HAVE_JUDY
    Pvoid_t comp_name_judy;
#else
    JRB comp_name_jrb;
#endif
    char **comp_name_idx;
    int comp_name_serial;
    size_t comp_name_total_stringmem;
    int comp_name_longest;

    /*
     * treesearch_gtk2.c
     */
    struct string_chain_t *treeopen_chain_head; /* from bitvec.c */
    struct string_chain_t *treeopen_chain_curr; /* from bitvec.c */
    char do_dynamic_treefilter; /* from treesearch_gtk2.c */
    GtkWidget *treesearch_gtk2_window_vbox; /* from treesearch_gtk2.c */
    char *selected_hierarchy_name; /* from treesearch_gtk2.c */
    char *selected_sig_name; /* from treesearch_gtk2.c */
    GtkWidget *gtk2_tree_frame; /* from treesearch_gtk2.c */
    GtkWidget *filter_entry; /* from treesearch_gtk2.c */
    struct xl_tree_node *open_tree_nodes; /* from treesearch_gtk2.c */
    char autoname_bundles; /* from treesearch_gtk2.c 483 */
    GtkWidget *window1_treesearch_gtk2_c_3; /* from treesearch_gtk2.c 484 */
    GtkWidget *entry_a_treesearch_gtk2_c_2; /* from treesearch_gtk2.c 485 */
    char *entrybox_text_local_treesearch_gtk2_c_3; /* from treesearch_gtk2.c 486 */
    void (*cleanup_e_treesearch_gtk2_c_3)(void); /* from treesearch_gtk2.c 487 */
    GwTree *sig_root_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 488 */
    GwTree *sst_sig_root_treesearch_gtk2_c_1; /* from treesearch_gtk2.c */
    char *filter_str_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 489 */
    int filter_typ_treesearch_gtk2_c_1;
    int filter_typ_polarity_treesearch_gtk2_c_1;
    int filter_matlen_treesearch_gtk2_c_1;
    unsigned char filter_noregex_treesearch_gtk2_c_1;
    GtkListStore *sig_store_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 490 */
    GtkTreeSelection *sig_selection_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 491 */
    int is_active_treesearch_gtk2_c_6; /* from treesearch_gtk2.c 492 */
    struct autocoalesce_free_list *afl_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 494 */
    int pre_import_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 499 */
    Traces tcache_treesearch_gtk2_c_2; /* from treesearch_gtk2.c 500 */
    GtkWidget *dnd_sigview; /* from treesearch_gtk2.c */
    GtkPaned *sst_vpaned; /* from treesearch_gtk2.c */
    int fetchlow;
    int fetchhigh;
    GtkTreeStore *treestore_main;
    GtkWidget *treeview_main;
    enum sst_cb_action sst_dbl_action_type;

    /*
     * ttranslate.c
     */
    int current_translate_ttrans;
    int current_filter_ttranslate_c_1;
    int num_ttrans_filters;
    char **ttranssel_filter;
    struct pipe_ctx **ttrans_filter;
    int is_active_ttranslate_c_2;
    char *fcurr_ttranslate_c_1;
    GtkWidget *window_ttranslate_c_5;
    char *ttranslate_args;
    GtkListStore *sig_store_ttranslate;
    GtkTreeSelection *sig_selection_ttranslate;

    /*
     * vcd.c
     */
    unsigned char do_hier_compress; /* from vcd.c */
    char *prev_hier_uncompressed_name; /* from vcd.c */
    jmp_buf *vcd_jmp_buf; /* from vcd.c */
    int vcd_warning_filesize; /* from vcd.c 502 */
    char autocoalesce; /* from vcd.c 503 */
    char autocoalesce_reversal; /* from vcd.c 504 */
    char convert_to_reals; /* from vcd.c 506 */
    char make_vcd_save_file; /* from vcd.c 508 */
    char vcd_preserve_glitches; /* from vcd.c 509 */
    char vcd_preserve_glitches_real;
    FILE *vcd_save_handle; /* from vcd.c 510 */
    char vcd_hier_delimeter[2]; /* from vcd.c 522 */
    int escaped_names_found_vcd_c_1; /* from vcd.c 528 */
    struct slist *slistroot; /* from vcd.c 529 */
    struct slist *slistcurr; /* from vcd.c 530 */
    char *slisthier; /* from vcd.c 531 */
    int slisthier_len; /* from vcd.c 532 */
    GwHistEnt *he_curr_vcd_c_1; /* from vcd.c 543 */
    GwHistEnt *he_fini_vcd_c_1; /* from vcd.c 544 */

    /*
     * vcd_recoder.c
     */
    VcdFile *vcd_file;

    /*
     * vcd_saver.c
     */
    FILE *f_vcd_saver_c_1; /* from vcd_saver.c 630 */
    char buf_vcd_saver_c_3[16]; /* from vcd_saver.c 631 */
    struct vcdsav_tree_node **hp_vcd_saver_c_1; /* from vcd_saver.c 632 */
    struct namehier *nhold_vcd_saver_c_1; /* from vcd_saver.c 633 */

    /*
     * vlist.c
     */
    char vlist_prepack;
    off_t vlist_bytes_written;
    int vlist_compression_depth; /* from vlist.c 634 */

    /*
     * wavewindow.c
     */
    char highlight_wavewindow; /* from wavewindow.c */
    char black_and_white; /* from wavewindow.c */
    char signalwindow_width_dirty; /* from wavewindow.c 644 */
    char enable_ghost_marker; /* from wavewindow.c 645 */
    char enable_horiz_grid; /* from wavewindow.c 646 */
    char enable_vert_grid; /* from wavewindow.c 647 */
    char use_big_fonts; /* from wavewindow.c 648 */
    char use_nonprop_fonts; /* from wavewindow.c 649 */
    char do_resize_signals; /* from wavewindow.c 650 */
    char first_unsized_signals;
    int initial_signal_window_width;
    char constant_marker_update; /* from wavewindow.c 651 */
    char use_roundcaps; /* from wavewindow.c 652 */
    char show_base; /* from wavewindow.c 653 */
    char wave_scrolling; /* from wavewindow.c 654 */
    int vector_padding; /* from wavewindow.c 655 */
    unsigned int in_button_press_wavewindow_c_1; /* from wavewindow.c 656 */
    char left_justify_sigs; /* from wavewindow.c 657 */
    char zoom_pow10_snap; /* from wavewindow.c 658 */
    char zoom_dyn; /* from menu.c */
    char zoom_dyne; /* from menu.c */
    int cursor_snap; /* from wavewindow.c 659 */
    float old_wvalue; /* from wavewindow.c 660 */
    GwBlackoutRegions *blackout_regions; /* from wavewindow.c 661 */
    GwTime zoom; /* from wavewindow.c 662 */
    GwTime scale; /* from wavewindow.c 663 */
    GwTime nsperframe; /* from wavewindow.c 664 */
    double pixelsperframe; /* from wavewindow.c 665 */
    double hashstep; /* from wavewindow.c 666 */
    GwTime prevtim_wavewindow_c_1; /* from wavewindow.c 667 */
    double pxns; /* from wavewindow.c 668 */
    double nspx; /* from wavewindow.c 669 */
    double zoombase; /* from wavewindow.c 670 */
    int waveheight; /* from wavewindow.c 672 */
    int wavecrosspiece; /* from wavewindow.c 673 */
    int wavewidth; /* from wavewindow.c 674 */
    struct font_engine_font_t *wavefont; /* from wavewindow.c 675 */
    struct font_engine_font_t *wavefont_smaller; /* from wavewindow.c 676 */
    GtkWidget *wavearea; /* from wavewindow.c 677 */
    GtkWidget *vscroll_wavewindow_c_1; /* from wavewindow.c 678 */
    GtkWidget *hscroll_wavewindow_c_2; /* from wavewindow.c 679 */
    gpointer wave_vslider2; /* from wavewindow.c 681 */
    gpointer wave_hslider; /* from wavewindow.c 682 */
    int named_marker_lock_idx; /* from menu.c */
    int which_t_color;
    char fill_in_smaller_rgb_areas_wavewindow_c_1; /* from wavewindow.c 719 */
    GwTime prev_markertime; /* from wavewindow.c */
    int analog_redraw_skip_count; /* from wavewindow.c */
    int str_wid_x;
    int str_wid_width;
    int str_wid_bigw;
    int str_wid_state;
    int str_wid_slider;
    int str_wid_height;
    GwTime ruler_origin;
    GwTime ruler_step;
    char fill_waveform;
    char lz_removal;
    gboolean disable_antialiasing;

    double cr_line_width;
    double cairo_050_offset;
#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
    gdouble wavearea_gesture_initial_zoom;
#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
    gdouble wavearea_gesture_initial_zoom_x_distance;
    GwTime wavearea_gesture_initial_x1tim;
#endif
    GtkGesture *wavearea_gesture_swipe;
    GDateTime *swipe_init_time;
    GwTime swipe_init_start;
    gdouble wavearea_gesture_swipe_velocity_x;
    int wavearea_drag_start_x;
    int wavearea_drag_start_y;
    char wavearea_drag_active;
#endif
    char use_gestures;
    gboolean use_dark;
    gboolean save_on_exit;

    /*
     * zoombuttons.c
     */
    char do_zoom_center; /* from zoombuttons.c 720 */
    char do_initial_zoom_fit; /* from zoombuttons.c 721 */
    char do_initial_zoom_fit_used;
};

struct Global *initialize_globals(void);
void reload_into_new_context(void);
void strcpy2_into_new_context(struct Global *g, char **newstrref, char **oldstrref);
void free_and_destroy_page_context(void);
void dead_context_sweep(void);

void install_focus_cb(GtkWidget *w, intptr_t ptr_offset);

gulong gtkwave_signal_connect_x(gpointer object,
                                const gchar *name,
                                GCallback func,
                                gpointer data,
                                char *f,
                                intptr_t line);
gulong gtkwave_signal_connect_object_x(gpointer object,
                                       const gchar *name,
                                       GCallback func,
                                       gpointer data,
                                       char *f,
                                       intptr_t line);

#ifdef GTKWAVE_SIGNAL_CONNECT_DEBUG
#define gtkwave_signal_connect(a, b, c, d) gtkwave_signal_connect_x(a, b, c, d, __FILE__, __LINE__)
#define gtkwave_signal_connect_object(a, b, c, d) \
    gtkwave_signal_connect_object_x(a, b, c, d, __FILE__, __LINE__)
#else
#define gtkwave_signal_connect(a, b, c, d) gtkwave_signal_connect_x(a, b, c, d, NULL, 0)
#define gtkwave_signal_connect_object(a, b, c, d) \
    gtkwave_signal_connect_object_x(a, b, c, d, NULL, 0)
#endif

void set_GLOBALS_x(struct Global *g, const char *file, int line);

#ifdef GTKWAVE_GLOBALS_DEBUG
#define set_GLOBALS(a) set_GLOBALS_x(a, __FILE__, __LINE__)
#else
#define set_GLOBALS(a) set_GLOBALS_x(a, NULL, 0)
#endif

void logbox_reload(void);

extern struct Global *GLOBALS;
#endif
