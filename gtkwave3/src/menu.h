/*
 * Copyright (c) Tony Bybell 1999-2017.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

/* example-start menu menufactory.h */

#ifndef __MENUFACTORY_H__
#define __MENUFACTORY_H__

#include <gtk/gtk.h>
#include <stdio.h>

#ifndef _MSC_VER
	#include <strings.h>
#endif

#include <errno.h>
#include "currenttime.h"
#include "fgetdynamic.h"
#include "strace.h"
#include "debug.h"
#include "symbol.h"
#include "main.h"

void do_popup_menu (GtkWidget *my_widget, GdkEventButton *event);
void do_sst_popup_menu (GtkWidget *my_widget, GdkEventButton *event);
void get_main_menu (GtkWidget *, GtkWidget **menubar);
void menu_set_sensitive(void);
int file_quit_cmd_callback (GtkWidget *widget, gpointer data);
int set_wave_menu_accelerator(char *str);
int execute_script(char *name, int dealloc_name);

void kill_main_menu_accelerators(void); /* for conflicts with twinwave */

struct stringchain_t
{
struct stringchain_t *next;
char *name;
};

#ifdef MAC_INTEGRATION
#define WAVE_USE_MLIST_T
#endif

#ifdef WAVE_USE_MLIST_T
typedef void    (*gtkwave_mlist_callback)  ();

struct gtkwave_mlist_t
{
  gchar *path;
  gchar *accelerator;

  gtkwave_mlist_callback callback;
  guint                  callback_action;

  /* possible values:
   * "<Item>"           -> create a simple item
   * "<ToggleItem>"     -> create a toggle item
   * "<Separator>"      -> create a separator
   */
  gchar          *item_type;

  /* Extra data for some item types:
   *  ImageItem  -> pointer to inlined pixbuf stream
   *  StockItem  -> name of stock item
   */
  gconstpointer extra_data;
};

typedef struct gtkwave_mlist_t gtkwave_mlist_t;

GtkWidget *alt_menu_top(GtkWidget *window);
GtkWidget *alt_menu(gtkwave_mlist_t *mi, int nmenu_items, GtkWidget **wlist, GtkAccelGroup *accel, gboolean is_menubar);

#else
#define gtkwave_mlist_t GtkItemFactoryEntry
#endif

