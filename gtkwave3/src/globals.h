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

#if defined __MINGW32__ || defined _MSC_VER
#include <windows.h>
#include <io.h>
#endif

#include "ae2.h"
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
#include "globals.h"
#include "gnu_regex.h"
#include "gtk12compat.h"
#include "lx2.h"
#include "lxt.h"
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
#include "vzt.h"
#include "version.h"
#include "wavealloca.h"
#include "jrb.h"
#include "extload.h"

#ifdef _WAVE_HAVE_JUDY
#include <Judy.h>
#endif


struct Global{

/*
 * ae2.c
 */
#ifdef AET2_IS_PRESENT
#ifdef AET2_ALIASDB_IS_PRESENT
FILE *adb_alias_stream_file;
ADB_DB adb;
unsigned long adb_max_terms;
ADB_TERM *adb_terms;
ADB_TERM **adb_aliases;
unsigned short *adb_num_terms;
unsigned short *adb_idx_first;
unsigned short *adb_idx_last;
unsigned char *adb_alloc_pool_base;
size_t adb_alloc_idx;
#endif
unsigned long ae2_num_facs;
unsigned long ae2_num_aliases;
unsigned long ae2_num_sections;
struct lx2_entry **ae2_lx2_table;
FILE *ae2_f;
AE2_HANDLE *ae2;
AE2_FACREF *ae2_fr;
TimeType ae2_start_limit_cyc;
TimeType ae2_end_limit_cyc;
char *ae2_process_mask;
#endif
TimeType ae2_start_cyc;
TimeType ae2_end_cyc;
TimeType *ae2_time_xlate;
char disable_ae2_alias;


/*
 * analyzer.c
 */
TraceFlagsType default_flags; /* from analyzer.c 5 */
unsigned int default_fpshift;
Times tims; /* from analyzer.c 6 */
Traces traces; /* from analyzer.c 7 */
int hier_max_level; /* from analyzer.c 8 */
int hier_max_level_shadow; /* from analyzer.c */
TimeType timestart_from_savefile;
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
TimeType shift_timebase; /* from bsearch.c 10 */
TimeType shift_timebase_default_for_add; /* from bsearch.c 11 */
TimeType max_compare_time_tc_bsearch_c_1; /* from bsearch.c 12 */
TimeType *max_compare_pos_tc_bsearch_c_1; /* from bsearch.c 13 */
TimeType max_compare_time_bsearch_c_1; /* from bsearch.c 14 */
struct HistEnt *max_compare_pos_bsearch_c_1; /* from bsearch.c 15 */
struct HistEnt **max_compare_index; /* from bsearch.c 16 */
TimeType vmax_compare_time_bsearch_c_1; /* from bsearch.c 17 */
struct VectorEnt *vmax_compare_pos_bsearch_c_1; /* from bsearch.c 18 */
struct VectorEnt **vmax_compare_index; /* from bsearch.c 19 */
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
struct wave_gcchain_t *wave_gcchain;
int color_back; /* from color.c 25  */
int color_baseline; /* from color.c 26 */
int color_grid; /* from color.c 27 */
int color_grid2; /* from color.c */
int color_high; /* from color.c 28 */
int color_low; /* from color.c 29 */
int color_mark; /* from color.c 30 */
int color_mid; /* from color.c 31 */
int color_time; /* from color.c 32 */
int color_timeb; /* from color.c 33 */
int color_trans; /* from color.c 34 */
int color_umark; /* from color.c 35 */
int color_value; /* from color.c 36 */
int color_vbox; /* from color.c 37 */
int color_vtrans; /* from color.c 38 */
int color_x; /* from color.c 39 */
int color_xfill; /* from color.c 40 */
int color_0; /* from color.c 41 */
int color_1; /* from color.c 42 */
int color_ufill; /* from color.c 43 */
int color_u; /* from color.c 44 */
int color_wfill; /* from color.c 45 */
int color_w; /* from color.c 46 */
int color_dashfill; /* from color.c 47 */
int color_dash; /* from color.c 48 */
int color_white; /* from color.c 49 */
int color_black; /* from color.c 50 */
int color_ltgray; /* from color.c 51 */
int color_normal; /* from color.c 52 */
int color_mdgray; /* from color.c 53 */
int color_dkgray; /* from color.c 54 */
int color_dkblue; /* from color.c 55 */
int color_brkred;
int color_ltblue;
int color_gmstrd;
int color_highfill;
int color_1fill;


/*
 * currenttime.c
 */
TimeType global_time_offset;
char is_vcd; /* from currenttime.c 56 */
char partial_vcd; /* from currenttime.c 57 */
char use_maxtime_display; /* from currenttime.c 58 */
char use_frequency_delta; /* from currenttime.c 59 */
GtkWidget *max_or_marker_label_currenttime_c_1; /* from currenttime.c 60 */
GtkWidget *base_or_curtime_label_currenttime_c_1; /* from currenttime.c 61 */
TimeType cached_currenttimeval_currenttime_c_1; /* from currenttime.c 62 */
TimeType currenttime; /* from currenttime.c 63 */
TimeType max_time; /* from currenttime.c 64 */
TimeType min_time; /* from currenttime.c 65 */
char display_grid; /* from currenttime.c 66 */
TimeType time_scale; /* from currenttime.c 67 */
char time_dimension; /* from currenttime.c 68 */
char scale_to_time_dimension; /* from currenttime.c */
GtkWidget *maxtimewid_currenttime_c_1; /* from currenttime.c 70 */
GtkWidget *curtimewid_currenttime_c_1; /* from currenttime.c 71 */
char *maxtext_currenttime_c_1; /* from currenttime.c 72 */
char *curtext_currenttime_c_1; /* from currenttime.c 73 */
TimeType time_trunc_val_currenttime_c_1; /* from currenttime.c 77 */
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
GtkWidget *window_entry_c_1; /* from entry.c 81 */
GtkWidget *entry_entry_c_1; /* from entry.c 82 */
char *entrybox_text; /* from entry.c 83 */
void (*cleanup_entry_c_1)(void); /* from entry.c 84 */
int entry_raise_timer;


/* extload.c */
unsigned int extload_ffr_import_count; /* from extload.c */
void *extload_ffr_ctx; /* from extload.c */
FILE *extload; /* from extload.c */
unsigned int *extload_idcodes; /* from extload.c */
int *extload_inv_idcodes; /* from extload.c */
#if !defined __MINGW32__ && !defined _MSC_VER
time_t extload_lastmod; /* from extload.c */
char extload_already_errored; /* from extload.c */
#endif
char **extload_namecache;
int *extload_namecache_max;
int *extload_namecache_lens;
int *extload_namecache_patched;
struct symbol *extload_sym_block;
struct Node *extload_node_block;
void *extload_xc;
struct symbol *extload_prevsymroot;
struct symbol *extload_prevsym;
struct tree **extload_npar;
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
TimeType fetchwindow; /* from fetchbuttons.c 85 */


/*
 * fgetdynamic.c
 */
int fgetmalloc_len; /* from fgetdynamic.c 86 */


/*
 * file.c
 */
#if GTK_CHECK_VERSION(2,4,0)
GtkWidget *pFileChoose;
char *pFileChooseFilterName;
GPatternSpec *pPatternSpec;
#endif
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
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN) && GTK_CHECK_VERSION(2,8,0)
PangoRenderer *fonts_renderer;
GdkGC *fonts_gc;
GdkScreen *fonts_screen;
PangoContext *fonts_context;
PangoLayout *fonts_layout;
#endif
char use_pango_fonts;


