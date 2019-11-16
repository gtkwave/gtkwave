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
#include "gnu-getopt.h"
#include "gnu_regex.h"
#include "gtk12compat.h"
#include "lx2.h"
#include "lxt.h"
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
#include "vzt.h"
#include "wavealloca.h"

#include "lxt2_read.h"
#include "vzt_read.h"
#include "fst.h"
#include "hierpack.h"

#include "fsdb_wrapper_api.h"

#ifdef __MINGW32__
#define sleep(x) Sleep(x)
#endif

#if !defined __MINGW32__ && !defined _MSC_VER
#include <unistd.h>
#include <sys/mman.h>
#else
#include <windows.h>
#include <io.h>
#endif


struct Global *GLOBALS = NULL;

/* make this const so if we try to write to it we coredump */
static const struct Global globals_base_values =
{
/*
 * ae2.c
 */
#ifdef AET2_IS_PRESENT
#ifdef AET2_ALIASDB_IS_PRESENT
NULL, /* adb_alias_stream_file */
0, /* adb */
0, /* adb_max_terms */
NULL, /* adb_terms */
NULL, /* adb_aliases */
NULL, /* adb_num_terms */
NULL, /* adb_idx_first */
NULL, /* adb_idx_last */
NULL, /* adb_alloc_pool_base */
0, /* adb_alloc_idx */
#endif
0, /* ae2_num_facs */
0, /* ae2_num_aliases */
0, /* ae2_num_sections */
NULL, /* ae2_lx2_table */
NULL, /* ae2_f */
NULL, /* ae2 */
NULL, /* ae2_fr */
LLDescriptor(0), /* ae2_start_limit_cyc */
LLDescriptor(0), /* ae2_end_limit_cyc */
NULL, /* ae2_process_mask */
#endif
LLDescriptor(0), /* ae2_start_cyc */
LLDescriptor(0), /* ae2_end_cyc */
NULL, /* ae2_time_xlate */
0, /* disable_ae2_alias */


/*
 * analyzer.c
 */
TR_RJUSTIFY, /* default_flags 5 */
0, /* default_fpshift */
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0.0, 0.0}, /* tims 6 */
{0, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL, 0}, /* traces 9 */
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
LLDescriptor(0), /* shift_timebase 10 */
LLDescriptor(0), /* shift_timebase_default_for_add 11 */
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
NULL, /* wave_gcchain */
-1, /* color_back 25 */
-1, /* color_baseline 26 */
-1, /* color_grid 27 */
-1, /* color_grid2 27 */
-1, /* color_high 28 */
-1, /* color_low 29 */
-1, /* color_mark 30 */
-1, /* color_mid 31 */
-1, /* color_time 32 */
-1, /* color_timeb 33 */
-1, /* color_trans 34 */
-1, /* color_umark 35 */
-1, /* color_value 36 */
-1, /* color_vbox 37 */
-1, /* color_vtrans 38 */
-1, /* color_x 39 */
-1, /* color_xfill 40 */
-1, /* color_0 41 */
-1, /* color_1 42 */
-1, /* color_ufill 43 */
-1, /* color_u 44 */
-1, /* color_wfill 45 */
-1, /* color_w 46 */
-1, /* color_dashfill 47 */
-1, /* color_dash 48 */
-1, /* color_white 49 */
-1, /* color_black 50 */
-1, /* color_ltgray 51 */
-1, /* color_normal 52 */
-1, /* color_mdgray 53 */
-1, /* color_dkgray 54 */
-1, /* color_dkblue 55 */
-1, /* color_brkred */
-1, /* color_ltblue */
-1, /* color_gmstrd */
-1, /* color_highfill */
-1, /* color_1fill */


/*
 * currenttime.c
 */
LLDescriptor(0), /* global_time_offset */
0, /* is_vcd 56 */
0, /* partial_vcd */
1, /* use_maxtime_display 57 */
0, /* use_frequency_delta 58 */
NULL, /* max_or_marker_label_currenttime_c_1 59 */
NULL, /* base_or_curtime_label_currenttime_c_1 60 */
0, /* cached_currenttimeval_currenttime_c_1 61 */
0, /* currenttime 62 */
0, /* max_time 63 */
-1, /* min_time 64 */
~0, /* display_grid 65 */
1, /* time_scale 66 */
'n', /* time_dimension 67 */
0,   /* scale_to_time_dimension */
0, /* maxtimewid_currenttime_c_1 69 */
0, /* curtimewid_currenttime_c_1 70 */
0, /* maxtext_currenttime_c_1 71 */
0, /* curtext_currenttime_c_1 72 */
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
0, /* window_entry_c_1 80 */
0, /* entry_entry_c_1 81 */
NULL, /* entrybox_text 82 */
0, /* cleanup_entry_c_1 83 */
0, /* entry_raise_timer */

/*
 * extload.c
 */
0, /* extload_ffr_import_count */
NULL, /* extload_ffr_ctx */
NULL, /* extload */
NULL, /* extload_idcodes */
NULL, /* extload_inv_idcodes */
#if !defined __MINGW32__ && !defined _MSC_VER
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
#if GTK_CHECK_VERSION(2,4,0)
NULL, /* pFileChoose */
NULL, /* pFileChooseFilterName */
NULL, /* pPatternSpec */
#endif
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
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN) && GTK_CHECK_VERSION(2,8,0)
NULL, /* fonts_renderer */
NULL, /* fonts_gc */
NULL, /* fonts_screen */
NULL, /* fonts_context */
NULL, /* fonts_layout */
#endif
1, /* use_pango_fonts */


/*
 * fst.c
 */
NULL, /* fst_fst_c_1 */
NULL, /* fst_scope_name */
0, /* fst_scope_name_len */
0, /* first_cycle_fst_c_3 */
0, /* last_cycle_fst_c_3 */
0, /* total_cycles_fst_c_3 */
NULL, /* fst_table_fst_c_1 */
NULL, /* mvlfacs_fst_c_3 */
NULL, /* mvlfacs_fst_alias */
NULL, /* mvlfacs_fst_rvs_alias */
0, /* fst_maxhandle */
0, /* busycnt_fst_c_2 */
NULL, /* double_curr_fst */
NULL, /* double_fini_fst */
0, /* nonimplicit_direction_encountered */
0, /* supplemental_datatypes_encountered */
0, /* supplemental_vartypes_encountered */
0, /* is_vhdl_component_format */
NULL, /* subvar_jrb */
0, /* subvar_jrb_count */
NULL, /* subvar_pnt */
0, /* fst_filetype */
0, /* subvar_jrb_count_locked */
0, /* stem_file_idx */
0, /* stem_line_number */
NULL, /* stem_path_string_table */
NULL, /* stem_struct_base */
NULL, /* istem_struct_base */
0, /* stem_path_string_table_siz */
0, /* stem_path_string_table_alloc */
0, /* stem_struct_base_siz */
0, /* stem_struct_base_siz_alloc */
0, /* istem_struct_base_siz */
0, /* istem_struct_base_siz_alloc */
0, /* stem_valid  */
0, /* istem_valid */
NULL, /* fst_synclock_str */
NULL, /* synclock_jrb */
NULL, /* xl_enum_filter */
0, /* num_xl_enum_filter */
0, /* queued_xl_enum_filter */
NULL, /* enum_nptrs_jrb */


/*
 * ghw.c
 */
0, /* nxp_ghw_c_1 93 */
0, /* sym_which_ghw_c_1 95 */
NULL, /* gwt_ghw_c_1 96 */
NULL, /* gwt_corr_ghw_c_1 97 */
1, /* xlat_1164_ghw_c_1 98 */
0, /* is_ghw 99 */
NULL, /* asbuf */
0, /* nbr_sig_ref_ghw_c_1 101 */
0, /* num_glitches_ghw_c_1 102 */
0, /*  num_glitch_regions_ghw_c_1 */
0, /* fac_name_ghw_c_1 104 */
0, /* fac_name_len_ghw_c_1 105 */
0, /* fac_name_max_ghw_c_1 106 */
0, /* last_fac_ghw_c_1 107 */
0, /* warned_ghw_c_1 108 */

/*
 * globals.c
 */
NULL, /* dead_context */
NULL, /* gtk_context_bridge_ptr */

/*
 * help.c
 */
0, /* helpbox_is_active 110 */
0, /* text_help_c_1 111 */
0, /* vscrollbar_help_c_1 112 */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
{NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, NULL}, /* iter_help_c_1 113 */
0, /* bold_tag_help_c_1 114 */
#endif
0, /* window_help_c_2 115 */


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
 * hiersearch.c
 */
1, /* hier_grouping 116 */
0, /* window_hiersearch_c_3 117 */
0, /* entry_main_hiersearch_c_1 118 */
0, /* clist_hiersearch_c_1 119 */
0, /* bundle_direction_hiersearch_c_1 120 */
0, /* cleanup_hiersearch_c_3 121 */
0, /* num_rows_hiersearch_c_1 122 */
0, /* selected_rows_hiersearch_c_1 123 */
0, /* window1_hiersearch_c_1 124 */
0, /* entry_hiersearch_c_2 125 */
NULL, /* entrybox_text_local_hiersearch_c_1 126 */
NULL, /* cleanup_e_hiersearch_c_1 127 */
NULL, /* h_selectedtree_hiersearch_c_1 128 */
NULL, /* current_tree_hiersearch_c_1 129 */
NULL, /* treechain_hiersearch_c_1 130 */
0, /* is_active_hiersearch_c_1 131 */


/*
 * logfile.c
 */
NULL, /* logfiles */
NULL, /* fontname_logfile 133 */
NULL, /* font_logfile_c_1 134 */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
{NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, NULL}, /* iter_logfile_c_2 135 */
NULL, /* bold_tag_logfile_c_2 136 */
NULL, /* mono_tag_logfile_c_1 137 */
NULL, /* size_tag_logfile_c_1 138 */
#endif


/*
 * lx2.c
 */
LXT2_IS_INACTIVE, /* is_lx2 139 */
NULL, /* lx2_lx2_c_1 140 */
0, /* first_cycle_lx2_c_1 141 */
0, /* last_cycle */
0, /* total_cycles */
NULL, /* lx2_table_lx2_c_1 142 */
NULL, /* mvlfacs_lx2_c_1 143 */
0, /* busycnt_lx2_c_1 144 */


/*
 * lxt.c
 */
NULL, /* mm_lxt_mmap_addr */
0, /* mm_lxt_mmap_len */
#if defined __MINGW32__ || defined _MSC_VER
NULL, /* HANDLE hIn */
NULL, /* HANDLE hInMap */
NULL, /* char *win_fname = NULL; */
#endif
0, /* fpos_lxt_c_1 145 */
0, /* is_lxt 146 */
0, /* lxt_clock_compress_to_z 147 */
NULL, /* mm_lxt_c_1 148 */
NULL, /* mmcache_lxt_c_1 */
0, /* version_lxt_c_1 149 */
NULL, /* mvlfacs_lxt_c_2 150 */
0, /* first_cycle_lxt_c_2 151 */
0, /* last_cycle */
0, /* total_cycles */
0, /* maxchange_lxt_c_1 152 */
0, /* maxindex */
0, /* f_len_lxt_c_1 153 */
NULL, /* positional_information_lxt_c_1 154 */
NULL, /* time_information 155 */
0, /* change_field_offset_lxt_c_1 156 */
0, /* facname_offset_lxt_c_1 157 */
0, /* facgeometry_offset_lxt_c_1 158 */
0, /* time_table_offset_lxt_c_1 159 */
0, /* time_table_offset64_lxt_c_1 160 */
0, /* sync_table_offset_lxt_c_1 161 */
0, /* initial_value_offset_lxt_c_1 162 */
0, /* timescale_offset_lxt_c_1 163 */
0, /* double_test_offset_lxt_c_1 164 */
0, /* zdictionary_offset_lxt_c_1 165 */
0, /* zfacname_predec_size_lxt_c_1 166 */
0, /* zfacname_size_lxt_c_1 167 */
0, /* zfacgeometry_size_lxt_c_1 168 */
0, /* zsync_table_size_lxt_c_1 169 */
0, /* ztime_table_size_lxt_c_1 170 */
0, /* zchg_predec_size_lxt_c_1 171 */
0, /* zchg_size_lxt_c_1 172 */
0, /* zdictionary_predec_size_lxt_c_1 173 */
AN_X, /* initial_value_lxt_c_1 174 */
0, /* dict_num_entries_lxt_c_1 175 */
0, /* dict_string_mem_required_lxt_c_1 176 */
0, /* dict_16_offset_lxt_c_1 177 */
0, /* dict_24_offset_lxt_c_1 178 */
0, /* dict_32_offset_lxt_c_1 179 */
0, /* dict_width_lxt_c_1 180 */
NULL, /* dict_string_mem_array_lxt_c_1 181 */
0, /* exclude_offset_lxt_c_1 182 */
0, /* lxt_timezero_offset */
NULL, /* lt_buf_lxt_c_1 183 */
0, /* lt_len_lxt_c_1 184 */
-1, /* fd_lxt_c_1 185 */
{0,0,0,0,0,0,0,0}, /* double_mask_lxt_c_1 186 */
0, /* double_is_native_lxt_c_1 187 */
0, /* max_compare_time_tc_lxt_c_2 189 */
0, /* max_compare_pos_tc_lxt_c_2 */
NULL, /* resolve_lxt_alias_to */
NULL, /* lastchange */


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
0, /* use_scrollbar_only 220 */
0, /* force_toolbars 221 */
0, /* hide_sst 222 */
1, /* sst_expanded 223 */
#ifdef WAVE_USE_GTK2
0, /* socket_xid 224 */
#endif
0, /* disable_menus 225 */
NULL, /* ftext_main_main_c_1 226 */
#ifdef WAVE_USE_GTK2
1, /* use_toolbutton_interface */
#else
0, /* use_toolbutton_interface */
#endif

