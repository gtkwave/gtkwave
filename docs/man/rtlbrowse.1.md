---
date: 3.3.28
myst:
  title_to_header: true
section: 1
title: rtlbrowse
---

## NAME

rtlbrowse - Allows hierarchical browsing of Verilog HDL source code and
library design files.

## SYNTAX

rtlbrowse \<*stemsfilename*\>

## DESCRIPTION

Allows hierarchical browsing of Verilog HDL source code and library
design files. Navigation through the hierarchy may be done by clicking
open areas of the tree widget and clicking on the individual levels of
hierarchy. Inside the source code, selecting the module instantiation
name by double clicking or selecting part of the name through
drag-clicking will descend deeper into the RTL hierarchy. Note that it
performs optional source code annotation when called as a helper
application by *gtkwave*(1) and when the primary marker is set. Source
code annotation is not available for all supported dumpfile types. It is
directly available for LXT2, VZT, FST, and AET2. For VCD, use the
**-o**,**\--optimize** option of *gtkwave*(1) in order to optimize the
VCD file into FST. All other dumpfile types (LXT, GHW) are unsupported
at this time.

## EXAMPLES

To run this program the standard way type:

rtlbrowse stemsfile

:   The RTL is then brought up in a GTK tree viewer. Stems must have
    been previously generated with *xml2stems*(1) or some other tool
    capable of generating compatible stemsfiles. Note that *gtkwave*(1)
    will bring up this program as a client application for source code
    annotation. It does that by bringing up the viewer with the shared
    memory ID of a segment of memory in the viewer rather than using a
    stems filename.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*xml2stems*(1) *gtkwave*(1)
