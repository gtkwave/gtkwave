# Tcl Command Syntax

## Tcl Commands

Besides being able to access the menu options (e.g.,
`gtkwave::/File/Quit`), within Tcl scripts there are more commands
available for manipulating the viewer.

`addCommentTracesFromList`: adds comment traces to the viewer
:   Syntax: `set num_found [ gtkwave::addCommentTracesFromList list ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set clk48 [list]
    lappend clk48 "$facname1"
    lappend clk48 "$facname2"
    ...
    set num_added [ gtkwave::addCommentTracesFromList $clk48 ]
    ```

`addSignalsFromList`: adds signals to the viewer
:   Syntax: `set num_found [ gtkwave::addSignalsFromList list ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set clk48 [list]
    lappend clk48 "$facname1"
    lappend clk48 "$facname2"
    ...
    set num_added [ gtkwave::addSignalsFromList $clk48 ]
    ```

`deleteSignalsFromList`: deletes signals from the viewer.
:   This deletes only the first instance found unless the signal is specified multiple times in the list.

    Syntax: `set num_deleted [ gtkwave::deleteSignalsFromList list ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set clk48 [list]
    lappend clk48 "$facname1"
    lappend clk48 "$facname2"
    ...
    set num_deleted [ gtkwave::deleteSignalsFromList $clk48 ]
    ```

`deleteSignalsFromListIncludingDuplicates`: deletes signals from the viewer.
:   This deletes all the instances found so there is no need to specify the same signal multiple times in the list.

    Syntax: `set num_deleted [ gtkwave::deleteSignalsFromListIncludingDuplicates list ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set clk48 [list]
    lappend clk48 "$facname1"
    lappend clk48 "$facname2"
    ...
    set num_deleted [ gtkwave::deleteSignalsFromListIncludingDuplicates $clk48 ]
    ```

`findNextEdge`: advances the marker to the next edge for highlighted signals
:   Syntax: `set marker_time [ gtkwave::findNextEdge ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::highlightSignalsFromList "top.clk"
    set time_value [ gtkwave::findNextEdge ]
    puts "time_value: $time_value"
    ```

`findPrevEdge`: moves the marker to the previous edge for highlighted signals
:   Syntax: `set marker_time [ gtkwave::findPrevEdge ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::highlightSignalsFromList "top.clk"
    set time_value [ gtkwave::findPrevEdge ]
    puts "time_value: $time_value"
    ```

`forceOpenTreeNode`: forces open one tree node in the Signal Search Tree and closes the rest.
:   If upper levels are not open, the tree will remain
    closed however once the upper levels are opened, the hierarchy specified
    will become open. If path is missing or is an empty string, the function
    returns the current hierarchy path selected by the SST or -1 in case of
    error.

    Syntax: `gtkwave::forceOpenTreeNode hierarchy_path`{l=tcl}

    Returned value:  
    `0` - success  
    `1` - path not found in the tree  
    `-1` - SST tree does not exist

    ```{code-block} tcl
    :caption: Example
    set path tb.HDT.cpu
    switch -- [ gtkwavetcl::forceOpenTreeNode $path ] {
        -1 {puts "Error: SST is not supported here"}
        1 {puts "Error: '$path' was not recorder to dump file"}
        0 {}
    }
    ```

`getArgv`: returns a list of arguments which were used to start gtkwave from the command line
:   Syntax: `set argvlist [ gtkwave::getArgv ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set argvs [ gtkwave::getArgv ]
    puts "$argvs"
    ```

`getBaselineMarker`: returns the numeric value of the baseline marker time
:   Syntax: `set baseline_time [ gtkwave::getBaselineMarker ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set baseline [ gtkwave::getBaselineMarker ]
    puts "$baseline"
    ```

`getDisplayedSignals`: returns a list of all signals currently on display
:   Syntax: `set display_list [ gtkwave::getDisplayedSignals ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set display_list [ gtkwave::getDisplayedSignals ]
    puts "$display_list"
    ```

