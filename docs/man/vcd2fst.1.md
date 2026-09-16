---
date: 3.3.53
myst:
  title_to_header: true
section: 1
title: vcd2fst
---

## NAME

vcd2fst - Converts VCD files to FST files

## SYNTAX

vcd2fst \[*option*\]\... \[*VCDFILE*\] \[*FSTFILE*\]

## DESCRIPTION

Converts VCD files to FST files.

## OPTIONS

**-v,\--vcdname** \<*filename*\>

:   Specify VCD/FSDB/VPD/WLF input filename. Processing of filetypes
    other than VCD requires that the appropriate 2vcd converter was
    found during ./configure.

**-f,\--fstname** \<*filename*\>

:   Specify FST output filename.

**-4,\--fourpack**

:   Indicates that LZ4 should be used for value change data (default).

**-F,\--fastpack**

:   Indicates that fastlz should be used instead of LZ4 for value change
    data.

**-Z,\--zlibpack**

:   Indicates that zlib should be used instead of LZ4 for value change
    data.

**-c,\--compress**

:   Indicates that the entire file should be run through gzip on close.
    This results in much smaller files at the expense of a one-time
    decompression penalty on file open during reads.

**-p,\--parallel**

:   Indicates that parallel mode should be enabled. This spawns a worker
    thread to continue with FST block processing while conversion
    continues on the main thread for new FST block data.

**-h,\--help**

:   Show help screen.

## EXAMPLES

Note that you should specify dumpfile.vcd directly or use \"-\" for
stdin.

vcd2fst dumpfile.vcd dumpfile.fst \--compress

:   This indicates that the FST file should be post-compressed on close.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*fst2vcd*(1) *vcd2lxt*(1) *vcd2lxt2*(1) *lxt2vcd*(1) *vcd2vzt*(1)
*vzt2vcd*(1) *gtkwave*(1)