/*
 * markerbox.c
 */
NULL, /* window_markerbox_c_4 231 */
{0}, /* entries_markerbox_c_1 232 */
NULL, /* cleanup_markerbox_c_4 233 */
0, /* dirty_markerbox_c_1 234 */
{0}, /* shadow_markers_markerbox_c_1 235 */
{NULL}, /* marker_names */
{NULL}, /* shadow_marker_names */


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
#ifndef WAVE_USE_MLIST_T
NULL, /* item_factory_menu_c_1 241 */
#endif
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
NULL, /* filesel_lxt_writesave 250 */
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
NULL, /* mo_area_mouseover_c_1 256 */
NULL, /* mo_pixmap_mouseover_c_1 257 */
NULL, /* mo_dk_gray_mouseover_c_1 258 */
NULL, /* mo_black_mouseover_c_1 259 */
0, /* mo_width_mouseover_c_1 260 */
0, /* mo_height_mouseover_c_1 260 */


/*
 * pagebuttons.c
 */
1.0, /* page_divisor 261 */


/*
 * pixmaps.c
 */
NULL, /* redo_pixmap */
NULL, /* redo_mask */
NULL, /* larrow_pixmap 263 */
NULL, /* larrow_mask 264 */
NULL, /* rarrow_pixmap 266 */
NULL, /* rarrow_mask 267 */
NULL, /* zoomin_pixmap 269 */
NULL, /* zoomin_mask 270 */
NULL, /* zoomout_pixmap 272 */
NULL, /* zoomout_mask 273 */
NULL, /* zoomfit_pixmap 275 */
NULL, /* zoomfit_mask 276 */
NULL, /* zoomundo_pixmap 278 */
NULL, /* zoomundo_mask 279 */
NULL, /* zoom_larrow_pixmap 281 */
NULL, /* zoom_larrow_mask 282 */
NULL, /* zoom_rarrow_pixmap 284 */
NULL, /* zoom_rarrow_mask 285 */
NULL, /* prev_page_pixmap 287 */
NULL, /* prev_page_mask 288 */
NULL, /* next_page_pixmap 290 */
NULL, /* next_page_mask 291 */
NULL, /* wave_info_pixmap 293 */
NULL, /* wave_info_mask 294 */
NULL, /* wave_alert_pixmap 296 */
NULL, /* wave_alert_mask 297 */
NULL, /* hiericon_module_pixmap */
NULL, /* hiericon_module_mask */
NULL, /* hiericon_task_pixmap */
NULL, /* hiericon_task_mask */
NULL, /* hiericon_function_pixmap */
NULL, /* hiericon_function_mask */
NULL, /* hiericon_begin_pixmap */
NULL, /* hiericon_begin_mask */
NULL, /* hiericon_fork_pixmap */
NULL, /* hiericon_fork_mask */
NULL, /* hiericon_interface_pixmap */
NULL, /* hiericon_interface_mask */
NULL, /* hiericon_svpackage_pixmap */
NULL, /* hiericon_svpackage_mask */
NULL, /* hiericon_program_pixmap */
NULL, /* hiericon_program_mask */
NULL, /* hiericon_class_pixmap */
NULL, /* hiericon_class_mask */
NULL, /* hiericon_record_pixmap */
NULL, /* hiericon_record_mask */
NULL, /* hiericon_generate_pixmap */
NULL, /* hiericon_generate_mask */
NULL, /* hiericon_design_pixmap */
NULL, /* hiericon_design_mask */
NULL, /* hiericon_block_pixmap */
NULL, /* hiericon_block_mask */
NULL, /* hiericon_generateif_pixmap */
NULL, /* hiericon_generateif_mask */
NULL, /* hiericon_generatefor_pixmap */
NULL, /* hiericon_generatefor_mask */
NULL, /* hiericon_instance_pixmap */
NULL, /* hiericon_instance_mask */
NULL, /* hiericon_package_pixmap */
NULL, /* hiericon_package_mask */
NULL, /* hiericon_signal_pixmap */
NULL, /* hiericon_signal_mask */
NULL, /* hiericon_portin_pixmap */
NULL, /* hiericon_portin_mask */
NULL, /* hiericon_portout_pixmap */
NULL, /* hiericon_portout_mask */
NULL, /* hiericon_portinout_pixmap */
NULL, /* hiericon_portinout_mask */
NULL, /* hiericon_buffer_pixmap */
NULL, /* hiericon_buffer_mask */
NULL, /* hiericon_linkage_pixmap */
NULL, /* hiericon_linkage_mask */


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
NULL, /* clist_ptranslate_c_2 316 */


/*
 * rc.c
 */
0, /* rc_line_no 318 */
1, /* possibly_use_rc_defaults 319 */
NULL, /* editor_string */


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
{0,0,0,0}, /* target_mutex_renderopt_c_1 328 */
{0,0,0,0,0}, /* page_mutex_renderopt_c_1 330 */
{0,0,0}, /* render_mutex_renderopt_c_1 332 */
0, /* page_size_type_renderopt_c_1 333 */


/*
 * savefile.c
 */
NULL, /* sfn */
NULL, /* lcname */


/*
 * search.c
 */
{NULL,NULL,NULL,NULL,NULL}, /* menuitem_search */
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
{0,0,0,0,0}, /* regex_mutex_search_c_1 352 */
0, /* regex_which_search_c_1 353 */
NULL, /* window_search_c_7 354 */
NULL, /* entry_search_c_3 355 */
NULL, /* clist_search_c_3 356 */
NULL, /* searchbox_text_search_c_1 358 */
0, /* bundle_direction_search_c_2 359 */
NULL, /* cleanup_search_c_5 360 */
0, /* num_rows_search_c_2 361 */
0, /* selected_rows_search_c_2 362 */


/*
 * showchange.c
 */
NULL, /* button1_showchange_c_1 363 */
NULL, /* button1_showchange_c_2 363 */
NULL, /* button1_showchange_c_3 363 */
NULL, /* button1_showchange_c_4 363 */
NULL, /* button1_showchange_c_5 363 */
NULL, /* button1_showchange_c_6 363 */
NULL, /* button1_showchange_c_7 363 */
NULL, /* button1_showchange_c_8 363 */
NULL, /* button1_showchange_c_9 363 */
NULL, /* toggle1_showchange_c_1 364 */
NULL, /* toggle2_showchange_c_1 364 */
NULL, /* toggle3_showchange_c_1 364 */
NULL, /* toggle4_showchange_c_1 364 */
NULL, /* toggle5_showchange_c_1 364 */
NULL, /* window_showchange_c_8 365 */
NULL, /* cleanup_showchange_c_6 366 */
NULL, /* tcache_showchange_c_1 367 */
0, /* flags_showchange_c_1 368 */


/*
 * signalwindow.c
 */
NULL, /* signalarea 369 */
NULL, /* signalfont 370 */
NULL, /* signalpixmap 371 */
#ifdef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND
0, /* force_hide_show */
#endif
0, /* max_signal_name_pixel_width 372 */
0, /* signal_pixmap_width 373 */
0, /* signal_fill_width 374 */
0, /* old_signal_fill_width 375 */
0, /* old_signal_fill_height */
1, /* right_align_active */
1, /* fontheight 376 */
0, /* dnd_state 377 */
0, /* dnd_cursor_timer */
NULL, /* hscroll_signalwindow_c_1 378 */
NULL, /* signal_hslider 379 */
0, /* cachedhiflag_signalwindow_c_1 380 */
-1, /* cachedwhich_signalwindow_c_1 381 */
NULL, /* cachedtrace 382 */
NULL, /* shift_click_trace 383 */
0, /* trtarget_signalwindow_c_1 384 */
NULL, /* starting_unshifted_trace */
0, /* standard_trace_dnd_degate */
0, /* use_standard_trace_select */
1, /* use_standard_clicking */
0, /* std_collapse_pressed */
0, /* std_dnd_tgt_on_signalarea */
0, /* std_dnd_tgt_on_wavearea */
0, /* signalarea_has_focus */
NULL, /* signalarea_event_box */
0, /* keypress_handler_id */
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
NULL, /* wave_splash_pixmap 389 */
NULL, /* wave_splash_mask 390 */
NULL, /* splash_splash_c_1 391 */
NULL, /* darea_splash_c_1 392 */
NULL, /* gt_splash_c_1 393 */
0, /* timeout_tag 394 */
0, /* load_complete_splash_c_1 395 */
2, /* cnt_splash_c_1 396 */
0, /* prev_bar_x_splash_c_1 397 */


/*
 * status.c
 */
NULL, /* text_status_c_2 398 */
NULL, /* vscrollbar_status_c_2 399 */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
{NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL, 0, 0, 0, NULL}, /* iter_status_c_3 400 */
NULL, /* bold_tag_status_c_3 401 */
#endif


/*
 * strace.c
 */
NULL, /* strace_ctx (defined in strace.h for multiple strace sessions) */

{
{NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,{0,0,0,0,0,0},{0,0,0,0,0,0},0,0,0,0,0,0,0},  /* strace_windows[0] */
#ifdef WAVE_USE_GTK2
 {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,0,{0,0,0,0,0,0},{0,0,0,0,0,0},0,0,0,0,0,0,0}
#endif
}, /* strace_windows[1] */
#if WAVE_NUM_STRACE_WINDOWS != 2
#ifdef WAVE_USE_GTK2
#error the number of strace windows as defined in strace.h does not match globals.c!
#else
#if WAVE_NUM_STRACE_WINDOWS != 1
#error gtk1 only supports 1 strace window, sorry!
#endif
#endif
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
NULL, /* firstnode 430 */
NULL, /* curnode 431 */
0, /* hashcache 432 */


/*
 * tcl_commands.c
 */
NULL, /* previous_braced_tcl_string */


/*
 * tcl_helper.c
 */
0, /* in_tcl_callback */


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
NULL, /* clist_translate_c_4 442 */


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
1, /* fast_tree_sort 450 */
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
 * treesearch_gtk1.c
 */
NULL, /* GtkWidget *window1_treesearch_gtk1_c;  */
NULL, /* GtkWidget *entry_a_treesearch_gtk1_c;  */
NULL, /* char *entrybox_text_local_treesearch_gtk1_c;  */
NULL, /* void (*cleanup_e_treesearch_gtk1_c)(); */
NULL, /* struct tree *selectedtree_treesearch_gtk1_c;  */
0, /* int is_active_treesearch_gtk1_c;  */
NULL, /* GtkWidget *window_treesearch_gtk1_c;  */
NULL, /* GtkWidget *tree_treesearch_gtk1_c; */
0, /* char bundle_direction_treesearch_gtk1_c;  */
NULL, /* void (*cleanup_treesearch_gtk1_c)(); */


/*
 * treesearch_gtk2.c
 */
#ifdef MAC_INTEGRATION
NULL, /* dnd_helper_quartz */
#endif
NULL, /* treeopen_chain_head */
NULL, /* treeopen_chain_curr */
0, /* tree_dnd_begin */
0, /* tree_dnd_requested */
1, /* do_dynamic_treefilter */
NULL, /* treesearch_gtk2_window_vbox */
NULL, /* selected_hierarchy_name */
NULL, /* selected_sig_name */
NULL, /* gtk2_tree_frame */
NULL, /* filter_entry */
NULL, /* any_tree_node */
NULL, /* open_tree_nodes */
0, /* autoname_bundles 453 */
NULL, /* window1_treesearch_gtk2_c_3 454 */
NULL, /* entry_a_treesearch_gtk2_c_2 455 */
NULL, /* entrybox_text_local_treesearch_gtk2_c_3 456 */
NULL, /* cleanup_e_treesearch_gtk2_c_3 457 */
NULL, /* sig_root_treesearch_gtk2_c_1 458 */
NULL, /* sst_sig_root_treesearch_gtk2_c_1 */
NULL, /* filter_str_treesearch_gtk2_c_1 459 */
ND_DIR_UNSPECIFIED, /* filter_typ_treesearch_gtk2_c_1 */
0, /* filter_typ_polarity_treesearch_gtk2_c_1 */
0, /* filter_matlen_treesearch_gtk2_c_1 */
0, /* filter_noregex_treesearch_gtk2_c_1 */
#if defined(WAVE_USE_GTK2)
NULL, /* sig_store_treesearch_gtk2_c_1 460 */
NULL, /* sig_selection_treesearch_gtk2_c_1 461 */
#endif
0, /* is_active_treesearch_gtk2_c_6 462 */
NULL, /* ctree_main 463 */
NULL, /* afl_treesearch_gtk2_c_1 464 */
NULL, /* window_treesearch_gtk2_c_12 465 */
NULL, /* tree_treesearch_gtk2_c_1 466 */
NULL, /* cleanup_treesearch_gtk2_c_8 468 */
0, /* pre_import_treesearch_gtk2_c_1 469 */
{0,0,NULL,NULL,NULL,NULL,0,NULL,NULL,0}, /* tcache_treesearch_gtk2_c_2 470 */
0, /* dnd_tgt_on_signalarea_treesearch_gtk2_c_1 471 */
0, /* dnd_tgt_on_wavearea_treesearch_gtk2_c_1 */
NULL, /* dnd_sigview */
NULL, /* sst_vpaned */
0, /* fetchlow */
0, /* fetchhigh */
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
NULL, /* clist_ttranslate_c_2 */
NULL, /* ttranslate_args */


