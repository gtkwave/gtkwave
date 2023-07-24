/*

FST dumper for NC Verilog / Verilog-XL
to compile/run under AIX: 

ar -xv /lib/libz.a   # to get libz.so.1
xlc -O3 -c sys_fst.c fstapi.c fastlz.c
ld -G -o sys_fst.so sys_fst.o fstapi.o fastlz.o libz.so.1 -bnoentry -bexpall -lld -lc

[nc]verilog r.v +loadvpi=sys_fst.so:sys_fst_register +access+r


FST dumper for VCS
to compile/run under LINUX:

gcc -O2 -c -fPIC *.c
ld -G -o sys_fst.so ../../src/libz/*.o *.o
vcs +v2k -R +vpi +acc+2 +memchbk -full64 t.v -P sys_fst.tab sys_fst.so

sys_fst.tab:

$fstdumpfile check=sys_dumpfile_compiletf call=sys_dumpfile_calltf acc+=r:*
$fstdumpvars check=sys_dumpvars_compiletf call=sys_dumpvars_calltf acc+=r:*
$fstdumpoff  check=sys_dumpoff_compiletf  call=sys_dumpoff_calltf  acc+=r:*

 */

#include  <vpi_user.h>
#include  <acc_user.h>
#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include  <assert.h>
#include  <time.h>
#include  <inttypes.h>
#include  "fstapi.h"

struct fst_info {
    struct fst_info *dump_chain;
    vpiHandle       item;
    s_vpi_value     value;

    fstHandle       fstSym;
    unsigned        is_real:1;
    unsigned        is_changed:1;
};

/*************************************************/

static struct fstContext *ctx = NULL;
static uint64_t        prev64 = 0;

static char    *dump_path = NULL;

static int      dump_is_off = 0;

static struct fst_info *fst_dump_list = 0;

static int      dumpvars_status = 0;	/* 0:fresh 1:cb installed,
					 * 2:callback done */
static uint64_t dumpvars_time;


static          uint64_t
timerec_to_time64(s_vpi_time * vt)
{
    uint64_t      hi = vt->high;
    uint64_t      lo = vt->low;

    return ((hi << 32) | lo);
}


static int
dump_header_pending(void)
{
    return (dumpvars_status != 2);
}


int variable_cb_rosync(p_cb_data cause);


int
variable_cb_rosync(p_cb_data cause)
{
    p_vpi_time      tim = cause->time;
    uint64_t        now64 = timerec_to_time64(tim);
    struct fst_info *a_info = fst_dump_list;
    s_vpi_value     value;

   if((now64 > prev64) || (!now64))
	{
	fstWriterEmitTimeChange(ctx, now64);
    	prev64 = now64;
	}

    while (a_info) {
	if (!a_info->is_real) {
	    value.value.str = NULL;
	    value.format = vpiBinStrVal;

	    vpi_get_value(a_info->item, &value);
	    fstWriterEmitValueChange(ctx, a_info->fstSym, value.value.str);
	} else {
	    double          d;
	    value.format = vpiRealVal;

	    vpi_get_value(a_info->item, &value);
	    d = value.value.real;
	    fstWriterEmitValueChange(ctx, a_info->fstSym, &d);
	}


	a_info->is_changed = 0;
	struct fst_info *a_info_next = a_info->dump_chain;
	a_info->dump_chain = NULL;
	a_info = a_info_next;
    }

    fst_dump_list = NULL;
    return (0);

}


static void
install_rosync_cb(void)
{
    struct t_cb_data cb;
    struct t_vpi_time time;

    memset(&cb, 0, sizeof(cb));
    memset(&time, 0, sizeof(time));
    time.type = vpiSimTime;

    cb.time = &time;
    cb.reason = cbReadOnlySynch;
    cb.cb_rtn = variable_cb_rosync;
    cb.user_data = NULL;
    cb.obj = NULL;

    vpi_free_object(vpi_register_cb(&cb));
}


int
variable_cb(p_cb_data cause)
{
    struct fst_info *info = (struct fst_info *) cause->user_data;

    if (dump_is_off)
	return (0);
    if (dump_header_pending())
	return (0);
    if (info->is_changed) 
	return (0);

    if (!fst_dump_list) {
	install_rosync_cb();
    }
    info->is_changed = 1;
    info->dump_chain = fst_dump_list;
    fst_dump_list = info;

    return (0);
}