/*
 * fst.c
 */
void *fst_fst_c_1;
const char *fst_scope_name;
int fst_scope_name_len;
TimeType first_cycle_fst_c_3;
TimeType last_cycle_fst_c_3;
TimeType total_cycles_fst_c_3;
struct lx2_entry *fst_table_fst_c_1;
struct fac *mvlfacs_fst_c_3;
fstHandle *mvlfacs_fst_alias;
fstHandle *mvlfacs_fst_rvs_alias;
fstHandle fst_maxhandle;
int busycnt_fst_c_2;
double *double_curr_fst;
double *double_fini_fst;
char nonimplicit_direction_encountered;
char supplemental_datatypes_encountered;
char supplemental_vartypes_encountered;
char is_vhdl_component_format;
JRB subvar_jrb;
unsigned int subvar_jrb_count;
char **subvar_pnt;
unsigned char fst_filetype;
unsigned subvar_jrb_count_locked : 1;
uint32_t stem_file_idx;
uint32_t stem_line_number;
char **stem_path_string_table;
struct stem_struct_t *stem_struct_base;
struct stem_struct_t *istem_struct_base;
uint32_t stem_path_string_table_siz;
uint32_t stem_path_string_table_alloc;
uint32_t stem_struct_base_siz;
uint32_t stem_struct_base_siz_alloc;
uint32_t istem_struct_base_siz;
uint32_t istem_struct_base_siz_alloc;
unsigned stem_valid : 1;
unsigned istem_valid : 1;
char *fst_synclock_str;
JRB synclock_jrb;
#ifdef _WAVE_HAVE_JUDY
Pvoid_t *xl_enum_filter;
#else
struct xl_tree_node **xl_enum_filter;
#endif
int num_xl_enum_filter;
fstEnumHandle queued_xl_enum_filter;
JRB enum_nptrs_jrb;


/*
 * ghw.c
 */
struct Node **nxp_ghw_c_1; /* from ghw.c 95 */
int sym_which_ghw_c_1; /* from ghw.c 98 */
struct ghw_tree_node *gwt_ghw_c_1; /* from ghw.c 99 */
struct ghw_tree_node *gwt_corr_ghw_c_1; /* from ghw.c 100 */
int xlat_1164_ghw_c_1; /* from ghw.c 101 */
char is_ghw; /* from ghw.c 102 */
char *asbuf; /* from ghw.c 103 */
int nbr_sig_ref_ghw_c_1; /* from ghw.c 104 */
int num_glitches_ghw_c_1; /* from ghw.c 105 */
int num_glitch_regions_ghw_c_1; /* from ghw.c 106 */
char *fac_name_ghw_c_1; /* from ghw.c 108 */
int fac_name_len_ghw_c_1; /* from ghw.c 109 */
int fac_name_max_ghw_c_1; /* from ghw.c 110 */
int last_fac_ghw_c_1; /* from ghw.c 111 */
int warned_ghw_c_1; /* from ghw.c 112 */


/*
 * globals.c
 */
struct Global ***dead_context;		/* for deallocating tabbed contexts later (when no race conditions exist) */
struct Global **gtk_context_bridge_ptr; /* from globals.c, migrates to reloaded contexts to link buttons to ctx */


/*
 * help.c
 */
int helpbox_is_active; /* from help.c 114 */
GtkWidget *text_help_c_1; /* from help.c 115 */
GtkWidget *vscrollbar_help_c_1; /* from help.c 116 */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
GtkTextIter iter_help_c_1; /* from help.c 117 */
GtkTextTag *bold_tag_help_c_1; /* from help.c 118 */
#endif
GtkWidget *window_help_c_2; /* from help.c 119 */


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
 * hiersearch.c
 */
char hier_grouping; /* from hiersearch.c 120 */
GtkWidget *window_hiersearch_c_3; /* from hiersearch.c 121 */
GtkWidget *entry_main_hiersearch_c_1; /* from hiersearch.c 122 */
GtkWidget *clist_hiersearch_c_1; /* from hiersearch.c 123 */
char bundle_direction_hiersearch_c_1; /* from hiersearch.c 124 */
void (*cleanup_hiersearch_c_3)(void); /* from hiersearch.c 125 */
int num_rows_hiersearch_c_1; /* from hiersearch.c 126 */
int selected_rows_hiersearch_c_1; /* from hiersearch.c 127 */
GtkWidget *window1_hiersearch_c_1; /* from hiersearch.c 128 */
GtkWidget *entry_hiersearch_c_2; /* from hiersearch.c 129 */
char *entrybox_text_local_hiersearch_c_1; /* from hiersearch.c 130 */
void (*cleanup_e_hiersearch_c_1)(void); /* from hiersearch.c 131 */
struct tree *h_selectedtree_hiersearch_c_1; /* from hiersearch.c 132 */
struct tree *current_tree_hiersearch_c_1; /* from hiersearch.c 133 */
struct treechain *treechain_hiersearch_c_1; /* from hiersearch.c 134 */
int is_active_hiersearch_c_1; /* from hiersearch.c 135 */


/*
 * logfile.c
 */
void **logfiles;
char *fontname_logfile; /* from logfile.c 137 */
GdkFont *font_logfile_c_1; /* from logfile.c 138 */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
GtkTextIter iter_logfile_c_2; /* from logfile.c 139 */
GtkTextTag *bold_tag_logfile_c_2; /* from logfile.c 140 */
GtkTextTag *mono_tag_logfile_c_1; /* from logfile.c 141 */
GtkTextTag *size_tag_logfile_c_1; /* from logfile.c 142 */
#endif


