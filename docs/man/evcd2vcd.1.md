---
date: 3.2.2
myst:
  title_to_header: true
section: 1
title: evcd2vcd
---

## NAME

evcd2vcd - Converts EVCD files to VCD files

## SYNTAX

evcd2vcd \[*option*\]\... \[*EVCDFILE*\]

## DESCRIPTION

Converts EVCD files with bidirectional port definitions to VCD files
with separate in and out ports.

## OPTIONS

**-f,\--filename** \<*filename*\>

:   Specify EVCD input filename.

**-h,\--help**

:   Show help screen.

## EXAMPLES

Note that you should specify dumpfile.vcd directly or use \"-\" for
stdin.

evcd2vcd dumpfile.evcd

:   VCD is emitted to stdout.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*vcd2fst*(1) *fst2vcd*(1) *vcd2lxt*(1) *vcd2lxt2*(1) *lxt2vcd*(1)
*vcd2vzt*(1) *vzt2vcd*(1) *gtkwave*(1)
