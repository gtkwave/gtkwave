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
#include "wavealloca.h"
#include "vlist.h"
#include "debug.h"

#ifdef AET2_IS_PRESENT
#define WAVE_ARRAY_SUPPORT
#endif


/* struct Node bitfield widths */
#define WAVE_VARXT_WIDTH (16)
#define WAVE_VARXT_MAX_ID ((1 << WAVE_VARXT_WIDTH) - 1)
#define WAVE_VARDT_WIDTH (6)
#define WAVE_VARDIR_WIDTH (3)
#define WAVE_VARTYPE_WIDTH (6)


typedef struct _SearchProgressData {
    GtkWidget *window;
    GtkWidget *pbar;
    GtkAdjustment *adj;
    int timer;	/* might be used later.. */
    gfloat value, oldvalue;
} SearchProgressData;


#define BITATTRIBUTES_MAX 32768

typedef struct ExpandInfo *eptr;
typedef struct ExpandReferences *exptr;
typedef struct Node	  *nptr;
typedef struct HistEnt	  *hptr;
typedef struct Bits       *bptr;
typedef struct VectorEnt  *vptr;
typedef struct BitVector  *bvptr;
typedef struct BitAttributes *baptr;

typedef unsigned long  Ulong;
typedef unsigned int   Uint;

enum TraceReorderMode  { TR_SORT_INS, TR_SORT_NORM, TR_SORT_LEX, TR_SORT_RVS };


/* vvv   Bit representation   vvv */

enum AnalyzerBits  { AN_0, AN_X, AN_Z, AN_1, AN_H, AN_U, AN_W, AN_L, AN_DASH, AN_RSV9, AN_RSVA, AN_RSVB, AN_RSVC, AN_RSVD, AN_RSVE, AN_RSVF, AN_COUNT };
#define AN_NORMAL  { AN_0, AN_X, AN_Z, AN_1, AN_H, AN_U, AN_W, AN_L, AN_DASH, AN_DASH, AN_DASH, AN_DASH, AN_DASH, AN_DASH, AN_DASH, AN_DASH }
#define AN_INVERSE { AN_1, AN_X, AN_Z, AN_0, AN_L, AN_U, AN_W, AN_H, AN_DASH, AN_DASH, AN_DASH, AN_DASH, AN_DASH, AN_DASH, AN_DASH, AN_DASH }

#define AN_MSK     (AN_COUNT-1) /* max index into AN_STR, AN_COUNT *must* be a power of two unless logic AND with AN_MSK is changed */

/* positional ascii 0123456789ABCDEF, question marks should not happen unless something slips through the cracks as AN_RSVA to AN_RSVF are reserved */
#define AN_STR      "0xz1huwl-???????"
#define AN_STR_INV  "1xz0luwh-???????"

#define AN_USTR     "0XZ1HUWL-???????"
#define AN_USTR_INV "1XZ0LUWH-???????"

/* for writing out 4 state formats (read GHW, write LXT) */
#define AN_STR4ST   "0xz11xz0xxxxxxxx"
#define AN_USTR4ST  "0XZ11XZ0XXXXXXXX"

/* for hex/oct conversion in baseconvert.c */
#define AN_HEX_STR  "0123456789ABCDEFxzwu-XZWU"
#define AN_OCT_STR  "01234567xzwu-"

/* now the recoded "extra" values... */
#define RCV_X (1 | (0<<1))
#define RCV_Z (1 | (1<<1))
#define RCV_H (1 | (2<<1))
#define RCV_U (1 | (3<<1))
#define RCV_W (1 | (4<<1))
#define RCV_L (1 | (5<<1))
#define RCV_D (1 | (6<<1))

#define RCV_STR "xzhuwl-?"
/*               01234567 */

/* ^^^   Bit representation   ^^^ */

#if (SIZEOF_VOID_P == SIZEOF_DOUBLE)
#define WAVE_HAS_H_DOUBLE
#endif


#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

typedef struct HistEnt
{
hptr next;	      /* next transition in history */

union
  {
  unsigned char h_val;  /* value: AN_STR[val] or AnalyzerBits which correspond */
  char *h_vector;	/* pointer to a whole vector of h_val type bits */
#ifdef WAVE_HAS_H_DOUBLE
  double h_double;
#endif
  } v;

TimeType time;        /* time of transition */
unsigned char flags;  /* so far only set on glitch/real condition */
} HistEnt;

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif


