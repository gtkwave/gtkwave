# Why Use GTKWave?

GTKWave has been developed to perform debug tasks on large systems on a
chip and has been used in this capacity as an offline replacement for
third-party debug tools. It is 64-bit clean and is ready for the largest
of designs given that it is run on a workstation with a sufficient
amount of physical memory. The file formats LXT2 and VZT have been
specifically designed to cope with large, real-world designs, and AET2
(available to IBM EDA tool users only) and FST have been designed to
handle extremely large designs efficiently.

For Verilog, GTKWave allows users to debug simulation results at both
the net level by providing a bird\'s eye view of multiple signal values
over varying periods of time and also at the RTL level through
annotation of signal values back into the RTL for a given timestep. The
RTL browser frees up users from needing to be concerned with the actual
location of where a given module resides in the RTL as the view provided
by the RTL browser defaults to the module level. This provides quick
access to modules in the RTL as navigation has been reduced simply to
moving up and down in a tree structure that represents the actual
design.

Source code annotation is currently not available for VHDL, however all
of GTKWave\'s other debug features are readily accessible. VHDL support
is planned for a future release.
