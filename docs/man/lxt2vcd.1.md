---
date: 3.3.39
myst:
  title_to_header: true
section: 1
title: lxt2vcd
---

## NAME

lxt2vcd - Converts LXT2 files to VCD

## SYNTAX

lxt2vcd \[*option*\]\... \[*LXT2FILE*\]

## DESCRIPTION

Converts LXT2 files to VCD files on stdout. If an output filename is not
specified, the VCD is emitted to stdout. Note that \"regular\" LXT2
files will convert to VCD files with monotonically increasing time
values. LXT2 files which are dumped with the \"partial\" option (to
speed up access in wave viewers) will dump with monotonically increasing
time values per 2k block of nets. This may be fixed in later versions of
*lxt2vcd*.

## OPTIONS

**-l,\--lxtname** \<*filename*\>

:   Specify LXT2 input filename.

**-o,\--output** \<*filename*\>

:   Specify optional VCD output filename.

**-f,\--flatearth**

:   Emit flattened hierarchies.

```{=html}
<!-- -->
```

**-n,\--notruncate**

:   Do not shorten bitvectors. This disables binary value compression as
    described in the IEEE-1364 specification. (i.e., all values except
    for \'1\' left propagate as a sign bit on vectors which do not fill
    up their entire declared width)

**-h,\--help**

:   Display help then exit.

## EXAMPLES

To run this program the standard way type:

lxt2vcd filename.lxt

:   The VCD conversion is emitted to stdout.

## LIMITATIONS

*lxt2vcd* does not re-create glitches as these are coalesced together
into one value change during the writing of the LXT2 file.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*vcd2lxt2*(1) *vcd2lxt*(1) *gtkwave*(1)
