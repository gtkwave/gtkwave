#
# simple example of using tcl with gtkwave:
# 1) query the dumpfile for signals with "clk" or
#    [1:48] in the signal name
# 2) show full signal hierarchy
# 3) zoom full
# 4) set marker to 128 units
# 5) generate the postscript to a file
# 6) exit gtkwave
#

set nfacs [ gtkwave::getNumFacs ]
set dumpname [ gtkwave::getDumpFileName ]
set dmt [ gtkwave::getDumpType ]

puts "number of signals in dumpfile '$dumpname' of type $dmt: $nfacs"

set clk48 [list]

for {set i 0} {$i < $nfacs } {incr i} {
    set facname [ gtkwave::getFacName $i ]

#    set facdir  [ gtkwave::getFacDir $i ]
#    set facvtype  [ gtkwave::getFacVtype $i ]
#    set facdtype  [ gtkwave::getFacDtype $i ]
#    puts "facname: $facname, facdir: $facdir, facvtype: $facvtype, facdtype: $facdtype"

    set indx [ string first "\[1:48\]" $facname  ]
    if {$indx == -1} {
    set indx [ string first clk $facname  ]
	}	

    if {$indx != -1} {
    	lappend clk48 "$facname"
	}
}

set ll [ llength $clk48 ]
puts "number of signals found matching either 'clk' or '\[1:48\]': $ll"

set num_added [ gtkwave::addSignalsFromList $clk48 ]
puts "num signals added: $num_added"

gtkwave::/Edit/Set_Trace_Max_Hier 0

gtkwave::setMarker 128
gtkwave::setNamedMarker A 400 "Example Named Marker"

gtkwave::/Time/Zoom/Zoom_Full

gtkwave::/File/Print_To_File PS {Letter (8.5" x 11")} Full $dumpname.ps
gtkwave::/File/Quit


# notes on toggle menu items:
# without an argument these toggle, otherwise with an argument it sets the value to 1 or 0
# gtkwave::/Edit/Color_Format/Keep_xz_Colors
# gtkwave::/Search/Autocoalesce
# gtkwave::/Search/Autocoalesce_Reversal
# gtkwave::/Search/Autoname_Bundles
# gtkwave::/Search/Search_Hierarchy_Grouping
# gtkwave::/Markers/Alternate_Wheel_Mode
# gtkwave::/Markers/Wave_Scrolling
# gtkwave::/View/Fullscreen
# gtkwave::/View/Show_Toolbar
# gtkwave::/View/Show_Grid
# gtkwave::/View/Show_Wave_Highlight
# gtkwave::/View/Show_Filled_High_Values
# gtkwave::/View/Leading_Zero_Removal
# gtkwave::/View/Show_Mouseover
# gtkwave::/View/Mouseover_Copies_To_Clipboard
# gtkwave::/View/Show_Base_Symbols
# gtkwave::/View/Standard_Trace_Select
# gtkwave::/View/Dynamic_Resize
# gtkwave::/View/Center_Zooms
# gtkwave::/View/Constant_Marker_Update
# gtkwave::/View/Draw_Roundcapped_Vectors
# gtkwave::/View/Zoom_Pow10_Snap
# gtkwave::/View/Partial_VCD_Dynamic_Zoom_Full
# gtkwave::/View/Partial_VCD_Dynamic_Zoom_To_End
# gtkwave::/View/Full_Precision
# gtkwave::/View/LXT_Clock_Compress_to_Z

# these can only set to the one selected
# gtkwave::/View/Scale_To_Time_Dimension/None
# gtkwave::/View/Scale_To_Time_Dimension/sec
# gtkwave::/View/Scale_To_Time_Dimension/ms
# gtkwave::/View/Scale_To_Time_Dimension/us
# gtkwave::/View/Scale_To_Time_Dimension/ns
# gtkwave::/View/Scale_To_Time_Dimension/ps
# gtkwave::/View/Scale_To_Time_Dimension/fs

