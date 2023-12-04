/*
 * Copyright (c) Kermin Elliott Fleming 2007-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"
#include "analyzer.h"
#include "bsearch.h"
#include "busy.h"
#include "clipping.h"
#include "color.h"
#include "currenttime.h"
#include "debug.h"
#include "fgetdynamic.h"
#include "ghw.h"
#include "globals.h"
#include <getopt.h>
#include "gtk23compat.h"
#include "main.h"
#include "menu.h"
#include "pipeio.h"
#include "pixmaps.h"
#include "print.h"
#include "ptranslate.h"
#include "ttranslate.h"
#include "rc.h"
#include "regex_wave.h"
#include "strace.h"
#include "symbol.h"
#include "translate.h"
#include "tree.h"
#include "vcd.h"
#include "vcd_saver.h"
#include "vlist.h"
#include "lx2.h"
#include "signal_list.h"
#include "gw-time-display.h"

#include "fst.h"
#include "hierpack.h"

#include "fsdb_wrapper_api.h"

#ifdef __MINGW32__
#define sleep(x) Sleep(x * 1000)
#endif

#if !defined __MINGW32__
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#include <io.h>
#endif

struct Global *GLOBALS = NULL;

/* make this const so if we try to write to it we coredump */
static const struct Global globals_base_values = {
    NULL, // project
    NULL, // stems

    /*
     * analyzer.c
     */
    TR_RJUSTIFY, /* default_flags 5 */
    0, /* default_fpshift */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* tims 6 */
    {0, 0, NULL, NULL, NULL, NULL, 0, 0}, /* traces 9 */
    0, /* hier_max_level 8 */
    0, /* hier_max_level_shadow */
    0, /* timestart_from_savefile */
    0, /* timestart_from_savefile_valid */
    0, /* group_depth */
    0, /* hier_ignore_escapes */

    /*
     * baseconvert.c
     */
    0, /* color_active_in_filter 9 */

    /*
     * bsearch.c
     */
    GW_TIME_CONSTANT(0), /* shift_timebase 10 */
    GW_TIME_CONSTANT(0), /* shift_timebase_default_for_add 11 */
    0, /* max_compare_time_tc_bsearch_c_1 12 */
    0, /* max_compare_pos_tc_bsearch_c_1 13 */
    0, /* max_compare_time_bsearch_c_1 14 */
    0, /* max_compare_pos_bsearch_c_1 15 */
    0, /* max_compare_index 16 */
    0, /* vmax_compare_time_bsearch_c_1 17 */
    0, /* vmax_compare_pos_bsearch_c_1 18 */
    0, /* vmax_compare_index 19 */
    0, /* maxlen_trunc 20 */
    0, /* maxlen_trunc_pos_bsearch_c_1 21 */
    0, /* trunc_asciibase_bsearch_c_1 22 */

    /*
     * busy.c
     */
    NULL, /* busycursor_busy_c_1 23 */
    0, /* busy_busy_c_1 24 */

    /*
     * color.c
     */
    0, /* keep_xz_colors */
    NULL, /* color_theme */

    /*
     * currenttime.c
     */
    GW_TIME_CONSTANT(0), /* global_time_offset */
    0, /* is_vcd 56 */
    0, /* partial_vcd */
    1, /* use_maxtime_display 57 */
    0, /* use_frequency_delta 58 */
    0, /* cached_currenttimeval_currenttime_c_1 61 */
    0, /* max_time 63 */
    -1, /* min_time 64 */
    ~0, /* display_grid 65 */
    0, /* fullscreen */
    1, /* show_toolbar */
    NULL, /* time_box */
    1, /* time_scale 66 */
    'n', /* time_dimension 67 */
    0, /* scale_to_time_dimension */
    // 0, /* maxtimewid_currenttime_c_1 69 */
    // 0, /* curtimewid_currenttime_c_1 70 */
    1, /* time_trunc_val_currenttime_c_1 76 */
    0, /* use_full_precision 77 */

    /*
     * debug.c
     */
    NULL, /* alloc2_chain */
    0, /* outstanding */
    NULL, /* atoi_cont_ptr 78 */
    0, /* disable_tooltips 79 */

    /*
     * entry.c
     */
    NULL, /* entrybox_text 82 */

    /*
     * extload.c
     */
    0, /* extload_ffr_import_count */
    NULL, /* extload_ffr_ctx */
    NULL, /* extload */
    NULL, /* extload_idcodes */
    NULL, /* extload_inv_idcodes */
#if !defined __MINGW32__
    0, /* extload_lastmod */
    0, /* extload_already_errored */
#endif
    NULL, /* extload_namecache */
    NULL, /* extload_namecache_max */
    NULL, /* extload_namecache_lens */
    NULL, /* extload_namecache_patched */
    NULL, /* extload_sym_block */
    NULL, /* extload_node_block */
    NULL, /* extload_xc */
    NULL, /* extload_prevsymroot */
    NULL, /* extload_prevsym */
    NULL, /* extload_npar */
    0, /* extload_i */
    0, /* extload_hlen */
    0, /* extload_vt_prev */
    0, /* extload_vd_prev */
    0, /* f_name_build_buf_len */
    NULL, /* f_name_build_buf */
    0, /* extload_max_tree */
    0, /* extload_curr_tree */

    /*
     * fetchbuttons.c
     */
    100, /* fetchwindow 84 */

    /*
     * fgetdynamic.c
     */
    0, /* fgetmalloc_len 85 */

    /*
     * file.c
     */
    NULL, /* pFileChoose */
    NULL, /* pFileChooseFilterName */
    NULL, /* pPatternSpec */
    0, /* fs_file_c_1 86 */
    NULL, /* fileselbox_text 87 */
    0, /* filesel_ok 88 */
    0, /* cleanup_file_c_2 89 */
    0, /* bad_cleanup_file_c1 */

    /*
     * fonts.c
     */
    NULL, /* fontname_signals 90 */
    NULL, /* fontname_waves 91 */
    NULL, /* fonts_screen */
    NULL, /* fonts_context */
    NULL, /* fonts_layout */
    1, /* use_pango_fonts */

    /*
     * fst.c
     */
    NULL, /* fst_fst_c_1 */
    0, /* nonimplicit_direction_encountered */
    0, /* supplemental_datatypes_encountered */
    0, /* supplemental_vartypes_encountered */
    0, /* is_vhdl_component_format */
    NULL, /* xl_enum_filter */
    0, /* num_xl_enum_filter */
    NULL, /* enum_nptrs_jrb */

    /*
     * ghw.c
     */
    0, /* is_ghw 99 */

    /*
     * globals.c
     */
    NULL, /* dead_context */
    NULL, /* gtk_context_bridge_ptr */

    /*
     * hierpack.c
     */
    NULL, /* hp_buf */
    NULL, /* hp_offs */
    0, /* hp_prev */
    0, /* hp_buf_siz */
    NULL, /* fmem_buf */
    0, /* fmem_buf_siz */
    0, /* fmem_buf_offs */
    0, /* fmem_uncompressed_siz */
    0, /* disable_auto_comphier */

    /*
     * logfile.c
     */
    NULL, /* logfiles */
    NULL, /* fontname_logfile 133 */
    {NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, NULL}, /* iter_logfile_c_2 135 */
    NULL, /* bold_tag_logfile_c_2 136 */
    NULL, /* mono_tag_logfile_c_1 137 */
    NULL, /* size_tag_logfile_c_1 138 */

    /*
     * lx2.c
     */
    LXT2_IS_INACTIVE, /* is_lx2 139 */

    /*
     * main.c
     */
    1, /* is_gtkw_save_file */
    0, /* dumpfile_is_modified */
    NULL, /* missing_file_toolbar */
    NULL, /* argvlist */
#if defined(HAVE_LIBTCL)
    NULL, /* interp */
#endif
    NULL, /* repscript_name */
    500, /* repscript_period */
    NULL, /* tcl_init_cmd */
    0, /* tcl_running */
    0, /* block_xy_update */
    NULL, /* winname */
    0, /* num_notebook_pages */
    1, /* num_notebook_pages_cumulative */
    0, /* context_tabposition */
    0, /* this_context_page */
    0, /* second_page_created */
    NULL, /* contexts */
    NULL, /* notebook */
    NULL, /* loaded_file_name */
    NULL, /* unoptimized_vcd_file_name  */
    NULL, /* skip_start */
    NULL, /* skip_end */
    MISSING_FILE, /* loaded_file_type */
    0, /* is_optimized_stdin_vcd */
    NULL, /* whoami 190 */
    NULL, /* logfile 191 */
    NULL, /* stems_name 192 */
    WAVE_ANNO_NONE, /* stems_type 193 */
    NULL, /* aet_name 194 */
    NULL, /* anno_ctx 195 */
    NULL, /* dual_ctx 196 */
    0, /* dual_id 197 */
    0, /* dual_attach_id_main_c_1 198 */
    0, /* dual_race_lock 199 */
    NULL, /* mainwindow 200 */
    NULL, /* signalwindow 201 */
    NULL, /* wavewindow 202 */
    NULL, /* toppanedwindow 203 */
    NULL, /* panedwindow */
    0, /* toppanedwindow_size_cache */
    0, /* panedwindow_size_cache */
    0, /* vpanedwindow_size_cache */
    NULL, /* sstpane 204 */
    NULL, /* expanderwindow 205 */
    0, /* disable_window_manager 206 */
    0, /* disable_empty_gui */
    1, /* paned_pack_semantics 207 */
    0, /* zoom_was_explicitly_set 208 */
    1000, /* initial_window_x 209 */
    600, /* initial_window_y */
    -1, /* initial_window_width */
    -1, /* initial_window_height 210 */
    0, /* xy_ignore_main_c_1 211 */
    0, /* optimize_vcd 212 */
    1, /* num_cpus 213 */
    -1, /* initial_window_xpos 214 */
    -1, /* initial_window_ypos 214 */
    0, /* initial_window_set_valid 215 */
    -1, /* initial_window_xpos_set 216 */
    -1, /* initial_window_ypos_set */
    0, /* initial_window_get_valid 217 */
    -1, /* initial_window_xpos_get 218 */
    -1, /* initial_window_ypos_get 218 */
    0, /* xpos_delta 219 */
    0, /* ypos_delta 219 */
    1, /* sst_expanded 223 */
    0, /* socket_xid 224 */
    0, /* disable_menus 225 */
    NULL, /* ftext_main_main_c_1 226 */
    0, /* dbl_mant_dig_override */

    /*
     * menu.c
     */
    NULL, /* cutcopylist */
    0, /* enable_fast_exit 236 */
    0, /* quiet_checkmenu */
    NULL, /* wave_script_args 237 */
    0, /* ignore_savefile_pane_pos */
    0, /* ignore_savefile_pos 238 */
    0, /* ignore_savefile_size 239 */
    NULL, /* regexp_string_menu_c_1 242 */
    NULL, /* trace_to_alias_menu_c_1 243 */
    NULL, /* showchangeall_menu_c_1 244 */
    NULL, /* filesel_newviewer_menu_c_1 245 */
    NULL, /* filesel_logfile_menu_c_1 246 */
    NULL, /* filesel_scriptfile_menu */
    NULL, /* filesel_writesave 247 */
    NULL, /* filesel_imagegrab */
    0, /* save_success_menu_c_1 248 */
    NULL, /* filesel_vcd_writesave 249 */
    NULL, /* filesel_tim_writesave */
    0, /* lock_menu_c_1 251 */
    0, /* lock_menu_c_2 252 */
    NULL, /* buf_menu_c_1 253 128 */
    NULL, /* signal_popup_menu */
    NULL, /* sst_signal_popup_menu */

    /*
     * mouseover.c
     */
    1, /* disable_mouseover 254 */
    0, /* clipboard_mouseover */
    NULL, /* mouseover_mouseover_c_1 255 */

    /*
     * pagebuttons.c
     */
    1.0, /* page_divisor 261 */

    /*
     * pixmaps.c
     */
    NULL, /* hierarchy_icons */

    /*
     * print.c
     */
    72, /* inch_print_c_1 298 */
    1.0, /* ps_chwidth_print_c_1 299 */
    0, /* ybound_print_c_1 300 */
    0, /* pr_signal_fill_width_print_c_1 301 */
    0, /* ps_nummaxchars_print_c_1 302 */
    1, /* ps_fullpage 303 */
    66, /* ps_maxveclen 304 */
    0, /* liney_max 305 */

    /*
     * ptranslate.c
     */
    0, /* current_translate_proc 308 */
    0, /* current_filter_ptranslate_c_1 309 */
    0, /* num_proc_filters 310 */
    NULL, /* procsel_filter 311 */
    NULL, /* proc_filter 312 */
    0, /* is_active_ptranslate_c_2 313 */
    NULL, /* fcurr_ptranslate_c_1 314 */
    NULL, /* window_ptranslate_c_5 315 */
    NULL, /* sig_store_ptranslate; */
    NULL, /* sig_selection_ptranslate */

    /*
     * rc.c
     */
    0, /* rc_line_no 318 */
    1, /* possibly_use_rc_defaults 319 */
    NULL, /* editor_string */
    FALSE, /* editor_run_in_terminal */

    /*
     * regex.c
     */
    NULL, /* preg_regex_c_1 321 */
    NULL, /* regex_ok_regex_c_1 322 */

/*
 * renderopt.c
 */
#ifdef WAVE_GTK_UNIX_PRINT
    NULL, /* gprs */
    NULL, /* gps */
    NULL, /* gp_tfn */
#endif
    0, /* is_active_renderopt_c_3 323 */
    0, /* window_renderopt_c_6 324 */
    NULL, /* filesel_print_pdf_renderopt_c_1 */
    NULL, /* filesel_print_ps_renderopt_c_1 325 */
    NULL, /* filesel_print_mif_renderopt_c_1 326 */
    {0, 0, 0, 0}, /* target_mutex_renderopt_c_1 328 */
    {0, 0, 0, 0, 0}, /* page_mutex_renderopt_c_1 330 */
    {0, 0, 0}, /* render_mutex_renderopt_c_1 332 */
    0, /* page_size_type_renderopt_c_1 333 */

    /*
     * savefile.c
     */
    NULL, /* sfn */
    NULL, /* lcname */

    /*
     * search.c
     */
    {NULL, NULL, NULL, NULL, NULL}, /* menuitem_search */
    NULL, /* window1_search_c_2 340 */
    NULL, /* entry_a_search_c_1 341 */
    NULL, /* entrybox_text_local_search_c_2 342 */
    NULL, /* cleanup_e_search_c_2 343 */
    NULL, /* pdata 344 */
    0, /* is_active_search_c_4 345 */
    0, /* is_insert_running_search_c_1 346 */
    0, /* is_replace_running_search_c_1 347 */
    0, /* is_append_running_search_c_1 348 */
    0, /* is_searching_running_search_c_1 349 */
    {0, 0, 0, 0, 0}, /* regex_mutex_search_c_1 352 */
    0, /* regex_which_search_c_1 353 */
    NULL, /* window_search_c_7 354 */
    NULL, /* entry_search_c_3 355 */
    NULL, /* searchbox_text_search_c_1 358 */
    0, /* bundle_direction_search_c_2 359 */
    NULL, /* cleanup_search_c_5 360 */
    0, /* num_rows_search_c_2 361 */
    0, /* selected_rows_search_c_2 362 */
    NULL, /* sig_store_search */
    NULL, /* sig_selection_search */
    NULL, /* sig_view_search */

    /*
     * signalwindow.c
     */
    NULL, /* signalarea 369 */
    NULL, /* signalfont 370 */
    0, /* max_signal_name_pixel_width 372 */
    0, /* signal_pixmap_width 373 */
    1, /* fontheight 376 */
    0, /* dnd_state 377 */
    0, /* cached_mouseover_x */
    0, /* cached_mouseover_y */
    0, /* mouseover_counter */
    0, /* button2_debounce_flag */
    0, /* dragzoom_threshold */

    /*
     * simplereq.c
     */
    NULL, /* window_simplereq_c_9 385 */
    NULL, /* cleanup 386 */

    /*
     * splash.c
     */
    0, /* splash_is_loading */
    0, /* splash_fix_win_title */
    1, /* splash_disable 387 */
    NULL, /* splash_splash_c_1 391 */
    NULL, /* darea_splash_c_1 392 */
    NULL, /* gt_splash_c_1 393 */
    0, /* timeout_tag 394 */
    0, /* load_complete_splash_c_1 395 */
    2, /* cnt_splash_c_1 396 */
    0, /* prev_bar_x_splash_c_1 397 */
    NULL, /* wave_splash_pixbuf */

    /*
     * status.c
     */
    NULL, /* text_status_c_2 398 */
    {NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, NULL}, /* iter_status_c_3 400 */
    NULL, /* bold_tag_status_c_3 401 */

    /*
     * strace.c
     */
    NULL, /* strace_ctx (defined in strace.h for multiple strace sessions) */

    {{NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      0,
      0,
      0,
      0,
      0,
      0,
      0}, /* strace_windows[0] */
     {NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      0,
      {0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0},
      0,
      0,
      0,
      0,
      0,
      0,
      0}}, /* strace_windows[1] */
#if WAVE_NUM_STRACE_WINDOWS != 2
#error the number of strace windows as defined in strace.h does not match globals.c!
#endif
    0, /* strace_current_window */
    1, /* strace_repeat_count */

/*
 * symbol.c
 */
#ifdef _WAVE_HAVE_JUDY
    NULL, /* sym_judy */
    NULL, /* s_selected */
#endif
    NULL, /* sym_hash 424 */
    NULL, /* facs 425 */
    0, /* facs_are_sorted 426 */
    0, /* facs_have_symbols_state_machine */
    0, /* numfacs 427 */
    0, /* regions 428 */
    0, /* longestname 429 */
    0, /* hashcache 432 */

    /*
     * tcl_commands.c
     */
    NULL, /* previous_braced_tcl_string */

    /*
     * tcl_helper.c
     */
    0, /* in_tcl_callback */
    0, /* tcl_menu_toggle_item */

    /*
     * timeentry.c
     */
    NULL, /* from_entry 433 */
    NULL, /* to_entry */

    /*
     * translate.c
     */
    0, /* current_translate_file 434 */
    0, /* current_filter_translate_c_2 435 */
    0, /* num_file_filters 436 */
    NULL, /* filesel_filter 437 */
    NULL, /* xl_file_filter 438 */
    0, /* is_active_translate_c_5 439 */
    NULL, /* fcurr_translate_c_2 440 */
    NULL, /* window_translate_c_11 441 */
    NULL, /* sig_store_translate; */
    NULL, /* sig_selection_translate */

/*
 * tree.c
 */
#ifdef _WAVE_HAVE_JUDY
    NULL, /* sym_tree */
    NULL, /* sym_tree_addresses */
#endif
    NULL, /* treeroot 443 */
    NULL, /* mod_tree_parent */
    NULL, /* module_tree_c_1 444 */
    0, /* module_len_tree_c_1 445 */
    NULL, /* terminals_tchain_tree_c_1 446 */
    '.', /* hier_delimeter 447 */
    0, /* hier_was_explicitly_set 448 */
    0x00, /* alt_hier_delimeter 449 */
    NULL, /* facs2_tree_c_1 451 */
    0, /* facs2_pos_tree_c_1 452 */
    NULL, /* talloc_pool_base */
    0, /* talloc_idx */
    NULL, /* sst_exclude_filename */
    0, /* exclhiermask */
    NULL, /* exclcompname */
    NULL, /* exclinstname */

/*
 * tree_component.c
 */
#ifdef _WAVE_HAVE_JUDY
    NULL, /* comp_name_judy */
#else
    NULL, /* comp_name_jrb */
#endif
    NULL, /* comp_name_idx */
    0, /* comp_name_serial */
    0, /* comp_name_total_stringmem */
    0, /* comp_name_longest */

    /*
     * treesearch_gtk2.c
     */
    NULL, /* treeopen_chain_head */
    NULL, /* treeopen_chain_curr */
    1, /* do_dynamic_treefilter */
    NULL, /* treesearch_gtk2_window_vbox */
    NULL, /* selected_hierarchy_name */
    NULL, /* selected_sig_name */
    NULL, /* gtk2_tree_frame */
    NULL, /* filter_entry */
    NULL, /* open_tree_nodes */
    0, /* autoname_bundles 453 */
    NULL, /* window1_treesearch_gtk2_c_3 454 */
    NULL, /* entry_a_treesearch_gtk2_c_2 455 */
    NULL, /* entrybox_text_local_treesearch_gtk2_c_3 456 */
    NULL, /* cleanup_e_treesearch_gtk2_c_3 457 */
    NULL, /* sig_root_treesearch_gtk2_c_1 458 */
    NULL, /* sst_sig_root_treesearch_gtk2_c_1 */
    NULL, /* filter_str_treesearch_gtk2_c_1 459 */
    GW_VAR_DIR_UNSPECIFIED, /* filter_typ_treesearch_gtk2_c_1 */
    0, /* filter_typ_polarity_treesearch_gtk2_c_1 */
    0, /* filter_matlen_treesearch_gtk2_c_1 */
    0, /* filter_noregex_treesearch_gtk2_c_1 */
    NULL, /* sig_store_treesearch_gtk2_c_1 460 */
    NULL, /* sig_selection_treesearch_gtk2_c_1 461 */
    0, /* is_active_treesearch_gtk2_c_6 462 */
    NULL, /* afl_treesearch_gtk2_c_1 464 */
    0, /* pre_import_treesearch_gtk2_c_1 469 */
    {0, 0, NULL, NULL, NULL, NULL, 0, 0}, /* tcache_treesearch_gtk2_c_2 470 */
    NULL, /* dnd_sigview */
    NULL, /* sst_vpaned */
    0, /* fetchlow */
    0, /* fetchhigh */
    NULL, /* treestore_main */
    NULL, /* treeview_main */
    SST_ACTION_INSERT, /* sst_dbl_action_type */

    /*
     * ttranslate.c
     */
    0, /* current_translate_ttrans */
    0, /* current_filter_ttranslate_c_1 */
    0, /* num_ttrans_filters */
    NULL, /* ttranssel_filter */
    NULL, /* ttrans_filter */
    0, /* is_active_ttranslate_c_2 */
    NULL, /* fcurr_ttranslate_c_1 */
    NULL, /* window_ttranslate_c_5 */
    NULL, /* ttranslate_args */
    NULL, /* sig_store_ttranslate; */
    NULL, /* sig_selection_ttranslate */

    /*
     * vcd.c
     */
    0, /* do_hier_compress */
    NULL, /* prev_hier_uncompressed_name */
    NULL, /* vcd_jmp_buf */
    -1, /* vcd_warning_filesize 472 */
    1, /* autocoalesce 473 */
    0, /* autocoalesce_reversal */
    0, /* convert_to_reals 475 */
    0, /* make_vcd_save_file 477 */
    0, /* vcd_preserve_glitches 478 */
    0, /* vcd_preserve_glitches_real */
    NULL, /* vcd_save_handle 479 */
    {0, 0}, /* vcd_hier_delimeter 491 */
    0, /* escaped_names_found_vcd_c_1 494 */
    NULL, /* slistroot 495 */
    NULL, /* slistcurr */
    NULL, /* slisthier 496 */
    0, /* slisthier_len 497x */
    NULL, /* he_curr_vcd_c_1 506 */
    NULL, /* he_fini */

    /*
     * vcd_recoder.c
     */
    NULL, /* vcd_file */

    /*
     * vcd_saver.c
     */
    NULL, /* f_vcd_saver_c_1 579 */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* buf_vcd_saver_c_3 580 */
    NULL, /* hp_vcd_saver_c_1 581 */
    NULL, /* nhold_vcd_saver_c_1 582 */

    /*
     * vlist.c
     */
    0, /* vlist_prepack */
    0, /* vlist_bytes_written */
    4, /* vlist_compression_depth 583 */

    /*
     * wavewindow.c
     */
    0, /* highlight_wavewindow */
    0, /* black_and_white */
    1, /* signalwindow_width_dirty 590 */
    1, /* enable_ghost_marker 591 */
    1, /* enable_horiz_grid 592 */
    1, /* enable_vert_grid 593 */
    0, /* use_big_fonts 594 */
    0, /* use_nonprop_fonts */
    ~0, /* do_resize_signals 595 */
    ~0, /* first_unsized_signals */
    0, /* initial_signal_window_width */
    0, /* constant_marker_update 596 */
    0, /* use_roundcaps 597 */
    ~0, /* show_base 598 */
    ~0, /* wave_scrolling 599 */
    4, /* vector_padding 600 */
    0, /* in_button_press_wavewindow_c_1 601 */
    0, /* left_justify_sigs 602 */
    0, /* zoom_pow10_snap 603 */
    0, /* zoom_dyn */
    0, /* zoom_dyne */
    0, /* cursor_snap 604 */
    -1.0, /* old_wvalue 605 */
    NULL, /* blackout_regions 606 */
    0, /* zoom 607 */
    1, /* scale */
    1, /* nsperframe */
    1, /* pixelsperframe 608 */
    1.0, /* hashstep 609 */
    -1, /* prevtim_wavewindow_c_1 610 */
    1.0, /* pxns 611 */
    1.0, /* nspx */
    2.0, /* zoombase 612 */
    1, /* waveheight 614 */
    0, /* wavecrosspiece */
    1, /* wavewidth 615 */
    NULL, /* wavefont 616 */
    NULL, /* wavefont_smaller 617 */
    NULL, /* wavearea 618 */
    NULL, /* vscroll_wavewindow_c_1 619 */
    NULL, /* hscroll_wavewindow_c_2 620 */
    NULL, /* wave_vslider2 622 */
    NULL, /* wave_hslider */
    -1, /* named_marker_lock_idx */
    0, /* which_t_color */
    0, /* fill_in_smaller_rgb_areas_wavewindow_c_1 659 */
    -1, /* prev_markertime */
    20, /* analog_redraw_skip_count */
    0, /* str_wid_x */
    0, /* str_wid_width */
    0, /* str_wid_bigw */
    0, /* str_wid_state */
    0, /* str_wid_slider */
    0, /* str_wid_height */
    0, /* ruler_origin */
    0, /* ruler_step */
    0, /* fill_waveform */
    0, /* lz_removal */
    FALSE, /* lz_removal */

    1.0, /* cr_line_width */
    0.5, /* cairo_050_offset */
#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
    0.0, /* wavearea_gesture_initial_zoom */
#ifdef WAVE_GTK3_GESTURE_ZOOM_IS_1D
    1.0, /* wavearea_gesture_initial_zoom_x_distance */
    0, /* wavearea_gesture_initial_x1tim */
#endif
    NULL, /* wavearea_gesture_swipe */
    NULL, /* swipe_init_time */
    0, /* swipe_init_start */
    0.0, /* wavearea_gesture_swipe_velocity_x */
    0, /* wavearea_drag_start_x */
    0, /* wavearea_drag_start_y */
    0, /* wavearea_drag_active */
#endif
    -1, /* use_gestures */
    FALSE, /*use_dark */
    FALSE, /*save_on_exit */

    /*
     * zoombuttons.c
     */
    1, /* do_zoom_center 660 */
    0, /* do_initial_zoom_fit 661 */
    0, /* do_initial_zoom_fit_used */
};

