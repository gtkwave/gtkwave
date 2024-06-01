/*
 * Copyright (c) Tony Bybell 1999-2014.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "globals.h"

#ifndef ANALYZER_H
#define ANALYZER_H

#include <gtk/gtk.h>
#include <stdlib.h>
#include "debug.h"

typedef struct _SearchProgressData
{
    GtkWidget *window;
    GtkWidget *pbar;
    int timer; /* might be used later.. */
    gfloat value, oldvalue;
} SearchProgressData;

#define BITATTRIBUTES_MAX 32768

typedef struct ExpandInfo *eptr;

typedef unsigned long Ulong;
typedef unsigned int Uint;

enum TraceReorderMode
{
    TR_SORT_INS,
    TR_SORT_NORM,
    TR_SORT_LEX,
    TR_SORT_RVS
};

/* vvv   Bit representation   vvv */

#define AN_NORMAL \
    { \
        GW_BIT_0, GW_BIT_X, GW_BIT_Z, GW_BIT_1, GW_BIT_H, GW_BIT_U, GW_BIT_W, GW_BIT_L, GW_BIT_DASH, GW_BIT_DASH, GW_BIT_DASH, GW_BIT_DASH, \
            GW_BIT_DASH, GW_BIT_DASH, GW_BIT_DASH, GW_BIT_DASH \
    }
#define AN_INVERSE \
    { \
        GW_BIT_1, GW_BIT_X, GW_BIT_Z, GW_BIT_0, GW_BIT_L, GW_BIT_U, GW_BIT_W, GW_BIT_H, GW_BIT_DASH, GW_BIT_DASH, GW_BIT_DASH, GW_BIT_DASH, \
            GW_BIT_DASH, GW_BIT_DASH, GW_BIT_DASH, GW_BIT_DASH \
    }

/* positional ascii 0123456789ABCDEF, question marks should not happen unless something slips
 * through the cracks as AN_RSVA to AN_RSVF are reserved */

#define AN_USTR "0XZ1HUWL-???????"
#define AN_USTR_INV "1XZ0LUWH-???????"

/* for writing out 4 state formats (read GHW, write LXT) */
#define AN_STR4ST "0xz11xz0xxxxxxxx"
#define AN_USTR4ST "0XZ11XZ0XXXXXXXX"

/* for hex/oct conversion in baseconvert.c */
#define AN_HEX_STR "0123456789ABCDEFxzwu-XZWU"
#define AN_OCT_STR "01234567xzwu-"


/* ^^^   Bit representation   ^^^ */

#define MAX_HISTENT_TIME ((GwTime)(~((GW_UTIME_CONSTANT(-1)) << (sizeof(GwTime) * 8 - 1))))

typedef struct ExpandInfo /* only used when expanding atomic vex.. */
{
    GwNode **narray;
    int msb, lsb;
    int width;
} ExpandInfo;

typedef uint64_t TraceFlagsType;
#define TRACEFLAGSSCNFMT SCNx64
#define TRACEFLAGSPRIFMT PRIx64
#define TRACEFLAGSPRIuFMT PRIu64

typedef struct
{
    GwTime first; /* beginning time of trace */
    GwTime last; /* end time of trace */
    GwTime start; /* beginning time of trace on screen */
    GwTime end; /* ending time of trace on screen */
    GwTime prevmarker; /* from last drawmarker()	        */
    GwTime resizemarker; /* from last MaxSignalLength()          */
    GwTime resizemarker2; /* from 2nd last MaxSignalLength()      */
    GwTime timecache; /* to get around floating pt limitation */
    GwTime laststart; /* caches last set value                */

    gdouble zoom; /* current zoom  */
    gdouble prevzoom; /* for zoom undo */
} Times;

typedef struct
{
    int total; /* total number of traces */
    int visible; /* total number of (uncollapsed) traces */
    GwTrace *first; /* ptr. to first trace in list */
    GwTrace *last; /* end of list of traces */
    GwTrace *buffer; /* cut/copy buffer of traces */
    GwTrace *bufferlast; /* last element of bufferchain */
    int buffercount; /* number of traces in buffer */

    unsigned dirty : 1; /* to notify Tcl that traces were added/deleted/moved */
} Traces;