`getDumpFileName`: returns the filename for the loaded dumpfile
:   Syntax: `set loaded_file_name [ gtkwave::getDumpFileName ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set nfacs [ gtkwave::getNumFacs ]
    set dumpname [ gtkwave::getDumpFileName ]
    set dmt [ gtkwave::getDumpType ]
    puts "number of signals in dumpfile '$dumpname' of type $dmt: $nfacs"
    ```

`getDumpType`: returns the dump type as a string (VCD, PVCD, LXT, LXT2, GHW, VZT)
:   Syntax: `set dump_type [ gtkwave::getDumpType ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set nfacs [ gtkwave::getNumFacs ]
    set dumpname [ gtkwave::getDumpFileName ]
    set dmt [ gtkwave::getDumpType ]
    puts "number of signals in dumpfile '$dumpname' of type $dmt: $nfacs"
    ```

`getFacName`: returns a string for the facility name which corresponds to a given facility number
:   Syntax: `set fac_name [ gtkwave::getFacName fac_number ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set nfacs [ gtkwave::getNumFacs ]
    for {set i 0} {$i < $nfacs } {incr i} {
        set facname [ gtkwave::getFacName $i ]
        puts "$i: $facname"
    }
    ```

`getFacDir`: returns a string for the direction which corresponds to a given facility number
:   Syntax: `set fac_dir [ gtkwave::getFacDir fac_number ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set nfacs [ gtkwave::getNumFacs ]
    for {set i 0} {$i < $nfacs } {incr i} {
        set facdir [ gtkwave::getFacDir $i ]
        puts "$i: $facdir"
    }
    ```

`getFacVtype`: returns a string for the variable type which corresponds to a given facility number
:   Syntax: `set fac_vtype [ gtkwave::getFacVtype fac_number ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set nfacs [ gtkwave::getNumFacs ]
    for {set i 0} {$i < $nfacs } {incr i} {
        set facvtype [ gtkwave::getFacVtype $i ]
        puts "$i: $facvtype"
    }
    ```

`getFacDtype`: returns a string for the data type which corresponds to a given facility number
:   Syntax: `set fac_dtype [ gtkwave::getFacDtype fac_number ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set nfacs [ gtkwave::getNumFacs ]
    for {set i 0} {$i < $nfacs } {incr i} {
        set facdtype [ gtkwave::getFacDtype $i ]
        puts "$i: $facdtype"
    }
    ```

`getFontHeight`: returns the font height for signal names
:   Syntax: `set font_height [ gtkwave::getFontHeight ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set font_height [ gtkwave::getFontHeight ]
    puts "$font_height"
    ```

`getFromEntry`: returns the time value string in the "From:" box.
:   Syntax: `set from_entry [ gtkwave::getFromEntry ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set from_entry [ gtkwave::getFromEntry ]
    puts "$from_entry"
    ```

`getHierMaxLevel`: returns the max hier value which is set in the viewer
:   Syntax: `set hier_max_level [ gtkwave::getHierMaxLevel ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set max_level [ gtkwave::getHierMaxLevel ]
    puts "$max_level"
    ```

`getLeftJustifySigs`: returns 1 if signals are left justified, else 0
:   Syntax: `set left_justify [ gtkwave::getLeftJustifySigs ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set justify [ gtkwave::getLeftJustifySigs ]
    puts "$justify"
    ```

`getLongestName`: returns number of characters of the longest name in the dumpfile
:   Syntax: `set longestname_len [ gtkwave::getLongestName ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set longest [ gtkwave::getLongestName ]
    puts "$longest"
    ```

`getMarker`: returns the numeric value of the primary marker position
:   Syntax: `set marker_time [ gtkwave::getMarker ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set marker_time [ gtkwave::getMarker ]
    puts "$marker_time"
    ```

`getMaxTime`: returns the numeric value of the last time value in the dumpfile
:   Syntax: `set max_time [ gtkwave::getMaxTime ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set max_time [ gtkwave::getMaxTime ]
    puts "$max_time"
    ```