/*
 * prototypes (because of struct Global header recursion issues
 */
void *calloc_2_into_context(struct Global *g, size_t nmemb, size_t size);

/*
 * context manipulation functions
 */
struct Global *initialize_globals(void)
{
    struct Global *g = calloc(1, sizeof(struct Global)); /* allocate viewer context */

    memcpy(g, &globals_base_values, sizeof(struct Global)); /* fill in the blanks */

    g->gtk_context_bridge_ptr = calloc(1, sizeof(struct Global *));
    *(g->gtk_context_bridge_ptr) = g;

    g->buf_menu_c_1 = calloc_2_into_context(g, 1, 65537); /* do remaining mallocs into new ctx */
    g->regexp_string_menu_c_1 = calloc_2_into_context(g, 1, 129);
    g->regex_ok_regex_c_1 = calloc_2_into_context(g, WAVE_REGEX_TOTAL, sizeof(int));
    g->preg_regex_c_1 = calloc_2_into_context(g, WAVE_REGEX_TOTAL, sizeof(regex_t));

    g->strace_ctx = &g->strace_windows[0]; /* arbitrarily point to first one */

    g->hierarchy_icons = gw_hierarchy_icons_new();
    g->project = gw_project_new();
    g->color_theme = gw_color_theme_new();

    return (g); /* what to do with ctx is at discretion of caller */
}