/*
 * vcd.c
 */
0, /* do_hier_compress */
NULL, /* prev_hier_uncompressed_name */
NULL, /* vcd_jmp_buf */
-1, /* vcd_warning_filesize 472 */
1, /* autocoalesce 473 */
0, /* autocoalesce_reversal */
-1, /* vcd_explicit_zero_subscripts 474 */
0, /* convert_to_reals 475 */
1, /* atomic_vectors 476 */
0, /* make_vcd_save_file 477 */
0, /* vcd_preserve_glitches 478 */
0, /* vcd_preserve_glitches_real */
NULL, /* vcd_save_handle 479 */
NULL, /* vcd_handle_vcd_c_1 480 */
0, /* vcd_is_compressed_vcd_c_1 481 */
0, /* vcdbyteno_vcd_c_1 482 */
0, /* error_count_vcd_c_1 483 */
0, /* header_over_vcd_c_1 484 */
0, /* dumping_off_vcd_c_1 485 */
-1, /* start_time_vcd_c_1 486 */
-1, /* end_time_vcd_c_1 487 */
-1, /* current_time_vcd_c_1 488 */
0, /* num_glitches_vcd_c_2 489 */
0, /* num_glitch_regions_vcd_c_2 490 */
{0, 0}, /* vcd_hier_delimeter 491 */
NULL, /* pv_vcd_c_1 492 */
NULL, /* rootv_vcd_c_1 */
NULL, /* vcdbuf_vcd_c_1 493 */
NULL, /* vst */
NULL, /* vend */
0, /* escaped_names_found_vcd_c_1 494 */
NULL, /* slistroot 495 */
NULL, /* slistcurr */
NULL, /* slisthier 496 */
0, /* slisthier_len 497x */
1024, /* T_MAX_STR_vcd_c_1 499 */
NULL, /* yytext_vcd_c_1 500 */
0, /* yylen_vcd_c_1 501 */
0, /* yylen_cache */
NULL, /* vcdsymroot_vcd_c_1 502 */
NULL, /* vcdsymcurr */
NULL, /* sorted_vcd_c_1 503 */
NULL, /* indexed_vcd_c_1 504 */
0, /* numsyms_vcd_c_1 505 */
NULL, /* he_curr_vcd_c_1 506 */
NULL, /* he_fini */
~0, /* vcd_minid_vcd_c_1 508 */
0, /* vcd_maxid_vcd_c_1 509 */
0, /* err_vcd_c_1 510 */
0, /* vcd_fsiz_vcd_c_1 511 */
NULL, /* varsplit_vcd_c_1 512 */
NULL, /* varsplitcurr */
0, /* var_prevch_vcd_c_1 513 */
0, /* vcd_already_backtracked */


/*
 * vcd_partial.c
 */
0, /* vcdbyteno_vcd_partial_c_2 516 */
0, /* error_count_vcd_partial_c_2 517 */
0, /* header_over_vcd_partial_c_2 518 */
0, /* dumping_off_vcd_partial_c_2 519 */
-1, /* start_time_vcd_partial_c_2 520 */
-1, /* end_time_vcd_partial_c_2 521 */
-1, /* current_time_vcd_partial_c_2 522 */
0, /* num_glitches_vcd_partial_c_3 523 */
0, /* num_glitch_regions_vcd_partial_c_3 524 */
NULL, /* pv_vcd_partial_c_2 525 */
NULL, /* rootv */
NULL, /* vcdbuf_vcd_partial_c_2 526 */
NULL, /* vst */
NULL, /* vend */
NULL, /* consume_ptr_vcd_partial_c_1 527 */
NULL, /* buf_vcd_partial_c_2 528 */
100000, /* consume_countdown_vcd_partial_c_1 529 */
1024, /* T_MAX_STR_vcd_partial_c_2 531 */
NULL, /* yytext_vcd_partial_c_2 532 */
0, /* yylen_vcd_partial_c_2 533 */
0, /* yylen_cache */
NULL, /* vcdsymroot_vcd_partial_c_2 534 */
NULL, /* vcdsymcurr */
NULL, /* sorted_vcd_partial_c_2 535 */
NULL, /* indexed_vcd_partial_c_2 536 */
0, /* numsyms_vcd_partial_c_2 538 */
~0, /* vcd_minid_vcd_partial_c_2 540 */
0, /* vcd_maxid_vcd_partial_c_2 541 */
0, /* err_vcd_partial_c_2 542 */
NULL, /* varsplit_vcd_partial_c_2 543 */
NULL, /* vsplitcurr */
0, /* var_prevch_vcd_partial_c_2 544 */
0, /* timeset_vcd_partial_c_1 547 */


/*
 * vcd_recoder.c
 */
NULL, /* time_vlist_vcd_recoder_c_1 548 */
NULL, /* time_vlist_vcd_recoder_write */
NULL, /* fastload_depacked */
NULL, /* fastload_current */
0, /* time_vlist_count_vcd_recoder_c_1 549 */
NULL, /* vcd_handle_vcd_recoder_c_2 550 */
0, /* vcd_is_compressed_vcd_recoder_c_2 551 */
VCD_FSL_NONE, /* use_fastload */
0, /* vcdbyteno_vcd_recoder_c_3 552 */
0, /* error_count_vcd_recoder_c_3 553 */
0, /* header_over_vcd_recoder_c_3 554 */
0, /* dumping_off_vcd_recoder_c_3 555 */
-1, /* start_time_vcd_recoder_c_3 556 */
-1, /* end_time_vcd_recoder_c_3 557 */
-1, /* current_time_vcd_recoder_c_3 558 */
0, /* num_glitches_vcd_recoder_c_4 559 */
0, /* num_glitch_regions_vcd_recoder_c_4 560 */
NULL, /* pv_vcd_recoder_c_3 561 */
NULL, /* rootv */
NULL, /* vcdbuf_vcd_recoder_c_3 562 */
NULL, /* vst */
NULL, /* vend */
1024, /* T_MAX_STR_vcd_recoder_c_3 564 */
NULL, /* yytext_vcd_recoder_c_3 565 */
0, /* yylen_vcd_recoder_c_3 566 */
0, /* yylen_cache */
NULL, /* vcdsymroot_vcd_recoder_c_3 567 */
NULL, /* vcdsymcurr */
NULL, /* sorted_vcd_recoder_c_3 568 */
NULL, /* indexed_vcd_recoder_c_3 569 */
0, /* numsyms_vcd_recoder_c_3 570 */
~0, /* vcd_minid_vcd_recoder_c_3 571 */
0, /* vcd_maxid_vcd_recoder_c_3 572 */
0, /* err_vcd_recoder_c_3 573 */
0, /* vcd_fsiz_vcd_recoder_c_2 574 */
NULL, /* varsplit_vcd_recoder_c_3 575 */
NULL, /* vsplitcurr */
0, /* var_prevch_vcd_recoder_c_3 576 */
0, /* vcd_hash_max */
0, /* vcd_hash_kill */


/*
 * vcd_saver.c
 */
NULL, /* f_vcd_saver_c_1 579 */
{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0}, /* buf_vcd_saver_c_3 580 */
NULL, /* hp_vcd_saver_c_1 581 */
NULL, /* nhold_vcd_saver_c_1 582 */


/*
 * vlist.c
 */
0, /* vlist_spill_to_disk */
0, /* vlist_prepack */
NULL, /* vlist_handle */
0, /* vlist_bytes_written */
4, /* vlist_compression_depth 583 */


/*
 * vzt.c
 */
NULL, /* vzt_vzt_c_1 584 */
0, /* first_cycle_vzt_c_3 585 */
0, /* last_cycle */
0, /* total_cycles */
NULL, /* vzt_table_vzt_c_1 586 */
NULL, /* mvlfacs_vzt_c_3 587 */
0, /* busycnt_vzt_c_2 588 */


/*
 * wavewindow.c
 */
0, /* highlight_wavewindow */
1, /* alt_wheel_mode */
0, /* use_scrollwheel_as_y */
0, /* enable_slider_zoom */
0, /* m1x_wavewindow_c_1 589 */
0, /* m2x_wavewindow_c_1 */
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
NULL, /* topmost_trace 613 */
1, /* waveheight 614 */
0, /* wavecrosspiece */
1, /* wavewidth 615 */
NULL, /* wavefont 616 */
NULL, /* wavefont_smaller 617 */
NULL, /* wavearea 618 */
NULL, /* vscroll_wavewindow_c_1 619 */
NULL, /* hscroll_wavewindow_c_2 620 */
NULL, /* wavepixmap_wavewindow_c_1 621 */
NULL, /* wave_vslider 622 */
NULL, /* wave_hslider */
{0}, /* named_markers 623 */
-1, /* named_marker_lock_idx */
0, /* made_gc_contexts_wavewindow_c_1 624 */
0, /* which_t_color */
NULL, /* gc_white 650 */
NULL, /* gc_black 651 */
{NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}, /* gc */
WAVE_RAINBOW_INITIALIZER, /* gc_rainbow */
0, /* made_sgc_contexts_wavewindow_c_1 649 */
0, /* fill_in_smaller_rgb_areas_wavewindow_c_1 659 */
-1, /* prev_markertime */
{NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL}, /* gccache */
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
struct Global *g = calloc(1,sizeof(struct Global));	/* allocate viewer context */

memcpy(g, &globals_base_values, sizeof(struct Global));	/* fill in the blanks */

g->gtk_context_bridge_ptr = calloc(1, sizeof(struct Global *));
*(g->gtk_context_bridge_ptr) = g;

g->buf_menu_c_1 = calloc_2_into_context(g, 1, 65537);	/* do remaining mallocs into new ctx */
g->regexp_string_menu_c_1 = calloc_2_into_context(g, 1, 129);
g->regex_ok_regex_c_1 = calloc_2_into_context(g, WAVE_REGEX_TOTAL, sizeof(int));
g->preg_regex_c_1 = calloc_2_into_context(g, WAVE_REGEX_TOTAL, sizeof(regex_t));

g->strace_ctx = &g->strace_windows[0];			/* arbitrarily point to first one */

return(g);						/* what to do with ctx is at discretion of caller */
}

void strcpy2_into_new_context(struct Global *g, char **newstrref, char **oldstrref)
{
char *o = *oldstrref;
char *n;

if(o)	/* only allocate + copy string if nonnull pointer */
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
if(*wp)
	{
	wave_gtk_grab_remove(*wp);
	gtk_widget_destroy(*wp);
	*wp = NULL;
	}
}

