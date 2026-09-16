# Tcl Callbacks

When gtkwave performs various functions, global callback variables
prepended with `gtkwave::`{l=tcl} are modified within the Tcl interpreter. By
using the trace write feature in Tcl, scripts can achieve a very tight
integration with gtkwave. Global variables which may be used to register
callback procedures are as follows:

`gtkwave::cbCloseTabNumber`{l=tcl} contains the value returned is the number of
the tab which is going to be closed, starting from zero. As this is set
before the tab actually closes, scripts can interrogate for further
information.

`gtkwave::cbCloseTraceGroup`{l=tcl} contains the name of the expanded trace or
trace group being closed.

`gtkwave::cbCurrentActiveTab`{l=tcl} contains the number of the tab currently
selected. Note that when new tabs are being created, this callback
sometimes will oscillate between the old and new tab number, finally
settling on the new tab being created.

`gtkwave::cbError`{l=tcl} contains an error string such as "reload failed",
"gtkwave::loadFile prohibited in callback", "gtkwave::reLoadFile
prohibited in callback", or "gtkwave::setTabActive prohibited in
callback".

`gtkwave::cbFromEntryUpdated`{l=tcl} contains the value stored in the "From:"
widget when it is updated.

`gtkwave::cbOpenTraceGroup`{l=tcl} contains the name of a trace being expanded or
trace group being opened.

`gtkwave::cbQuitProgram`{l=tcl} contains the tab number which initiated a Quit
operation. Tabs are numbered starting from zero.

`gtkwave::cbReloadBegin`{l=tcl} contains the name of a trace being reloaded. This
is called at the start of a reload sequence.

`gtkwave::cbReloadEnd`{l=tcl} contains the name of a trace being reloaded. This
is called at the end of a reload sequence.

`gtkwave::cbStatusText`{l=tcl} contains the status text which goes to stderr.

`gtkwave::cbTimerPeriod`{l=tcl} contains the timer period in milliseconds
(default is 250), and this callback is invoked every timer period
expiration. If Tcl code modifies this value, the timer period can be
changed dynamically.

`gtkwave::cbToEntryUpdated`{l=tcl} contains the value stored in the "To:" widget
when it is updated.

`gtkwave::cbTracesUpdated`{l=tcl} contains the total number of traces. This is
called when traces are added, deleted, etc. from the viewer.

`gtkwave::cbTreeCollapse`{l=tcl} contains the flattened hierarchical name of the
SST tree node being collapsed.

`gtkwave::cbTreeExpand`{l=tcl} contains the flattened hierarchical name of the
SST tree node being expanded.

`gtkwave::cbTreeSelect`{l=tcl} contains the flattened hierarchical name of the
SST tree node being selected.

`gtkwave::cbTreeSigDoubleClick`{l=tcl} contains the name of the signal being
double-clicked in the signals section of the SST.

`gtkwave::cbTreeSigSelect`{l=tcl} contains the name of the signal being selected
in the signals section of the SST.

`gtkwave::cbTreeSigUnselect`{l=tcl} contains the name of the signal being
unselected in the signals section of the SST.

`gtkwave::cbTreeUnselect`{l=tcl} contains the flattened hierarchical name of the
SST tree node being unselected.

An example Tcl script follows to illustrate usage.

```{code-block} tcl
proc tracer {varname args} {
    upvar 0 $varname var
    puts "$varname was updated to be \"$var\""
}

proc tracer_error {varname args} {
    upvar 0 $varname var
    puts "*** ERROR: $varname was updated to be \"$var\""
}

set ie [ info exists tracer_defined ]

if { $ie == 0 } {
    set tracer_defined 1

    trace add variable gtkwave::cbTreeExpand
        write "tracer gtkwave::cbTreeExpand"

    trace add variable gtkwave::cbTreeCollapse
        write "tracer gtkwave::cbTreeCollapse"

    trace add variable gtkwave::cbTreeSelect
        write "tracer gtkwave::cbTreeSelect"

    trace add variable gtkwave::cbTreeUnselect
        write "tracer gtkwave::cbTreeUnselect"

    trace add variable gtkwave::cbTreeSigSelect
        write "tracer gtkwave::cbTreeSigSelect"

    trace add variable gtkwave::cbTreeSigUnselect
        write "tracer gtkwave::cbTreeSigUnselect"

    trace add variable gtkwave::cbTreeSigDoubleClick
        write "tracer gtkwave::cbTreeSigDoubleClick"
}

puts "Exiting script!"
```