`getMinTime`: returns the numeric value of the first time value in the dumpfile
:   Syntax: `set min_time [ gtkwave::getMinTime ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set min_time [ gtkwave::getMinTime ]
    puts "$min_time"
    ```

`getNamedMarker`: returns the numeric value of the named marker position
:   Syntax: `set time_value [ gtkwave::getNamedMarker which ]`{l=tcl}
    such that which = A-Z or a-z

    ```{code-block} tcl
    :caption: Example
    set marker_time [ gtkwave::getNamedMarker A ]
    puts "$marker_time"
    ```

`getNumFacs`: returns the number of facilities encountered in the dumpfile
:   Syntax: `set numfacs [ gtkwave::getNumFacs ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set nfacs [ gtkwave::getNumFacs ]
    set dumpname [ gtkwave::getDumpFileName ]
    set dmt [ gtkwave::getDumpType ]
    puts "number of signals in dumpfile '$dumpname' of type $dmt: $nfacs"
    ```

`getNumTabs`: returns the number of tabs shown on the viewer
:   Syntax: `set numtabs [ gtkwave::getNumTabs ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set ntabs [ gtkwave::getNumTabs ]
    puts "number of tabs: $ntabs"
    ```

`getPixelsUnitTime`: returns the number of pixels per unit time
:   Syntax: `set pxut [ gtkwave::getPixelsUnitTime ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set pxut [ gtkwave::getPixelsUnitTime ]
    puts "$pxut"
    ```

`getSaveFileName`: returns the save filename
:   Syntax: `set save_file_name [ gtkwave::getSaveFileName ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set savename [ gtkwave::getSaveFileName ]
    puts "$savename"
    ```

`getStemsFileName`: returns the stems filename
:   Syntax: `set stems_file_name [ gtkwave::getStemsFileName ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set stemsname [ gtkwave::getStemsFileName ]
    puts "$stemsname"
    ```

`getTimeDimension`: returns the first character of the time units that the trace was saved in (e.g., "u" for us, "n" for "ns", "s" for sec, etc.)
:   Syntax: `set dimension_first_char [ gtkwave::getTimeDimension ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set dimch [ gtkwave::getTimeDimension ]
    puts "$dimch"
    ```

`getTimeZero`: returns the numeric value for what represents the time #0 in the dumpfile. This is only of interest if the $timezero directive is encountered in the dumpfile.
:   Syntax: `set zero_time [ gtkwave::getTimeZero ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set zero_time [ gtkwave::getTimeZero ]
    puts "$zero_time"
    ```

`getToEntry`: returns the time value string in the "To:" box.
:   Syntax: `set to_entry [ gtkwave::getToEntry ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set to_entry [ gtkwave::getFromEntry ]
    puts "$to_entry"
    ```

`getTotalNumTraces`: returns the total number of traces that are being displayed currently
:   Syntax: `set total_traces [ gtkwave::getTotalNumTraces ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set totnum [ gtkwave::getTotalNumTraces ]
    puts "$totnum"
    ```

`getTraceFlagsFromIndex`: returns the decimal value of the sum of all flags for a given trace
:   Syntax: `set flags [ gtkwave::getTraceFlagsFromIndex trace_number ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set tflags [ gtkwave::getTraceFlagsFromIndex 0 ]
    puts "$tflags"
    ```

`getTraceFlagsFromName`: returns the decimal value of the sum of all flags for a given trace
:   Syntax: `set flags [ gtkwave::getTraceFlagsFromName trace_name ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set tflags [ gtkwave::getTraceFlagsFromName {top.des.k1x[1:48]} ]
    puts "$tflags"
    ```

`getTraceNameFromIndex`: returns the name of a trace when given the index value
:   Syntax: `set trace_name [ gtkwave::getTraceNameFromIndex trace_number ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set tname [ gtkwave::getTraceNameFromIndex 1 ]
    puts "$tname"
    ```