void strcpy2_into_new_context(struct Global *g, char **newstrref, char **oldstrref)
{
    char *o = *oldstrref;
    char *n;

    if (o) /* only allocate + copy string if nonnull pointer */
    {
        n = calloc_2_into_context(g, 1, strlen(o) + 1);
        strcpy(n, o);
        *newstrref = n;
    }
}

/*
 * widget destruction functions
 */
static void widget_ungrab_destroy(GtkWidget **wp)
{
    if (*wp) {
        wave_gtk_grab_remove(*wp);
        gtk_widget_destroy(*wp);
        *wp = NULL;
    }
}

static void widget_only_destroy(GtkWidget **wp)
{
    if (*wp) {
        gtk_widget_destroy(*wp);
        *wp = NULL;
    }
}

/*
 * setjump to avoid -Wclobbered issues
 */
static int handle_setjmp(void)
{
    struct Global *setjmp_globals;
    int load_was_success = 0;

    GLOBALS->vcd_jmp_buf = calloc(1, sizeof(jmp_buf));
    setjmp_globals =
        calloc(1, sizeof(struct Global)); /* allocate yet another copy of viewer context */
    memcpy(setjmp_globals, GLOBALS, sizeof(struct Global)); /* clone */
    GLOBALS->alloc2_chain = NULL; /* will merge this in after load if successful */
    GLOBALS->outstanding = 0; /* zero out count of chunks in this ctx */

    if (!setjmp(*(GLOBALS->vcd_jmp_buf))) /* loader exception handling */
    {
        switch (GLOBALS->loaded_file_type) /* on fail, longjmp called in these loaders */
        {
            case VCD_RECODER_FILE:
                vcd_recoder_main(GLOBALS->loaded_file_name);
                break;
            case GHW_FILE:
            case FST_FILE:
            case DUMPLESS_FILE:
            case MISSING_FILE:
            default:
                break;
        }

#ifdef _WAVE_HAVE_JUDY
        {
            Pvoid_t PJArray = (Pvoid_t)setjmp_globals->alloc2_chain;
            int rcValue;
            Word_t Index;

            Index = 0;
            for (rcValue = Judy1First(PJArray, &Index, PJE0); rcValue != 0;
                 rcValue = Judy1Next(PJArray, &Index, PJE0)) {
                Judy1Set((Pvoid_t)&GLOBALS->alloc2_chain, Index, PJE0);
            }

            GLOBALS->outstanding += setjmp_globals->outstanding;
            Judy1FreeArray(&PJArray, PJE0);
        }
#else
        {
            void **t, **t2;

            t = (void **)setjmp_globals->alloc2_chain;
            while (t) {
                t2 = (void **)*(t + 1);
                if (t2) {
                    t = t2;
                } else {
                    *(t + 1) = GLOBALS->alloc2_chain;
                    if (GLOBALS->alloc2_chain) {
                        t2 = (void **)GLOBALS->alloc2_chain;
                        *(t2 + 0) = t;
                    }
                    GLOBALS->alloc2_chain = setjmp_globals->alloc2_chain;
                    GLOBALS->outstanding += setjmp_globals->outstanding;
                    break;
                }
            }
        }
#endif
        free(GLOBALS->vcd_jmp_buf);
        GLOBALS->vcd_jmp_buf = NULL;
        free(setjmp_globals);
        setjmp_globals = NULL;

        load_was_success = 1;
    } else {
        free(GLOBALS->vcd_jmp_buf);
        GLOBALS->vcd_jmp_buf = NULL;

        free_outstanding(); /* free anything allocated in loader ctx */

        memcpy(GLOBALS, setjmp_globals, sizeof(struct Global)); /* copy over old ctx */
        free(setjmp_globals); /* remove cached old ctx */
        /* now try again, jump through recovery sequence below */
    }

    return (load_was_success);
}