typedef struct
{
    GwTrace *buffer; /* cut/copy buffer of traces */
    GwTrace *bufferlast; /* last element of bufferchain */
    int buffercount; /* number of traces in buffer */
} TempBuffer;

enum TraceEntFlagBits
{
    TR_HIGHLIGHT_B,
    TR_HEX_B,
    TR_DEC_B,
    TR_BIN_B,
    TR_OCT_B,
    TR_RJUSTIFY_B,
    TR_INVERT_B,
    TR_REVERSE_B,
    TR_EXCLUDE_B,
    TR_BLANK_B,
    TR_SIGNED_B,
    TR_ASCII_B,
    TR_COLLAPSED_B,
    TR_FTRANSLATED_B,
    TR_PTRANSLATED_B,
    TR_ANALOG_STEP_B,
    TR_ANALOG_INTERPOLATED_B,
    TR_ANALOG_BLANK_STRETCH_B,
    TR_REAL_B,
    TR_ANALOG_FULLSCALE_B,
    TR_ZEROFILL_B,
    TR_ONEFILL_B,
    TR_CLOSED_B,
    TR_GRP_BEGIN_B,
    TR_GRP_END_B,
    TR_BINGRAY_B,
    TR_GRAYBIN_B,
    TR_REAL2BITS_B,
    TR_TTRANSLATED_B,
    TR_POPCNT_B,
    TR_FPDECSHIFT_B,
    TR_TIME_B,
    TR_ENUM_B,
    TR_CURSOR_B,
    TR_FFO_B,

    TR_RSVD_B /* for use internally such as temporary caching of highlighting, not for use in traces
               */
};

#define TR_HIGHLIGHT (UINT64_C(1) << TR_HIGHLIGHT_B)
#define TR_HEX (UINT64_C(1) << TR_HEX_B)
#define TR_ASCII (UINT64_C(1) << TR_ASCII_B)
#define TR_DEC (UINT64_C(1) << TR_DEC_B)
#define TR_BIN (UINT64_C(1) << TR_BIN_B)
#define TR_OCT (UINT64_C(1) << TR_OCT_B)
#define TR_RJUSTIFY (UINT64_C(1) << TR_RJUSTIFY_B)
#define TR_INVERT (UINT64_C(1) << TR_INVERT_B)
#define TR_REVERSE (UINT64_C(1) << TR_REVERSE_B)
#define TR_EXCLUDE (UINT64_C(1) << TR_EXCLUDE_B)
#define TR_BLANK (UINT64_C(1) << TR_BLANK_B)
#define TR_SIGNED (UINT64_C(1) << TR_SIGNED_B)
#define TR_ANALOG_STEP (UINT64_C(1) << TR_ANALOG_STEP_B)
#define TR_ANALOG_INTERPOLATED (UINT64_C(1) << TR_ANALOG_INTERPOLATED_B)
#define TR_ANALOG_BLANK_STRETCH (UINT64_C(1) << TR_ANALOG_BLANK_STRETCH_B)
#define TR_REAL (UINT64_C(1) << TR_REAL_B)
#define TR_ANALOG_FULLSCALE (UINT64_C(1) << TR_ANALOG_FULLSCALE_B)
#define TR_ZEROFILL (UINT64_C(1) << TR_ZEROFILL_B)
#define TR_ONEFILL (UINT64_C(1) << TR_ONEFILL_B)
#define TR_CLOSED (UINT64_C(1) << TR_CLOSED_B)

#define TR_GRP_BEGIN (UINT64_C(1) << TR_GRP_BEGIN_B)
#define TR_GRP_END (UINT64_C(1) << TR_GRP_END_B)
#define TR_GRP_MASK (TR_GRP_BEGIN | TR_GRP_END)

#define TR_BINGRAY (UINT64_C(1) << TR_BINGRAY_B)
#define TR_GRAYBIN (UINT64_C(1) << TR_GRAYBIN_B)
#define TR_GRAYMASK (TR_BINGRAY | TR_GRAYBIN)