`getTraceScrollbarRowValue`: returns the scrollbar value (which corresponds to the trace index for the topmost trace on screen)
:   Syntax: `set scroller_value [ gtkwave::getTraceScrollbarRowValue ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set scroller [ gtkwave::getTraceScrollbarRowValue ]
    puts "$scroller"
    ```

`getTraceValueAtMarkerFromIndex`: returns the value under the marker for the trace numbered trace index
:   Syntax: `set ascii_value [ gtkwave::getTraceValueAtMarkerFromIndex trace_number ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set tvi [ gtkwave::getTraceValueAtMarkerFromIndex 2 ]
    puts "$tvi"
    ```

`getTraceValueAtMarkerFromName`: returns the value under the primary marker for the given trace name
:   Syntax: `set ascii_value [ gtkwave::getTraceValueAtMarkerFromName fac_name ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set tvn [ gtkwave::getTraceValueAtMarkerFromName {top.des.k2x[1:48]} ]
    puts "$tvn"
    ```

`getTraceValueAtNamedMarkerFromName`: returns the value under the named marker for the given trace name
:   Syntax: `set ascii_value [ gtkwave::getTraceValueAtNamedMarkerFromName which fac_name ]`{l=tcl}
    such that which = A-Z or a-z

    ```{code-block} tcl
    :caption: Example
    set tvn [ gtkwave::getTraceValueAtNamedMarkerFromName A {top.des.k2x[1:48]} ]
    puts "$tvn"
    ```

`getUnitTimePixels`: returns the number of time units per pixel
:   Syntax: `set utpx [ gtkwave::getUnitTimePixels ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set utpx [ gtkwave::getUnitTimePixels ]
    puts "$utpx"
    ```

`getVisibleNumTraces`: returns number of non-collapsed traces
:   Syntax: `set num_visible_traces [ gtkwave::getVisibleNumTraces ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set nvt [ gtkwave::getVisibleNumTraces ]
    puts "$nvt"
    ```

`getWaveHeight`: returns the height of the wave window in pixels
:   Syntax: `set wave_height [ gtkwave::getWaveHeight ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set wht [ gtkwave::getWaveHeight ]
    puts "$wht"
    ```

`getWaveWidth`: returns the width of the wave window in pixels
:   Syntax: `set wave_width [ gtkwave::getWaveWidth ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set wwt [ gtkwave::getWaveWidth ]
    puts "$wwt"
    ```

`getWindowEndTime`: returns the end time of the wave window

:   Syntax: `set end_time_value [ gtkwave::getWindowEndTime ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set wet [ gtkwave::getWindowEndTime ]
    puts "$wet"
    ```

`getWindowStartTime`: returns the start time of the wave window

:   Syntax: `set start_time_value [ gtkwave::getWindowStartTime ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set wst [ gtkwave::getWindowStartTime ]
    puts "$wst"
    ```

`getZoomFactor`: returns the zoom factor of the wave window

:   Syntax: `set zoom_value [ gtkwave::getZoomFactor ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set zf [ gtkwave::getZoomFactor ]
    puts "$zf"
    ```

`highlightSignalsFromList`: highlights the facilities contained in the list argument
:   Syntax: `set num_highlighted [ gtkwave::highlightSignalsFromList list ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set clk48 [list]

    lappend clk48 "$facname1"
    lappend clk48 "$facname2"
    ...
    set num_highlighted [ gtkwave::highlightSignalsFromList $clk48 ]
    ```

`installFileFilter`: installs file filter number which across all highlighted traces. Using zero for which removes the filter.
:   Syntax: `set num_updated [ gtkwave::installFileFilter which ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set num_updated [ gtkwave::installFileFilter 0 ]
    puts "$num_updated"
    ```

`installProcFilter`: installs process filter number which across all highlighted traces. Using zero for which removes the filter.
:   Syntax: `set num_updated [ gtkwave::installProcFilter which ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set num_updated [ gtkwave::installProcFilter 0 ]
    puts "$num_updated"
    ```