/*
 * reload from old into the new context
 */
void reload_into_new_context_2(void)
{
    FILE *statefile;
    struct Global *new_globals;
    /* gint tree_frame_x = -1; */ /* scan-build */
    gint tree_frame_y = -1;
    gdouble tree_vadj_value = 0.0;
    gdouble tree_hadj_value = 0.0;
    gdouble treeview_vadj_value = 0.0;
    gdouble treeview_hadj_value = 0.0;
    int fix_from_time = 0, fix_to_time = 0;
    GwTime from_time = GW_TIME_CONSTANT(0), to_time = GW_TIME_CONSTANT(0);
    char timestr[32];
    int load_was_success = 0;
    int reload_fail_delay = 1;
    char *save_tmpfilename = NULL;
    char *reload_tmpfilename = NULL;
    int fd_dummy = -1;
    int s_ctx_iter;

    /* save these in case we decide to write out the rc file later as a user option */
    char cached_ignore_savefile_pane_pos = GLOBALS->ignore_savefile_pane_pos;
    char cached_ignore_savefile_pos = GLOBALS->ignore_savefile_pos;
    char cached_ignore_savefile_size = GLOBALS->ignore_savefile_size;
    char cached_splash_disable = GLOBALS->splash_disable;

    if ((GLOBALS->loaded_file_type == MISSING_FILE) || (GLOBALS->is_optimized_stdin_vcd)) {
        fprintf(stderr, "GTKWAVE | Nothing to reload!\n");
        return;
    }

    logbox_reload();

    /* kill any pending splash screens (e.g., from Tcl "wish") */
    splash_button_press_event(NULL, NULL);

    /* fix problem where ungrab doesn't occur if button pressed + simultaneous reload accelerator
     * key occurs */
    if (GLOBALS->in_button_press_wavewindow_c_1) {
        XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
    }

    /* let all GTK/X events spin through in order to keep menus from freezing open during reload */
#ifndef MAC_INTEGRATION
    if (GLOBALS->text_status_c_2) {
        wave_gtk_grab_add(
            GLOBALS->text_status_c_2); /* grab focus to a known widget with no real side effects */
        gtkwave_main_iteration(); /* spin on GTK event loop */
        wave_gtk_grab_remove(GLOBALS->text_status_c_2); /* ungrab focus */
    }
#endif

    printf("GTKWAVE | Reloading waveform...\n");
    gtkwavetcl_setvar(WAVE_TCLCB_RELOAD_BEGIN,
                      GLOBALS->loaded_file_name,
                      WAVE_TCLCB_RELOAD_BEGIN_FLAGS);

    /* Save state to file */
    save_tmpfilename = tmpnam_2(NULL, &fd_dummy);
    statefile = save_tmpfilename ? fopen(save_tmpfilename, "wb") : NULL;
    if (statefile == NULL) {
        fprintf(stderr, "Failed to create temp file required for file reload.\n");
        if (save_tmpfilename) {
            perror("Why");
            free_2(save_tmpfilename);
        }
        gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR, "reload failed", WAVE_TCLCB_ERROR_FLAGS);
        return;
    }
    if (fd_dummy >= 0)
        close(fd_dummy);
    write_save_helper(save_tmpfilename, statefile);
    fclose(statefile);

    reload_tmpfilename = strdup(save_tmpfilename);
    free_2(save_tmpfilename);

    /* save off size of tree frame if active */
    if (GLOBALS->gtk2_tree_frame) {
        /* upper tree */
        GtkAdjustment *vadj =
            gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(GLOBALS->treeview_main));
        GtkAdjustment *hadj =
            gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(GLOBALS->treeview_main));

        if (vadj)
            tree_vadj_value = gtk_adjustment_get_value(vadj);
        if (hadj)
            tree_hadj_value = gtk_adjustment_get_value(hadj);

        GtkAllocation allocation;
        gtk_widget_get_allocation(GLOBALS->gtk2_tree_frame, &allocation);
        /* tree_frame_x = allocation.width; */ /* scan-build */
        tree_frame_y = allocation.height;

        /* lower signal set */
        vadj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(GLOBALS->dnd_sigview));
        hadj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(GLOBALS->dnd_sigview));

        if (vadj)
            treeview_vadj_value = gtk_adjustment_get_value(vadj);
        if (hadj)
            treeview_hadj_value = gtk_adjustment_get_value(hadj);
    }

    /* Kill any open processes */
    remove_all_proc_filters();
    remove_all_ttrans_filters();

    /* Instantiate new global status */
    new_globals = initialize_globals();
    free(new_globals->gtk_context_bridge_ptr); /* don't need this one as we're copying over the old
                                                  one... */
    new_globals->gtk_context_bridge_ptr = GLOBALS->gtk_context_bridge_ptr;

    /* SMP */
    new_globals->num_cpus = GLOBALS->num_cpus;

    /* tcl interpreter */
#if defined(HAVE_LIBTCL)
    new_globals->interp = GLOBALS->interp;
#endif

    /* Marker positions */
    // TODO: fix
    // memcpy(new_globals->named_markers, GLOBALS->named_markers, sizeof(GLOBALS->named_markers));
    new_globals->named_marker_lock_idx = GLOBALS->named_marker_lock_idx;

    /* notebook page flipping */
    new_globals->num_notebook_pages = GLOBALS->num_notebook_pages;
    new_globals->num_notebook_pages_cumulative = GLOBALS->num_notebook_pages_cumulative;
    new_globals->this_context_page = GLOBALS->this_context_page;
    new_globals->contexts = GLOBALS->contexts; /* this value is a *** chameleon!  malloc'd region is
                                                  outside debug.c control! */
    new_globals->notebook = GLOBALS->notebook;
    new_globals->second_page_created = GLOBALS->second_page_created;
    (*new_globals->contexts)[new_globals->this_context_page] = new_globals;
    new_globals->dead_context = GLOBALS->dead_context; /* this value is a ** chameleon!  malloc'd
                                                          region is outside debug.c control! */

    /* to cut down on temporary visual noise from incorrect zoom factor on reload */
    new_globals->pixelsperframe = GLOBALS->pixelsperframe;
    new_globals->nsperframe = GLOBALS->nsperframe;
    new_globals->pxns = GLOBALS->pxns;
    new_globals->nspx = GLOBALS->nspx;
    new_globals->nspx = GLOBALS->nspx;
    new_globals->zoom = GLOBALS->zoom;

    /* Default colors, X contexts, pixmaps, drawables, etc from signalwindow.c and wavewindow.c */
    new_globals->possibly_use_rc_defaults = GLOBALS->possibly_use_rc_defaults;

    new_globals->signalarea = GLOBALS->signalarea;
    new_globals->wavearea = GLOBALS->wavearea;
#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
    new_globals->wavearea_gesture_swipe = GLOBALS->wavearea_gesture_swipe;
