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
gtkwave::/Time/Zoom/Zoom_Full

gtkwave::setMarker 128
gtkwave::setNamedMarker A 400 "Example Named Marker"

gtkwave::/File/Print_To_File PS {Letter (8.5" x 11")} Full $dumpname.ps
gtkwave::/File/Quit