static void widget_only_destroy(GtkWidget **wp)
{
if(*wp)
	{
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
setjmp_globals =  calloc(1,sizeof(struct Global));      /* allocate yet another copy of viewer context */
memcpy(setjmp_globals, GLOBALS, sizeof(struct Global));	/* clone */
GLOBALS->alloc2_chain = NULL;				/* will merge this in after load if successful */
GLOBALS->outstanding = 0;				/* zero out count of chunks in this ctx */

if(!setjmp(*(GLOBALS->vcd_jmp_buf)))			/* loader exception handling */
	{
	switch(GLOBALS->loaded_file_type)		/* on fail, longjmp called in these loaders */
		{
   		case LXT_FILE: lxt_main(GLOBALS->loaded_file_name); break;
   		case VCD_FILE: vcd_main(GLOBALS->loaded_file_name); break;
		case VCD_RECODER_FILE: vcd_recoder_main(GLOBALS->loaded_file_name); break;
		default: break;
		}

#ifdef _WAVE_HAVE_JUDY
	{
	Pvoid_t  PJArray = (Pvoid_t)setjmp_globals->alloc2_chain;
	int rcValue;
	Word_t Index;

	Index = 0;
	for (rcValue = Judy1First(PJArray, &Index, PJE0); rcValue != 0; rcValue = Judy1Next(PJArray, &Index, PJE0))
		{
	        Judy1Set ((Pvoid_t)&GLOBALS->alloc2_chain, Index, PJE0);
		}

	GLOBALS->outstanding += setjmp_globals->outstanding;
	Judy1FreeArray(&PJArray, PJE0);
	}
#else
	{
	void **t, **t2;

	t = (void **)setjmp_globals->alloc2_chain;
	while(t)
		{
		t2 = (void **) *(t+1);
		if(t2)
			{
			t = t2;
			}
			else
			{
			*(t+1) = GLOBALS->alloc2_chain;
			if(GLOBALS->alloc2_chain)
				{
				t2 = (void **)GLOBALS->alloc2_chain;
				*(t2+0) = t;
				}
			GLOBALS->alloc2_chain = setjmp_globals->alloc2_chain;
			GLOBALS->outstanding += setjmp_globals->outstanding;
			break;
			}
		}
	}
#endif
	free(GLOBALS->vcd_jmp_buf); GLOBALS->vcd_jmp_buf = NULL;
	free(setjmp_globals); setjmp_globals = NULL;

	load_was_success = 1;
	}
	else
	{
	free(GLOBALS->vcd_jmp_buf); GLOBALS->vcd_jmp_buf = NULL;
	if(GLOBALS->vcd_handle_vcd_c_1)
		{
		if(GLOBALS->vcd_is_compressed_vcd_c_1)
		        {
		        pclose(GLOBALS->vcd_handle_vcd_c_1);
		        GLOBALS->vcd_handle_vcd_c_1 = NULL;
		        }
		        else
		        {
		        fclose(GLOBALS->vcd_handle_vcd_c_1);
		        GLOBALS->vcd_handle_vcd_c_1 = NULL;
		        }
		}
	if(GLOBALS->vcd_handle_vcd_recoder_c_2)
		{
		if(GLOBALS->vcd_is_compressed_vcd_recoder_c_2)
	        				{
	        				pclose(GLOBALS->vcd_handle_vcd_recoder_c_2);
	        				GLOBALS->vcd_handle_vcd_recoder_c_2 = NULL;
	        				}
	        				else
	        				{
	        				fclose(GLOBALS->vcd_handle_vcd_recoder_c_2);
	        				GLOBALS->vcd_handle_vcd_recoder_c_2 = NULL;
	        				}
		}


	if(GLOBALS->vlist_handle) { fclose(GLOBALS->vlist_handle); GLOBALS->vlist_handle = NULL; }
	if(GLOBALS->mm_lxt_mmap_addr)
		{
		munmap(GLOBALS->mm_lxt_mmap_addr, GLOBALS->mm_lxt_mmap_len);
		GLOBALS->mm_lxt_mmap_addr = NULL;
		}
	free_outstanding(); /* free anything allocated in loader ctx */

	memcpy(GLOBALS, setjmp_globals, sizeof(struct Global)); /* copy over old ctx */
	free(setjmp_globals);					/* remove cached old ctx */
	/* now try again, jump through recovery sequence below */
	}

return(load_was_success);
}


/*
 * reload from old into the new context
 */
void reload_into_new_context_2(void)
{
 FILE *statefile;
 struct Global *new_globals;
 /* gint tree_frame_x = -1; */ /* scan-build */
#if WAVE_USE_GTK2
 gint tree_frame_y = -1;
 gdouble tree_vadj_value = 0.0;
 gdouble tree_hadj_value = 0.0;
 gdouble treeview_vadj_value = 0.0;
 gdouble treeview_hadj_value = 0.0;
#endif
 int fix_from_time = 0, fix_to_time = 0;
 TimeType from_time = LLDescriptor(0), to_time = LLDescriptor(0);
 char timestr[32];
 struct stringchain_t *hier_head = NULL, *hier_curr = NULL;
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

 if((GLOBALS->loaded_file_type == MISSING_FILE)||(GLOBALS->is_optimized_stdin_vcd))
	{
	fprintf(stderr, "GTKWAVE | Nothing to reload!\n");
	return;
	}

 logbox_reload();

 /* kill any pending splash screens (e.g., from Tcl "wish") */
 splash_button_press_event(NULL, NULL);

 /* fix problem where ungrab doesn't occur if button pressed + simultaneous reload accelerator key occurs */
 if(GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

 /* let all GTK/X events spin through in order to keep menus from freezing open during reload */
#ifndef MAC_INTEGRATION
 if(GLOBALS->text_status_c_2)
	{
	wave_gtk_grab_add(GLOBALS->text_status_c_2);		/* grab focus to a known widget with no real side effects */
	gtkwave_main_iteration();				/* spin on GTK event loop */
	wave_gtk_grab_remove(GLOBALS->text_status_c_2);		/* ungrab focus */
	}
#endif

 printf("GTKWAVE | Reloading waveform...\n");
 gtkwavetcl_setvar(WAVE_TCLCB_RELOAD_BEGIN, GLOBALS->loaded_file_name, WAVE_TCLCB_RELOAD_BEGIN_FLAGS);

 /* Save state to file */
 save_tmpfilename = tmpnam_2(NULL, &fd_dummy);
 statefile = save_tmpfilename ? fopen(save_tmpfilename,"wb") : NULL;
 if(statefile == NULL) {
   fprintf(stderr, "Failed to create temp file required for file reload.\n");
   if(save_tmpfilename) { perror("Why"); free_2(save_tmpfilename); }
   gtkwavetcl_setvar_nonblocking(WAVE_TCLCB_ERROR,"reload failed",WAVE_TCLCB_ERROR_FLAGS);
   return;
 }
 if(fd_dummy >=0) close(fd_dummy);
 write_save_helper(save_tmpfilename, statefile);
 fclose(statefile);

 reload_tmpfilename = strdup(save_tmpfilename);
 free_2(save_tmpfilename);

 /* save off size of tree frame if active */
#if WAVE_USE_GTK2
 if(GLOBALS->gtk2_tree_frame)
	{
	/* upper tree */
	GtkCList *cl = GTK_CLIST(GLOBALS->ctree_main);
	GtkAdjustment *vadj = gtk_clist_get_vadjustment(cl);
	GtkAdjustment *hadj = gtk_clist_get_hadjustment(cl);

	if(vadj) tree_vadj_value = vadj->value;
	if(hadj) tree_hadj_value = hadj->value;

	/* tree_frame_x = (GLOBALS->gtk2_tree_frame)->allocation.width; */ /* scan-build */
	tree_frame_y = (GLOBALS->gtk2_tree_frame)->allocation.height;

	/* lower signal set */
	vadj = gtk_tree_view_get_vadjustment((GtkTreeView *)GLOBALS->dnd_sigview);
	hadj = gtk_tree_view_get_hadjustment((GtkTreeView *)GLOBALS->dnd_sigview);

	if(vadj) treeview_vadj_value = vadj->value;
	if(hadj) treeview_hadj_value = hadj->value;
	}
#endif

 /* Kill any open processes */
 remove_all_proc_filters();
 remove_all_ttrans_filters();

 /* Instantiate new global status */
 new_globals = initialize_globals();
 free(new_globals->gtk_context_bridge_ptr); /* don't need this one as we're copying over the old one... */
 new_globals->gtk_context_bridge_ptr = GLOBALS->gtk_context_bridge_ptr;

 /* SMP */
 new_globals->num_cpus = GLOBALS->num_cpus;

 /* tcl interpreter */
#if defined(HAVE_LIBTCL)
 new_globals->interp = GLOBALS->interp;
#endif

 /* lxt.c */
 if(GLOBALS->fd_lxt_c_1 >= 0)
	{
	close(GLOBALS->fd_lxt_c_1);
	GLOBALS->fd_lxt_c_1 = -1;
	}


 /* Marker positions */
 memcpy(new_globals->named_markers, GLOBALS->named_markers, sizeof(GLOBALS->named_markers));
 new_globals->named_marker_lock_idx = GLOBALS->named_marker_lock_idx;

 /* notebook page flipping */
 new_globals->num_notebook_pages = GLOBALS->num_notebook_pages;
 new_globals->num_notebook_pages_cumulative = GLOBALS->num_notebook_pages_cumulative;
 new_globals->this_context_page = GLOBALS->this_context_page;
 new_globals->contexts = GLOBALS->contexts; /* this value is a *** chameleon!  malloc'd region is outside debug.c control! */
 new_globals->notebook = GLOBALS->notebook;
 new_globals->second_page_created = GLOBALS->second_page_created;
 (*new_globals->contexts)[new_globals->this_context_page] = new_globals;
 new_globals->dead_context = GLOBALS->dead_context; /* this value is a ** chameleon!  malloc'd region is outside debug.c control! */

 /* to cut down on temporary visual noise from incorrect zoom factor on reload */
 new_globals->pixelsperframe = GLOBALS->pixelsperframe;
 new_globals->nsperframe = GLOBALS->nsperframe;
 new_globals->pxns = GLOBALS->pxns;
 new_globals->nspx = GLOBALS->nspx;
 new_globals->nspx = GLOBALS->nspx;
 new_globals->zoom = GLOBALS->zoom;

 /* Default colors, X contexts, pixmaps, drawables, etc from signalwindow.c and wavewindow.c */
 new_globals->possibly_use_rc_defaults = GLOBALS->possibly_use_rc_defaults;

 new_globals->signalarea_event_box = GLOBALS->signalarea_event_box;
 new_globals->keypress_handler_id = GLOBALS->keypress_handler_id;
 new_globals->signalarea = GLOBALS->signalarea;
 new_globals->wavearea = GLOBALS->wavearea;
 new_globals->wavepixmap_wavewindow_c_1 = GLOBALS->wavepixmap_wavewindow_c_1;
 new_globals->signalpixmap = GLOBALS->signalpixmap;
 new_globals->wave_splash_pixmap = GLOBALS->wave_splash_pixmap;
 new_globals->wave_splash_mask = GLOBALS->wave_splash_mask;

 new_globals->black_and_white = GLOBALS->black_and_white;

 new_globals->gc_white = GLOBALS->gc_white;
 new_globals->gc_black = GLOBALS->gc_black;
 memcpy(&new_globals->gc, &GLOBALS->gc, sizeof(struct wave_gcmaster_t));
 new_globals->made_sgc_contexts_wavewindow_c_1 = GLOBALS->made_sgc_contexts_wavewindow_c_1;
 new_globals->made_gc_contexts_wavewindow_c_1 = GLOBALS->made_gc_contexts_wavewindow_c_1;
 memcpy(&new_globals->gccache, &GLOBALS->gccache, sizeof(struct wave_gcmaster_t));
 memcpy(&new_globals->gc_rainbow, &GLOBALS->gc_rainbow, 2 * WAVE_NUM_RAINBOW * sizeof(GdkGC *));

 new_globals->mainwindow = GLOBALS->mainwindow;
 new_globals->signalwindow = GLOBALS->signalwindow;
 new_globals->wavewindow = GLOBALS->wavewindow;
 new_globals->toppanedwindow = GLOBALS->toppanedwindow;
 new_globals->panedwindow = GLOBALS->panedwindow;
 new_globals->sstpane = GLOBALS->sstpane;
 new_globals->sst_vpaned = GLOBALS->sst_vpaned;
 new_globals->expanderwindow = GLOBALS->expanderwindow;
 new_globals->signal_hslider = GLOBALS->signal_hslider;
 new_globals->wave_vslider = GLOBALS->wave_vslider;
 new_globals->wave_hslider = GLOBALS->wave_hslider;
 new_globals->hscroll_wavewindow_c_2 = GLOBALS->hscroll_wavewindow_c_2;
 new_globals->max_or_marker_label_currenttime_c_1 = GLOBALS->max_or_marker_label_currenttime_c_1;
 new_globals->maxtext_currenttime_c_1 = (char *) calloc_2_into_context(new_globals,1,40);
 memcpy(new_globals->maxtext_currenttime_c_1, GLOBALS->maxtext_currenttime_c_1,40);
 new_globals->maxtimewid_currenttime_c_1 = GLOBALS->maxtimewid_currenttime_c_1;
 new_globals->curtext_currenttime_c_1 = (char *) calloc_2_into_context(new_globals,1,40);
 memcpy(new_globals->curtext_currenttime_c_1, GLOBALS->curtext_currenttime_c_1, 40);
 new_globals->base_or_curtime_label_currenttime_c_1 = GLOBALS->base_or_curtime_label_currenttime_c_1;
 new_globals->curtimewid_currenttime_c_1 = GLOBALS->curtimewid_currenttime_c_1;
 new_globals->from_entry = GLOBALS->from_entry;
 new_globals->to_entry = GLOBALS->to_entry;

 new_globals->fontheight = GLOBALS->fontheight;
 new_globals->wavecrosspiece = GLOBALS->wavecrosspiece;

 new_globals->signalfont = calloc_2_into_context(new_globals, 1, sizeof(struct font_engine_font_t));
 memcpy(new_globals->signalfont, GLOBALS->signalfont, sizeof(struct font_engine_font_t));

 new_globals->wavefont = calloc_2_into_context(new_globals, 1, sizeof(struct font_engine_font_t));
 memcpy(new_globals->wavefont, GLOBALS->wavefont, sizeof(struct font_engine_font_t));

 new_globals->wavefont_smaller = calloc_2_into_context(new_globals, 1, sizeof(struct font_engine_font_t));
 memcpy(new_globals->wavefont_smaller, GLOBALS->wavefont_smaller, sizeof(struct font_engine_font_t));

#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN) && GTK_CHECK_VERSION(2,8,0)
 new_globals->fonts_renderer = GLOBALS->fonts_renderer;
 new_globals->fonts_gc = GLOBALS->fonts_gc;
 new_globals->fonts_screen = GLOBALS->fonts_screen;
 new_globals->fonts_context = GLOBALS->fonts_context;
 new_globals->fonts_layout = GLOBALS->fonts_layout;
#endif
 new_globals->use_pango_fonts = GLOBALS->use_pango_fonts;

 new_globals->ruler_origin = GLOBALS->ruler_origin;
 new_globals->ruler_step = GLOBALS->ruler_step;

 /* busy.c */
 new_globals->busycursor_busy_c_1 = GLOBALS->busycursor_busy_c_1;
 new_globals->busy_busy_c_1 = GLOBALS->busy_busy_c_1;

 /* pixmaps.c */
 new_globals->redo_pixmap = GLOBALS->redo_pixmap;
 new_globals->larrow_pixmap = GLOBALS->larrow_pixmap;
 new_globals->rarrow_pixmap = GLOBALS->rarrow_pixmap;
 new_globals->zoomout_pixmap = GLOBALS->zoomout_pixmap;
 new_globals->zoomin_pixmap = GLOBALS->zoomin_pixmap;
 new_globals->zoomfit_pixmap = GLOBALS->zoomfit_pixmap;
 new_globals->zoomundo_pixmap = GLOBALS->zoomundo_pixmap;
 new_globals->zoom_larrow_pixmap = GLOBALS->zoom_larrow_pixmap;
 new_globals->zoom_rarrow_pixmap = GLOBALS->zoom_rarrow_pixmap;
 new_globals->prev_page_pixmap = GLOBALS->prev_page_pixmap;
 new_globals->next_page_pixmap = GLOBALS->next_page_pixmap;
 new_globals->wave_info_pixmap = GLOBALS->wave_info_pixmap;
 new_globals->wave_alert_pixmap = GLOBALS->wave_alert_pixmap;

 clone_icon_pointers_across_contexts(new_globals, GLOBALS);

 /* rc.c */
 new_globals->scale_to_time_dimension = GLOBALS->scale_to_time_dimension;
 new_globals->zoom_dyn = GLOBALS->zoom_dyn;
 new_globals->zoom_dyne = GLOBALS->zoom_dyne;
 new_globals->use_scrollwheel_as_y = GLOBALS->use_scrollwheel_as_y;
 new_globals->enable_slider_zoom = GLOBALS->enable_slider_zoom;
 new_globals->context_tabposition = GLOBALS->context_tabposition;
 new_globals->use_standard_clicking = GLOBALS->use_standard_clicking;
 new_globals->dragzoom_threshold = GLOBALS->dragzoom_threshold;

 new_globals->ignore_savefile_pane_pos = 1; /* to keep window from resizing/jumping */
 new_globals->ignore_savefile_pos = 1; /* to keep window from resizing/jumping */
 new_globals->ignore_savefile_size = 1; /* to keep window from resizing/jumping */

 new_globals->color_back = GLOBALS->color_back;
 new_globals->color_baseline = GLOBALS->color_baseline;
 new_globals->color_grid = GLOBALS->color_grid;
 new_globals->color_grid2 = GLOBALS->color_grid2;
 new_globals->color_high = GLOBALS->color_high;
 new_globals->color_highfill = GLOBALS->color_highfill;
 new_globals->color_low = GLOBALS->color_low;
 new_globals->color_1 = GLOBALS->color_1;
 new_globals->color_1fill = GLOBALS->color_1fill;
 new_globals->color_0 = GLOBALS->color_0;
 new_globals->color_mark = GLOBALS->color_mark;
 new_globals->color_mid = GLOBALS->color_mid;
 new_globals->color_time = GLOBALS->color_time;
 new_globals->color_timeb = GLOBALS->color_timeb;
 new_globals->color_trans = GLOBALS->color_trans;
 new_globals->color_umark = GLOBALS->color_umark;
 new_globals->color_value = GLOBALS->color_value;
 new_globals->color_vbox = GLOBALS->color_vbox;
 new_globals->color_vtrans = GLOBALS->color_vtrans;
 new_globals->color_x = GLOBALS->color_x;
 new_globals->color_xfill = GLOBALS->color_xfill;
 new_globals->color_u = GLOBALS->color_u;
 new_globals->color_ufill = GLOBALS->color_ufill;
 new_globals->color_w = GLOBALS->color_w;
 new_globals->color_wfill = GLOBALS->color_wfill;
 new_globals->color_dash = GLOBALS->color_dash;
 new_globals->color_dashfill = GLOBALS->color_dashfill;
 new_globals->color_white = GLOBALS->color_white;
 new_globals->color_black = GLOBALS->color_black;
 new_globals->color_ltgray = GLOBALS->color_ltgray;
 new_globals->color_normal = GLOBALS->color_normal;
 new_globals->color_mdgray = GLOBALS->color_mdgray;
 new_globals->color_dkgray = GLOBALS->color_dkgray;
 new_globals->color_dkblue = GLOBALS->color_dkblue;
 new_globals->color_brkred = GLOBALS->color_brkred;
 new_globals->color_ltblue = GLOBALS->color_ltblue;
 new_globals->color_gmstrd = GLOBALS->color_gmstrd;

 new_globals->atomic_vectors = GLOBALS->atomic_vectors;
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
 new_globals->alt_wheel_mode = GLOBALS->alt_wheel_mode;
 new_globals->initial_signal_window_width = GLOBALS->initial_signal_window_width;
 new_globals->enable_fast_exit = GLOBALS->enable_fast_exit;
 new_globals->enable_ghost_marker = GLOBALS->enable_ghost_marker;
 new_globals->enable_horiz_grid = GLOBALS->enable_horiz_grid;
 new_globals->make_vcd_save_file = GLOBALS->make_vcd_save_file;
 new_globals->enable_vert_grid = GLOBALS->enable_vert_grid;
 new_globals->force_toolbars = GLOBALS->force_toolbars;
 new_globals->hide_sst = GLOBALS->hide_sst;
 new_globals->sst_expanded = GLOBALS->sst_expanded;
 new_globals->hier_grouping = GLOBALS->hier_grouping;
 new_globals->hier_max_level = GLOBALS->hier_max_level;
 new_globals->hier_max_level_shadow = GLOBALS->hier_max_level_shadow;
 new_globals->paned_pack_semantics = GLOBALS->paned_pack_semantics;
 new_globals->left_justify_sigs = GLOBALS->left_justify_sigs;
 new_globals->lxt_clock_compress_to_z = GLOBALS->lxt_clock_compress_to_z;
 new_globals->extload_max_tree = GLOBALS->extload_max_tree;
 new_globals->ps_maxveclen = GLOBALS->ps_maxveclen;
 new_globals->show_base = GLOBALS->show_base;
 new_globals->display_grid = GLOBALS->display_grid;
 new_globals->highlight_wavewindow = GLOBALS->highlight_wavewindow;
 new_globals->fill_waveform = GLOBALS->fill_waveform;
 new_globals->use_standard_trace_select = GLOBALS->use_standard_trace_select;
 new_globals->use_big_fonts = GLOBALS->use_big_fonts;
 new_globals->use_full_precision = GLOBALS->use_full_precision;
 new_globals->use_frequency_delta = GLOBALS->use_frequency_delta;
 new_globals->use_maxtime_display = GLOBALS->use_maxtime_display;
 new_globals->use_nonprop_fonts = GLOBALS->use_nonprop_fonts;
 new_globals->use_roundcaps = GLOBALS->use_roundcaps;
 new_globals->use_scrollbar_only = GLOBALS->use_scrollbar_only;
 new_globals->vcd_explicit_zero_subscripts = GLOBALS->vcd_explicit_zero_subscripts;
 new_globals->vcd_preserve_glitches = GLOBALS->vcd_preserve_glitches;
 new_globals->vcd_preserve_glitches_real = GLOBALS->vcd_preserve_glitches_real;
 new_globals->vcd_warning_filesize = GLOBALS->vcd_warning_filesize;
 new_globals->vector_padding = GLOBALS->vector_padding;
 new_globals->vlist_prepack = GLOBALS->vlist_prepack;
 new_globals->vlist_spill_to_disk = GLOBALS->vlist_spill_to_disk;
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

 new_globals->disable_ae2_alias = GLOBALS->disable_ae2_alias;
 new_globals->hier_ignore_escapes = GLOBALS->hier_ignore_escapes;

 new_globals->splash_disable = 1; /* to disable splash for reload */
 new_globals->strace_repeat_count = GLOBALS->strace_repeat_count; /* for edgebuttons and also strace */

 new_globals->logfiles = GLOBALS->logfiles; /* this value is a ** chameleon!  malloc'd region is outside debug.c control! */
 new_globals->sst_dbl_action_type = GLOBALS->sst_dbl_action_type;
 new_globals->save_on_exit = GLOBALS->save_on_exit;

 strcpy2_into_new_context(new_globals, &new_globals->argvlist, &GLOBALS->argvlist);
 strcpy2_into_new_context(new_globals, &new_globals->editor_name, &GLOBALS->editor_name);
 strcpy2_into_new_context(new_globals, &new_globals->fontname_logfile, &GLOBALS->fontname_logfile);
 strcpy2_into_new_context(new_globals, &new_globals->fontname_signals, &GLOBALS->fontname_signals);
 strcpy2_into_new_context(new_globals, &new_globals->fontname_waves, &GLOBALS->fontname_waves);
 strcpy2_into_new_context(new_globals, &new_globals->cutcopylist, &GLOBALS->cutcopylist);
 strcpy2_into_new_context(new_globals, &new_globals->tcl_init_cmd, &GLOBALS->tcl_init_cmd);
 strcpy2_into_new_context(new_globals, &new_globals->repscript_name, &GLOBALS->repscript_name);
 new_globals->repscript_period = GLOBALS->repscript_period;
#if GTK_CHECK_VERSION(2,4,0)
 strcpy2_into_new_context(new_globals, &new_globals->pFileChooseFilterName, &GLOBALS->pFileChooseFilterName);
#endif

 /* search.c */
 new_globals->regex_which_search_c_1 = GLOBALS->regex_which_search_c_1; /* preserve search type */

 /* ttranslate.c */
 strcpy2_into_new_context(new_globals, &new_globals->ttranslate_args, &GLOBALS->ttranslate_args);

 /* vlist.c */
 if(GLOBALS->vlist_handle)
	{
	vlist_kill_spillfile();
	new_globals->use_fastload = (GLOBALS->use_fastload != VCD_FSL_NONE) ? VCD_FSL_WRITE : VCD_FSL_NONE;
	}

 /* lxt2.c / vzt.c / ae2.c */
 strcpy2_into_new_context(new_globals, &new_globals->skip_start, &GLOBALS->skip_start);
 strcpy2_into_new_context(new_globals, &new_globals->skip_end, &GLOBALS->skip_end);

 /* main.c */
 new_globals->missing_file_toolbar = GLOBALS->missing_file_toolbar;
 new_globals->is_gtkw_save_file = GLOBALS->is_gtkw_save_file;
 new_globals->use_toolbutton_interface = GLOBALS->use_toolbutton_interface;
 new_globals->optimize_vcd = GLOBALS->optimize_vcd;
 strcpy2_into_new_context(new_globals, &new_globals->winname, &GLOBALS->winname); /* for page swapping */

 /* menu.c */
 new_globals->wave_script_args = GLOBALS->wave_script_args;
#ifndef WAVE_USE_MLIST_T
 new_globals->item_factory_menu_c_1 = GLOBALS->item_factory_menu_c_1;
#endif
 strcpy2_into_new_context(new_globals, &new_globals->filesel_writesave, &GLOBALS->filesel_writesave);
 new_globals->save_success_menu_c_1 = GLOBALS->save_success_menu_c_1;
 new_globals->signal_popup_menu = GLOBALS->signal_popup_menu;
 new_globals->sst_signal_popup_menu = GLOBALS->sst_signal_popup_menu;

 strcpy2_into_new_context(new_globals, &new_globals->filesel_vcd_writesave, &GLOBALS->filesel_vcd_writesave);
 strcpy2_into_new_context(new_globals, &new_globals->filesel_lxt_writesave, &GLOBALS->filesel_lxt_writesave);
 strcpy2_into_new_context(new_globals, &new_globals->filesel_tim_writesave, &GLOBALS->filesel_tim_writesave);

 strcpy2_into_new_context(new_globals, &new_globals->stems_name, &GLOBALS->stems_name); /* remaining fileselbox() vars not handled elsewhere */
 strcpy2_into_new_context(new_globals, &new_globals->filesel_logfile_menu_c_1, &GLOBALS->filesel_logfile_menu_c_1);
 strcpy2_into_new_context(new_globals, &new_globals->filesel_scriptfile_menu, &GLOBALS->filesel_scriptfile_menu);
 strcpy2_into_new_context(new_globals, &new_globals->fcurr_ttranslate_c_1, &GLOBALS->fcurr_ttranslate_c_1);
 strcpy2_into_new_context(new_globals, &new_globals->fcurr_ptranslate_c_1, &GLOBALS->fcurr_ptranslate_c_1);
 strcpy2_into_new_context(new_globals, &new_globals->fcurr_translate_c_2, &GLOBALS->fcurr_translate_c_2);

 strcpy2_into_new_context(new_globals, &new_globals->filesel_imagegrab, &GLOBALS->filesel_imagegrab);

 /* renderopt.c */
#ifdef WAVE_GTK_UNIX_PRINT
 new_globals->gprs = GLOBALS->gprs;
 new_globals->gps = GLOBALS->gps;
#endif


 /* status.c */
 new_globals->text_status_c_2 = GLOBALS->text_status_c_2;
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
 memcpy(&new_globals->iter_status_c_3, &GLOBALS->iter_status_c_3, sizeof(GtkTextIter));
#endif

 /* treesearch_gtk2.c */
 new_globals->do_dynamic_treefilter = GLOBALS->do_dynamic_treefilter;
 new_globals->treesearch_gtk2_window_vbox = GLOBALS->treesearch_gtk2_window_vbox;
 new_globals->window_treesearch_gtk2_c_12 = GLOBALS->window_treesearch_gtk2_c_12;
 new_globals->dnd_sigview = GLOBALS->dnd_sigview;

 strcpy2_into_new_context(new_globals, &new_globals->filter_str_treesearch_gtk2_c_1, &GLOBALS->filter_str_treesearch_gtk2_c_1);
 strcpy2_into_new_context(new_globals, &new_globals->selected_hierarchy_name, &GLOBALS->selected_hierarchy_name);
 new_globals->filter_typ_treesearch_gtk2_c_1 = GLOBALS->filter_typ_treesearch_gtk2_c_1;
 new_globals->filter_matlen_treesearch_gtk2_c_1 = GLOBALS->filter_matlen_treesearch_gtk2_c_1;
 new_globals->filter_noregex_treesearch_gtk2_c_1 = GLOBALS->filter_noregex_treesearch_gtk2_c_1;

 strcpy2_into_new_context(new_globals, &new_globals->sst_exclude_filename, &GLOBALS->sst_exclude_filename);
 if(GLOBALS->exclcompname)
        {
        jrb_free_tree(GLOBALS->exclcompname); /* strings get freed by automatic _2 mechanism */
        GLOBALS->exclcompname = NULL;
        }
 if(GLOBALS->exclinstname)
        {
        jrb_free_tree(GLOBALS->exclinstname); /* strings get freed by automatic _2 mechanism */
        GLOBALS->exclinstname = NULL;
        }

 /* timeentry.c */
 new_globals->from_entry = GLOBALS->from_entry;
 new_globals->to_entry = GLOBALS->to_entry;

 if(GLOBALS->tims.first != GLOBALS->min_time)
	{
	fix_from_time = 1;
	from_time = GLOBALS->tims.first;
	}

 if(GLOBALS->tims.last != GLOBALS->max_time)
	{
	fix_to_time = 1;
	to_time = GLOBALS->tims.last;
	}

 /* twinwave stuff */
 new_globals->dual_attach_id_main_c_1 = GLOBALS->dual_attach_id_main_c_1;
 new_globals->dual_id = GLOBALS->dual_id;
#ifdef WAVE_USE_GTK2
 new_globals->socket_xid = GLOBALS->socket_xid;
#endif
 new_globals->dual_ctx = GLOBALS->dual_ctx;

 /* Times struct */
 memcpy(&(new_globals->tims), &(GLOBALS->tims), sizeof(Times));

 /* File name and type */
 new_globals->loaded_file_type = GLOBALS->loaded_file_type;

 strcpy2_into_new_context(new_globals, &new_globals->loaded_file_name, &GLOBALS->loaded_file_name);

 if(new_globals->optimize_vcd) {
   strcpy2_into_new_context(new_globals, &new_globals->unoptimized_vcd_file_name, &GLOBALS->unoptimized_vcd_file_name);
 }


 /* fst.c, though might be others later */
 if(GLOBALS->subvar_jrb)
	{
 	jrb_free_tree(GLOBALS->subvar_jrb);
 	GLOBALS->subvar_jrb = NULL;
 	GLOBALS->subvar_jrb_count = 0;
	}

 if(GLOBALS->synclock_jrb)
	{
 	jrb_free_tree(GLOBALS->synclock_jrb);
	GLOBALS->synclock_jrb = NULL;
	}

 if(GLOBALS->enum_nptrs_jrb)
	{
	jrb_free_tree(GLOBALS->enum_nptrs_jrb);
	GLOBALS->enum_nptrs_jrb = NULL;
	}

#ifdef _WAVE_HAVE_JUDY
 if(GLOBALS->num_xl_enum_filter)
	{
	int ie;
	for(ie=0;ie<GLOBALS->num_xl_enum_filter;ie++)
		{
		JudyHSFreeArray(&GLOBALS->xl_enum_filter[ie], NULL);
		}

	GLOBALS->num_xl_enum_filter = 0;
	}
#endif

 /* deallocate any loader-related stuff */
 switch(GLOBALS->loaded_file_type) {
   case LXT_FILE:
                if(GLOBALS->mm_lxt_mmap_addr)
                        {
                        munmap(GLOBALS->mm_lxt_mmap_addr, GLOBALS->mm_lxt_mmap_len);
                        }
                break;

   case LX2_FILE: lxt2_rd_close(GLOBALS->lx2_lx2_c_1); break;
   case VZT_FILE: vzt_rd_close(GLOBALS->vzt_vzt_c_1); break;
   case FST_FILE: fstReaderClose(GLOBALS->fst_fst_c_1); GLOBALS->fst_fst_c_1 = NULL; break;
   case AE2_FILE:
#ifdef AET2_IS_PRESENT
#ifdef AET2_ALIASDB_IS_PRESENT
	if(GLOBALS->adb) { adb_close_db(GLOBALS->adb); GLOBALS->adb = 0; }
	/* if(GLOBALS->m_alias_stream_file) { fclose(GLOBALS->m_alias_stream_file); GLOBALS->m_alias_stream_file = NULL; } */
	ae2_read_end(GLOBALS->ae2); fclose(GLOBALS->ae2_f);
#endif
#endif
	break;

#ifdef EXTLOAD_SUFFIX
   case EXTLOAD_FILE:
	if(GLOBALS->extload_ffr_ctx)
		{
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
   case VCD_FILE:
   case VCD_RECODER_FILE:
   default:
	/* do nothing */ break;
 }


 /* window destruction (of windows that aren't the parent window) */

#if !defined _MSC_VER
 kill_stems_browser();	/* for now, need to rework the stems browser dumpfile access routines to support this properly */
 new_globals->stems_type = GLOBALS->stems_type;
 strcpy2_into_new_context(new_globals, &new_globals->aet_name, &GLOBALS->aet_name);
#endif

 /* remaining windows which are completely destroyed and haven't migrated over */
 widget_only_destroy(&GLOBALS->window_ptranslate_c_5);		/* ptranslate.c */
 WAVE_STRACE_ITERATOR(s_ctx_iter)
	{
	GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
	widget_only_destroy(&GLOBALS->strace_ctx->window_strace_c_10);	/* strace.c */
	}
 widget_only_destroy(&GLOBALS->window_translate_c_11);		/* translate.c */
 widget_only_destroy(&GLOBALS->window_treesearch_gtk1_c);	/* treesearch_gtk1.c */
 widget_only_destroy(&GLOBALS->window_help_c_2); 		/* help.c : reload is gated off during help so this should never execute */

 /* windows which in theory should never destroy as they will have grab focus which means reload will not be called */
 widget_ungrab_destroy(&GLOBALS->window_entry_c_1);		/* entry.c */
 widget_ungrab_destroy(&GLOBALS->window1_hiersearch_c_1);	/* hiersearch.c */
 widget_ungrab_destroy(&GLOBALS->window_markerbox_c_4);		/* markerbox.c */
 widget_ungrab_destroy(&GLOBALS->window1_search_c_2);		/* search.c */
 widget_ungrab_destroy(&GLOBALS->window_showchange_c_8);	/* showchange.c */
 widget_ungrab_destroy(&GLOBALS->window_simplereq_c_9);		/* simplereq.c */
 widget_ungrab_destroy(&GLOBALS->window1_treesearch_gtk1_c);	/* treesearch_gtk1.c */
 widget_ungrab_destroy(&GLOBALS->window1_treesearch_gtk2_c_3);	/* treesearch_gtk2.c */

 /* supported migration of window contexts... */
 if(GLOBALS->window_hiersearch_c_3)
        {
	struct treechain *tc = GLOBALS->treechain_hiersearch_c_1;
	while(tc)
		{
		char *tclname;

		if(!hier_curr)
			{
			hier_head = hier_curr = calloc_2_into_context(new_globals,1,sizeof(struct stringchain_t));
			}
			else
			{
			hier_curr->next = calloc_2_into_context(new_globals,1,sizeof(struct stringchain_t));
			hier_curr = hier_curr->next;
			}

		tclname = &tc->label->name[0];
		strcpy2_into_new_context(new_globals, &hier_curr->name, &tclname);

		tc = tc->next;
		}

	new_globals->window_hiersearch_c_3 = GLOBALS->window_hiersearch_c_3;
	new_globals->entry_main_hiersearch_c_1 = GLOBALS->entry_main_hiersearch_c_1;
	new_globals->clist_hiersearch_c_1 = GLOBALS->clist_hiersearch_c_1;
	new_globals->bundle_direction_hiersearch_c_1 = GLOBALS->bundle_direction_hiersearch_c_1;
	new_globals->cleanup_hiersearch_c_3 = GLOBALS->cleanup_hiersearch_c_3;
	new_globals->is_active_hiersearch_c_1 = GLOBALS->is_active_hiersearch_c_1;
        }

 if(GLOBALS->mouseover_mouseover_c_1) /* mouseover regenerates as the pointer moves so no real context lost */
        {
        gtk_widget_destroy(GLOBALS->mouseover_mouseover_c_1); GLOBALS->mouseover_mouseover_c_1 = NULL;
        gdk_pixmap_unref(GLOBALS->mo_pixmap_mouseover_c_1);   GLOBALS->mo_pixmap_mouseover_c_1 = NULL;
        }

 if(GLOBALS->window_renderopt_c_6)
        {
	new_globals->is_active_renderopt_c_3 = GLOBALS->is_active_renderopt_c_3;
	new_globals->window_renderopt_c_6 = GLOBALS->window_renderopt_c_6;

	memcpy(new_globals->target_mutex_renderopt_c_1, GLOBALS->target_mutex_renderopt_c_1, sizeof(GLOBALS->target_mutex_renderopt_c_1));
	memcpy(new_globals->page_mutex_renderopt_c_1, GLOBALS->page_mutex_renderopt_c_1, sizeof(GLOBALS->page_mutex_renderopt_c_1));
	memcpy(new_globals->render_mutex_renderopt_c_1, GLOBALS->render_mutex_renderopt_c_1, sizeof(GLOBALS->render_mutex_renderopt_c_1));

	strcpy2_into_new_context(new_globals, &new_globals->filesel_print_pdf_renderopt_c_1, &GLOBALS->filesel_print_pdf_renderopt_c_1);
	strcpy2_into_new_context(new_globals, &new_globals->filesel_print_ps_renderopt_c_1, &GLOBALS->filesel_print_ps_renderopt_c_1);
	strcpy2_into_new_context(new_globals, &new_globals->filesel_print_mif_renderopt_c_1, &GLOBALS->filesel_print_mif_renderopt_c_1);

	new_globals->page_size_type_renderopt_c_1 = GLOBALS->page_size_type_renderopt_c_1;
        }

 if(GLOBALS->window_search_c_7)
        {
	int i;

	new_globals->pdata = calloc_2_into_context(new_globals, 1, sizeof(SearchProgressData));
	memcpy(new_globals->pdata, GLOBALS->pdata, sizeof(SearchProgressData));

	new_globals->is_active_search_c_4 = GLOBALS->is_active_search_c_4;
	new_globals->regex_which_search_c_1 = GLOBALS->regex_which_search_c_1;
	new_globals->window_search_c_7 = GLOBALS->window_search_c_7;
	new_globals->entry_search_c_3 = GLOBALS->entry_search_c_3;
	new_globals->clist_search_c_3 = GLOBALS->clist_search_c_3;

	strcpy2_into_new_context(new_globals, &new_globals->searchbox_text_search_c_1, &GLOBALS->searchbox_text_search_c_1);

	for(i=0;i<5;i++)
		{
		new_globals->menuitem_search[i] = GLOBALS->menuitem_search[i];
		new_globals->regex_mutex_search_c_1[i] = GLOBALS->regex_mutex_search_c_1[i];
		}
        }


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
 for(;;)
	{
	set_window_busy(NULL);

 /* Check to see if we need to reload a vcd file */
#if !defined _MSC_VER && !defined __MINGW32__
	if(GLOBALS->optimize_vcd)
		{
	   	optimize_vcd_file();
	 	}
#endif
	/* Load new file from disk, no reload on partial vcd or vcd from stdin. */
	switch(GLOBALS->loaded_file_type)
		{
#ifdef EXTLOAD_SUFFIX
   		case EXTLOAD_FILE:
			extload_main(GLOBALS->loaded_file_name, GLOBALS->skip_start, GLOBALS->skip_end);
			load_was_success = (GLOBALS->extload != NULL) && (!GLOBALS->extload_already_errored);
			GLOBALS->extload_already_errored = 0;
			GLOBALS->extload_lastmod = 0;
			break;
#endif

   		case LX2_FILE:
			lx2_main(GLOBALS->loaded_file_name,GLOBALS->skip_start,GLOBALS->skip_end);
			load_was_success = (GLOBALS->lx2_lx2_c_1 != NULL);
			break;

   		case VZT_FILE:
			vzt_main(GLOBALS->loaded_file_name,GLOBALS->skip_start,GLOBALS->skip_end);
			load_was_success = (GLOBALS->vzt_vzt_c_1 != NULL);
			break;

   		case FST_FILE:
			fst_main(GLOBALS->loaded_file_name,GLOBALS->skip_start,GLOBALS->skip_end);
			load_was_success = (GLOBALS->fst_fst_c_1 != NULL);
			break;

   		case AE2_FILE:
#ifdef AET2_IS_PRESENT
			ae2_main(GLOBALS->loaded_file_name,GLOBALS->skip_start,GLOBALS->skip_end);
			load_was_success = (GLOBALS->ae2 != NULL);
#else
			load_was_success = 0; /* never executes as AE2_FILE won't be set */
#endif
			break;

   		case GHW_FILE:
			load_was_success = (ghw_main(GLOBALS->loaded_file_name) != 0);
			break;

		case LXT_FILE:
   		case VCD_FILE:
		case VCD_RECODER_FILE:
			load_was_success = handle_setjmp();
			break;
		default:
			break;
 		}

	set_window_idle(NULL);

	if(load_was_success)
		{
		break;
		}
		else
		{
		/* recovery sequence */
		printf("GTKWAVE | Reload failure, reattempt in %d second%s...\n", reload_fail_delay, (reload_fail_delay==1) ? "" : "s");
		fflush(stdout);
		sleep(reload_fail_delay);
		switch(reload_fail_delay)
			{
			case 1: reload_fail_delay = 2; break;
			case 2: reload_fail_delay = 5; break;
			case 5: reload_fail_delay = 10; break;
			case 10: reload_fail_delay = 30; break;
			case 30: reload_fail_delay = 60; break;
			default: break;
			}
		}
	}

 /* deallocate the symbol hash table */
 sym_hash_destroy(GLOBALS);

 /* Setup timings we probably need to redraw here */
 GLOBALS->tims.last=GLOBALS->max_time;
 GLOBALS->tims.first=GLOBALS->min_time;

 if(fix_from_time)
	{
	if((from_time >= GLOBALS->min_time) && (from_time <= GLOBALS->max_time))
		{
		GLOBALS->tims.first = from_time;
		}
	}

 if(fix_to_time)
	{
	if((to_time >= GLOBALS->min_time) && (to_time <= GLOBALS->max_time) && (to_time > GLOBALS->tims.first))
		{
		GLOBALS->tims.last = to_time;
		}
	}

 if(GLOBALS->tims.start < GLOBALS->tims.first) 	    { GLOBALS->tims.start = GLOBALS->tims.first; }
 if(GLOBALS->tims.end > GLOBALS->tims.last) 	    { GLOBALS->tims.end = GLOBALS->tims.last; }
 if(GLOBALS->tims.marker < GLOBALS->tims.first)     { GLOBALS->tims.marker = GLOBALS->tims.first; }
 if(GLOBALS->tims.marker > GLOBALS->tims.last)      { GLOBALS->tims.marker = GLOBALS->tims.last; }
 if(GLOBALS->tims.prevmarker < GLOBALS->tims.first) { GLOBALS->tims.prevmarker = GLOBALS->tims.first; }
 if(GLOBALS->tims.prevmarker > GLOBALS->tims.last)  { GLOBALS->tims.prevmarker = GLOBALS->tims.last; }
 if(GLOBALS->tims.laststart < GLOBALS->tims.first)  { GLOBALS->tims.laststart = GLOBALS->tims.first; }
 if(GLOBALS->tims.laststart > GLOBALS->tims.last)   { GLOBALS->tims.laststart = GLOBALS->tims.last; }

 reformat_time(timestr, GLOBALS->tims.first + GLOBALS->global_time_offset, GLOBALS->time_dimension);
 gtk_entry_set_text(GTK_ENTRY(GLOBALS->from_entry),timestr);
 reformat_time(timestr, GLOBALS->tims.last + GLOBALS->global_time_offset, GLOBALS->time_dimension);
 gtk_entry_set_text(GTK_ENTRY(GLOBALS->to_entry),timestr);


 /* Change SST - if it exists */
 /* XXX need to destroy/free the old tree widgets. */
 #if GTK_CHECK_VERSION(2,4,0)
 if(!GLOBALS->hide_sst) {
   gint pane_pos = gtk_paned_get_position(GLOBALS->sst_vpaned);
   gtk_widget_hide(GLOBALS->expanderwindow);
   gtk_container_remove(GTK_CONTAINER(GLOBALS->expanderwindow), GLOBALS->sstpane);
   GLOBALS->sstpane = treeboxframe("SST", GTK_SIGNAL_FUNC(mkmenu_treesearch_cleanup));
   gtk_container_add(GTK_CONTAINER(GLOBALS->expanderwindow), GLOBALS->sstpane);
   gtk_paned_set_position(GLOBALS->sst_vpaned, pane_pos);
   gtk_widget_show(GLOBALS->expanderwindow);
   if(GLOBALS->dnd_sigview)
        {
	dnd_setup(GLOBALS->dnd_sigview, GLOBALS->signalarea, 0);
	}

   if(GLOBALS->clist_search_c_3)
	{
	dnd_setup(GLOBALS->clist_search_c_3, GLOBALS->signalarea, 0);
	}
 }
 #endif
 #if WAVE_USE_GTK2
 if(GLOBALS->window_treesearch_gtk2_c_12)
	{
   	gtk_container_remove(GTK_CONTAINER(GLOBALS->window_treesearch_gtk2_c_12), GLOBALS->treesearch_gtk2_window_vbox);
   	treebox(NULL, GTK_SIGNAL_FUNC(mkmenu_treesearch_cleanup), GLOBALS->window_treesearch_gtk2_c_12);
	}

   if((GLOBALS->filter_str_treesearch_gtk2_c_1) && (GLOBALS->filter_entry))
	{
        gtk_entry_set_text(GTK_ENTRY(GLOBALS->filter_entry), GLOBALS->filter_str_treesearch_gtk2_c_1);
	wave_regex_compile(GLOBALS->filter_str_treesearch_gtk2_c_1 + GLOBALS->filter_matlen_treesearch_gtk2_c_1, WAVE_REGEX_TREE);
	}
 #endif

 /* Reload state from file */
 {
 char is_gtkw_save_file_cached = GLOBALS->is_gtkw_save_file;
 read_save_helper(reload_tmpfilename, NULL, NULL, NULL, NULL, NULL);
 GLOBALS->is_gtkw_save_file = is_gtkw_save_file_cached;
 }

 /* again doing this here (read_save_helper does it) seems to be necessary in order to keep display in sync */
 GLOBALS->signalwindow_width_dirty=1;
 MaxSignalLength();
 signalarea_configure_event(GLOBALS->signalarea, NULL);
 wavearea_configure_event(GLOBALS->wavearea, NULL);

 /* unlink temp */
 unlink(reload_tmpfilename);
 free(reload_tmpfilename); /* intentional use of free(), of strdup'd string from earlier */

 /* part 2 of SST (which needs to be done after the tree is expanded from loading the savefile...) */
 #if WAVE_USE_GTK2
 if((!GLOBALS->hide_sst)||(GLOBALS->window_treesearch_gtk2_c_12)) {
   GtkCList *cl = GTK_CLIST(GLOBALS->ctree_main);
   char *hiername_cache = NULL;	/* some strange race condition side effect in gtk selecting/deselecting blows this out so cache it */

   if(GLOBALS->selected_hierarchy_name)
	{
	hiername_cache = strdup_2(GLOBALS->selected_hierarchy_name);
   	select_tree_node(GLOBALS->selected_hierarchy_name);
	}

   if(tree_vadj_value != 0.0)
	{
	GtkAdjustment *vadj = gtk_clist_get_vadjustment(cl);

	if((vadj) && (tree_vadj_value >= vadj->lower) && (tree_vadj_value <= vadj->upper))
		{
		vadj->value = tree_vadj_value;
		gtk_clist_set_vadjustment(cl, vadj);

		gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(vadj)), "changed");
		gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(vadj)), "value_changed");
		}
	}

   if(tree_hadj_value != 0.0)
	{
	GtkAdjustment *hadj = gtk_clist_get_hadjustment(cl);

	if((hadj) && (tree_hadj_value >= hadj->lower) && (tree_hadj_value <= hadj->upper))
		{
		hadj->value = tree_hadj_value;
		gtk_clist_set_hadjustment(cl, hadj);

		gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(hadj)), "changed");
		gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(hadj)), "value_changed");
		}
	}

    if(hiername_cache)
	{
	if(GLOBALS->selected_hierarchy_name)
		{
		free_2(GLOBALS->selected_hierarchy_name);
		}
	GLOBALS->selected_hierarchy_name = hiername_cache;
	}


   if(tree_frame_y != -1)
	{
	/* this doesn't work...this sets the *minimum* size */
	/* gtk_widget_set_size_request(GLOBALS->gtk2_tree_frame, -1, tree_frame_y); */
	}

 if(GLOBALS->filter_entry) { gtk_signal_emit_by_name (GTK_OBJECT (GLOBALS->filter_entry), "activate"); }
 }
 #endif

 /* part 2 of search (which needs to be done after the new dumpfile is loaded) */
 if(GLOBALS->window_search_c_7)
        {
	search_enter_callback(GLOBALS->entry_search_c_3, NULL);
	}

 /* part 2 of hier search (which needs to be done after the new dumpfile is loaded) */
 if(GLOBALS->window_hiersearch_c_3)
        {
	if(!hier_curr)
        	{
        	GLOBALS->current_tree_hiersearch_c_1=GLOBALS->treeroot;
        	GLOBALS->h_selectedtree_hiersearch_c_1=NULL;
        	}
		else
		{
		struct tree *t = GLOBALS->treeroot;
		hier_curr = hier_head;

		while((hier_curr)&&(t))
			{
			if(!strcmp(hier_curr->name, t->name))
				{
			        if(t->child)
			                {
			                struct treechain *tc, *tc2;

			                tc=GLOBALS->treechain_hiersearch_c_1;
			                if(tc)
			                        {
			                        while(tc->next) tc=tc->next;

			                        tc2=calloc_2(1,sizeof(struct treechain));
			                        tc2->label=t;
			                        tc2->tree=GLOBALS->current_tree_hiersearch_c_1;
			                        tc->next=tc2;
			                        }
			                        else
			                        {
			                        GLOBALS->treechain_hiersearch_c_1=calloc_2(1,sizeof(struct treechain));
			                        GLOBALS->treechain_hiersearch_c_1->tree=GLOBALS->current_tree_hiersearch_c_1;
			                        GLOBALS->treechain_hiersearch_c_1->label=t;
			                        }

			                GLOBALS->current_tree_hiersearch_c_1=t->child;
			                }

				t = t->child;
				hier_curr = hier_curr->next;
				continue;
				}
			t = t->next;
			}

		hier_curr = hier_head;

		while(hier_head)
			{
			hier_head = hier_curr->next;
			free_2(hier_curr->name);
			free_2(hier_curr);
			hier_curr = hier_head;
			}
		}

	refresh_hier_tree(GLOBALS->current_tree_hiersearch_c_1);
	}

 /* restore these in case we decide to write out the rc file later as a user option */
 GLOBALS->ignore_savefile_pane_pos = cached_ignore_savefile_pane_pos;
 GLOBALS->ignore_savefile_pos = cached_ignore_savefile_pos;
 GLOBALS->ignore_savefile_size = cached_ignore_savefile_size;
 GLOBALS->splash_disable = cached_splash_disable;

 printf("GTKWAVE | ...waveform reloaded\n");
 gtkwavetcl_setvar(WAVE_TCLCB_RELOAD_END, GLOBALS->loaded_file_name, WAVE_TCLCB_RELOAD_END_FLAGS);

 /* update lower signal set in SST to correct position */