#endif
    new_globals->wave_splash_pixbuf = GLOBALS->wave_splash_pixbuf;

    new_globals->black_and_white = GLOBALS->black_and_white;

    new_globals->mainwindow = GLOBALS->mainwindow;
    new_globals->signalwindow = GLOBALS->signalwindow;
    new_globals->wavewindow = GLOBALS->wavewindow;
    new_globals->toppanedwindow = GLOBALS->toppanedwindow;
    new_globals->panedwindow = GLOBALS->panedwindow;
    new_globals->sstpane = GLOBALS->sstpane;
    new_globals->sst_vpaned = GLOBALS->sst_vpaned;
    new_globals->expanderwindow = GLOBALS->expanderwindow;
    new_globals->wave_hslider = GLOBALS->wave_hslider;
    new_globals->hscroll_wavewindow_c_2 = GLOBALS->hscroll_wavewindow_c_2;
    new_globals->from_entry = GLOBALS->from_entry;
    new_globals->to_entry = GLOBALS->to_entry;

    new_globals->fontheight = GLOBALS->fontheight;
    new_globals->wavecrosspiece = GLOBALS->wavecrosspiece;

    new_globals->signalfont =
        calloc_2_into_context(new_globals, 1, sizeof(struct font_engine_font_t));
    memcpy(new_globals->signalfont, GLOBALS->signalfont, sizeof(struct font_engine_font_t));

    new_globals->wavefont =
        calloc_2_into_context(new_globals, 1, sizeof(struct font_engine_font_t));
    memcpy(new_globals->wavefont, GLOBALS->wavefont, sizeof(struct font_engine_font_t));

    new_globals->wavefont_smaller =
        calloc_2_into_context(new_globals, 1, sizeof(struct font_engine_font_t));
    memcpy(new_globals->wavefont_smaller,
           GLOBALS->wavefont_smaller,
           sizeof(struct font_engine_font_t));

    new_globals->fonts_screen = GLOBALS->fonts_screen;
    new_globals->fonts_context = GLOBALS->fonts_context;
    new_globals->fonts_layout = GLOBALS->fonts_layout;
    new_globals->use_pango_fonts = GLOBALS->use_pango_fonts;

    new_globals->ruler_origin = GLOBALS->ruler_origin;
    new_globals->ruler_step = GLOBALS->ruler_step;

    /* busy.c */
    new_globals->busycursor_busy_c_1 = GLOBALS->busycursor_busy_c_1;
    new_globals->busy_busy_c_1 = GLOBALS->busy_busy_c_1;

    /* rc.c */
    new_globals->scale_to_time_dimension = GLOBALS->scale_to_time_dimension;
    new_globals->zoom_dyn = GLOBALS->zoom_dyn;
    new_globals->zoom_dyne = GLOBALS->zoom_dyne;
    new_globals->context_tabposition = GLOBALS->context_tabposition;
    new_globals->dragzoom_threshold = GLOBALS->dragzoom_threshold;
    new_globals->cr_line_width = GLOBALS->cr_line_width;
    new_globals->cairo_050_offset = GLOBALS->cairo_050_offset;

    new_globals->ignore_savefile_pane_pos = 1; /* to keep window from resizing/jumping */
    new_globals->ignore_savefile_pos = 1; /* to keep window from resizing/jumping */
    new_globals->ignore_savefile_size = 1; /* to keep window from resizing/jumping */

    new_globals->color_theme = g_object_ref(GLOBALS->color_theme);

    new_globals->autoname_bundles = GLOBALS->autoname_bundles;
    new_globals->autocoalesce = GLOBALS->autocoalesce;
    new_globals->autocoalesce_reversal = GLOBALS->autocoalesce_reversal;
    new_globals->constant_marker_update = GLOBALS->constant_marker_update;
    new_globals->convert_to_reals = GLOBALS->convert_to_reals;
    new_globals->disable_mouseover = GLOBALS->disable_mouseover;
    new_globals->clipboard_mouseover = GLOBALS->clipboard_mouseover;
    new_globals->keep_xz_colors = GLOBALS->keep_xz_colors;
    new_globals->disable_tooltips = GLOBALS->disable_tooltips;
    new_globals->do_hier_compress = GLOBALS->do_hier_compress;
    new_globals->disable_auto_comphier = GLOBALS->disable_auto_comphier;
    new_globals->do_initial_zoom_fit = GLOBALS->do_initial_zoom_fit;
    new_globals->do_initial_zoom_fit_used = GLOBALS->do_initial_zoom_fit_used;
    new_globals->do_resize_signals = GLOBALS->do_resize_signals;
    new_globals->initial_signal_window_width = GLOBALS->initial_signal_window_width;
    new_globals->enable_fast_exit = GLOBALS->enable_fast_exit;
    new_globals->enable_ghost_marker = GLOBALS->enable_ghost_marker;
    new_globals->enable_horiz_grid = GLOBALS->enable_horiz_grid;
    new_globals->make_vcd_save_file = GLOBALS->make_vcd_save_file;
    new_globals->enable_vert_grid = GLOBALS->enable_vert_grid;
    new_globals->sst_expanded = GLOBALS->sst_expanded;
    new_globals->hier_max_level = GLOBALS->hier_max_level;
    new_globals->hier_max_level_shadow = GLOBALS->hier_max_level_shadow;
    new_globals->paned_pack_semantics = GLOBALS->paned_pack_semantics;
    new_globals->left_justify_sigs = GLOBALS->left_justify_sigs;
    new_globals->extload_max_tree = GLOBALS->extload_max_tree;
    new_globals->ps_maxveclen = GLOBALS->ps_maxveclen;
    new_globals->show_base = GLOBALS->show_base;
    new_globals->display_grid = GLOBALS->display_grid;
    new_globals->fullscreen = GLOBALS->fullscreen;
    new_globals->show_toolbar = GLOBALS->show_toolbar;
    new_globals->time_box = GLOBALS->time_box;
    new_globals->highlight_wavewindow = GLOBALS->highlight_wavewindow;
    new_globals->fill_waveform = GLOBALS->fill_waveform;
    new_globals->lz_removal = GLOBALS->lz_removal;
    new_globals->use_big_fonts = GLOBALS->use_big_fonts;
    new_globals->use_full_precision = GLOBALS->use_full_precision;
    new_globals->use_frequency_delta = GLOBALS->use_frequency_delta;
    new_globals->use_maxtime_display = GLOBALS->use_maxtime_display;
    new_globals->use_nonprop_fonts = GLOBALS->use_nonprop_fonts;
    new_globals->use_roundcaps = GLOBALS->use_roundcaps;
    new_globals->vcd_preserve_glitches = GLOBALS->vcd_preserve_glitches;
    new_globals->vcd_preserve_glitches_real = GLOBALS->vcd_preserve_glitches_real;
    new_globals->vcd_warning_filesize = GLOBALS->vcd_warning_filesize;
    new_globals->vector_padding = GLOBALS->vector_padding;
    new_globals->vlist_prepack = GLOBALS->vlist_prepack;
    new_globals->vlist_compression_depth = GLOBALS->vlist_compression_depth;
    new_globals->wave_scrolling = GLOBALS->wave_scrolling;
    new_globals->do_zoom_center = GLOBALS->do_zoom_center;
    new_globals->zoom_pow10_snap = GLOBALS->zoom_pow10_snap;
    new_globals->alt_hier_delimeter = GLOBALS->alt_hier_delimeter;
    new_globals->cursor_snap = GLOBALS->cursor_snap;
    new_globals->hier_delimeter = GLOBALS->hier_delimeter;
    new_globals->hier_was_explicitly_set = GLOBALS->hier_was_explicitly_set;
    new_globals->page_divisor = GLOBALS->page_divisor;
    new_globals->ps_maxveclen = GLOBALS->ps_maxveclen;
    new_globals->vector_padding = GLOBALS->vector_padding;
    new_globals->zoombase = GLOBALS->zoombase;

    new_globals->hier_ignore_escapes = GLOBALS->hier_ignore_escapes;

    new_globals->splash_disable = 1; /* to disable splash for reload */
    new_globals->strace_repeat_count =
        GLOBALS->strace_repeat_count; /* for edgebuttons and also strace */

    new_globals->logfiles = GLOBALS->logfiles; /* this value is a ** chameleon!  malloc'd region is
                                                  outside debug.c control! */
    new_globals->sst_dbl_action_type = GLOBALS->sst_dbl_action_type;
    new_globals->use_gestures = GLOBALS->use_gestures;
    new_globals->use_dark = GLOBALS->use_dark;
    new_globals->save_on_exit = GLOBALS->save_on_exit;
    new_globals->dbl_mant_dig_override = GLOBALS->dbl_mant_dig_override;

    strcpy2_into_new_context(new_globals, &new_globals->argvlist, &GLOBALS->argvlist);
    strcpy2_into_new_context(new_globals, &new_globals->editor_name, &GLOBALS->editor_name);
    strcpy2_into_new_context(new_globals,
                             &new_globals->fontname_logfile,
                             &GLOBALS->fontname_logfile);
    strcpy2_into_new_context(new_globals,
                             &new_globals->fontname_signals,
                             &GLOBALS->fontname_signals);
    strcpy2_into_new_context(new_globals, &new_globals->fontname_waves, &GLOBALS->fontname_waves);
    strcpy2_into_new_context(new_globals, &new_globals->cutcopylist, &GLOBALS->cutcopylist);
    strcpy2_into_new_context(new_globals, &new_globals->tcl_init_cmd, &GLOBALS->tcl_init_cmd);
    strcpy2_into_new_context(new_globals, &new_globals->repscript_name, &GLOBALS->repscript_name);
    new_globals->repscript_period = GLOBALS->repscript_period;
    strcpy2_into_new_context(new_globals,
                             &new_globals->pFileChooseFilterName,
                             &GLOBALS->pFileChooseFilterName);

    /* search.c */
    new_globals->regex_which_search_c_1 =
        GLOBALS->regex_which_search_c_1; /* preserve search type */

    /* ttranslate.c */
    strcpy2_into_new_context(new_globals, &new_globals->ttranslate_args, &GLOBALS->ttranslate_args);

    /* lxt2.c / vzt.c / ae2.c */
    strcpy2_into_new_context(new_globals, &new_globals->skip_start, &GLOBALS->skip_start);
    strcpy2_into_new_context(new_globals, &new_globals->skip_end, &GLOBALS->skip_end);

    /* main.c */
    new_globals->missing_file_toolbar = GLOBALS->missing_file_toolbar;
    new_globals->is_gtkw_save_file = GLOBALS->is_gtkw_save_file;
    new_globals->optimize_vcd = GLOBALS->optimize_vcd;
    strcpy2_into_new_context(new_globals,
                             &new_globals->winname,
                             &GLOBALS->winname); /* for page swapping */

    /* menu.c */
    new_globals->wave_script_args = GLOBALS->wave_script_args;
    strcpy2_into_new_context(new_globals,
                             &new_globals->filesel_writesave,
                             &GLOBALS->filesel_writesave);
    new_globals->save_success_menu_c_1 = GLOBALS->save_success_menu_c_1;
    new_globals->signal_popup_menu = GLOBALS->signal_popup_menu;
    new_globals->sst_signal_popup_menu = GLOBALS->sst_signal_popup_menu;

    strcpy2_into_new_context(new_globals,
                             &new_globals->filesel_vcd_writesave,
                             &GLOBALS->filesel_vcd_writesave);
    strcpy2_into_new_context(new_globals,
                             &new_globals->filesel_tim_writesave,
                             &GLOBALS->filesel_tim_writesave);

    strcpy2_into_new_context(
        new_globals,
        &new_globals->stems_name,
        &GLOBALS->stems_name); /* remaining fileselbox() vars not handled elsewhere */
    strcpy2_into_new_context(new_globals,
                             &new_globals->filesel_logfile_menu_c_1,
                             &GLOBALS->filesel_logfile_menu_c_1);
    strcpy2_into_new_context(new_globals,
                             &new_globals->filesel_scriptfile_menu,
                             &GLOBALS->filesel_scriptfile_menu);
    strcpy2_into_new_context(new_globals,
                             &new_globals->fcurr_ttranslate_c_1,
                             &GLOBALS->fcurr_ttranslate_c_1);
    strcpy2_into_new_context(new_globals,
                             &new_globals->fcurr_ptranslate_c_1,
                             &GLOBALS->fcurr_ptranslate_c_1);
    strcpy2_into_new_context(new_globals,
                             &new_globals->fcurr_translate_c_2,
                             &GLOBALS->fcurr_translate_c_2);

    strcpy2_into_new_context(new_globals,
                             &new_globals->filesel_imagegrab,
                             &GLOBALS->filesel_imagegrab);

    /* renderopt.c */
#ifdef WAVE_GTK_UNIX_PRINT
    new_globals->gprs = GLOBALS->gprs;
    new_globals->gps = GLOBALS->gps;