/*
 * lx2.c
 */
unsigned char is_lx2; /* from lx2.c 143 */
struct lxt2_rd_trace *lx2_lx2_c_1; /* from lx2.c 144 */
TimeType first_cycle_lx2_c_1; /* from lx2.c 145 */
TimeType last_cycle_lx2_c_1; /* from lx2.c 146 */
TimeType total_cycles_lx2_c_1; /* from lx2.c 147 */
struct lx2_entry *lx2_table_lx2_c_1; /* from lx2.c 148 */
struct fac *mvlfacs_lx2_c_1; /* from lx2.c 149 */
int busycnt_lx2_c_1; /* from lx2.c 150 */


/*
 * lxt.c
 */
char *mm_lxt_mmap_addr;
size_t mm_lxt_mmap_len;
#if defined __MINGW32__ || defined _MSC_VER
HANDLE hIn, hInMap;
char *win_fname;
#endif
int fpos_lxt_c_1; /* from lxt.c 151 */
char is_lxt; /* from lxt.c 152 */
char lxt_clock_compress_to_z; /* from lxt.c 153 */
void *mm_lxt_c_1; /* from lxt.c 154 */
void *mmcache_lxt_c_1; /* from lxt.c 155 */
int version_lxt_c_1; /* from lxt.c 156 */
struct fac *mvlfacs_lxt_c_2; /* from lxt.c 157 */
TimeType first_cycle_lxt_c_2; /* from lxt.c 158 */
TimeType last_cycle_lxt_c_2; /* from lxt.c 159 */
TimeType total_cycles_lxt_c_2; /* from lxt.c 160 */
int maxchange_lxt_c_1; /* from lxt.c 161 */
int maxindex_lxt_c_1; /* from lxt.c 162 */
int f_len_lxt_c_1; /* from lxt.c 163 */
int *positional_information_lxt_c_1; /* from lxt.c 164 */
TimeType *time_information; /* from lxt.c 165 */
int change_field_offset_lxt_c_1; /* from lxt.c 166 */
int facname_offset_lxt_c_1; /* from lxt.c 167 */
int facgeometry_offset_lxt_c_1; /* from lxt.c 168 */
int time_table_offset_lxt_c_1; /* from lxt.c 169 */
int time_table_offset64_lxt_c_1; /* from lxt.c 170 */
int sync_table_offset_lxt_c_1; /* from lxt.c 171 */
int initial_value_offset_lxt_c_1; /* from lxt.c 172 */
int timescale_offset_lxt_c_1; /* from lxt.c 173 */
int double_test_offset_lxt_c_1; /* from lxt.c 174 */
int zdictionary_offset_lxt_c_1; /* from lxt.c 175 */
unsigned int zfacname_predec_size_lxt_c_1; /* from lxt.c 176 */
unsigned int zfacname_size_lxt_c_1; /* from lxt.c 177 */
unsigned int zfacgeometry_size_lxt_c_1; /* from lxt.c 178 */
unsigned int zsync_table_size_lxt_c_1; /* from lxt.c 179 */
unsigned int ztime_table_size_lxt_c_1; /* from lxt.c 180 */
unsigned int zchg_predec_size_lxt_c_1; /* from lxt.c 181 */
unsigned int zchg_size_lxt_c_1; /* from lxt.c 182 */
unsigned int zdictionary_predec_size_lxt_c_1; /* from lxt.c 183 */
unsigned char initial_value_lxt_c_1; /* from lxt.c 184 */
unsigned int dict_num_entries_lxt_c_1; /* from lxt.c 185 */
unsigned int dict_string_mem_required_lxt_c_1; /* from lxt.c 186 */
int dict_16_offset_lxt_c_1; /* from lxt.c 187 */
int dict_24_offset_lxt_c_1; /* from lxt.c 188 */
int dict_32_offset_lxt_c_1; /* from lxt.c 189 */
unsigned int dict_width_lxt_c_1; /* from lxt.c 190 */
char **dict_string_mem_array_lxt_c_1; /* from lxt.c 191 */
int exclude_offset_lxt_c_1; /* from lxt.c 192 */
int lxt_timezero_offset;
char *lt_buf_lxt_c_1; /* from lxt.c 193 */
int lt_len_lxt_c_1; /* from lxt.c 194 */
int fd_lxt_c_1; /* from lxt.c 195 */
unsigned char double_mask_lxt_c_1[8]; /* from lxt.c 196 */
char double_is_native_lxt_c_1; /* from lxt.c 197 */
int max_compare_time_tc_lxt_c_2; /* from lxt.c 199 */
int max_compare_pos_tc_lxt_c_2; /* from lxt.c 200 */
struct Node **resolve_lxt_alias_to;
unsigned int *lastchange;


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
char use_scrollbar_only; /* from main.c 237 */
char force_toolbars; /* from main.c 238 */
int hide_sst; /* from main.c 239 */
int sst_expanded; /* from main.c 240 */
#ifdef WAVE_USE_GTK2
GdkNativeWindow socket_xid; /* from main.c 241 */
#endif
int disable_menus; /* from main.c 242 */
char *ftext_main_main_c_1; /* from main.c 243 */
char use_toolbutton_interface; /* from main.c */


/*
 * markerbox.c
 */
GtkWidget *window_markerbox_c_4; /* from markerbox.c 248 */
GtkWidget *entries_markerbox_c_1[WAVE_NUM_NAMED_MARKERS]; /* from markerbox.c 249 */
void (*cleanup_markerbox_c_4)(void); /* from markerbox.c 250 */
int dirty_markerbox_c_1; /* from markerbox.c 251 */
TimeType shadow_markers_markerbox_c_1[WAVE_NUM_NAMED_MARKERS]; /* from markerbox.c 252 */
char *marker_names[WAVE_NUM_NAMED_MARKERS]; /* from markerbox.c */
char *shadow_marker_names[WAVE_NUM_NAMED_MARKERS]; /* from markerbox.c */

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
#ifndef WAVE_USE_MLIST_T
GtkItemFactory *item_factory_menu_c_1; /* from menu.c 258 */
#endif
char *regexp_string_menu_c_1; /* from menu.c 259 */
struct TraceEnt *trace_to_alias_menu_c_1; /* from menu.c 260 */
struct TraceEnt *showchangeall_menu_c_1; /* from menu.c 261 */
char *filesel_newviewer_menu_c_1; /* from menu.c 262 */
char *filesel_logfile_menu_c_1; /* from menu.c 263 */
char *filesel_scriptfile_menu; /* from menu.c */
char *filesel_writesave; /* from menu.c 264 */
char *filesel_imagegrab; /* from menu.c */
char save_success_menu_c_1; /* from menu.c 265 */
char *filesel_vcd_writesave; /* from menu.c 266 */
char *filesel_lxt_writesave; /* from menu.c 267 */
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
GtkWidget *mo_area_mouseover_c_1; /* from mouseover.c 273 */
GdkDrawable *mo_pixmap_mouseover_c_1; /* from mouseover.c 274 */
GdkGC *mo_dk_gray_mouseover_c_1; /* from mouseover.c 275 */
GdkGC *mo_black_mouseover_c_1; /* from mouseover.c 276 */
int mo_width_mouseover_c_1; /* from mouseover.c 277 */
int mo_height_mouseover_c_1; /* from mouseover.c 278 */


