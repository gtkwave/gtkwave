# What Is GTKWave?

GTKWave as a collection of binaries is comprised of two interlocking
tools: the *gtkwave* viewer application and *rtlbrowse*. In addition, a
collection of helper applications are used to facilitate such tasks as
file conversions and simulation data mining. They are intended to
function together in a cohesive system although their modular design
allows each to function independently of the others if need be.

*gtkwave* is the waveform analyzer and is the primary tool used for
visualization. It provides a method for viewing simulation results for
both analog and digital data, allows for various search operations and
temporal manipulations, can save partial results (i.e., "signals of
interest") extracted from a full simulation dump, and finally can
generate PostScript and FrameMaker output for hard copy.

*rtlbrowse* is used to view and navigate through RTL source code that
has been parsed and processed into a stems file by the helper
application *xml2stems*. It allows for viewing of RTL at both the file
and module level and when invoked by *gtkwave*, allows for source code
annotation.

The helper applications perform various specialized tasks such as file
conversion, RTL parsing, and other data manipulation operations
considered outside of the scope of what a visualization tool needs to
perform.