`installTransFilter`: installs transaction process filter number which across all highlighted traces. Using zero for which removes the filter.
:   Syntax: set num_updated [ gtkwave::installTransFilter which ]

    ```{code-block} tcl
    :caption: Example
    set num_updated [ gtkwave::installTransFilter 0 ]
    puts "$num_updated"
    ```

`loadFile`: loads a new file
:   Syntax: `gtkwave::loadFile filename`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::loadFile "$filename"
    ```

`nop`: calls the GTK main loop in order to update the gtkwave GUI
:   Syntax: `gtkwave::nop`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::nop
    ```

`presentWindow`: raises the main window in the stacking order or deiconifies it
:   Syntax: `gtkwave::presentWindow`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::presentWindow
    ```

`reLoadFile`: reloads the current active file
:   Syntax: `gtkwave::reLoadFile`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::reLoadFile
    ```

`setBaselineMarker`: sets the time for the baseline marker (-1 removes it)
:   Syntax: `gtkwave::setBaselineMarker time_value`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setBaselineMarker 128
    ```

`setCurrentTranslateEnums`: sets the enum list to function as the current translate file and returns the corresponding which value to be used with gtkwave::installFileFilter. As a real file is not used, the results of this are not recreated when a save file is loaded or if the waveform is reloaded.
:   Syntax: `set which_f [ gtkwave::setCurrentTranslateEnums elist ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set enums [list]

    lappend enums 0000000000000000 IDLE
    lappend enums FFFFFFFFFFFFFFFF BUSY
    lappend enums 3000000000000000 OTHER
    lappend enums 0123456789ABCDEF HEXSTATE
    lappend enums 1111111111111111 "All 1s"

    set which_f [ gtkwave::setCurrentTranslateFile $enums ]

    puts "$which_f"
    ```

`setCurrentTranslateFile`: sets the filename to the current translate file and returns the corresponding which value to be used with gtkwave::installFileFilter.
:   Syntax: `set which_f [ gtkwave::setCurrentTranslateFile filename ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set which_f [ gtkwave::setCurrentTranslateFile ./zzz.txt ]
    puts "$which_f"
    ```

`setCurrentTranslateProc`: sets the filename to the current translate process (executable) and returns the corresponding which value to be used with gtkwave::installProcFilter.
:   Syntax: `set which_f [ gtkwave::setCurrentTranslateProc filename ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set which_f [ gtkwave::setCurrentTranslateProc ./zzz.exe ]
    puts "$which_f"
    ```

`setCurrentTranslateTransProc`: sets the filename to the current transaction translate process (executable) and returns the corresponding which value to be used with gtkwave::installTransFilter.
:   Syntax: `set which_f [ gtkwave::setCurrentTranslateTransProc filename ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set which_f [ gtkwave::setCurrentTranslateTransProc ./zzz.exe ]
    puts "$which_f"
    ```

`setFromEntry`: sets the time in the "From:" box.
:   Syntax: `gtkwave::setFromEntry time_value`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setFromEntry 100
    ```

`setLeftJustifySigs`: turns left justification for signal names on or off
:   Syntax: `gtkwave::setLeftJustifySigs on_off_value`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setLeftJustifySigs on
    gtkwave::setLeftJustifySigs off
    ```

`setMarker`: sets the time for the primary marker (-1 removes it)
:   Syntax: `gtkwave::setMarker time_value`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setMarker 128
    ```

`setNamedMarker`: sets named marker A-Z (a-z) to a given time value and optionally renames the marker text to a string (-1 removes the marker)
:   Syntax: `gtkwave::setNamedMarker which time_value [string]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setNamedMarker A 400 "Example Named Marker"
    gtkwave::setNamedMarker A 400
    ```

`setTabActive`: sets the active tab in the viewer (0..getNumTabs-1)
:   Syntax: `gtkwave::setTabActive which`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setTabActive 0
    ```

`setToEntry`: sets the time in the "To:" box.
:   Syntax: `gtkwave::setToEntry time_value`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setToEntry 600
    ```

`setTraceHighlightFromIndex`: highlights or unhighlights the specified trace
:   Syntax: `gtkwave::setTraceHighlightFromIndex trace_index on_off`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setTraceHighlightFromIndex 2 on
    gtkwave::setTraceHighlightFromIndex 2 off
    ```

`setTraceHighlightFromNameMatch`: highlights or unhighlights the specified trace
:   Syntax: `gtkwave::setTraceHighlightFromNameMatch fac_name on_off`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setTraceHighlightFromNameMatch top.des.clk on
    gtkwave::setTraceHighlightFromNameMatch top.clk off
    ```

`setTraceScrollbarRowValue`: sets the scrollbar for traces a number of traces down from the very top
:   Syntax: `gtkwave::setTraceScrollbarRowValue scroller_value`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setTraceScrollbarRowValue 10
    ```

`setWindowStartTime`: scrolls the traces such that the start time is at the left margin (as long as the zoom level permits this)
:   Syntax: `gtkwave::setWindowStartTime start_time`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setWindowStartTime 100
    ```

`setZoomFactor`: sets the zoom factor for the trace data (i.e., how compressed it is with respect to time)
:   Syntax: `gtkwave::setZoomFactor zoom_value`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setZoomFactor -3
    ```

`setZoomRangeTimes`: sets the visible time range for the trace data
:   Syntax: `gtkwave::setZoomRangeTimes time1 time2`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setZoomRangeTimes 100 217
    ```

`showSignal`: sets the scrollbar for traces a number of traces down from the very top (0), center (1), or bottom (2)
:   Syntax: `gtkwave::setTraceScrollbarRowValue scroller_value position`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    gtkwave::setTraceScrollbarRowValue 10 0
    ```

`signalChangeList`: returns time and value changes for the signals indicated by the argument names
:   Syntax: `gtkwave::signalChangeList signal_name options`{l=tcl}

    Where *options* is are one or more of the following:  
    - *-start_time* start-time (default 0)  
    - *-end_time* end-time (default last sample in dump file)  
    - *-max* maximum-number-of-samples (default 0x7fffffff)  
    - *-dir* forward\|backward (default forward)

    The function returns a Tcl list of value changes for the *signal-name*
    starting at *start-time* and ending at *end-time* or an empty list in
    any other case.

    Even members of the list hold the time of change and odd members hold
    the value that is associated with the time the precedes it. Values are
    given as strings in the base of the signal.

    If *signal-name* is not present then the first highlighted signal is
    taken.

    Length of the list is defined by both *end-time* and *max*, whichever
    comes first.

    To specify backward search, *end-time* should be smaller than
    *start-time* or *dir* should have the value of *backward* and *end-time*
    is not defined.

    A conflict between timing (start/end-time) and direction
    (forward/backward) returns an empty list.

    Examples:

    1. prints the first 100 changes of the signal tb.HDT.cpu.CS starting at time 10000

    ```{code-block} tcl
    :caption: Example
    set signal tb.HDT.cpu.CS
    set start_time 10000
    foreach {time value} [gtkwave::signalChangeList $signal -start_time $start_time -max 100] {
        puts "Time: $time value: $value"
    }
    ```

    2. retrieve the value of tb.HDT.cpu.CS at time 123456

    ```{code-block} tcl
    :caption: Example
    lassign [gtkwave::signalChangeList tb.HDT.cpu.CS -start_time 123456 -max 1] dont_care value*
    ```

`unhighlightSignalsFromList`: unhighlights the facilities contained in the list argument
:   Syntax: `set num_unhighlighted [ gtkwave::unhighlightSignalsFromList list ]`{l=tcl}

    ```{code-block} tcl
    :caption: Example
    set clk48 [list]
    lappend clk48 "$facname1"
    lappend clk48 "$facname2"
    ...
    set num_highlighted [ gtkwave::unhighlightSignalsFromList $clk48 ]
    ```

## Tcl Callbacks

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