/*
 * pagebuttons.c
 */
double page_divisor; /* from pagebuttons.c 279 */


/*
 * pixmaps.c
 */
GdkDrawable *redo_pixmap; /* from pixmaps.c */
GdkDrawable *redo_mask; /* from pixmaps.c */
GdkDrawable *larrow_pixmap; /* from pixmaps.c 281 */
GdkDrawable *larrow_mask; /* from pixmaps.c 282 */
GdkDrawable *rarrow_pixmap; /* from pixmaps.c 284 */
GdkDrawable *rarrow_mask; /* from pixmaps.c 285 */
GdkDrawable *zoomin_pixmap; /* from pixmaps.c 287 */
GdkDrawable *zoomin_mask; /* from pixmaps.c 288 */
GdkDrawable *zoomout_pixmap; /* from pixmaps.c 290 */
GdkDrawable *zoomout_mask; /* from pixmaps.c 291 */
GdkDrawable *zoomfit_pixmap; /* from pixmaps.c 293 */
GdkDrawable *zoomfit_mask; /* from pixmaps.c 294 */
GdkDrawable *zoomundo_pixmap; /* from pixmaps.c 296 */
GdkDrawable *zoomundo_mask; /* from pixmaps.c 297 */
GdkDrawable *zoom_larrow_pixmap; /* from pixmaps.c 299 */
GdkDrawable *zoom_larrow_mask; /* from pixmaps.c 300 */
GdkDrawable *zoom_rarrow_pixmap; /* from pixmaps.c 302 */
GdkDrawable *zoom_rarrow_mask; /* from pixmaps.c 303 */
GdkDrawable *prev_page_pixmap; /* from pixmaps.c 305 */
GdkDrawable *prev_page_mask; /* from pixmaps.c 306 */
GdkDrawable *next_page_pixmap; /* from pixmaps.c 308 */
GdkDrawable *next_page_mask; /* from pixmaps.c 309 */
GdkDrawable *wave_info_pixmap; /* from pixmaps.c 311 */
GdkDrawable *wave_info_mask; /* from pixmaps.c 312 */
GdkDrawable *wave_alert_pixmap; /* from pixmaps.c 314 */
GdkDrawable *wave_alert_mask; /* from pixmaps.c 315 */
GdkDrawable *hiericon_module_pixmap; /* from pixmaps.c */
GdkDrawable *hiericon_module_mask; /* from pixmaps.c */
GdkDrawable *hiericon_task_pixmap; /* from pixmaps.c */
GdkDrawable *hiericon_task_mask; /* from pixmaps.c */
GdkDrawable *hiericon_function_pixmap; /* from pixmaps.c */
GdkDrawable *hiericon_function_mask; /* from pixmaps.c */
GdkDrawable *hiericon_begin_pixmap; /* from pixmaps.c */
GdkDrawable *hiericon_begin_mask; /* from pixmaps.c */
GdkDrawable *hiericon_fork_pixmap; /* from pixmaps.c */
GdkDrawable *hiericon_fork_mask; /* from pixmaps.c */
GdkDrawable *hiericon_interface_pixmap;
GdkDrawable *hiericon_interface_mask;
GdkDrawable *hiericon_svpackage_pixmap;
GdkDrawable *hiericon_svpackage_mask;
GdkDrawable *hiericon_program_pixmap;
GdkDrawable *hiericon_program_mask;
GdkDrawable *hiericon_class_pixmap;
GdkDrawable *hiericon_class_mask;
GdkDrawable *hiericon_record_pixmap;
GdkDrawable *hiericon_record_mask;
GdkDrawable *hiericon_generate_pixmap;
GdkDrawable *hiericon_generate_mask;
GdkDrawable *hiericon_design_pixmap;
GdkDrawable *hiericon_design_mask;
GdkDrawable *hiericon_block_pixmap;
GdkDrawable *hiericon_block_mask;
GdkDrawable *hiericon_generateif_pixmap;
GdkDrawable *hiericon_generateif_mask;
GdkDrawable *hiericon_generatefor_pixmap;
GdkDrawable *hiericon_generatefor_mask;
GdkDrawable *hiericon_instance_pixmap;
GdkDrawable *hiericon_instance_mask;
GdkDrawable *hiericon_package_pixmap;
GdkDrawable *hiericon_package_mask;
GdkDrawable *hiericon_signal_pixmap;
GdkDrawable *hiericon_signal_mask;
GdkDrawable *hiericon_portin_pixmap;
GdkDrawable *hiericon_portin_mask;
GdkDrawable *hiericon_portout_pixmap;
GdkDrawable *hiericon_portout_mask;
GdkDrawable *hiericon_portinout_pixmap;
GdkDrawable *hiericon_portinout_mask;
GdkDrawable *hiericon_buffer_pixmap;
GdkDrawable *hiericon_buffer_mask;
GdkDrawable *hiericon_linkage_pixmap;
GdkDrawable *hiericon_linkage_mask;


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
GtkWidget *clist_ptranslate_c_2; /* from ptranslate.c 334 */


/*
 * rc.c
 */
int rc_line_no; /* from rc.c 336 */
int possibly_use_rc_defaults; /* from rc.c 337 */
char *editor_name; /* from rc.c */


/*
 * regex.c
 */
