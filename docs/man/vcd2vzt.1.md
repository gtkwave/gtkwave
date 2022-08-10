---
date: 3.1.21
myst:
  title_to_header: true
section: 1
title: vcd2vzt
---

## NAME

vcd2vzt - Converts VCD files to VZT files

## SYNTAX

vcd2vzt \[*option*\]\... \[*VCDFILE*\] \[*VZTFILE*\]

## DESCRIPTION

Converts VCD files to VZT files.

## OPTIONS

**-v,\--vcdname** \<*filename*\>

:   Specify VCD input filename.

**-l,\--vztname** \<*filename*\>

:   Specify VZT output filename.

**-d,\--depth** \<*value*\>

:   Specify 0..9 gzip compression depth, default is 4.

**-m,\--maxgranule** \<*value*\>

:   Specify number of granules per section, default is 8. One granule is
    equal to 32 timesteps.

**-b,\--break** \<*value*\>

:   Specify break size (default = 0 = off). When the break size is
    exceeded, the VZT dumper will dump all state information at the next
    convenient granule plus dictionary boundary.

**-z,\--ziptype** \<*value*\>

:   Specify zip type (default = 0 gzip, 1 = bzip2, 2 = lzma). This
    allows you to override the default compression algorithm to use a
    more effective one at the expense of greater runtime. Note that
    bzip2 does not decompress as fast as gzip so the viewer will be
    about two times slower when decompressing blocks.

**-t,\--twostate**

:   Forces MVL2 twostate mode (default is MVL4). When enabled, the trace
    will only store 0/1 values for binary facilities. This is useful for
    functional simulation and will speed up dumping as well as make
    traces somewhat smaller.

**-r, \--rle**

:   Uses an bitwise RLE compression on the value table. Default is off.
    When enabled, this causes the trace data table to be stored using an
    alternate representation which can improve compression in many
    cases.

**-h,\--help**

:   Show help screen.

## EXAMPLES

Note that you should specify dumpfile.vcd directly or use \"-\" for
stdin.

vcd2vzt dumpfile.vcd dumpfile.lxt \--depth 9 \--break 1073741824

:   This sets the compression level to 9 and sets the break size to 1GB.

vcd2vzt dumpfile.vcd dumpfile.lxt \--depth 9 \--maxgranule 512

:   Allows more granules per section which allows for greater
    compression at the expense of memory usage.

## LIMITATIONS

*vcd2vzt* does not store glitches as these are coalesced together into
one value change during the writing of the VZT file.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*vzt2vcd*(1) *lxt2vcd*(1) *vcd2lxt2*(1) *gtkwave*(1)