#if WAVE_USE_GTK2
   if((GLOBALS->dnd_sigview) && ((treeview_vadj_value != 0.0) || (treeview_hadj_value != 0.0)))
	{
	   struct Global *G2 = GLOBALS;
	   gtk_events_pending_gtk_main_iteration();

	   if((G2 == GLOBALS)&&(GLOBALS->dnd_sigview))
		{
		   if(treeview_vadj_value != 0.0)
			{
			GtkAdjustment *vadj = gtk_tree_view_get_vadjustment((GtkTreeView *)GLOBALS->dnd_sigview);

			if((vadj) && (treeview_vadj_value >= vadj->lower) && (treeview_vadj_value <= vadj->upper))
				{
				vadj->value = treeview_vadj_value;
				gtk_tree_view_set_vadjustment((GtkTreeView *)GLOBALS->dnd_sigview, vadj);

				gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(vadj)), "changed");
				gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(vadj)), "value_changed");
				}
			}

		   if(treeview_hadj_value != 0.0)
			{
			GtkAdjustment *hadj = gtk_tree_view_get_hadjustment((GtkTreeView *)GLOBALS->dnd_sigview);

			if((hadj) && (treeview_hadj_value >= hadj->lower) && (treeview_hadj_value <= hadj->upper))
				{
				hadj->value = treeview_hadj_value;
				gtk_tree_view_set_hadjustment((GtkTreeView *)GLOBALS->dnd_sigview, hadj);

				gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(hadj)), "changed");
				gtk_signal_emit_by_name (GTK_OBJECT (GTK_ADJUSTMENT(hadj)), "value_changed");
				}
			}
		}
	}