struct re_pattern_buffer *preg_regex_c_1; /* from regex.c 339 */
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
GtkWidget *clist_search_c_3; /* from search.c 375 */
char *searchbox_text_search_c_1; /* from search.c 377 */
char bundle_direction_search_c_2; /* from search.c 378 */
void (*cleanup_search_c_5)(void); /* from search.c 379 */
int num_rows_search_c_2; /* from search.c 380 */
int selected_rows_search_c_2; /* from search.c 381 */


/*
 * showchange.c
 */
GtkWidget *button1_showchange_c_1; /* from showchange.c 382 */
GtkWidget *button2_showchange_c_1; /* from showchange.c 383 */
GtkWidget *button3_showchange_c_1; /* from showchange.c 384 */
GtkWidget *button4_showchange_c_1; /* from showchange.c 385 */
GtkWidget *button5_showchange_c_1; /* from showchange.c 386 */
GtkWidget *button6_showchange_c_1; /* from showchange.c 387 */
GtkWidget *button7_showchange_c_1; /* from showchange.c 387 */
GtkWidget *button8_showchange_c_1; /* from showchange.c 387 */
GtkWidget *button9_showchange_c_1; /* from showchange.c 387 */
GtkWidget *toggle1_showchange_c_1; /* from showchange.c 388 */
GtkWidget *toggle2_showchange_c_1; /* from showchange.c 389 */
GtkWidget *toggle3_showchange_c_1; /* from showchange.c 390 */
GtkWidget *toggle4_showchange_c_1; /* from showchange.c 391 */
GtkWidget *toggle5_showchange_c_1; /* from showchange.c 391 */
GtkWidget *window_showchange_c_8; /* from showchange.c 392 */
void (*cleanup_showchange_c_6)(void); /* from showchange.c 393 */
struct TraceEnt *tcache_showchange_c_1; /* from showchange.c 394 */
TraceFlagsType flags_showchange_c_1; /* from showchange.c 395 */


/*
 * signalwindow.c
 */
GtkWidget *signalarea; /* from signalwindow.c 396 */
struct font_engine_font_t *signalfont; /* from signalwindow.c 397 */
GdkDrawable *signalpixmap; /* from signalwindow.c 398 */
#ifdef WAVE_ALLOW_QUARTZ_FLUSH_WORKAROUND
char force_hide_show;
#endif
int max_signal_name_pixel_width; /* from signalwindow.c 399 */
int signal_pixmap_width; /* from signalwindow.c 400 */
int signal_fill_width; /* from signalwindow.c 401 */
int old_signal_fill_width; /* from signalwindow.c 402 */
int old_signal_fill_height; /* from signalwindow.c 403 */
int right_align_active; /* from signalwindow.c */
int fontheight; /* from signalwindow.c 404 */
char dnd_state; /* from signalwindow.c 405 */
unsigned int dnd_cursor_timer; /* from signalwindow.c */
GtkWidget *hscroll_signalwindow_c_1; /* from signalwindow.c 406 */
GtkObject *signal_hslider; /* from signalwindow.c 407 */
unsigned int cachedhiflag_signalwindow_c_1; /* from signalwindow.c 408 */
int cachedwhich_signalwindow_c_1; /* from signalwindow.c 409 */
struct TraceEnt *cachedtrace; /* from signalwindow.c 410 */
struct TraceEnt *shift_click_trace; /* from signalwindow.c 411 */
int trtarget_signalwindow_c_1; /* from signalwindow.c 412 */
Trptr starting_unshifted_trace; /* from signalwindow.c */
unsigned char standard_trace_dnd_degate; /* from signalwindow.c */
unsigned char use_standard_trace_select; /* from signalwindow.c */
unsigned char use_standard_clicking; /* from signalwindow.c */
unsigned char std_collapse_pressed; /* from signalwindow.c */
unsigned char std_dnd_tgt_on_signalarea; /* from signalwindow.c */
unsigned char std_dnd_tgt_on_wavearea; /* from signalwindow.c */
unsigned char signalarea_has_focus; /* from signalwindow.c */
GtkWidget *signalarea_event_box; /* from signalwindow.c */
gint keypress_handler_id; /* from signalwindow.c */
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
GdkDrawable *wave_splash_pixmap; /* from splash.c 417 */
GdkDrawable *wave_splash_mask; /* from splash.c 418 */
GtkWidget *splash_splash_c_1; /* from splash.c 419 */
GtkWidget *darea_splash_c_1; /* from splash.c 420 */
GTimer *gt_splash_c_1; /* from splash.c 421 */
int timeout_tag; /* from splash.c 422 */
int load_complete_splash_c_1; /* from splash.c 423 */
int cnt_splash_c_1; /* from splash.c 424 */
int prev_bar_x_splash_c_1; /* from splash.c 425 */


/*
 * status.c
 */
GtkWidget *text_status_c_2; /* from status.c 426 */
GtkWidget *vscrollbar_status_c_2; /* from status.c 427 */
#if defined(WAVE_USE_GTK2) && !defined(GTK_ENABLE_BROKEN)
GtkTextIter iter_status_c_3; /* from status.c 428 */
GtkTextTag *bold_tag_status_c_3; /* from status.c 429 */
#endif


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
struct symchain *firstnode; /* from symbol.c 459 */
struct symchain *curnode; /* from symbol.c 460 */
int hashcache; /* from symbol.c 461 */


/*
 * tcl_commands.c
 */
char *previous_braced_tcl_string;


/*
 * tcl_helper.c
 */
char in_tcl_callback;


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
GtkWidget *clist_translate_c_4; /* from translate.c 472 */


/*
 * tree.c
 */
#ifdef _WAVE_HAVE_JUDY
Pvoid_t sym_tree;
Pvoid_t sym_tree_addresses;
#endif
struct tree *treeroot; /* from tree.c 473 */
struct tree *mod_tree_parent; /* from tree.c */
char *module_tree_c_1; /* from tree.c 474 */
int module_len_tree_c_1; /* from tree.c 475 */
struct tree *terminals_tchain_tree_c_1; /* from tree.c 476 */
char hier_delimeter; /* from tree.c 477 */
char hier_was_explicitly_set; /* from tree.c 478 */
char alt_hier_delimeter; /* from tree.c 479 */
int fast_tree_sort; /* from tree.c 480 */
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
 * treesearch_gtk1.c
 */
GtkWidget *window1_treesearch_gtk1_c;  /* manual adds by ajb... */
GtkWidget *entry_a_treesearch_gtk1_c;
char *entrybox_text_local_treesearch_gtk1_c;
void (*cleanup_e_treesearch_gtk1_c)(void);
struct tree *selectedtree_treesearch_gtk1_c;
int is_active_treesearch_gtk1_c;
GtkWidget *window_treesearch_gtk1_c;
GtkWidget *tree_treesearch_gtk1_c;
char bundle_direction_treesearch_gtk1_c;
void (*cleanup_treesearch_gtk1_c)(void); /* ...end of manual adds */