enum HistEntFlagBits
{ HIST_GLITCH_B, HIST_REAL_B, HIST_STRING_B };

#define HIST_GLITCH (1<<HIST_GLITCH_B)
#define HIST_REAL   (1<<HIST_REAL_B)
#define HIST_STRING (1<<HIST_STRING_B)

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

typedef struct VectorEnt
{
TimeType time;
vptr next;
unsigned char flags;  /* so far only set on strings */
unsigned char v[]; /* C99 */
} VectorEnt;

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif


#define MAX_HISTENT_TIME ((TimeType)(~( (ULLDescriptor(-1)) << (sizeof(TimeType) * 8 - 1))))


typedef struct ExpandInfo	/* only used when expanding atomic vex.. */
{
nptr	*narray;
int	msb, lsb;
int	width;
} ExpandInfo;

typedef struct ExpandReferences
{
nptr   parent;			/* which atomic vec we expanded from */
int    parentbit;		/* which bit from that atomic vec */
int    actual;			/* bit number to be used in [] */
int    refcnt;
} ExpandReferences;

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

struct Node
  {
    exptr    expansion; /* indicates which nptr this node was expanded from (if it was expanded at all) and (when implemented) refcnts */
    char     *nname;	/* ascii name of node */
    HistEnt  head;	/* first entry in transition history */
    hptr     curr;      /* ptr. to current history entry */

    hptr     *harray;   /* fill this in when we make a trace.. contains  */
			/*  a ptr to an array of histents for bsearching */
    union {
      struct fac *mvlfac; /* for use with mvlsim aets */
      struct vlist_packer_t *mvlfac_packer_vlist;
      struct vlist_t *mvlfac_vlist;
      char *value;	/* for use when unrolling ae2 values */
    } mv; 		/* anon union is a gcc extension so use mv instead.  using this union avoids crazy casting warnings */

    int msi, lsi;	/* for 64-bit, more efficient than having as an external struct ExtNode*/

    int      numhist;	/* number of elements in the harray */

#ifdef WAVE_ARRAY_SUPPORT
    unsigned int array_height, this_row;
#endif

    unsigned varxt   : WAVE_VARXT_WIDTH; /* reference inside subvar_pnt[] */
    unsigned vardt   : WAVE_VARDT_WIDTH; /* see nodeVarDataType, this is an internal value */
    unsigned vardir  : WAVE_VARDIR_WIDTH; /* see nodeVarDir, this is an internal value (currently used only by extload and FST) */
    unsigned vartype : WAVE_VARTYPE_WIDTH; /* see nodeVarType, this is an internal value */

    unsigned extvals : 1; /* was formerly a pointer to ExtNode "ext", now simply a flag */
  };


#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif


#define WAVE_NODEVARTYPE_STR \
static const char *vartype_strings[] = { \
    "", \
    "event", \
    "integer", \
    "parm", \
    "real", \
    "real_parm", \
    "reg", \
    "supply0", \
    "supply1", \
    "time", \
    "tri", \
    "triand", \
    "trior", \
    "trireg", \
    "tri0", \
    "tri1", \
    "wand", \
    "wire", \
    "wor", \
    "array", \
    "realtime", \
    "port", \
    "string", \
    "bit", \
    "logic", \
    "int", \
    "s_int", \
    "l_int", \
    "byte", \
    "enum", \
    "s_real", \
    "signal", \
    "var", \
    "const", \
    "file", \
    "memory", \
    "net", \
    "alias", \
    "missing" \
};


enum nodeVarType {
    ND_UNSPECIFIED_DEFAULT = 0,

