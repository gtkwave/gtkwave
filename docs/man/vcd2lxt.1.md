---
date: 1.3.34
myst:
  title_to_header: true
section: 1
title: vcd2lxt
---

## NAME

vcd2lxt - Converts VCD files to interlaced or linear LXT files

## SYNTAX

vcd2lxt \[*VCDFILE*\] \[*LXTFILE*\] \[*option*\]\...

## DESCRIPTION

Converts VCD files to interlaced or linear LXT files. Noncompressed
interlaced files will provide the fastest access, linear files will
provide the slowest yet have the greatest compression ratios.

## OPTIONS

**-stats**

:   Prints out statistics on all nets in VCD file in addition to
    performing the conversion.

**-clockpack**

:   Apply two-way subtraction algorithm in order to identify nets whose
    value changes by a constant XOR or whose value increases/decreases
    by a constant amount per constant unit of time. This option can
    reduce dumpfile size dramatically as value changes can be
    represented by an equation rather than explicitly as a triple of
    time, net, and value.

**-chgpack**

:   Emit data to file after being filtered through zlib (gzip).

**-linear**

:   Write out LXT in \"linear\" format with no backpointers. These are
    re-generated during initialization in *gtkwave*. Additionally, use
    libbz2 (bzip2) as the compression filter.

**-dictpack** \<*size*\>

:   Store value changes greater than or equal to *size* bits as an index
    into a dictionary. Experimentation shows that a value of 18 is
    optimal for most cases.

## EXAMPLES

Note that you should specify dumpfile.vcd directly or use \"-\" for
stdin.

vcd2lxt dumpfile.vcd dumpfile.lxt -clockpack -chgpack -dictpack 18

:   This turns on clock packing, zlib compression, and enables the
    dictionary encoding. Note that using no options writes out a normal
    LXT file.

vcd2lxt dumpfile.vcd dumpfile.lxt -clockpack -linear -dictpack 18

:   Uses linear mode for even smaller files.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*lxt2vcd*(1) *vcd2lxt2*(1) *gtkwave*(1)