#endif

    /* status.c */
    new_globals->text_status_c_2 = GLOBALS->text_status_c_2;
    memcpy(&new_globals->iter_status_c_3, &GLOBALS->iter_status_c_3, sizeof(GtkTextIter));

    /* treesearch_gtk2.c */
    new_globals->do_dynamic_treefilter = GLOBALS->do_dynamic_treefilter;
    new_globals->treesearch_gtk2_window_vbox = GLOBALS->treesearch_gtk2_window_vbox;
    new_globals->dnd_sigview = GLOBALS->dnd_sigview;

    strcpy2_into_new_context(new_globals,
                             &new_globals->filter_str_treesearch_gtk2_c_1,
                             &GLOBALS->filter_str_treesearch_gtk2_c_1);
    strcpy2_into_new_context(new_globals,
                             &new_globals->selected_hierarchy_name,
                             &GLOBALS->selected_hierarchy_name);
    new_globals->filter_typ_treesearch_gtk2_c_1 = GLOBALS->filter_typ_treesearch_gtk2_c_1;
    new_globals->filter_matlen_treesearch_gtk2_c_1 = GLOBALS->filter_matlen_treesearch_gtk2_c_1;
    new_globals->filter_noregex_treesearch_gtk2_c_1 = GLOBALS->filter_noregex_treesearch_gtk2_c_1;

    strcpy2_into_new_context(new_globals,
                             &new_globals->sst_exclude_filename,
                             &GLOBALS->sst_exclude_filename);
    if (GLOBALS->exclcompname) {
        jrb_free_tree(GLOBALS->exclcompname); /* strings get freed by automatic _2 mechanism */
        GLOBALS->exclcompname = NULL;
    }
    if (GLOBALS->exclinstname) {
        jrb_free_tree(GLOBALS->exclinstname); /* strings get freed by automatic _2 mechanism */
        GLOBALS->exclinstname = NULL;
    }

    /* timeentry.c */
    new_globals->from_entry = GLOBALS->from_entry;
    new_globals->to_entry = GLOBALS->to_entry;

    if (GLOBALS->tims.first != GLOBALS->min_time) {
        fix_from_time = 1;
        from_time = GLOBALS->tims.first;
    }

    if (GLOBALS->tims.last != GLOBALS->max_time) {
        fix_to_time = 1;
        to_time = GLOBALS->tims.last;
    }

    /* twinwave stuff */
    new_globals->dual_attach_id_main_c_1 = GLOBALS->dual_attach_id_main_c_1;
    new_globals->dual_id = GLOBALS->dual_id;
    new_globals->socket_xid = GLOBALS->socket_xid;
    new_globals->dual_ctx = GLOBALS->dual_ctx;

    /* Times struct */
    memcpy(&(new_globals->tims), &(GLOBALS->tims), sizeof(Times));

    /* File name and type */
    new_globals->loaded_file_type = GLOBALS->loaded_file_type;

    strcpy2_into_new_context(new_globals,
                             &new_globals->loaded_file_name,
                             &GLOBALS->loaded_file_name);

    if (new_globals->optimize_vcd) {
        strcpy2_into_new_context(new_globals,
                                 &new_globals->unoptimized_vcd_file_name,
                                 &GLOBALS->unoptimized_vcd_file_name);
    }

    if (GLOBALS->enum_nptrs_jrb) {
        jrb_free_tree(GLOBALS->enum_nptrs_jrb);
        GLOBALS->enum_nptrs_jrb = NULL;
    }

#ifdef _WAVE_HAVE_JUDY
    if (GLOBALS->num_xl_enum_filter) {
        int ie;
        for (ie = 0; ie < GLOBALS->num_xl_enum_filter; ie++) {
            JudyHSFreeArray(&GLOBALS->xl_enum_filter[ie], NULL);
        }

        GLOBALS->num_xl_enum_filter = 0;
    }
#endif

    /* deallocate any loader-related stuff */
    switch (GLOBALS->loaded_file_type) {
        case FST_FILE:
            g_clear_pointer(&GLOBALS->fst_file, fst_file_close);
            break;

#ifdef EXTLOAD_SUFFIX
        case EXTLOAD_FILE:
            if (GLOBALS->extload_ffr_ctx) {
#ifdef WAVE_FSDB_READER_IS_PRESENT
                fsdbReaderClose(GLOBALS->extload_ffr_ctx);
#endif
                GLOBALS->extload_ffr_ctx = NULL;
            }
            break;
#endif

        case MISSING_FILE:
        case DUMPLESS_FILE:
        case GHW_FILE:
        case VCD_RECODER_FILE:
        default:
            /* do nothing */ break;
    }

    /* window destruction (of windows that aren't the parent window) */

    kill_stems_browser(); /* for now, need to rework the stems browser dumpfile access routines to
                             support this properly */
    new_globals->stems_type = GLOBALS->stems_type;
    strcpy2_into_new_context(new_globals, &new_globals->aet_name, &GLOBALS->aet_name);

    /* remaining windows which are completely destroyed and haven't migrated over */
    widget_only_destroy(&GLOBALS->window_ptranslate_c_5); /* ptranslate.c */
    WAVE_STRACE_ITERATOR(s_ctx_iter)
    {
        GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
        widget_only_destroy(&GLOBALS->strace_ctx->window_strace_c_10); /* strace.c */
    }
    widget_only_destroy(&GLOBALS->window_translate_c_11); /* translate.c */

    /* windows which in theory should never destroy as they will have grab focus which means reload
     * will not be called */
    widget_ungrab_destroy(&GLOBALS->window1_search_c_2); /* search.c */
    widget_ungrab_destroy(&GLOBALS->window_simplereq_c_9); /* simplereq.c */
    widget_ungrab_destroy(&GLOBALS->window1_treesearch_gtk2_c_3); /* treesearch_gtk2.c */

    /* supported migration of window contexts... */

    if (GLOBALS->mouseover_mouseover_c_1) /* mouseover regenerates as the pointer moves so no real
                                             context lost */
    {
        gtk_widget_destroy(GLOBALS->mouseover_mouseover_c_1);
        GLOBALS->mouseover_mouseover_c_1 = NULL;
    }

    if (GLOBALS->window_renderopt_c_6) {
        new_globals->is_active_renderopt_c_3 = GLOBALS->is_active_renderopt_c_3;
        new_globals->window_renderopt_c_6 = GLOBALS->window_renderopt_c_6;

        memcpy(new_globals->target_mutex_renderopt_c_1,
               GLOBALS->target_mutex_renderopt_c_1,
               sizeof(GLOBALS->target_mutex_renderopt_c_1));
        memcpy(new_globals->page_mutex_renderopt_c_1,
               GLOBALS->page_mutex_renderopt_c_1,
               sizeof(GLOBALS->page_mutex_renderopt_c_1));
        memcpy(new_globals->render_mutex_renderopt_c_1,
               GLOBALS->render_mutex_renderopt_c_1,
               sizeof(GLOBALS->render_mutex_renderopt_c_1));

        strcpy2_into_new_context(new_globals,
                                 &new_globals->filesel_print_pdf_renderopt_c_1,
                                 &GLOBALS->filesel_print_pdf_renderopt_c_1);
        strcpy2_into_new_context(new_globals,
                                 &new_globals->filesel_print_ps_renderopt_c_1,
                                 &GLOBALS->filesel_print_ps_renderopt_c_1);
        strcpy2_into_new_context(new_globals,
                                 &new_globals->filesel_print_mif_renderopt_c_1,
                                 &GLOBALS->filesel_print_mif_renderopt_c_1);

        new_globals->page_size_type_renderopt_c_1 = GLOBALS->page_size_type_renderopt_c_1;
    }

    if (GLOBALS->window_search_c_7) {
        int i;

        new_globals->pdata = calloc_2_into_context(new_globals, 1, sizeof(SearchProgressData));
        memcpy(new_globals->pdata, GLOBALS->pdata, sizeof(SearchProgressData));

        new_globals->is_active_search_c_4 = GLOBALS->is_active_search_c_4;
        new_globals->regex_which_search_c_1 = GLOBALS->regex_which_search_c_1;
        new_globals->window_search_c_7 = GLOBALS->window_search_c_7;
        new_globals->entry_search_c_3 = GLOBALS->entry_search_c_3;
        new_globals->sig_store_search = GLOBALS->sig_store_search;
        new_globals->sig_selection_search = GLOBALS->sig_selection_search;
        new_globals->sig_view_search = GLOBALS->sig_view_search;

        strcpy2_into_new_context(new_globals,
                                 &new_globals->searchbox_text_search_c_1,
                                 &GLOBALS->searchbox_text_search_c_1);

        for (i = 0; i < 5; i++) {
            new_globals->menuitem_search[i] = GLOBALS->menuitem_search[i];
            new_globals->regex_mutex_search_c_1[i] = GLOBALS->regex_mutex_search_c_1[i];
        }
    }

#ifdef WAVE_ALLOW_GTK3_GESTURE_EVENT
    if (GLOBALS->swipe_init_time) {
        g_date_time_unref(GLOBALS->swipe_init_time);
        GLOBALS->swipe_init_time = NULL;
    }