    ND_VCD_EVENT           = 1,
    ND_VCD_INTEGER         = 2,
    ND_VCD_PARAMETER       = 3,
    ND_VCD_REAL            = 4,
    ND_VCD_REAL_PARAMETER  = 5,
    ND_VCD_REG             = 6,
    ND_VCD_SUPPLY0         = 7,
    ND_VCD_SUPPLY1         = 8,
    ND_VCD_TIME            = 9,
    ND_VCD_TRI             = 10,
    ND_VCD_TRIAND          = 11,
    ND_VCD_TRIOR           = 12,
    ND_VCD_TRIREG          = 13,
    ND_VCD_TRI0            = 14,
    ND_VCD_TRI1            = 15,
    ND_VCD_WAND            = 16,
    ND_VCD_WIRE            = 17,
    ND_VCD_WOR             = 18,
    ND_VCD_ARRAY           = 19,    /* used to define the rownum (index) port on the array */
    ND_VCD_REALTIME        = 20,

    ND_VCD_PORT		   = 21,
    ND_GEN_STRING          = 22,    /* generic string type */

    ND_SV_BIT              = 23,    /* SV types */
    ND_SV_LOGIC            = 24,
    ND_SV_INT              = 25,
    ND_SV_SHORTINT         = 26,
    ND_SV_LONGINT          = 27,
    ND_SV_BYTE             = 28,
    ND_SV_ENUM             = 29,
    ND_SV_SHORTREAL        = 30,

    ND_VHDL_SIGNAL         = 31,
    ND_VHDL_VARIABLE       = 32,
    ND_VHDL_CONSTANT       = 33,
    ND_VHDL_FILE           = 34,
    ND_VHDL_MEMORY         = 35,

    ND_GEN_NET		   = 36,    /* used for AE2 */
    ND_GEN_ALIAS	   = 37,
    ND_GEN_MISSING	   = 38,

    ND_VARTYPE_MAX	   = 38
   /* if this exceeds 63, need to update struct Node's "unsigned vartype : 6" declaration */
};


#define WAVE_NODEVARDATATYPE_STR \
static const char *vardatatype_strings[] = { \
    "", \
    "boolean", \
    "bit", \
    "bit_vec", \
    "ulogic", \
    "ulogic_v", \
    "logic", \
    "logic_v", \
    "unsigned", \
    "signed", \
    "integer", \
    "real", \
    "natural", \
    "positive", \
    "time", \
    "char", \
    "string" \
};


enum nodeVarDataType {
    ND_VDT_NONE                   = 0,

    ND_VDT_VHDL_BOOLEAN           = 1,
    ND_VDT_VHDL_BIT               = 2,
    ND_VDT_VHDL_BIT_VECTOR        = 3,
    ND_VDT_VHDL_STD_ULOGIC        = 4,
    ND_VDT_VHDL_STD_ULOGIC_VECTOR = 5,
    ND_VDT_VHDL_STD_LOGIC         = 6,
    ND_VDT_VHDL_STD_LOGIC_VECTOR  = 7,
    ND_VDT_VHDL_UNSIGNED          = 8,
    ND_VDT_VHDL_SIGNED            = 9,
    ND_VDT_VHDL_INTEGER           = 10,
    ND_VDT_VHDL_REAL              = 11,
    ND_VDT_VHDL_NATURAL           = 12,
    ND_VDT_VHDL_POSITIVE          = 13,
    ND_VDT_VHDL_TIME              = 14,
    ND_VDT_VHDL_CHARACTER         = 15,
    ND_VDT_VHDL_STRING            = 16,

    ND_VDT_MAX                    = 16
   /* if this exceeds 63, need to update struct Node's "unsigned vardt : 6" declaration */
};


#define WAVE_NODEVARDIR_STR \
static const char *vardir_strings[] = { \
    "", \
    "I", \
    "O", \
    "IO", \
    "B", \
    "L" \
};


enum nodeVarDir {
    ND_DIR_IMPLICIT        = 0,
    ND_DIR_IN              = 1,
    ND_DIR_OUT             = 2,
    ND_DIR_INOUT           = 3,
    ND_DIR_BUFFER          = 4,
    ND_DIR_LINKAGE         = 5,

    ND_DIR_MAX             = 5,
    ND_DIR_UNSPECIFIED     = 6
   /* if ND_DIR_MAX exceeds 7, need to update struct Node's "unsigned vardir : 3" declaration */
};


typedef uint64_t TraceFlagsType;
#define TRACEFLAGSSCNFMT SCNx64
#define TRACEFLAGSPRIFMT PRIx64
#define TRACEFLAGSPRIuFMT PRIu64