/*
 * treesearch_gtk2.c
 */
#ifdef MAC_INTEGRATION
char *dnd_helper_quartz;
#endif
struct string_chain_t *treeopen_chain_head; /* from bitvec.c */
struct string_chain_t *treeopen_chain_curr; /* from bitvec.c */
char tree_dnd_begin; /* from treesearch_gtk2.c */
char tree_dnd_requested; /* from treesearch_gtk2.c */
char do_dynamic_treefilter; /* from treesearch_gtk2.c */
GtkWidget *treesearch_gtk2_window_vbox; /* from treesearch_gtk2.c */
char *selected_hierarchy_name; /* from treesearch_gtk2.c */
char *selected_sig_name; /* from treesearch_gtk2.c */
GtkWidget *gtk2_tree_frame; /* from treesearch_gtk2.c */
GtkWidget *filter_entry; /* from treesearch_gtk2.c */
GtkCTreeNode *any_tree_node; /* from treesearch_gtk2.c */
struct xl_tree_node *open_tree_nodes; /* from treesearch_gtk2.c */
char autoname_bundles; /* from treesearch_gtk2.c 483 */
GtkWidget *window1_treesearch_gtk2_c_3; /* from treesearch_gtk2.c 484 */
GtkWidget *entry_a_treesearch_gtk2_c_2; /* from treesearch_gtk2.c 485 */
char *entrybox_text_local_treesearch_gtk2_c_3; /* from treesearch_gtk2.c 486 */
void (*cleanup_e_treesearch_gtk2_c_3)(void); /* from treesearch_gtk2.c 487 */
struct tree *sig_root_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 488 */
struct tree *sst_sig_root_treesearch_gtk2_c_1; /* from treesearch_gtk2.c */
char *filter_str_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 489 */
int filter_typ_treesearch_gtk2_c_1;
int filter_typ_polarity_treesearch_gtk2_c_1;
int filter_matlen_treesearch_gtk2_c_1;
unsigned char filter_noregex_treesearch_gtk2_c_1;
#if defined(WAVE_USE_GTK2)
GtkListStore *sig_store_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 490 */
GtkTreeSelection *sig_selection_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 491 */
#endif
int is_active_treesearch_gtk2_c_6; /* from treesearch_gtk2.c 492 */
GtkCTree *ctree_main; /* from treesearch_gtk2.c 493 */
struct autocoalesce_free_list *afl_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 494 */
GtkWidget *window_treesearch_gtk2_c_12; /* from treesearch_gtk2.c 495 */
GtkWidget *tree_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 496 */
void (*cleanup_treesearch_gtk2_c_8)(void); /* from treesearch_gtk2.c 498 */
int pre_import_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 499 */
Traces tcache_treesearch_gtk2_c_2; /* from treesearch_gtk2.c 500 */
unsigned char dnd_tgt_on_signalarea_treesearch_gtk2_c_1; /* from treesearch_gtk2.c 501 */
unsigned char dnd_tgt_on_wavearea_treesearch_gtk2_c_1; /* from treesearch_gtk2.c */
GtkWidget *dnd_sigview; /* from treesearch_gtk2.c */
GtkPaned *sst_vpaned; /* from treesearch_gtk2.c */
int fetchlow;
int fetchhigh;
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
GtkWidget *clist_ttranslate_c_2;
char *ttranslate_args;


/*
 * vcd.c
 */
unsigned char do_hier_compress; /* from vcd.c */
char *prev_hier_uncompressed_name; /* from vcd.c */
jmp_buf *vcd_jmp_buf; /* from vcd.c */
int vcd_warning_filesize; /* from vcd.c 502 */
char autocoalesce; /* from vcd.c 503 */
char autocoalesce_reversal; /* from vcd.c 504 */
int vcd_explicit_zero_subscripts; /* from vcd.c 505 */
char convert_to_reals; /* from vcd.c 506 */
char atomic_vectors; /* from vcd.c 507 */
char make_vcd_save_file; /* from vcd.c 508 */
char vcd_preserve_glitches; /* from vcd.c 509 */
char vcd_preserve_glitches_real;
FILE *vcd_save_handle; /* from vcd.c 510 */
FILE *vcd_handle_vcd_c_1; /* from vcd.c 511 */
char vcd_is_compressed_vcd_c_1; /* from vcd.c 512 */
off_t vcdbyteno_vcd_c_1; /* from vcd.c 513 */
int error_count_vcd_c_1; /* from vcd.c 514 */
int header_over_vcd_c_1; /* from vcd.c 515 */
int dumping_off_vcd_c_1; /* from vcd.c 516 */
TimeType start_time_vcd_c_1; /* from vcd.c 517 */
TimeType end_time_vcd_c_1; /* from vcd.c 518 */
TimeType current_time_vcd_c_1; /* from vcd.c 519 */
int num_glitches_vcd_c_2; /* from vcd.c 520 */
int num_glitch_regions_vcd_c_2; /* from vcd.c 521 */
char vcd_hier_delimeter[2]; /* from vcd.c 522 */
struct vcdsymbol *pv_vcd_c_1; /* from vcd.c 523 */
struct vcdsymbol *rootv_vcd_c_1; /* from vcd.c 524 */
char *vcdbuf_vcd_c_1; /* from vcd.c 525 */
char *vst_vcd_c_1; /* from vcd.c 526 */
char *vend_vcd_c_1; /* from vcd.c 527 */
int escaped_names_found_vcd_c_1; /* from vcd.c 528 */
struct slist *slistroot; /* from vcd.c 529 */
struct slist *slistcurr; /* from vcd.c 530 */
char *slisthier; /* from vcd.c 531 */
int slisthier_len; /* from vcd.c 532 */
int T_MAX_STR_vcd_c_1; /* from vcd.c 534 */
char *yytext_vcd_c_1; /* from vcd.c 535 */
int yylen_vcd_c_1; /* from vcd.c 536 */
int yylen_cache_vcd_c_1; /* from vcd.c 537 */
struct vcdsymbol *vcdsymroot_vcd_c_1; /* from vcd.c 538 */
struct vcdsymbol *vcdsymcurr_vcd_c_1; /* from vcd.c 539 */
struct vcdsymbol **sorted_vcd_c_1; /* from vcd.c 540 */
struct vcdsymbol **indexed_vcd_c_1; /* from vcd.c 541 */
int numsyms_vcd_c_1; /* from vcd.c 542 */
struct HistEnt *he_curr_vcd_c_1; /* from vcd.c 543 */
struct HistEnt *he_fini_vcd_c_1; /* from vcd.c 544 */
unsigned int vcd_minid_vcd_c_1; /* from vcd.c 546 */
unsigned int vcd_maxid_vcd_c_1; /* from vcd.c 547 */
int err_vcd_c_1; /* from vcd.c 548 */
off_t vcd_fsiz_vcd_c_1; /* from vcd.c 549 */
char *varsplit_vcd_c_1; /* from vcd.c 550 */
char *vsplitcurr_vcd_c_1; /* from vcd.c 551 */
int var_prevch_vcd_c_1; /* from vcd.c 552 */
char vcd_already_backtracked;


