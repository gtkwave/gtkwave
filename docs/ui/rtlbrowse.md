# RTLBrowse

RTLBrowse is called as a helper application by GTKWave and
is used to view and navigate through RTL source code. To use RTLBrowse,
Verilog source code must first be compiled into a `stems` file using the
`xml2stems`. This `stems` file, which contains hierarchy and component
instantiation information used to navigate quickly through the source
code. If GTKWave is started with the
`--stems` option, the stems file is parsed and RTLBrowse is launched.

The main window for RTLBrowse depicts the design as a tree-like
structure as shown in
[The RTLBrowse RTL Design Hierarchy window](#the-rtlbrowse-rtl-design-hierarchy-window).
Nodes in the tree may be clicked open or closed in order to navigate
through the design hierarchy. Missing modules (unparsed, but instantiated
 as components) will be marked as "\[MISSING\]".

:::{figure-md} the-rtlbrowse-rtl-design-hierarchy-window

![The RTLBrowse RTL Design Hierarchy window](../_static/images/rtlbrowse1.png)

The RTLBrowse RTL Design Hierarchy window
:::

When an item is selected, another window is opened showing only the
source code the selected module. If the primary marker is set, then the
source code will be annotated with values as shown in
[Source code annotated by RTLBrowse](#source-code-annotated-by-rtlbrowse).
If the primary marker moves or is deleted, then the values annotated into
the source code will be updated dynamically. The values shown are the full,
wide value of the signal. RTLBrowse currently does not perform bit
extractions on multi-bit vectors. If it is desired to see the full
source code file for a module, click on the "View Full File" button at
the bottom of the window.

Note that it is possible to descend deeper into the design hierarchy by
selecting the component name in the annotated or unannotated source
code.



:::{figure-md} source-code-annotated-by-rtlbrowse
![Source code annotated by RTLBrowse](../_static/images/rtlbrowse2.png)

Source code annotated by RTLBrowse
:::