typedef struct BitAttributes
  {
  TimeType  shift;
  TraceFlagsType flags;
  } BitAttributes;


#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

typedef struct Bits
  {
    char    *name;		/* name of this vector of bits   */
    int     nnbits;		/* number of bits in this vector */
    baptr attribs;  		/* for keeping track of combined timeshifts and inversions (and for savefile) */

    nptr    nodes[];		/* C99 pointers to the bits (nodes)  */
  } Bits;

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif


#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(push)
#pragma pack(1)
#endif

typedef struct BitVector
  {
    bvptr   transaction_cache;  /* for TR_TTRANSLATED traces */
    bvptr   transaction_chain;	/* for TR_TTRANSLATED traces */
    nptr    transaction_nd;	/* for TR_TTRANSLATED traces */

    char    *bvname;		/* name of this vector of bits           */
    int     nbits;		/* number of bits in this vector         */
    int     numregions;		/* number of regions that follow         */
    bptr    bits;		/* pointer to Bits structs for save file */
    vptr    vectors[];		/* C99 pointers to the vectors           */
  } BitVector;

#ifdef WAVE_USE_STRUCT_PACKING
#pragma pack(pop)
#endif


typedef struct
  {
    TimeType    first;		/* beginning time of trace */
    TimeType    last;		/* end time of trace */
    TimeType    start;		/* beginning time of trace on screen */
    TimeType    end;		/* ending time of trace on screen */
    TimeType    marker;
    TimeType    prevmarker;	/* from last drawmarker()	        */
    TimeType    resizemarker;   /* from last MaxSignalLength()          */
    TimeType    resizemarker2;  /* from 2nd last MaxSignalLength()      */
    TimeType    lmbcache;	/* initial marker pos                   */
    TimeType    timecache;	/* to get around floating pt limitation */
    TimeType    laststart;      /* caches last set value                */
    TimeType    baseline;       /* baseline (center button) marker      */

    gdouble    	zoom;		/* current zoom  */
    gdouble    	prevzoom;	/* for zoom undo */
  } Times;

typedef struct TraceEnt *Trptr;

typedef struct
  {
    int      total;		/* total number of traces */
    int      visible;		/* total number of (uncollapsed) traces */
    Trptr    first;		/* ptr. to first trace in list */
    Trptr    last;		/* end of list of traces */
    Trptr    buffer;		/* cut/copy buffer of traces */
    Trptr    bufferlast;	/* last element of bufferchain */
    int      buffercount;	/* number of traces in buffer */

    Trptr    scroll_top;	/* for autoscrolling */
    Trptr    scroll_bottom;
    unsigned dirty : 1;		/* to notify Tcl that traces were added/deleted/moved */
  } Traces;


typedef struct
  {
    Trptr    buffer;            /* cut/copy buffer of traces */
    Trptr    bufferlast;        /* last element of bufferchain */
    int      buffercount;       /* number of traces in buffer */
  } TempBuffer;


typedef struct TraceEnt
  {
    Trptr    t_next;		/* doubly linked list of traces */
    Trptr    t_prev;
    Trptr    t_grp;             /* pointer to group I'm in */
    Trptr    t_match;           /* If group begin pointer to group end and visa versa */

    char     *name;		/* current name */
    char     *name_full;   	/* full name */
    char     *asciivalue;	/* value that marker points to */
    char     *transaction_args; /* for TR_TTRANSLATED traces */
    TimeType asciitime;		/* time this value corresponds with */
    TimeType shift;		/* offset added to all entries in the trace */
    TimeType shift_drag;	/* cached initial offset for CTRL+LMB drag on highlighted */

    double   d_minval, d_maxval; /* cached value for when auto scaling is turned off */
    int      d_num_ext;		/* need to regen if differs from current in analog! */

    union
      {
	nptr    nd;		/* what makes up this trace */
	bvptr   vec;
      } n;

    TraceFlagsType flags;		/* see def below in TraceEntFlagBits */
    TraceFlagsType cached_flags;	/* used for tcl for saving flags during cut and paste */

    int	     f_filter;		/* file filter */
    int	     p_filter;		/* process filter */
    int	     t_filter;		/* transaction process filter */
    int      e_filter;          /* enum filter (from FST) */

    unsigned int t_color;	/* trace color index */
    unsigned char t_fpdecshift; /* for fixed point decimal */

    unsigned is_alias : 1;	/* set when it's an alias (safe to free t->name then) */
    unsigned is_depacked : 1;	/* set when it's been depacked from a compressed entry (safe to free t->name then) */
    unsigned vector : 1;	/* 1 if bit vector, 0 if node */
    unsigned shift_drag_valid : 1; /* qualifies shift_drag above */
    unsigned interactive_vector_needs_regeneration : 1; /* for interactive VCDs */
    unsigned minmax_valid : 1;	/* for d_minval, d_maxval */
    unsigned is_sort_group : 1; /* only used for sorting purposes */
    unsigned t_filter_converted : 1; /* used to mark that data conversion already occurred if t_filter != 0*/
  } TraceEnt;


