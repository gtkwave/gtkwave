---
date: 3.3.39
myst:
  title_to_header: true
section: 1
title: vzt2vcd
---

## NAME

vzt2vcd - Converts VZT files to VCD

## SYNTAX

vzt2vcd \[*option*\]\... \[*VZTFILE*\]

## DESCRIPTION

Converts VZT files to VCD files. If an output filename is not specified,
the VCD is emitted to stdout.

## OPTIONS

**-v,\--vztname** \<*filename*\>

:   Specify VZT input filename.

**-o,\--output** \<*filename*\>

:   Specify optional VCD output filename.

**-f,\--flatearth**

:   Emit flattened hierarchies.

**-c,\--coalesce**

:   Coalesce bitblasted vectors.

**-n,\--notruncate**

:   Do not shorten bitvectors. This disables binary value compression as
    described in the IEEE-1364 specification. (i.e., all values except
    for \'1\' left propagate as a sign bit on vectors which do not fill
    up their entire declared width)

**-h,\--help**

:   Display help then exit.

## EXAMPLES

To run this program the standard way type:

vzt2vcd filename.vzt

:   The VCD conversion is emitted to stdout.

## LIMITATIONS

*vzt2vcd* does not re-create glitches as these are coalesced together
into one value change during the writing of the VZT file.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*vcd2lxt2*(1) *vcd2lxt*(1) *lxt2vcd*(1) *gtkwave*(1)