/*
 * vcd_partial.c
 */
off_t vcdbyteno_vcd_partial_c_2; /* from vcd_partial.c 555 */
int error_count_vcd_partial_c_2; /* from vcd_partial.c 556 */
int header_over_vcd_partial_c_2; /* from vcd_partial.c 557 */
int dumping_off_vcd_partial_c_2; /* from vcd_partial.c 558 */
TimeType start_time_vcd_partial_c_2; /* from vcd_partial.c 559 */
TimeType end_time_vcd_partial_c_2; /* from vcd_partial.c 560 */
TimeType current_time_vcd_partial_c_2; /* from vcd_partial.c 561 */
int num_glitches_vcd_partial_c_3; /* from vcd_partial.c 562 */
int num_glitch_regions_vcd_partial_c_3; /* from vcd_partial.c 563 */
struct vcdsymbol *pv_vcd_partial_c_2; /* from vcd_partial.c 564 */
struct vcdsymbol *rootv_vcd_partial_c_2; /* from vcd_partial.c 565 */
char *vcdbuf_vcd_partial_c_2; /* from vcd_partial.c 566 */
char *vst_vcd_partial_c_2; /* from vcd_partial.c 567 */
char *vend_vcd_partial_c_2; /* from vcd_partial.c 568 */
char *consume_ptr_vcd_partial_c_1; /* from vcd_partial.c 569 */
char *buf_vcd_partial_c_2; /* from vcd_partial.c 570 */
int consume_countdown_vcd_partial_c_1; /* from vcd_partial.c 571 */
int T_MAX_STR_vcd_partial_c_2; /* from vcd_partial.c 573 */
char *yytext_vcd_partial_c_2; /* from vcd_partial.c 574 */
int yylen_vcd_partial_c_2; /* from vcd_partial.c 575 */
int yylen_cache_vcd_partial_c_2; /* from vcd_partial.c 576 */
struct vcdsymbol *vcdsymroot_vcd_partial_c_2; /* from vcd_partial.c 577 */
struct vcdsymbol *vcdsymcurr_vcd_partial_c_2; /* from vcd_partial.c 578 */
struct vcdsymbol **sorted_vcd_partial_c_2; /* from vcd_partial.c 579 */
struct vcdsymbol **indexed_vcd_partial_c_2; /* from vcd_partial.c 580 */
int numsyms_vcd_partial_c_2; /* from vcd_partial.c 582 */
unsigned int vcd_minid_vcd_partial_c_2; /* from vcd_partial.c 584 */
unsigned int vcd_maxid_vcd_partial_c_2; /* from vcd_partial.c 585 */
int err_vcd_partial_c_2; /* from vcd_partial.c 586 */
char *varsplit_vcd_partial_c_2; /* from vcd_partial.c 587 */
char *vsplitcurr_vcd_partial_c_2; /* from vcd_partial.c 588 */
int var_prevch_vcd_partial_c_2; /* from vcd_partial.c 589 */
int timeset_vcd_partial_c_1; /* from vcd_partial.c 592 */


/*
 * vcd_recoder.c
 */
struct vlist_t *time_vlist_vcd_recoder_c_1; /* from vcd_recoder.c 593 */
struct vlist_t *time_vlist_vcd_recoder_write; /* from vcd_recoder.c */
char *fastload_depacked; /* from vcd_recoder.c */
char *fastload_current; /* from vcd_recoder.c */
unsigned int time_vlist_count_vcd_recoder_c_1; /* from vcd_recoder.c 594 */
FILE *vcd_handle_vcd_recoder_c_2; /* from vcd_recoder.c 595 */
char vcd_is_compressed_vcd_recoder_c_2; /* from vcd_recoder.c 596 */
char use_fastload;
off_t vcdbyteno_vcd_recoder_c_3; /* from vcd_recoder.c 597 */
int error_count_vcd_recoder_c_3; /* from vcd_recoder.c 598 */
int header_over_vcd_recoder_c_3; /* from vcd_recoder.c 599 */
int dumping_off_vcd_recoder_c_3; /* from vcd_recoder.c 600 */
TimeType start_time_vcd_recoder_c_3; /* from vcd_recoder.c 601 */
TimeType end_time_vcd_recoder_c_3; /* from vcd_recoder.c 602 */
TimeType current_time_vcd_recoder_c_3; /* from vcd_recoder.c 603 */
int num_glitches_vcd_recoder_c_4; /* from vcd_recoder.c 604 */
int num_glitch_regions_vcd_recoder_c_4; /* from vcd_recoder.c 605 */
struct vcdsymbol *pv_vcd_recoder_c_3; /* from vcd_recoder.c 606 */
struct vcdsymbol *rootv_vcd_recoder_c_3; /* from vcd_recoder.c 607 */
char *vcdbuf_vcd_recoder_c_3; /* from vcd_recoder.c 608 */
char *vst_vcd_recoder_c_3; /* from vcd_recoder.c 609 */
char *vend_vcd_recoder_c_3; /* from vcd_recoder.c 610 */
int T_MAX_STR_vcd_recoder_c_3; /* from vcd_recoder.c 612 */
char *yytext_vcd_recoder_c_3; /* from vcd_recoder.c 613 */
int yylen_vcd_recoder_c_3; /* from vcd_recoder.c 614 */
int yylen_cache_vcd_recoder_c_3; /* from vcd_recoder.c 615 */
struct vcdsymbol *vcdsymroot_vcd_recoder_c_3; /* from vcd_recoder.c 616 */
struct vcdsymbol *vcdsymcurr_vcd_recoder_c_3; /* from vcd_recoder.c 617 */
struct vcdsymbol **sorted_vcd_recoder_c_3; /* from vcd_recoder.c 618 */
struct vcdsymbol **indexed_vcd_recoder_c_3; /* from vcd_recoder.c 619 */
int numsyms_vcd_recoder_c_3; /* from vcd_recoder.c 620 */
unsigned int vcd_minid_vcd_recoder_c_3; /* from vcd_recoder.c 621 */
unsigned int vcd_maxid_vcd_recoder_c_3; /* from vcd_recoder.c 622 */
int err_vcd_recoder_c_3; /* from vcd_recoder.c 623 */
off_t vcd_fsiz_vcd_recoder_c_2; /* from vcd_recoder.c 624 */
char *varsplit_vcd_recoder_c_3; /* from vcd_recoder.c 625 */
char *vsplitcurr_vcd_recoder_c_3; /* from vcd_recoder.c 626 */
int var_prevch_vcd_recoder_c_3; /* from vcd_recoder.c 627 */
unsigned int vcd_hash_max; /* from vcd_recoder.c */
int vcd_hash_kill; /* from vcd_recoder.c */


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
char vlist_spill_to_disk;
char vlist_prepack;
FILE *vlist_handle;
off_t vlist_bytes_written;
int vlist_compression_depth; /* from vlist.c 634 */