#endif

    /* erase any old tabbed contexts if they exist... */
    dead_context_sweep();

    /* let any destructors finalize on GLOBALS dereferences... (commented out for now) */
    /* gtkwave_main_iteration(); */

    /* Free the old context */
    free_outstanding();

    /* Free the old globals struct, memsetting it to zero in the hope of forcing crashes. */
    memset(GLOBALS, 0, sizeof(struct Global));
    free(GLOBALS);
    GLOBALS = NULL; /* valgrind fix */

    /* Set the GLOBALS pointer to the newly allocated struct. */
    set_GLOBALS(new_globals);
    *(GLOBALS->gtk_context_bridge_ptr) = GLOBALS;

    init_filetrans_data();
    init_proctrans_data();
    init_ttrans_data();
    /* load_all_fonts(); */

    sst_exclusion_loader();

    /* attempt to reload file and recover on loader errors until successful */
    for (;;) {
        set_window_busy(NULL);

        /* Check to see if we need to reload a vcd file */
#if !defined __MINGW32__
        if (GLOBALS->optimize_vcd) {
            optimize_vcd_file();
        }
#endif
        /* Load new file from disk, no reload on partial vcd or vcd from stdin. */
        switch (GLOBALS->loaded_file_type) {
#ifdef EXTLOAD_SUFFIX
            case EXTLOAD_FILE:
                extload_main(GLOBALS->loaded_file_name, GLOBALS->skip_start, GLOBALS->skip_end);
                load_was_success =
                    (GLOBALS->extload != NULL) && (!GLOBALS->extload_already_errored);
                GLOBALS->extload_already_errored = 0;
                GLOBALS->extload_lastmod = 0;
                break;
#endif

            case FST_FILE:
                fst_main(GLOBALS->loaded_file_name, GLOBALS->skip_start, GLOBALS->skip_end);
                load_was_success = (GLOBALS->fst_file != NULL);
                break;

            case GHW_FILE:
                load_was_success = (ghw_main(GLOBALS->loaded_file_name) != 0);
                break;

            case VCD_RECODER_FILE:
                load_was_success = handle_setjmp();
                break;

            case MISSING_FILE:
            case DUMPLESS_FILE:
            default:
                break;
        }

        set_window_idle(NULL);

        if (load_was_success) {
            break;
        } else {
            /* recovery sequence */
            set_window_busy(NULL);
            {
                printf("GTKWAVE | Reload failure, reattempt in %d second%s...\n",
                       reload_fail_delay,
                       (reload_fail_delay == 1) ? "" : "s");
                fflush(stdout);
            }
            sleep(reload_fail_delay);
            switch (reload_fail_delay) {
                case 1:
                    reload_fail_delay = 2;
                    break;
                case 2:
                    reload_fail_delay = 5;
                    break;
                case 5:
                    reload_fail_delay = 10;
                    break;
                case 10:
                    reload_fail_delay = 30;
                    break;
                case 30:
                    reload_fail_delay = 10;
                    break; /* recycle back so message changes if header bar */
                default:
                    break;
            }
            set_window_idle(NULL);
        }
    }

    /* deallocate the symbol hash table */
    sym_hash_destroy(GLOBALS);

    /* Setup timings we probably need to redraw here */
    GLOBALS->tims.last = GLOBALS->max_time;
    GLOBALS->tims.first = GLOBALS->min_time;

    if (fix_from_time) {
        if ((from_time >= GLOBALS->min_time) && (from_time <= GLOBALS->max_time)) {
            GLOBALS->tims.first = from_time;
        }
    }

    if (fix_to_time) {
        if ((to_time >= GLOBALS->min_time) && (to_time <= GLOBALS->max_time) &&
            (to_time > GLOBALS->tims.first)) {
            GLOBALS->tims.last = to_time;
        }
    }

    if (GLOBALS->tims.start < GLOBALS->tims.first) {
        GLOBALS->tims.start = GLOBALS->tims.first;
    }
    if (GLOBALS->tims.end > GLOBALS->tims.last) {
        GLOBALS->tims.end = GLOBALS->tims.last;
    }

    GwMarker *primary_marker = gw_project_get_primary_marker(GLOBALS->project);
    GwTime primary_pos = gw_marker_get_position(primary_marker);
    gw_marker_set_position(primary_marker,
                           CLAMP(primary_pos, GLOBALS->tims.first, GLOBALS->tims.last));

    if (GLOBALS->tims.prevmarker < GLOBALS->tims.first) {
        GLOBALS->tims.prevmarker = GLOBALS->tims.first;
    }
    if (GLOBALS->tims.prevmarker > GLOBALS->tims.last) {
        GLOBALS->tims.prevmarker = GLOBALS->tims.last;
    }
    if (GLOBALS->tims.laststart < GLOBALS->tims.first) {
        GLOBALS->tims.laststart = GLOBALS->tims.first;
    }
    if (GLOBALS->tims.laststart > GLOBALS->tims.last) {
        GLOBALS->tims.laststart = GLOBALS->tims.last;
    }

    reformat_time(timestr,
                  GLOBALS->tims.first + GLOBALS->global_time_offset,
                  GLOBALS->time_dimension);
    gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry), timestr);
    reformat_time(timestr,
                  GLOBALS->tims.last + GLOBALS->global_time_offset,
                  GLOBALS->time_dimension);
    gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry), timestr);

    /* Change SST - if it exists */
    /* XXX need to destroy/free the old tree widgets. */
    gint pane_pos = gtk_paned_get_position(GLOBALS->sst_vpaned);
    gtk_widget_hide(GLOBALS->expanderwindow);
    gtk_container_remove(GTK_CONTAINER(GLOBALS->expanderwindow), GLOBALS->sstpane);
    GLOBALS->sstpane = treeboxframe("SST");
    gtk_container_add(GTK_CONTAINER(GLOBALS->expanderwindow), GLOBALS->sstpane);
    gtk_paned_set_position(GLOBALS->sst_vpaned, pane_pos);
    gtk_widget_show(GLOBALS->expanderwindow);
    if (GLOBALS->dnd_sigview) {
        dnd_setup(GLOBALS->dnd_sigview, FALSE);
    }

    if (GLOBALS->sig_view_search) {
        dnd_setup(GLOBALS->sig_view_search, TRUE);
    }

    if ((GLOBALS->filter_str_treesearch_gtk2_c_1) && (GLOBALS->filter_entry)) {
        gtk_entry_set_text(GTK_ENTRY(GLOBALS->filter_entry),
                           GLOBALS->filter_str_treesearch_gtk2_c_1);
        wave_regex_compile(GLOBALS->filter_str_treesearch_gtk2_c_1 +
                               GLOBALS->filter_matlen_treesearch_gtk2_c_1,
                           WAVE_REGEX_TREE);
    }

    /* Reload state from file */
    {
        char is_gtkw_save_file_cached = GLOBALS->is_gtkw_save_file;
        read_save_helper(reload_tmpfilename, NULL, NULL, NULL, NULL, NULL);
        GLOBALS->is_gtkw_save_file = is_gtkw_save_file_cached;
    }

    GLOBALS->wavewidth = gtk_widget_get_allocated_width(GTK_WIDGET(GLOBALS->wavearea));
    GLOBALS->waveheight = gtk_widget_get_allocated_height(GTK_WIDGET(GLOBALS->wavearea));
    calczoom(GLOBALS->tims.zoom);

    /* again doing this here (read_save_helper does it) seems to be necessary in order to keep
     * display in sync */
    GLOBALS->signalwindow_width_dirty = 1;
    redraw_signals_and_waves();

    /* unlink temp */
    unlink(reload_tmpfilename);
    free(reload_tmpfilename); /* intentional use of free(), of strdup'd string from earlier */

    /* part 2 of SST (which needs to be done after the tree is expanded from loading the
     * savefile...) */
    char *hiername_cache = NULL; /* some strange race condition side effect in gtk
                                    selecting/deselecting blows this out so cache it */

    if (GLOBALS->selected_hierarchy_name) {
        hiername_cache = strdup_2(GLOBALS->selected_hierarchy_name);
        select_tree_node(GLOBALS->selected_hierarchy_name);
    }

    if (tree_vadj_value != 0.0) {
        GtkAdjustment *vadj =
            gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(GLOBALS->treeview_main));

        if ((vadj) && (tree_vadj_value >= gtk_adjustment_get_value(vadj)) &&
            (tree_vadj_value <= gtk_adjustment_get_upper(vadj))) {
            gtk_adjustment_set_value(vadj, tree_vadj_value);
            gtk_scrollable_set_vadjustment(GTK_SCROLLABLE(GLOBALS->treeview_main), vadj);

            g_signal_emit_by_name(GTK_ADJUSTMENT(vadj), "changed");
            g_signal_emit_by_name(GTK_ADJUSTMENT(vadj), "value_changed");
        }
    }

    if (tree_hadj_value != 0.0) {
        GtkAdjustment *hadj =
            gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(GLOBALS->treeview_main));

        if ((hadj) && (tree_hadj_value >= gtk_adjustment_get_lower(hadj)) &&
            (tree_hadj_value <= gtk_adjustment_get_upper(hadj))) {
            gtk_adjustment_set_value(hadj, tree_hadj_value);
            gtk_scrollable_set_hadjustment(GTK_SCROLLABLE(GLOBALS->treeview_main), hadj);

            g_signal_emit_by_name(GTK_ADJUSTMENT(hadj), "changed");
            g_signal_emit_by_name(GTK_ADJUSTMENT(hadj), "value_changed");
        }
    }

    if (hiername_cache) {
        if (GLOBALS->selected_hierarchy_name) {
            free_2(GLOBALS->selected_hierarchy_name);
        }
        GLOBALS->selected_hierarchy_name = hiername_cache;
    }

    if (tree_frame_y != -1) {
        /* this doesn't work...this sets the *minimum* size */
        /* gtk_widget_set_size_request(GLOBALS->gtk2_tree_frame, -1, tree_frame_y); */
    }

    if (GLOBALS->filter_entry) {
        g_signal_emit_by_name(GLOBALS->filter_entry, "activate");
    }

    /* part 2 of search (which needs to be done after the new dumpfile is loaded) */
    if (GLOBALS->window_search_c_7) {
        search_enter_callback(GLOBALS->entry_search_c_3, NULL);
    }

    /* restore these in case we decide to write out the rc file later as a user option */
    GLOBALS->ignore_savefile_pane_pos = cached_ignore_savefile_pane_pos;
    GLOBALS->ignore_savefile_pos = cached_ignore_savefile_pos;
    GLOBALS->ignore_savefile_size = cached_ignore_savefile_size;
    GLOBALS->splash_disable = cached_splash_disable;

    printf("GTKWAVE | ...waveform reloaded\n");
    gtkwavetcl_setvar(WAVE_TCLCB_RELOAD_END,
                      GLOBALS->loaded_file_name,
                      WAVE_TCLCB_RELOAD_END_FLAGS);

    /* update lower signal set in SST to correct position */
    if ((GLOBALS->dnd_sigview) && ((treeview_vadj_value != 0.0) || (treeview_hadj_value != 0.0))) {
        struct Global *G2 = GLOBALS;
        gtk_events_pending_gtk_main_iteration();

        if ((G2 == GLOBALS) && (GLOBALS->dnd_sigview)) {
            if (treeview_vadj_value != 0.0) {
                GtkAdjustment *vadj =
                    gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(GLOBALS->dnd_sigview));

                if ((vadj) && (treeview_vadj_value >= gtk_adjustment_get_lower(vadj)) &&
                    (treeview_vadj_value <= gtk_adjustment_get_upper(vadj))) {
                    gtk_adjustment_set_value(vadj, treeview_vadj_value);
                    gtk_scrollable_set_vadjustment(GTK_SCROLLABLE(GLOBALS->dnd_sigview), vadj);

                    g_signal_emit_by_name(GTK_ADJUSTMENT(vadj), "changed");
                    g_signal_emit_by_name(GTK_ADJUSTMENT(vadj), "value_changed");
                }
            }

            if (treeview_hadj_value != 0.0) {
                GtkAdjustment *hadj =
                    gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(GLOBALS->dnd_sigview));

                if ((hadj) && (treeview_hadj_value >= gtk_adjustment_get_lower(hadj)) &&
                    (treeview_hadj_value <= gtk_adjustment_get_upper(hadj))) {
                    gtk_adjustment_set_value(hadj, treeview_hadj_value);
                    gtk_scrollable_set_hadjustment(GTK_SCROLLABLE(GLOBALS->dnd_sigview), hadj);

                    g_signal_emit_by_name(GTK_ADJUSTMENT(hadj), "changed");
                    g_signal_emit_by_name(GTK_ADJUSTMENT(hadj), "value_changed");
                }
            }
        }
    }
}

void reload_into_new_context(void)
{
    static int reloading = 0;

    if (!reloading) {
#ifdef MAC_INTEGRATION
        osx_menu_sensitivity(FALSE);
#endif
        reload_into_new_context_2();
        reloading = 0;
#ifdef MAC_INTEGRATION
        if (GLOBALS->loaded_file_type != MISSING_FILE) {
            osx_menu_sensitivity(TRUE);
        }
#endif
    }
}

/*
 * for notebook page closing
 */