static int
dumpvars_cb(p_cb_data cause)
{
    if (dumpvars_status != 1)
	return (0);

    dumpvars_status = 2;

    dumpvars_time = timerec_to_time64(cause->time);

    return (0);
}


static int
install_dumpvars_callback(void)
{
    struct t_cb_data cb;
    struct t_vpi_time time;

    if (dumpvars_status == 1)
	return (0);

    if (dumpvars_status == 2) {
	vpi_mcd_printf(1, "Error:"
		       " $fstdumpvars ignored,"
		       " previously called at simtime %"PRIu64
		       "\n", dumpvars_time);
	return (1);
    }
    memset(&cb, 0, sizeof(cb));
    memset(&time, 0, sizeof(time));
    time.type = vpiSimTime;
    cb.time = &time;
    cb.reason = cbReadOnlySynch;
    cb.cb_rtn = dumpvars_cb;
    cb.user_data = NULL;
    cb.obj = NULL;

    vpi_free_object(vpi_register_cb(&cb));

    dumpvars_status = 1;
    return (0);
}


static int
end_of_sim_cb(p_cb_data cause)
{
    if (ctx) 	
	{
	fstWriterClose(ctx);
	ctx = NULL;
	prev64 = 0;
    	}
    return (0);
}

static int
next_time_cb(p_cb_data cause)
{
struct t_cb_data cb;
struct t_vpi_time vtime;

p_vpi_time      tim = cause->time;
uint64_t        now64 = timerec_to_time64(tim);

if(now64 > prev64)
	{
	fstWriterEmitTimeChange(ctx, now64);
	prev64 = now64;
	}

memset(&cb, 0, sizeof(cb));
memset(&vtime, 0, sizeof(vtime));

vtime.type = vpiSimTime;
cb.time = &vtime;
cb.reason = cbNextSimTime;
cb.cb_rtn = next_time_cb;
cb.user_data = NULL;
cb.obj = NULL;

vpi_free_object(vpi_register_cb(&cb));

return (0);
}


static void
open_dumpfile(void)
{
    struct t_cb_data cb;
    struct t_vpi_time vtime;

    if (dump_path == NULL) {
	dump_path = strdup("dump.fst");
    }
    {
	time_t          walltime;

	/*
	 * primary 
	 */
	ctx = fstWriterCreate(dump_path, 1);
	prev64 = 0;
	fstWriterSetPackType(ctx, FST_WR_PT_LZ4);
	/* fstWriterSetParallelMode(ctx, 1); */

	time(&walltime);
	fstWriterSetDate(ctx, asctime(localtime(&walltime)));

	fstWriterSetVersion(ctx, acc_product_version());

	free(dump_path);
	dump_path = NULL;

	memset(&cb, 0, sizeof(cb));
	memset(&vtime, 0, sizeof(vtime));

	vtime.type = vpiSimTime;
	cb.time = &vtime;
	cb.reason = cbEndOfSimulation;
	cb.cb_rtn = end_of_sim_cb;
	cb.user_data = NULL;
	cb.obj = NULL;

	vpi_free_object(vpi_register_cb(&cb));

	memset(&cb, 0, sizeof(cb));
	memset(&vtime, 0, sizeof(vtime));

	vtime.type = vpiSimTime;
	cb.time = &vtime;
	cb.reason = cbNextSimTime;
	cb.cb_rtn = next_time_cb;
	cb.user_data = NULL;
	cb.obj = NULL;

	vpi_free_object(vpi_register_cb(&cb));
    }
}


int
sys_dumpfile_compiletf(char *name)
{
    vpiHandle       sys = vpi_handle(vpiSysTfCall, 0);
    vpiHandle       argv = vpi_iterate(vpiArgument, sys);
    vpiHandle       item;

    char           *path;

    if (argv && (item = vpi_scan(argv))) {
	s_vpi_value     value;

	if (vpi_get(vpiType, item) != vpiConstant
	    || vpi_get(vpiConstType, item) != vpiStringConst) {
	    vpi_mcd_printf(1,
			   "FST Error:"
			   " %s parameter must be a string constant\n",
			   name);
	    return (0);
	}
	value.format = vpiStringVal;
	vpi_get_value(item, &value);
	path = strdup(value.value.str);

	vpi_free_object(argv);
    } else {
	path = strdup("dump.fst");
    }

    if (dump_path) {
	vpi_mcd_printf(1, "FST Warning:"
		       " Overriding dumpfile path %s with %s\n",
		       dump_path, path);
	free(dump_path);
    }
    dump_path = path;

    return (0);
}


