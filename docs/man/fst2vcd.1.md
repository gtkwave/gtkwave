---
date: 3.3.52
myst:
  title_to_header: true
section: 1
title: fst2vcd
---

## NAME

fst2vcd - Converts FST files to VCD

## SYNTAX

fst2vcd \[*option*\]\... \[*FSTFILE*\]

## DESCRIPTION

Converts FST files to VCD files. If an output filename is not specified,
the VCD is emitted to stdout.

## OPTIONS

**-f,\--fstname** \<*filename*\>

:   Specify FST input filename.

**-o,\--output** \<*filename*\>

:   Specify optional VCD output filename.

**-e,\--extensionsFr**

:   Emit FST extensions to VCD. Enabling this may create VCD files
    unreadable by other tools. This is generally intended to be used as
    a debugging tool when developing FST writer interfaces to
    simulators.

**-h,\--help**

:   Display help then exit.

## EXAMPLES

To run this program the standard way type:

fst2vcd filename.fst

:   The VCD conversion is emitted to stdout.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*vcd2fst*(1) *gtkwave*(1)