void free_and_destroy_page_context(void)
{
    int s_ctx_iter;

    /* deallocate any loader-related stuff */
    switch (GLOBALS->loaded_file_type) {
        case FST_FILE:
            g_clear_pointer(&GLOBALS->fst_file, fst_file_close);
            break;

#ifdef EXTLOAD_SUFFIX
        case EXTLOAD_FILE:
            if (GLOBALS->extload_ffr_ctx) {
#ifdef WAVE_FSDB_READER_IS_PRESENT
                fsdbReaderClose(GLOBALS->extload_ffr_ctx);
#endif
                GLOBALS->extload_ffr_ctx = NULL;
            }
            break;
#endif

        case MISSING_FILE:
        case DUMPLESS_FILE:
        case GHW_FILE:
        case VCD_RECODER_FILE:
        default:
            /* do nothing */ break;
    }

    /* window destruction (of windows that aren't the parent window) */

    widget_only_destroy(&GLOBALS->window_ptranslate_c_5); /* ptranslate.c */

    WAVE_STRACE_ITERATOR(s_ctx_iter)
    {
        GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
        widget_only_destroy(&GLOBALS->strace_ctx->window_strace_c_10); /* strace.c */
    }
    widget_only_destroy(&GLOBALS->window_translate_c_11); /* translate.c */

    /* windows which in theory should never destroy as they will have grab focus which means reload
     * will not be called */
    widget_ungrab_destroy(&GLOBALS->window1_search_c_2); /* search.c */
    widget_ungrab_destroy(&GLOBALS->window_simplereq_c_9); /* simplereq.c */
    widget_ungrab_destroy(&GLOBALS->window1_treesearch_gtk2_c_3); /* treesearch_gtk2.c */

    if (GLOBALS->mouseover_mouseover_c_1) /* mouseover regenerates as the pointer moves so no real
                                             context lost */
    {
        gtk_widget_destroy(GLOBALS->mouseover_mouseover_c_1);
        GLOBALS->mouseover_mouseover_c_1 = NULL;
    }

    widget_only_destroy(&GLOBALS->window_renderopt_c_6);
    widget_only_destroy(&GLOBALS->window_search_c_7);

    (*GLOBALS->dead_context)[0] = GLOBALS;

    /* let any destructors finalize on GLOBALS dereferences... */
    gtkwave_main_iteration();
}

/*
 * part 2 of free_and_destroy_page_context() as some context data seems to be used later by
 * gtk (i.e., the destroys might be deferred slightly causing corruption)
 */
void dead_context_sweep(void)
{
    struct Global *gp = (*GLOBALS->dead_context)[0];
    struct Global *g_curr;

    if (gp) {
        g_curr = GLOBALS;

        set_GLOBALS(gp);
        (*GLOBALS->dead_context)[0] = NULL;

        /* remove the bridge pointer */
        if (GLOBALS->gtk_context_bridge_ptr) {
            free(GLOBALS->gtk_context_bridge_ptr);
            GLOBALS->gtk_context_bridge_ptr = NULL;
        }

        /* Free all gcs */
        /* dealloc_all_gcs(); */

        /* Free the context */
        free_outstanding();

        /* Free the old globals struct, memsetting it to zero in the hope of forcing crashes. */
        memset(GLOBALS, 0, sizeof(struct Global));
        free(GLOBALS);
        GLOBALS = NULL; /* valgrind fix */

        set_GLOBALS(g_curr);
    }
}

/*
 * focus directed context switching of GLOBALS in multiple tabs mode
 */
static gint context_swapper(GtkWindow *w, GdkEvent *event, void *data)
{
    GdkEventType type;

    if (ignore_context_swap()) /* block context swap if explicitly directed (e.g., during loading)
                                */
    {
        return (FALSE);
    }

    type = event->type;

    /* printf("Window: %08x GtkEvent: %08x gpointer: %08x, type: %d\n", w, event, data, type); */

    if (type == GDK_ENTER_NOTIFY || type == GDK_FOCUS_CHANGE) {
        if (GLOBALS->num_notebook_pages >= 2) {
            unsigned int i;
            void **vp;
            GtkWindow *wcmp;

            for (i = 0; i < GLOBALS->num_notebook_pages; i++) {
                struct Global *test_g = (*GLOBALS->contexts)[i];

                vp = (void **)(((char *)test_g) + (intptr_t)data);
                wcmp = (GtkWindow *)(*vp);

                if (wcmp != NULL) {
                    if (wcmp == w) {
                        if (i != GLOBALS->this_context_page) {
                            struct Global *g_old = GLOBALS;

                            /* printf("Switching to: %d %08x\n", i, GTK_WINDOW(wcmp)); */

                            set_GLOBALS((*GLOBALS->contexts)[i]);

                            GLOBALS->autoname_bundles = g_old->autoname_bundles;
                            GLOBALS->autocoalesce_reversal = g_old->autocoalesce_reversal;
                            GLOBALS->autocoalesce = g_old->autocoalesce;
                            GLOBALS->wave_scrolling = g_old->wave_scrolling;
                            GLOBALS->constant_marker_update = g_old->constant_marker_update;
                            GLOBALS->do_zoom_center = g_old->do_zoom_center;
                            GLOBALS->use_roundcaps = g_old->use_roundcaps;
                            GLOBALS->do_resize_signals = g_old->do_resize_signals;
                            GLOBALS->initial_signal_window_width =
                                g_old->initial_signal_window_width;
                            GLOBALS->use_full_precision = g_old->use_full_precision;
                            GLOBALS->show_base = g_old->show_base;
                            GLOBALS->display_grid = g_old->display_grid;
                            GLOBALS->fullscreen = g_old->fullscreen;
                            GLOBALS->show_toolbar = g_old->show_toolbar;
                            GLOBALS->time_box = g_old->time_box;
                            GLOBALS->highlight_wavewindow = g_old->highlight_wavewindow;
                            GLOBALS->fill_waveform = g_old->fill_waveform;
                            GLOBALS->lz_removal = g_old->lz_removal;
                            GLOBALS->disable_mouseover = g_old->disable_mouseover;
                            GLOBALS->clipboard_mouseover = g_old->clipboard_mouseover;
                            GLOBALS->keep_xz_colors = g_old->keep_xz_colors;
                            GLOBALS->zoom_pow10_snap = g_old->zoom_pow10_snap;

                            GLOBALS->scale_to_time_dimension = g_old->scale_to_time_dimension;
                            GLOBALS->zoom_dyn = g_old->zoom_dyn;
                            GLOBALS->zoom_dyne = g_old->zoom_dyne;

                            GLOBALS->ignore_savefile_pane_pos = g_old->ignore_savefile_pane_pos;
                            GLOBALS->ignore_savefile_pos = g_old->ignore_savefile_pos;
                            GLOBALS->ignore_savefile_size = g_old->ignore_savefile_size;

                            GLOBALS->hier_ignore_escapes = g_old->hier_ignore_escapes;
                            GLOBALS->sst_dbl_action_type = g_old->sst_dbl_action_type;
                            GLOBALS->use_gestures = g_old->use_gestures;
                            GLOBALS->use_dark = g_old->use_dark;
                            GLOBALS->save_on_exit = g_old->save_on_exit;
                            GLOBALS->dbl_mant_dig_override = g_old->dbl_mant_dig_override;

                            gtk_notebook_set_current_page(GTK_NOTEBOOK(GLOBALS->notebook),
                                                          GLOBALS->this_context_page);
                        }

                        return (FALSE);
                    }
                }
            }
        }
    }

    return (FALSE);
}

void install_focus_cb(GtkWidget *w, intptr_t ptr_offset)
{
    g_signal_connect(w, "enter_notify_event", G_CALLBACK(context_swapper), (void *)ptr_offset);
    g_signal_connect(w, "focus_in_event", G_CALLBACK(context_swapper), (void *)ptr_offset);
}

/*
 * wrapped g_signal_connect/g_signal_connect_swapped functions for context watchdog monitoring...
 * note that this false triggers on configure_event as gtk sends events to multiple tabs and not
 * just the visible one!
 */
static gint ctx_swap_watchdog(struct Global **w)
{
    struct Global *watch = *w;

    if (GLOBALS->gtk_context_bridge_ptr != w) {
#ifdef GTKWAVE_SIGNAL_CONNECT_DEBUG
        fprintf(stderr,
                "GTKWAVE | WARNING: globals change caught by ctx_swap_watchdog()! %p vs %p, "
                "session %d vs %d\n",
                (void *)GLOBALS->gtk_context_bridge_ptr,
                (void *)w,
                (*GLOBALS->gtk_context_bridge_ptr)->this_context_page,
                watch->this_context_page);
#endif

        set_GLOBALS(watch);
    }

    return (0);
}

static gint ctx_prints(char *s)
{
    printf(">>> %s\n", s);
    return (0);
}
static gint ctx_printd(int d)
{
    printf(">>>\t%d\n", d);
    return (0);
}

gulong gtkwave_signal_connect_x(gpointer object,
                                const gchar *name,
                                GCallback func,
                                gpointer data,
                                char *f,
                                intptr_t line)
{
    gulong rc;

    if (f) {
        g_signal_connect_swapped(object, name, (GCallback)ctx_prints, (gpointer)f);
        g_signal_connect_swapped(object, name, (GCallback)ctx_printd, (gpointer)line);
    }

    g_signal_connect_swapped(object,
                             name,
                             (GCallback)ctx_swap_watchdog,
                             (gpointer)GLOBALS->gtk_context_bridge_ptr);
    rc = g_signal_connect(object, name, func, data);

    return (rc);
}

gulong gtkwave_signal_connect_object_x(gpointer object,
                                       const gchar *name,
                                       GCallback func,
                                       gpointer data,
                                       char *f,
                                       intptr_t line)
{
    gulong rc;

    if (f) {
        g_signal_connect_swapped(object, name, (GCallback)ctx_prints, (gpointer)f);
        g_signal_connect_swapped(object, name, (GCallback)ctx_printd, (gpointer)line);
    }

    g_signal_connect_swapped(object,
                             name,
                             (GCallback)ctx_swap_watchdog,
                             (gpointer)GLOBALS->gtk_context_bridge_ptr);
    rc = g_signal_connect_swapped(object, name, func, data);

    return (rc);
}

void set_GLOBALS_x(struct Global *g, const char *file, int line)
{
    char sstr[32];

    if (line) {
        printf("Globals old %p -> new %p (%s: %d)\n", (void *)GLOBALS, (void *)g, file, line);
    }

    if (GLOBALS != g) {
        /* fix problem where ungrab doesn't occur if button pressed + simultaneous context swap
         * occurs */
        if (GLOBALS && GLOBALS->in_button_press_wavewindow_c_1) {
            XXX_gdk_pointer_ungrab(GDK_CURRENT_TIME);
        }

        GLOBALS = g;

        if (GLOBALS->time_box != NULL) {
            gw_time_display_set_project(GW_TIME_DISPLAY(GLOBALS->time_box), GLOBALS->project);
        }

        sprintf(sstr, "%d", GLOBALS->this_context_page);
        gtkwavetcl_setvar(WAVE_TCLCB_CURRENT_ACTIVE_TAB, sstr, WAVE_TCLCB_CURRENT_ACTIVE_TAB_FLAGS);
    }
}