int
sys_dumpfile_calltf(char *name)
{
    return (0);
}


static int
draw_module_type(vpiHandle item, int typ)
{
    vpiHandle       iter = vpi_iterate(typ, item);
    vpiHandle       net;
    const char     *name;
    struct t_cb_data cb;
    struct fst_info *info;
    struct t_vpi_time time;
    int             vtyp;
    int             ilrange,
                    irrange;

    if (!iter)
	return (0);

    while ((net = vpi_scan(iter))) {
	int             siz;

	info = calloc(1, sizeof(*info));

	if (typ == vpiVariables) {
	    siz = vpi_get(vpiSize, net);
	    ilrange = siz - 1;
	    irrange = 0;
	} else if (vpi_get(vpiVector, net)) {
	    s_vpi_value     lvalue,
	                    rvalue;
	    vpiHandle       lrange = vpi_handle(vpiLeftRange, net);
	    vpiHandle       rrange = vpi_handle(vpiRightRange, net);

	    lvalue.value.integer = 0;
	    lvalue.format = vpiIntVal;
	    vpi_get_value(lrange, &lvalue);

	    rvalue.value.integer = 0;
	    rvalue.format = vpiIntVal;
	    vpi_get_value(rrange, &rvalue);

	    siz = vpi_get(vpiSize, net);

	    ilrange = lvalue.value.integer;
	    irrange = rvalue.value.integer;
	} else {
	    siz = 1;
	    ilrange = irrange = -1;
	}

	name = vpi_get_str(vpiName, net);

	switch (typ) {
	case vpiNet:
	    {
		int             vartype = vpi_get(vpiNetType, net);
		switch (vartype) {
		case vpiWand:
		    vtyp = FST_VT_VCD_WAND;
		    break;
		case vpiWor:
		    vtyp = FST_VT_VCD_WOR;
		    break;
		case vpiTri:
		    vtyp = FST_VT_VCD_TRI;
		    break;
		case vpiTri0:
		    vtyp = FST_VT_VCD_TRI0;
		    break;
		case vpiTri1:
		    vtyp = FST_VT_VCD_TRI1;
		    break;
		case vpiTriReg:
		    vtyp = FST_VT_VCD_TRIREG;
		    break;
		case vpiTriAnd:
		    vtyp = FST_VT_VCD_TRIAND;
		    break;
		case vpiTriOr:
		    vtyp = FST_VT_VCD_TRIOR;
		    break;
		case vpiSupply0:
		    vtyp = FST_VT_VCD_SUPPLY0;
		    break;
		case vpiSupply1:
		    vtyp = FST_VT_VCD_SUPPLY1;
		    break;
		case vpiWire:
		default:
		    vtyp = FST_VT_VCD_WIRE;
		    break;
		}
		break;
	    }
	case vpiReg:
	    vtyp = FST_VT_VCD_REG;
	    break;
	case vpiVariables:
	    {
		int             vartype = vpi_get(vpiType, net);
		switch (vartype) {
		case vpiTimeVar:
		    vtyp = FST_VT_VCD_TIME;
		    break;
		case vpiRealVar:
		    vtyp = FST_VT_VCD_REAL;
		    info->is_real = 1;
		    break;
		case vpiIntegerVar:
		default:
		    vtyp = FST_VT_VCD_INTEGER;
		    break;
		}
	    }
	    break;
	default:
	    vtyp = FST_VT_VCD_WIRE;
	    break;
	}

	if (((ilrange == -1) && (irrange == -1)) || (typ == vpiVariables)) {
	    info->fstSym =
		fstWriterCreateVar(ctx, vtyp, FST_VD_IMPLICIT, siz, name,
				   0);
	} else {
	    char           *n2 = malloc(strlen(name) + 64);
	    int len = ilrange - irrange;
	    if(len < 0) len = - len;
	    len++;

	    if (ilrange == irrange) {
		if(siz == len)
			{
			sprintf(n2, "%s [%d]", name, irrange);
			}
			else
			{
			sprintf(n2, "%s [%d][%d:0]", name, irrange, siz/len-1);
			}
	    } else {
		if(siz == len)
			{
			sprintf(n2, "%s [%d:%d]", name, ilrange, irrange);
			}
			else
			{
			sprintf(n2, "%s [%d:%d][%d:0]", name, ilrange, irrange, siz/len-1);
			}
	    }

	    info->fstSym =
		fstWriterCreateVar(ctx, vtyp, FST_VD_IMPLICIT, siz, n2, 0);
	    free(n2);
	}

	info->item = net;
	info->is_changed = 1;

	info->dump_chain = fst_dump_list;
    	fst_dump_list = info;


	memset(&cb, 0, sizeof(cb));
	memset(&time, 0, sizeof(time));
	time.type = vpiSimTime;

	cb.time = &time;
	cb.user_data = (char *) info;

	cb.value = &info->value; /* NC seems to be ok with NULL, but VCS needs &info->value */
	info->value.format = vpiObjTypeVal;
	cb.obj = net;
	cb.reason = cbValueChange;
	cb.cb_rtn = variable_cb;

	vpi_free_object(vpi_register_cb(&cb));
    }

    return (0);
}