/*
 * vzt.c
 */
struct vzt_rd_trace *vzt_vzt_c_1; /* from vzt.c 635 */
TimeType first_cycle_vzt_c_3; /* from vzt.c 636 */
TimeType last_cycle_vzt_c_3; /* from vzt.c 637 */
TimeType total_cycles_vzt_c_3; /* from vzt.c 638 */
struct lx2_entry *vzt_table_vzt_c_1; /* from vzt.c 639 */
struct fac *mvlfacs_vzt_c_3; /* from vzt.c 640 */
int busycnt_vzt_c_2; /* from vzt.c 641 */


/*
 * wavewindow.c
 */
char highlight_wavewindow; /* from wavewindow.c */
char alt_wheel_mode; /* from wavewindow.c */
char use_scrollwheel_as_y; /* from wavewindow.c */
char enable_slider_zoom; /* from wavewindow.c */
int m1x_wavewindow_c_1; /* from wavewindow.c 642 */
int m2x_wavewindow_c_1; /* from wavewindow.c 643 */
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
struct blackout_region_t *blackout_regions; /* from wavewindow.c 661 */
TimeType zoom; /* from wavewindow.c 662 */
TimeType scale; /* from wavewindow.c 663 */
TimeType nsperframe; /* from wavewindow.c 664 */
double pixelsperframe; /* from wavewindow.c 665 */
double hashstep; /* from wavewindow.c 666 */
TimeType prevtim_wavewindow_c_1; /* from wavewindow.c 667 */
double pxns; /* from wavewindow.c 668 */
double nspx; /* from wavewindow.c 669 */
double zoombase; /* from wavewindow.c 670 */
struct TraceEnt *topmost_trace; /* from wavewindow.c 671 */
int waveheight; /* from wavewindow.c 672 */
int wavecrosspiece; /* from wavewindow.c 673 */
int wavewidth; /* from wavewindow.c 674 */
struct font_engine_font_t *wavefont; /* from wavewindow.c 675 */
struct font_engine_font_t *wavefont_smaller; /* from wavewindow.c 676 */
GtkWidget *wavearea; /* from wavewindow.c 677 */
GtkWidget *vscroll_wavewindow_c_1; /* from wavewindow.c 678 */
GtkWidget *hscroll_wavewindow_c_2; /* from wavewindow.c 679 */
GdkDrawable *wavepixmap_wavewindow_c_1; /* from wavewindow.c 680 */
GtkObject *wave_vslider; /* from wavewindow.c 681 */
GtkObject *wave_hslider; /* from wavewindow.c 682 */
TimeType named_markers[WAVE_NUM_NAMED_MARKERS]; /* from wavewindow.c 683 */
int named_marker_lock_idx; /* from menu.c */
char made_gc_contexts_wavewindow_c_1; /* from wavewindow.c 684 */
int which_t_color;
GdkGC *gc_white; /* from wavewindow.c 710 */
GdkGC *gc_black; /* from wavewindow.c 711 */
struct wave_gcmaster_t gc;
GdkGC *gc_rainbow[2*WAVE_NUM_RAINBOW];
char made_sgc_contexts_wavewindow_c_1; /* from wavewindow.c 709 */
char fill_in_smaller_rgb_areas_wavewindow_c_1; /* from wavewindow.c 719 */
TimeType prev_markertime; /* from wavewindow.c */
struct wave_gcmaster_t gccache;
int analog_redraw_skip_count; /* from wavewindow.c */
int str_wid_x;
int str_wid_width;
int str_wid_bigw;
int str_wid_state;
int str_wid_slider;
int str_wid_height;
TimeType ruler_origin;
TimeType ruler_step;
char fill_waveform;
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

gulong gtkwave_signal_connect_x(GtkObject *object, const gchar *name, GtkSignalFunc func, gpointer data, char *f, intptr_t line);
gulong gtkwave_signal_connect_object_x(GtkObject *object, const gchar *name, GtkSignalFunc func, gpointer data, char *f, intptr_t line);

#ifdef GTKWAVE_SIGNAL_CONNECT_DEBUG
#define gtkwave_signal_connect(a,b,c,d) gtkwave_signal_connect_x(a,b,c,d,__FILE__,__LINE__)
#define gtkwave_signal_connect_object(a,b,c,d) gtkwave_signal_connect_object_x(a,b,c,d,__FILE__,__LINE__)
#else
#define gtkwave_signal_connect(a,b,c,d) gtkwave_signal_connect_x(a,b,c,d,NULL,0)
#define gtkwave_signal_connect_object(a,b,c,d) gtkwave_signal_connect_object_x(a,b,c,d,NULL,0)
#endif

void set_GLOBALS_x(struct Global *g, const char *file, int line);

#ifdef GTKWAVE_GLOBALS_DEBUG
#define set_GLOBALS(a) set_GLOBALS_x(a,__FILE__,__LINE__)
#else
#define set_GLOBALS(a) set_GLOBALS_x(a,NULL,0)
#endif

void logbox_reload(void);
void clone_icon_pointers_across_contexts(struct Global *a, struct Global *b);

extern struct Global *GLOBALS;
#endif