enum TraceEntFlagBits
{ TR_HIGHLIGHT_B, TR_HEX_B, TR_DEC_B, TR_BIN_B,
  TR_OCT_B, TR_RJUSTIFY_B, TR_INVERT_B, TR_REVERSE_B,
  TR_EXCLUDE_B, TR_BLANK_B, TR_SIGNED_B, TR_ASCII_B,
  TR_COLLAPSED_B, TR_FTRANSLATED_B, TR_PTRANSLATED_B, TR_ANALOG_STEP_B,
  TR_ANALOG_INTERPOLATED_B, TR_ANALOG_BLANK_STRETCH_B, TR_REAL_B, TR_ANALOG_FULLSCALE_B,
  TR_ZEROFILL_B, TR_ONEFILL_B, TR_CLOSED_B, TR_GRP_BEGIN_B,
  TR_GRP_END_B,
  TR_BINGRAY_B, TR_GRAYBIN_B,
  TR_REAL2BITS_B, TR_TTRANSLATED_B,
  TR_POPCNT_B,
  TR_FPDECSHIFT_B,
  TR_TIME_B,
  TR_ENUM_B,

  TR_RSVD_B /* for use internally such as temporary caching of highlighting, not for use in traces */
};

#define TR_HIGHLIGHT 		(UINT64_C(1)<<TR_HIGHLIGHT_B)
#define TR_HEX			(UINT64_C(1)<<TR_HEX_B)
#define TR_ASCII		(UINT64_C(1)<<TR_ASCII_B)
#define TR_DEC			(UINT64_C(1)<<TR_DEC_B)
#define TR_BIN			(UINT64_C(1)<<TR_BIN_B)
#define TR_OCT			(UINT64_C(1)<<TR_OCT_B)
#define TR_RJUSTIFY		(UINT64_C(1)<<TR_RJUSTIFY_B)
#define TR_INVERT		(UINT64_C(1)<<TR_INVERT_B)
#define TR_REVERSE		(UINT64_C(1)<<TR_REVERSE_B)
#define TR_EXCLUDE		(UINT64_C(1)<<TR_EXCLUDE_B)
#define TR_BLANK		(UINT64_C(1)<<TR_BLANK_B)
#define TR_SIGNED		(UINT64_C(1)<<TR_SIGNED_B)
#define TR_ANALOG_STEP 		(UINT64_C(1)<<TR_ANALOG_STEP_B)
#define TR_ANALOG_INTERPOLATED	(UINT64_C(1)<<TR_ANALOG_INTERPOLATED_B)
#define TR_ANALOG_BLANK_STRETCH	(UINT64_C(1)<<TR_ANALOG_BLANK_STRETCH_B)
#define TR_REAL			(UINT64_C(1)<<TR_REAL_B)
#define TR_ANALOG_FULLSCALE	(UINT64_C(1)<<TR_ANALOG_FULLSCALE_B)
#define TR_ZEROFILL		(UINT64_C(1)<<TR_ZEROFILL_B)
#define TR_ONEFILL		(UINT64_C(1)<<TR_ONEFILL_B)
#define TR_CLOSED		(UINT64_C(1)<<TR_CLOSED_B)

