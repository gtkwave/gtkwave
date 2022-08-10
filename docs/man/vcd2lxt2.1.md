---
date: 1.3.42
myst:
  title_to_header: true
section: 1
title: vcd2lxt2
---

## NAME

vcd2lxt2 - Converts VCD files to LXT2 files

## SYNTAX

vcd2lxt2 \[*option*\]\... \[*VCDFILE*\] \[*LXTFILE*\]

## DESCRIPTION

Converts VCD files to LXT2 files.

## OPTIONS

**-v,\--vcdname** \<*filename*\>

:   Specify VCD input filename.

**-l,\--lxtname** \<*filename*\>

:   Specify LXT2 output filename.

**-d,\--depth** \<*value*\>

:   Specify 0..9 gzip compression depth, default is 4.

**-m,\--maxgranule** \<*value*\>

:   Specify number of granules per section, default is 8. One granule is
    equal to 32 timsteps.

**-b,\--break** \<*value*\>

:   Specify break size (default = 0 = off). When the break size is
    exceeded, the LXT2 dumper will dump all state information at the
    next convenient granule plus dictionary boundary.

**-p,\--partialmode** \<*mode*\>

:   Specify partial zip mode 0 = monolithic, 1 = separation. Using a
    value of 1 expands LXT2 filesize but provides fast access for very
    large traces. Note that the default mode is neither monolithic nor
    separation: partial zip is disabled.

**-c,\--checkpoint** \<*mode*\>

:   Specify checkpoint mode. 0 is on which is default, and 1 is off.
    This is disabled when the break size is active.

**-h,\--help**

:   Show help screen.

## EXAMPLES

Note that you should specify dumpfile.vcd directly or use \"-\" for
stdin.

vcd2lxt dumpfile.vcd dumpfile.lxt \--depth 9 \--break 1073741824

:   This sets the compression level to 9 and sets the break size to 1GB.

vcd2lxt dumpfile.vcd dumpfile.lxt \--depth 9 \--maxgranule 256

:   Allows more granules per section which allows for greater
    compression.

## LIMITATIONS

*vcd2lxt2* does not store glitches as these are coalesced together into
one value change during the writing of the LXT2 file.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*lxt2vcd*(1) *vcd2lxt2*(1) *gtkwave*(1)