static int
draw_module(vpiHandle item, int typ)
{
    if (typ == vpiModule) {
	draw_module_type(item, vpiNet);
    }
    draw_module_type(item, vpiReg);
    draw_module_type(item, vpiVariables);

    return (0);
}


static int
draw_scope_fst(vpiHandle item, int depth, int depth_max)
{
    const char     *fstscopnam;
    char           *defname = NULL;
    vpiHandle       orig = item;

    if ((depth_max) && (depth >= depth_max))
	return (0);

    if (depth == 0) {
	int             vpitype = vpi_get(vpiType, item);
	int             fsttype;

        int lineno = vpi_get(vpiLineNo, item);
	const char *fname = vpi_get_str(vpiFile, item);
	fstWriterSetSourceInstantiationStem(ctx, fname, lineno, 1);

        lineno = vpi_get(vpiDefLineNo, item);
	fname = vpi_get_str(vpiDefFile, item);
	fstWriterSetSourceStem(ctx, fname, lineno, 1);

	switch (vpitype) {
	case vpiTaskFunc:
	case vpiTask:
	    fsttype = FST_ST_VCD_TASK;
	    break;
	case vpiFunction:
	    fsttype = FST_ST_VCD_FUNCTION;
	    break;
	case vpiNamedBegin:
	    fsttype = FST_ST_VCD_BEGIN;
	    break;
	case vpiNamedFork:
	    fsttype = FST_ST_VCD_FORK;
	    break;
	case vpiModule:
	default:
	    fsttype = FST_ST_VCD_MODULE;
	    defname = strdup(vpi_get_str(vpiDefName, item));
	    break;
	}

	fstscopnam = vpi_get_str(vpiName, item);
	if(defname && !strcmp(defname, fstscopnam)) { free(defname); defname = NULL; } /* no sense in storing a duplicate name */
	fstWriterSetScope(ctx, fsttype, fstscopnam, defname);
        if(defname) free(defname);

	draw_module(item, vpitype);
	if (vpitype == vpiModule) {
	    draw_scope_fst(item, depth + 1, depth_max);
	}

	fstWriterSetUpscope(ctx);
    } else {
	vpiHandle       iter = vpi_iterate(vpiInternalScope, orig);

	if (iter)
	    while ((item = vpi_scan(iter))) {
		int             vpitype = vpi_get(vpiType, item);
		int             fsttype;

	        int lineno = vpi_get(vpiLineNo, item);
		const char *fname = vpi_get_str(vpiFile, item);
		fstWriterSetSourceInstantiationStem(ctx, fname, lineno, 1);

	        lineno = vpi_get(vpiDefLineNo, item);
		fname = vpi_get_str(vpiDefFile, item);
		fstWriterSetSourceStem(ctx, fname, lineno, 1);

		switch (vpitype) {
		case vpiTaskFunc:
		case vpiTask:
		    fsttype = FST_ST_VCD_TASK;
		    break;
		case vpiFunction:
		    fsttype = FST_ST_VCD_FUNCTION;
		    break;
		case vpiNamedBegin:
		    fsttype = FST_ST_VCD_BEGIN;
		    break;
		case vpiNamedFork:
		    fsttype = FST_ST_VCD_FORK;
		    break;
		case vpiModule:
		default:
		    fsttype = FST_ST_VCD_MODULE;
	    	    defname = strdup(vpi_get_str(vpiDefName, item));
		    break;
		}


		fstscopnam = vpi_get_str(vpiName, item);
		if(defname && !strcmp(defname, fstscopnam)) { free(defname); defname = NULL; } /* no sense in storing a duplicate name */
		fstWriterSetScope(ctx, fsttype, fstscopnam, defname);
		if(defname) free(defname);

		draw_module(item, vpitype);

		if (vpitype == vpiModule) {
		    draw_scope_fst(item, depth + 1, depth_max);
		}
		fstWriterSetUpscope(ctx);
	    }
    }

    return (0);
}


