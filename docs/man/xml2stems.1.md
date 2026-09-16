---
date: 3.3.93
myst:
  title_to_header: true
section: 1
title: xml2stems
---

## NAME

xml2stems - Verilator XML to rtlbrowse stems conversion.

## SYNTAX

xml2stems \[*option*\]\... *INFILE* \[*OUTFILE*\]

## DESCRIPTION

Converts Verilator XML AST representation to stems file for use with
rtlbrowse. Intended to replace the obsolete vermin, and in the future,
advanced tool features may possibly use additional data provided by the
Verilator AST. Using \"-\" as an input filename reads from stdin.
Omitting the output filename outputs to stdout.

## OPTIONS

**-V**,**\--vl_sim**

:   Adds TOP hierarchy for Verilator sim. Not necessary for other
    simulators.

**-h**,**\--help**

:   Display help then exit.

## EXAMPLES

The following is a chain of commands to compile Verilog source and then
bring up gtkwave and rtlbrowse together for source code annotation. This
assumes the file des.fst was already generated from a prior simulation.

verilator -Wno-fatal des.v -xml-only \--bbox-sys

:   

xml2stems obj_dir/Vdes.xml des.stems

:   

gtkwave -t des.stems des.fst

:   

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*rtlbrowse*(1) *gtkwave*(1)