#endif
}


void reload_into_new_context(void)
{
static int reloading = 0;

if(!reloading)
	{
#ifdef MAC_INTEGRATION
	osx_menu_sensitivity(FALSE);
#endif
	reload_into_new_context_2();
	reloading = 0;
#ifdef MAC_INTEGRATION
	if(GLOBALS->loaded_file_type != MISSING_FILE)
		{
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
 switch(GLOBALS->loaded_file_type) {
   case LXT_FILE:
                if(GLOBALS->mm_lxt_mmap_addr)
                        {
                        munmap(GLOBALS->mm_lxt_mmap_addr, GLOBALS->mm_lxt_mmap_len);
                        }
                break;

   case LX2_FILE: lxt2_rd_close(GLOBALS->lx2_lx2_c_1); break;
   case VZT_FILE: vzt_rd_close(GLOBALS->vzt_vzt_c_1); break;
   case FST_FILE: fstReaderClose(GLOBALS->fst_fst_c_1); GLOBALS->fst_fst_c_1 = NULL; break;
   case AE2_FILE:
#ifdef AET2_IS_PRESENT
#ifdef AET2_ALIASDB_IS_PRESENT
	if(GLOBALS->adb) { adb_close_db(GLOBALS->adb); GLOBALS->adb = 0; }
	/* if(GLOBALS->adb_alias_stream_file) { fclose(GLOBALS->adb_alias_stream_file); GLOBALS->adb_alias_stream_file = NULL; } */
        ae2_read_end(GLOBALS->ae2); fclose(GLOBALS->ae2_f);
#endif
#endif
        break;

#ifdef EXTLOAD_SUFFIX
   case EXTLOAD_FILE:
	if(GLOBALS->extload_ffr_ctx)
		{
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
   case VCD_FILE:
   case VCD_RECODER_FILE:
   default:
	/* do nothing */ break;
 }

 /* window destruction (of windows that aren't the parent window) */

 widget_only_destroy(&GLOBALS->window_ptranslate_c_5);		/* ptranslate.c */

 WAVE_STRACE_ITERATOR(s_ctx_iter)
        {
        GLOBALS->strace_ctx = &GLOBALS->strace_windows[GLOBALS->strace_current_window = s_ctx_iter];
	widget_only_destroy(&GLOBALS->strace_ctx->window_strace_c_10);	/* strace.c */
	}
 widget_only_destroy(&GLOBALS->window_translate_c_11);		/* translate.c */
 widget_only_destroy(&GLOBALS->window_treesearch_gtk1_c);	/* treesearch_gtk1.c */
 widget_only_destroy(&GLOBALS->window_treesearch_gtk2_c_12);	/* treesearch_gtk2.c */
 widget_only_destroy(&GLOBALS->window_help_c_2); 		/* help.c : reload is gated off during help so this should never execute */

 /* windows which in theory should never destroy as they will have grab focus which means reload will not be called */
 widget_ungrab_destroy(&GLOBALS->window_entry_c_1);		/* entry.c */
 widget_ungrab_destroy(&GLOBALS->window1_hiersearch_c_1);	/* hiersearch.c */
 widget_ungrab_destroy(&GLOBALS->window_markerbox_c_4);		/* markerbox.c */
 widget_ungrab_destroy(&GLOBALS->window1_search_c_2);		/* search.c */
 widget_ungrab_destroy(&GLOBALS->window_showchange_c_8);	/* showchange.c */
 widget_ungrab_destroy(&GLOBALS->window_simplereq_c_9);		/* simplereq.c */
 widget_ungrab_destroy(&GLOBALS->window1_treesearch_gtk1_c);	/* treesearch_gtk1.c */
 widget_ungrab_destroy(&GLOBALS->window1_treesearch_gtk2_c_3);	/* treesearch_gtk2.c */

 /* supported migration of window contexts... */
 widget_only_destroy(&GLOBALS->window_hiersearch_c_3);

 if(GLOBALS->mouseover_mouseover_c_1) /* mouseover regenerates as the pointer moves so no real context lost */
        {
        gtk_widget_destroy(GLOBALS->mouseover_mouseover_c_1); GLOBALS->mouseover_mouseover_c_1 = NULL;
        gdk_pixmap_unref(GLOBALS->mo_pixmap_mouseover_c_1);   GLOBALS->mo_pixmap_mouseover_c_1 = NULL;
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

if(gp)
	{
	g_curr = GLOBALS;

	set_GLOBALS(gp);
	(*GLOBALS->dead_context)[0] = NULL;

	/* remove the bridge pointer */
	if(GLOBALS->gtk_context_bridge_ptr) { free(GLOBALS->gtk_context_bridge_ptr); GLOBALS->gtk_context_bridge_ptr = NULL; }

	/* Free all gcs */
	dealloc_all_gcs();

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

if(ignore_context_swap()) /* block context swap if explicitly directed (e.g., during loading) */
	{
	return(FALSE);
	}

type = event->type;

/* printf("Window: %08x GtkEvent: %08x gpointer: %08x, type: %d\n", w, event, data, type); */

switch(type)
	{
	case GDK_ENTER_NOTIFY:
	case GDK_FOCUS_CHANGE:
		if(GLOBALS->num_notebook_pages >= 2)
			{
			unsigned int i;
			void **vp;
			GtkWindow *wcmp;

			for(i=0;i<GLOBALS->num_notebook_pages;i++)
				{
				struct Global *test_g = (*GLOBALS->contexts)[i];

				vp = (void **)(((char *)test_g) + (intptr_t)data);
				wcmp = (GtkWindow *)(*vp);

				if(wcmp != NULL)
					{
					if(wcmp == w)
						{
						if(i!=GLOBALS->this_context_page)
							{
							struct Global *g_old = GLOBALS;

							/* printf("Switching to: %d %08x\n", i, GTK_WINDOW(wcmp)); */

							set_GLOBALS((*GLOBALS->contexts)[i]);

							GLOBALS->lxt_clock_compress_to_z = g_old->lxt_clock_compress_to_z;
							GLOBALS->autoname_bundles = g_old->autoname_bundles;
							GLOBALS->autocoalesce_reversal = g_old->autocoalesce_reversal;
							GLOBALS->autocoalesce = g_old->autocoalesce;
							GLOBALS->hier_grouping = g_old->hier_grouping;
							GLOBALS->wave_scrolling = g_old->wave_scrolling;
							GLOBALS->constant_marker_update = g_old->constant_marker_update;
							GLOBALS->do_zoom_center = g_old->do_zoom_center;
							GLOBALS->use_roundcaps = g_old->use_roundcaps;
							GLOBALS->do_resize_signals = g_old->do_resize_signals;
							GLOBALS->alt_wheel_mode = g_old->alt_wheel_mode;
							GLOBALS->initial_signal_window_width = g_old->initial_signal_window_width;
							GLOBALS->use_full_precision = g_old->use_full_precision;
							GLOBALS->show_base = g_old->show_base;
							GLOBALS->display_grid = g_old->display_grid;
							GLOBALS->highlight_wavewindow = g_old->highlight_wavewindow;
							GLOBALS->fill_waveform = g_old->fill_waveform;
							GLOBALS->use_standard_trace_select = g_old->use_standard_trace_select;
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
							GLOBALS->save_on_exit = g_old->save_on_exit;

							gtk_notebook_set_current_page(GTK_NOTEBOOK(GLOBALS->notebook), GLOBALS->this_context_page);
							}

						return(FALSE);
						}
					}
				}
			}
		break;

	default:
		break;
	}

return(FALSE);
}

void install_focus_cb(GtkWidget *w, intptr_t ptr_offset)
{
gtk_signal_connect (GTK_OBJECT(w), "enter_notify_event", GTK_SIGNAL_FUNC(context_swapper), (void *)ptr_offset);
gtk_signal_connect (GTK_OBJECT(w), "focus_in_event", GTK_SIGNAL_FUNC(context_swapper), (void *)ptr_offset);
}


/*
 * wrapped gtk_signal_connect/gtk_signal_connect_object functions for context watchdog monitoring...
 * note that this false triggers on configure_event as gtk sends events to multiple tabs and not
 * just the visible one!
 */
static gint ctx_swap_watchdog(struct Global **w)
{
struct Global *watch = *w;

if(GLOBALS->gtk_context_bridge_ptr != w)
	{
#ifdef GTKWAVE_SIGNAL_CONNECT_DEBUG
	fprintf(stderr, "GTKWAVE | WARNING: globals change caught by ctx_swap_watchdog()! %p vs %p, session %d vs %d\n",
		(void *)GLOBALS->gtk_context_bridge_ptr,(void *)w, (*GLOBALS->gtk_context_bridge_ptr)->this_context_page, watch->this_context_page);
#endif

	set_GLOBALS(watch);
	}

return(0);
}

static gint ctx_prints(char *s)
{
printf(">>> %s\n", s);
return(0);
}
static gint ctx_printd(int d)
{
printf(">>>\t%d\n", d);
return(0);
}

gulong gtkwave_signal_connect_x(GtkObject *object, const gchar *name, GtkSignalFunc func, gpointer data, char *f, intptr_t line)
{
gulong rc;

if(f)
	{
	gtk_signal_connect_object(object, name, (GtkSignalFunc)ctx_prints, (GtkObject *)f);
	gtk_signal_connect_object(object, name, (GtkSignalFunc)ctx_printd, (GtkObject *)line);
	}

gtk_signal_connect_object(object, name, (GtkSignalFunc)ctx_swap_watchdog, (GtkObject *)GLOBALS->gtk_context_bridge_ptr);
rc = gtk_signal_connect(object, name, func, data);

return(rc);
}


gulong gtkwave_signal_connect_object_x(GtkObject *object, const gchar *name, GtkSignalFunc func, gpointer data, char *f, intptr_t line)
{
gulong rc;

if(f)
	{
	gtk_signal_connect_object(object, name, (GtkSignalFunc)ctx_prints, (GtkObject *)f);
	gtk_signal_connect_object(object, name, (GtkSignalFunc)ctx_printd, (GtkObject *)line);
	}

gtk_signal_connect_object(object, name, (GtkSignalFunc)ctx_swap_watchdog, (GtkObject *)GLOBALS->gtk_context_bridge_ptr);
rc = gtk_signal_connect_object(object, name, func, data);

return(rc);
}


void set_GLOBALS_x(struct Global *g, const char *file, int line)
{
char sstr[32];

if(line)
	{
	printf("Globals old %p -> new %p (%s: %d)\n", (void *)GLOBALS, (void *)g, file, line);
	}

if(GLOBALS != g)
	{
	/* fix problem where ungrab doesn't occur if button pressed + simultaneous context swap occurs */
 	if(GLOBALS && GLOBALS->in_button_press_wavewindow_c_1) { gdk_pointer_ungrab(GDK_CURRENT_TIME); }

	GLOBALS = g;
	sprintf(sstr, "%d", GLOBALS->this_context_page);
	gtkwavetcl_setvar(WAVE_TCLCB_CURRENT_ACTIVE_TAB, sstr, WAVE_TCLCB_CURRENT_ACTIVE_TAB_FLAGS);
	}
}


/*
 * for ensuring continuity of pixmap/mask pointers
 */
void clone_icon_pointers_across_contexts(struct Global *a, struct Global *b)
{
 if(!a || !b) return;

 a->hiericon_module_pixmap = b->hiericon_module_pixmap;
 a->hiericon_module_mask = b->hiericon_module_mask;
 a->hiericon_task_pixmap = b->hiericon_task_pixmap;
 a->hiericon_task_mask = b->hiericon_task_mask;
 a->hiericon_function_pixmap = b->hiericon_function_pixmap;
 a->hiericon_function_mask = b->hiericon_function_mask;
 a->hiericon_begin_pixmap = b->hiericon_begin_pixmap;
 a->hiericon_begin_mask = b->hiericon_begin_mask;
 a->hiericon_fork_pixmap = b->hiericon_fork_pixmap;
 a->hiericon_fork_mask = b->hiericon_fork_mask;
 a->hiericon_interface_pixmap = b->hiericon_interface_pixmap;
 a->hiericon_interface_mask = b->hiericon_interface_mask;
 a->hiericon_svpackage_pixmap = b->hiericon_svpackage_pixmap;
 a->hiericon_svpackage_mask = b->hiericon_svpackage_mask;
 a->hiericon_program_pixmap = b->hiericon_program_pixmap;
 a->hiericon_program_mask = b->hiericon_program_mask;
 a->hiericon_class_pixmap = b->hiericon_class_pixmap;
 a->hiericon_class_mask = b->hiericon_class_mask;
 a->hiericon_record_pixmap = b->hiericon_record_pixmap;
 a->hiericon_record_mask = b->hiericon_record_mask;
 a->hiericon_generate_pixmap = b->hiericon_generate_pixmap;
 a->hiericon_generate_mask = b->hiericon_generate_mask;
 a->hiericon_design_pixmap = b->hiericon_design_pixmap;
 a->hiericon_design_mask = b->hiericon_design_mask;
 a->hiericon_block_pixmap = b->hiericon_block_pixmap;
 a->hiericon_block_mask = b->hiericon_block_mask;
 a->hiericon_generateif_pixmap = b->hiericon_generateif_pixmap;
 a->hiericon_generateif_mask = b->hiericon_generateif_mask;
 a->hiericon_generatefor_pixmap = b->hiericon_generatefor_pixmap;
 a->hiericon_generatefor_mask = b->hiericon_generatefor_mask;
 a->hiericon_instance_pixmap = b->hiericon_instance_pixmap;
 a->hiericon_instance_mask = b->hiericon_instance_mask;
 a->hiericon_package_pixmap = b->hiericon_package_pixmap;
 a->hiericon_package_mask = b->hiericon_package_mask;

 a->hiericon_signal_pixmap = b->hiericon_signal_pixmap;
 a->hiericon_signal_mask = b->hiericon_signal_mask;
 a->hiericon_portin_pixmap = b->hiericon_portin_pixmap;
 a->hiericon_portin_mask = b->hiericon_portin_mask;
 a->hiericon_portout_pixmap = b->hiericon_portout_pixmap;
 a->hiericon_portout_mask = b->hiericon_portout_mask;
 a->hiericon_portinout_pixmap = b->hiericon_portinout_pixmap;
 a->hiericon_portinout_mask = b->hiericon_portinout_mask;
 a->hiericon_buffer_pixmap = b->hiericon_buffer_pixmap;
 a->hiericon_buffer_mask = b->hiericon_buffer_mask;
 a->hiericon_linkage_pixmap = b->hiericon_linkage_pixmap;
 a->hiericon_linkage_mask = b->hiericon_linkage_mask;
}