enum WV_MenuItems {
WV_MENU_FONV,
WV_MENU_FONVT,
WV_MENU_FRW,
WV_MENU_WRVCD,
WV_MENU_WRLXT,
WV_MENU_WRTIM,
WV_MENU_WCLOSE,
WV_MENU_SEP2VCD,
WV_MENU_FPTF,
#if GTK_CHECK_VERSION(2,14,0)
WV_MENU_SGRAB,
#endif
WV_MENU_SEP1,
WV_MENU_FRSF,
WV_MENU_FWSF,
WV_MENU_FWSFAS,
WV_MENU_SEP2,
WV_MENU_FRLF,
WV_MENU_SEP2LF,
#if !defined _MSC_VER
WV_MENU_FRSTMF,
WV_MENU_SEP2STMF,
#endif
#if defined(HAVE_LIBTCL)
WV_MENU_TCLSCR,
WV_MENU_TCLSEP,
#endif
WV_MENU_FQY,
WV_MENU_ESTMH,
WV_MENU_ETH,
WV_MENU_SEP3,
WV_MENU_EIB,
WV_MENU_EIC,
WV_MENU_EIA,
WV_MENU_EC,
WV_MENU_ECY,
WV_MENU_EP,
WV_MENU_DEL,
WV_MENU_SEP3A,
WV_MENU_EAHT,
WV_MENU_ERHA,
WV_MENU_SEP4,
WV_MENU_EE,
WV_MENU_ECD,
WV_MENU_ECU,
WV_MENU_SEP5,
WV_MENU_EDFH,
WV_MENU_EDFD,
WV_MENU_EDFSD,
WV_MENU_EDFB,
WV_MENU_EDFO,
WV_MENU_EDFA,
WV_MENU_TIME,
WV_MENU_ENUM,
WV_MENU_EDRL,
WV_MENU_EDR2BON,
WV_MENU_EDR2BOFF,
WV_MENU_EDFRJON,
WV_MENU_EDFRJOFF,
WV_MENU_EDFION,
WV_MENU_EDFIOFF,
WV_MENU_EDFRON,
WV_MENU_EDFROFF,
WV_MENU_XLF_0,
WV_MENU_XLF_1,
WV_MENU_XLP_0,
WV_MENU_XLP_1,
WV_MENU_TTXLP_0,
WV_MENU_TTXLP_1,
WV_MENU_EDFAOFF,
WV_MENU_EDFASTEP,
WV_MENU_EDFAINTERPOL,
WV_MENU_EDFAINTERPOL2,
WV_MENU_EDFARSD,
WV_MENU_EDFARAD,
WV_MENU_RFILL0,
WV_MENU_RFILL1,
WV_MENU_RFILLOFF,
WV_MENU_B2G,
WV_MENU_G2B,
WV_MENU_GBNONE,
WV_MENU_POPON,
WV_MENU_POPOFF,
WV_MENU_FPSHIFTON,
WV_MENU_FPSHIFTOFF,
WV_MENU_FPSHIFTVAL,
WV_MENU_CLRFMT0,
WV_MENU_CLRFMT1,
WV_MENU_CLRFMT2,
WV_MENU_CLRFMT3,
WV_MENU_CLRFMT4,
WV_MENU_CLRFMT5,
WV_MENU_CLRFMT6,
WV_MENU_CLRFMT7,
WV_MENU_CLRFMTC,
WV_MENU_SEP5A,
WV_MENU_KEEPXZ,
WV_MENU_ESCAH,
WV_MENU_ESCFH,
WV_MENU_SEP6,
WV_MENU_WARP,
WV_MENU_UNWARP,
WV_MENU_UNWARPA,
WV_MENU_SEP7A,
WV_MENU_EEX,
WV_MENU_ESH,
WV_MENU_SEP6A,
/* WV_MENU_EXA, */
/* WV_MENU_CPA, */
WV_MENU_TG,
WV_MENU_AG,
WV_MENU_SEP6A1,
WV_MENU_EHR,
WV_MENU_EUHR,
WV_MENU_EHA,
WV_MENU_EUHA,
WV_MENU_SEP6B,
WV_MENU_ALPHA,
WV_MENU_ALPHA2,
WV_MENU_LEX,
WV_MENU_RVS,
WV_MENU_SPS,
#ifdef WAVE_USE_GTK2
WV_MENU_SPS2,
#endif
WV_MENU_SEP7B,
WV_MENU_SSR,
WV_MENU_SSH,
WV_MENU_SST,
WV_MENU_SEP7,
#if !defined __MINGW32__ && !defined _MSC_VER
WV_MENU_OPENHS,
WV_MENU_OPENIHS,
#endif
WV_MENU_OPENH,
WV_MENU_SEP7D,
WV_MENU_ACOL,
WV_MENU_ACOLR,
WV_MENU_ABON,
WV_MENU_HTGP,
WV_MENU_SEP7C,
WV_MENU_STRSE,
WV_MENU_TMTT,
WV_MENU_TZZA,
WV_MENU_TZZB,
WV_MENU_TZZI,
WV_MENU_TZZO,
WV_MENU_TZZBFL,
WV_MENU_TZZBF,
WV_MENU_TZZTS,
WV_MENU_TZZTE,
WV_MENU_TZUZ,
WV_MENU_TFFS,
WV_MENU_TFFR,
WV_MENU_TFFL,
WV_MENU_TDDR,
WV_MENU_TDDL,
WV_MENU_TSSR,
WV_MENU_TSSL,
WV_MENU_TPPR,
WV_MENU_TPPL,
WV_MENU_MSCMD,
WV_MENU_MDNM,
WV_MENU_MCNM,
WV_MENU_MCANM,
WV_MENU_MCAB,
WV_MENU_MDPM,
WV_MENU_SEP8,
WV_MENU_SLE,
WV_MENU_SRE,
WV_MENU_SEP8B,
WV_MENU_HSWM,
WV_MENU_MWSON,
WV_MENU_MLKLT,
WV_MENU_MLKRT,
WV_MENU_MLKOFF,
WV_MENU_VSG,
WV_MENU_SEP9,
WV_MENU_SHW,
WV_MENU_FILL1,
WV_MENU_SEP9B,
WV_MENU_VSMO,
#ifdef WAVE_USE_GTK2
WV_MENU_VSMC,
#endif
WV_MENU_SEP9A,
WV_MENU_VSBS,
WV_MENU_SEP10,
WV_MENU_ESTS,
WV_MENU_SEP10A,
WV_MENU_VDR,
WV_MENU_SEP11,
WV_MENU_VCZ,
WV_MENU_SEP12,
WV_MENU_VTDF,
WV_MENU_VTMM,
WV_MENU_SEP13,
WV_MENU_VCMU,
WV_MENU_SEP14,
WV_MENU_VDRV,
WV_MENU_SEP15,
WV_MENU_VLJS,
WV_MENU_VRJS,
WV_MENU_SEP16,
WV_MENU_VZPS,
WV_MENU_VZDYN,
WV_MENU_VZDYNE,
WV_MENU_VFTP,
WV_MENU_SEP17,
WV_MENU_RULER,
WV_MENU_RMRKS,
WV_MENU_SEP17A,
WV_MENU_USECOLOR,
WV_MENU_USEBW,
WV_MENU_SEP18,
WV_MENU_LXTCC2Z,
WV_MENU_SEP19,
WV_MENU_TDSCALEX,
WV_MENU_TDSCALES,
WV_MENU_TDSCALEM,
WV_MENU_TDSCALEU,
WV_MENU_TDSCALEN,
WV_MENU_TDSCALEP,
WV_MENU_TDSCALEF,
WV_MENU_HWH,
#ifdef MAC_INTEGRATION
WV_MENU_HWM,
#endif
WV_MENU_HWV,
WV_MENU_NUMITEMS
};

