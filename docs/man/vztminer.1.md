---
date: 3.2.1
myst:
  title_to_header: true
section: 1
title: vztminer
---

## NAME

vztminer - Data mining of VZT files

## SYNTAX

vztminer \[*option*\]\... \[*VZTFILE*\]

## DESCRIPTION

Mines VZT files for specific data values and generates gtkwave save
files to stdout for future reload.

## OPTIONS

**-d,\--dumpfile** \<*filename*\>

:   Specify VZT input dumpfile.

**-m,\--match** \<*value*\>

:   Specifies \"bitwise\" match data (binary, real, string)

**-x,\--hex** \<*value*\>

:   Specifies hexadecimal match data that will automatically be
    converted to binary for searches

**-n,\--namesonly**

:   Indicates that only facnames should be printed in a gtkwave savefile
    compatible format. By doing this, the file can be used to specify
    which traces are to be imported into gtkwave.

**-c,\--comprehensive**

:   Indicates that results are not to stop after the first match. This
    can be used to extract all the matching values in the trace.

**-h,\--help**

:   Show help screen.

## EXAMPLES

vztminer dumpfile.vzt \--hex 20470000 -n

This attempts to match the hex value 20470000 across all facilities and when the value is encountered, the facname only is printed to stdout in order to generate a gtkwave compatible save file.

:   

## LIMITATIONS

*vztminer* only prints the first time a value is encountered for a
specific net. This is done in order to cut down on the size of output
files and to aid in following data such as addresses through a
simulation model. Note also that the reader algorithm attempts to
reconstruct bitblasted nets back into their original vectors but this is
not always successful, specifically in the case where the individual
bitstrands are dumped in non-sequential order.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*lxt2miner*(1) *vzt2vcd*(1) *lxt2vcd*(1) *vcd2lxt2*(1) *gtkwave*(1)