#define TR_GRP_BEGIN		(UINT64_C(1)<<TR_GRP_BEGIN_B)
#define TR_GRP_END		(UINT64_C(1)<<TR_GRP_END_B)
#define TR_GRP_MASK		(TR_GRP_BEGIN|TR_GRP_END)

#define TR_BINGRAY		(UINT64_C(1)<<TR_BINGRAY_B)
#define TR_GRAYBIN		(UINT64_C(1)<<TR_GRAYBIN_B)
#define TR_GRAYMASK		(TR_BINGRAY|TR_GRAYBIN)

#define TR_REAL2BITS            (UINT64_C(1)<<TR_REAL2BITS_B)

#define TR_NUMMASK	(TR_ASCII|TR_HEX|TR_DEC|TR_BIN|TR_OCT|TR_SIGNED|TR_REAL|TR_TIME|TR_ENUM)

#define TR_COLLAPSED	(UINT64_C(1)<<TR_COLLAPSED_B)
#define TR_ISCOLLAPSED	(TR_BLANK|TR_COLLAPSED)

#define TR_FTRANSLATED	(UINT64_C(1)<<TR_FTRANSLATED_B)
#define TR_PTRANSLATED	(UINT64_C(1)<<TR_PTRANSLATED_B)
#define TR_TTRANSLATED  (UINT64_C(1)<<TR_TTRANSLATED_B)

#define TR_POPCNT	(UINT64_C(1)<<TR_POPCNT_B)
#define TR_FPDECSHIFT   (UINT64_C(1)<<TR_FPDECSHIFT_B)

#define TR_TIME         (UINT64_C(1)<<TR_TIME_B)
#define TR_ENUM         (UINT64_C(1)<<TR_ENUM_B)

#define TR_ANALOGMASK	(TR_ANALOG_STEP|TR_ANALOG_INTERPOLATED)

#define TR_RSVD		(UINT64_C(1)<<TR_RSVD_B)

Trptr GiveNextTrace(Trptr t);
Trptr GivePrevTrace(Trptr t);
int UpdateTracesVisible(void);

void DisplayTraces(int val);
int AddNodeTraceReturn(nptr nd, char *aliasname, Trptr *tret);
int AddNode(nptr nd, char *aliasname);
int AddNodeUnroll(nptr nd, char *aliasname);
int AddVector(bvptr vec, char *aliasname);
int AddBlankTrace(char *commentname);
int InsertBlankTrace(char *comment, TraceFlagsType different_flags);
void RemoveNode(nptr n);
void RemoveTrace(Trptr t, int dofree);
void FreeTrace(Trptr t);
Trptr CutBuffer(void);
void FreeCutBuffer(void);
Trptr PasteBuffer(void);
Trptr PrependBuffer(void);
int TracesReorder(int mode);
int DeleteBuffer(void);

void import_trace(nptr np);

eptr ExpandNode(nptr n);
void DeleteNode(nptr n);
nptr ExtractNodeSingleBit(nptr n, int bit);



/* hierarchy depths */
char *hier_extract(char *pnt, int levels);

/* vector matching */
char *attempt_vecmatch(char *s1, char *s2);

void updateTraceGroup(Trptr t);
int GetTraceNumber(Trptr t);
void EnsureGroupsMatch(void);

#define IsSelected(t)   (t->flags&TR_HIGHLIGHT)
#define IsGroupBegin(t) (t->flags&TR_GRP_BEGIN)
#define IsGroupEnd(t)   (t->flags&TR_GRP_END)
#define IsClosed(t)     (t->flags&TR_CLOSED)
#define HasWave(t)      (!(t->flags&(TR_BLANK|TR_ANALOG_BLANK_STRETCH)))
#define CanAlias(t)     HasWave(t)
#define HasAlias(t)     (t->name_full&&HasWave(t))
#define IsCollapsed(t)  (t->flags&TR_COLLAPSED)

unsigned IsShadowed(Trptr t);
char*    GetFullName(Trptr t, int *was_packed);

void OpenTrace(Trptr t);
void CloseTrace(Trptr t);
void ClearTraces(void);
void ClearGroupTraces(Trptr t);

char *varxt_fix(char *s);

#endif