/*
 * This function is also used in sys_fst to check the arguments of the fst
 * variant of $dumpvars. 
 */
int
sys_dumpvars_compiletf(char *name)
{
    vpiHandle       sys = vpi_handle(vpiSysTfCall, 0);
    vpiHandle       argv = vpi_iterate(vpiArgument, sys);
    vpiHandle       tmp;

    if (argv == 0)
	return (0);

    tmp = vpi_scan(argv);
    assert(tmp);

    switch (vpi_get(vpiType, tmp)) {
    case vpiConstant:
	if (vpi_get(vpiConstType, tmp) == vpiStringConst) {
	    vpi_printf("ERROR: %s argument must be "
		       "a number constant.\n", name);
	    vpi_control(vpiFinish, 1);
	}
	break;

    case vpiNet:
    case vpiReg:
    case vpiIntegerVar:
    case vpiMemoryWord:
	break;

    default:
	vpi_printf("ERROR: %s argument must be " "a number constant.\n",
		   name);
	vpi_control(vpiFinish, 1);
	break;
    }

    vpi_free_object(argv);
    return (0);
}


int
sys_dumpvars_calltf(char *name)
{
    unsigned        depth;
    s_vpi_value     value;
    vpiHandle       item = 0;
    vpiHandle       sys = vpi_handle(vpiSysTfCall, 0);
    vpiHandle       argv;

    if (ctx == 0) {
	open_dumpfile();
	if (ctx == 0)
	    return (0);
    }
    if (install_dumpvars_callback()) {
	return (0);
    }
    argv = vpi_iterate(vpiArgument, sys);

    depth = 0;
    if (argv && (item = vpi_scan(argv)))
	switch (vpi_get(vpiType, item)) {
	case vpiConstant:
	case vpiNet:
	case vpiReg:
	case vpiIntegerVar:
	case vpiMemoryWord:
	    value.format = vpiIntVal;
	    vpi_get_value(item, &value);
	    depth = value.value.integer;
	    break;
	}

    if (!argv) {
	vpiHandle       parent = vpi_handle(vpiScope, sys);
	while (parent) {
	    item = parent;
	    parent = vpi_handle(vpiScope, item);
	}

    } else if (!item || !(item = vpi_scan(argv))) {
	item = vpi_handle(vpiScope, sys);
	argv = 0x0;
    }
    for (; item; item = argv ? vpi_scan(argv) : 0x0) {
	draw_scope_fst(item, 0, depth);
    }


    {
	struct fst_info *a_info;
	int             prec = vpi_get(vpiTimePrecision, 0);

	fstWriterSetTimescale(ctx, prec);
	fstWriterEmitTimeChange(ctx, 0);

	install_rosync_cb();
    }

    return (0);
}


int
sys_dumpoff_compiletf(char *name)
{
    return (0);
}


int
sys_dumpoff_calltf(char *name)
{
    dump_is_off = 1;

    return (0);
}


void
sys_fst_register()
{
    s_vpi_systf_data tf_data;

    tf_data.type = vpiSysTask;
    tf_data.tfname = "$fstdumpfile";
    tf_data.calltf = sys_dumpfile_calltf;
    tf_data.compiletf = sys_dumpfile_compiletf;
    tf_data.sizetf = 0;
    tf_data.user_data = "$fstdumpfile";
    vpi_register_systf(&tf_data);

    tf_data.type = vpiSysTask;
    tf_data.tfname = "$fstdumpvars";
    tf_data.calltf = sys_dumpvars_calltf;
    tf_data.compiletf = sys_dumpvars_compiletf;
    tf_data.sizetf = 0;
    tf_data.user_data = "$fstdumpvars";
    vpi_register_systf(&tf_data);

    tf_data.type = vpiSysTask;
    tf_data.tfname = "$fstdumpoff";
    tf_data.calltf = sys_dumpoff_calltf;
    tf_data.compiletf = sys_dumpoff_compiletf;
    tf_data.sizetf = 0;
    tf_data.user_data = "$fstdumpoff";
    vpi_register_systf(&tf_data);
}