#define TR_REAL2BITS (UINT64_C(1) << TR_REAL2BITS_B)

#define TR_NUMMASK \
    (TR_ASCII | TR_HEX | TR_DEC | TR_BIN | TR_OCT | TR_SIGNED | TR_REAL | TR_TIME | TR_ENUM)
#define TR_ATTRIBS (TR_RJUSTIFY | TR_INVERT | TR_REVERSE | TR_EXCLUDE | TR_POPCNT | TR_FFO)

#define TR_COLLAPSED (UINT64_C(1) << TR_COLLAPSED_B)
#define TR_ISCOLLAPSED (TR_BLANK | TR_COLLAPSED)

#define TR_FTRANSLATED (UINT64_C(1) << TR_FTRANSLATED_B)
#define TR_PTRANSLATED (UINT64_C(1) << TR_PTRANSLATED_B)
#define TR_TTRANSLATED (UINT64_C(1) << TR_TTRANSLATED_B)

#define TR_POPCNT (UINT64_C(1) << TR_POPCNT_B)
#define TR_FPDECSHIFT (UINT64_C(1) << TR_FPDECSHIFT_B)

#define TR_TIME (UINT64_C(1) << TR_TIME_B)
#define TR_ENUM (UINT64_C(1) << TR_ENUM_B)

#define TR_CURSOR (UINT64_C(1) << TR_CURSOR_B)
#define TR_FFO (UINT64_C(1) << TR_FFO_B)

#define TR_ANALOGMASK (TR_ANALOG_STEP | TR_ANALOG_INTERPOLATED)

#define TR_RSVD (UINT64_C(1) << TR_RSVD_B)

GwTrace *GiveNextTrace(GwTrace *t);
GwTrace *GivePrevTrace(GwTrace *t);
int UpdateTracesVisible(void);

void DisplayTraces(int val);
int AddNodeTraceReturn(GwNode *nd, char *aliasname, GwTrace **tret);
int AddNode(GwNode *nd, char *aliasname);
int AddNodeUnroll(GwNode *nd, char *aliasname);
int AddVector(GwBitVector *vec, char *aliasname);
int AddBlankTrace(char *commentname);
int InsertBlankTrace(char *comment, TraceFlagsType different_flags);
void RemoveNode(GwNode *n);
void RemoveTrace(GwTrace *t, int dofree);
void FreeTrace(GwTrace *t);
GwTrace *CutBuffer(void);
void FreeCutBuffer(void);
GwTrace *PasteBuffer(void);
GwTrace *PrependBuffer(void);
int TracesReorder(int mode);
int DeleteBuffer(void);

void import_trace(GwNode *np);

eptr ExpandNode(GwNode *n);
void DeleteNode(GwNode *n);
GwNode *ExtractNodeSingleBit(GwNode *n, int bit);

/* hierarchy depths */
char *hier_extract(char *pnt, int levels);

/* vector matching */
char *attempt_vecmatch(char *s1, char *s2);

void updateTraceGroup(GwTrace *t);
int GetTraceNumber(GwTrace *t);
void EnsureGroupsMatch(void);

#define IsSelected(t) (t->flags & TR_HIGHLIGHT)
#define IsGroupBegin(t) (t->flags & TR_GRP_BEGIN)
#define IsGroupEnd(t) (t->flags & TR_GRP_END)
#define IsClosed(t) (t->flags & TR_CLOSED)
#define HasWave(t) (!(t->flags & (TR_BLANK | TR_ANALOG_BLANK_STRETCH)))
#define CanAlias(t) HasWave(t)
#define HasAlias(t) (t->name_full && HasWave(t))
#define IsCollapsed(t) (t->flags & TR_COLLAPSED)

unsigned IsShadowed(GwTrace *t);
char *GetFullName(GwTrace *t);

void OpenTrace(GwTrace *t);
void CloseTrace(GwTrace *t);
void ClearTraces(void);
void ClearGroupTraces(GwTrace *t);
void MarkTraceCursor(GwTrace *t);

char *varxt_fix(char *s);

#endif