enum WV_RecurseType {
WV_RECURSE_APPEND,
WV_RECURSE_INSERT,
WV_RECURSE_REPLACE,
};

void menu_new_viewer(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_write_vcd_file(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_write_lxt_file(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_print(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_read_save_file(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_write_save_file(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_write_save_file_as(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_read_log_file(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_read_stems_file(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_quit(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_set_max_hier(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_insert_blank_traces(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_insert_comment_traces(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_insert_analog_height_extension(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_alias(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_remove_aliases(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_cut_traces(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_copy_traces(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_paste_traces(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_combine_down(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_combine_up(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_hex(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_dec(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_signed(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_bin(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_oct(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_ascii(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_real(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_rjustify_on(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_rjustify_off(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_invert_on(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_invert_off(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_reverse_on(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_reverse_off(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_xlate_file_0(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_xlate_file_1(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_xlate_proc_0(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_xlate_proc_1(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_analog_off(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_analog_step(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_analog_interpol(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_showchangeall(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_showchange(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_warp_traces(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_unwarp_traces(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_unwarp_traces_all(gpointer null_data, guint callback_action, GtkWidget *widget);

void menu_dataformat_exclude_on(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_exclude_off(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_regexp_highlight(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_regexp_unhighlight(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_highlight_all(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_dataformat_unhighlight_all(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_alphabetize(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_alphabetize2(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_lexize(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_reverse(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_tracesearchbox(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_signalsearch(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_hiersearch(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_treesearch(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_autocoalesce(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_autocoalesce_reversal(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_autoname_bundles_on(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_hgrouping(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_movetotime(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_zoomsize(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_zoombase(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_fetchsize(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_markerbox(gpointer null_data, guint callback_action, GtkWidget *widget);

void drop_named_marker(gpointer null_data, guint callback_action, GtkWidget *widget);
void collect_named_marker(gpointer null_data, guint callback_action, GtkWidget *widget);
void collect_all_named_markers(gpointer null_data, guint callback_action, GtkWidget *widget);
void delete_unnamed_marker(gpointer null_data, guint callback_action, GtkWidget *widget);
void wave_scrolling_on(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_show_grid(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_show_mouseover(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_show_base(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_enable_dynamic_resize(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_center_zooms(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_toggle_delta_or_frequency(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_toggle_max_or_marker(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_enable_constant_marker_update(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_use_roundcaps(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_left_justify(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_right_justify(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_zoom10_snap(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_use_full_precision(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_remove_marked(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_lxt_clk_compress(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_help(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_version(gpointer null_data, guint callback_action, GtkWidget *widget);
void menu_toggle_group(gpointer null_data, guint callback_action, GtkWidget *widget);

gtkwave_mlist_t *retrieve_menu_items_array(int *num_items);

void menu_read_stems_cleanup(GtkWidget *widget, gpointer data);
void menu_new_viewer_tab_cleanup(GtkWidget *widget, gpointer data);
int menu_new_viewer_tab_cleanup_2(char *fname, int optimize_vcd);

void movetotime_cleanup(GtkWidget *widget, gpointer data);
void zoomsize_cleanup(GtkWidget *widget, gpointer data);


void set_scale_to_time_dimension_toggles(void);

void SetTraceScrollbarRowValue(int row, unsigned center);

bvptr combine_traces(int direction, Trptr single_trace_only);
unsigned create_group (char* name, Trptr t_composite);

/* currently only for OSX to disable OSX menus when grabbed */
void wave_gtk_grab_add(GtkWidget *w);
void wave_gtk_grab_remove(GtkWidget *w);
#ifdef MAC_INTEGRATION
void osx_menu_sensitivity(gboolean tr);
#endif

#endif
