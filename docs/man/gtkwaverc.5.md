---
date: 3.3.81
myst:
  title_to_header: true
section: 5
title: gtkwaverc
---

## NAME

gtkwaverc - GTKWave Configuration File

## SYNTAX

option \<*value*\>

The configuration file is a series of option and value pairs. Comment lines marked with an initial \'#\' character are permissible. Blank lines are ignored. 

:   

## DESCRIPTION

Configuration file for *gtkwave*(1). The search path for the
configuration file (if unspecified) is the current working directory
followed by the user\'s home directory.

## OPTIONS

**accel** \<*\"pathvalue\" accelerator*\>

:   This allows replacement of menu accelerator keys. See the .gtkwaverc
    file in the source distribution for examples on pathvalue and
    accelerator syntax. The special accelerator value of *(null)* means
    that no accelerator is bound to the menu item.

**alt_hier_delimeter** \<*value*\>

:   This allows another character in addition to the hier_delimeter to
    be used to delimit levels in the hierarchy for VCD. Only the first
    character in the value is significant. Note that this is normally
    off. The intended use is to resolve the hierarchies of netlist based
    models that often contain slashes to delimit hierarchy inside of
    \$var statements.

**alt_wheel_mode** \<*value*\> 

:   Default is on. Scrollwheel alone pans along a quarter at a time
    rather than a full page, so you don\'t get lost. Ctrl+wheel zooms
    in/out around the mouse cursor position, not the marker position.
    Alt+wheel edges left/right based on the currently selected signal.
    This makes measuring deltas easier.

**analog_redraw_skip_count** \<*value*\>

:   Specifies how many overlapping analog segments can be drawn for a
    given X position onscreen. (Default: 20) If there are gaps in analog
    traces, this value is too low.

**append_vcd_hier** \<*value*\>

:   Allows the specification of a prefix hierarchy for VCD files. This
    can be done in \"pieces,\" so that multiple layers of hierarchy are
    prepended to symbol names with the most significant addition
    occurring first (see .gtkwaverc in the examples/vcd directory). The
    intended use of this is to have the ability to add \"project\"
    prefixes which allow easier selection of everything from the tree
    hierarchy.

**atomic_vectors** \<*value*\>

:   Speeds up vcd loading and takes up less memory. This option is
    deprecated; it is currently the default.

**autocoalesce** \<*value*\>

:   A nonzero value enables autocoalescing of VCD vectors when
    applicable. This may be toggled dynamically during wave viewer
    usage.

**autocoalesce_reversal** \<*value*\>

:   causes split vectors to be reconstructed in reverse order (only if
    autocoalesce is also active).

**autoname_bundles** \<*value*\>

:   A nonzero value indicates that GTKWave will create its own bundle
    names rather than prompting the user for them.

**clipboard_mouseover** \<*value*\>

:   A nonzero value indicates that when mouseover is enabled, all values
    generated for the tooltips will be automatically copied into the
    clipboard so they may be pasted into other programs such as text
    editors, etc.

**color_0** \<*value*\>

:   trace color when 0.

**color_1** \<*value*\>

:   trace color when 1.

**color_1fill** \<*value*\>

:   trace color (inside of box) when 1.

**color_back** \<*value*\>

:   background color.

**color_baseline** \<*value*\>

:   middle mouse button marker color.

**color_black** \<*value*\>

:   color value for \"black\" in signal window.

**color_brkred** \<*value*\>

:   brick red color for comments.

**color_dash** \<*value*\>

:   trace color when don\'t care (\"-\").

**color_dashfill** \<*value*\>

:   trace color (inside of box) when don\'t care (\"-\").

**color_dkblue** \<*value*\>

:   color value for \"dark blue\" in signal window.

**color_dkgray** \<*value*\>

:   color value for \"dark gray\" in signal window.

**color_gmstrd** \<*value*\>

:   color value for trace groupings.

**color_grid** \<*value*\>

:   grid color (use Alt-G/Shift-Alt-G to show/hide grid). This is also
    the color used for **highlight_wavewindow** when enabled.

**color_grid2** \<*value*\>

:   grid color for secondary pattern search.

**color_high** \<*value*\>

:   trace color when high (\"H\").

**color_highfill** \<*value*\>

:   trace color (inside of box) when high (\"H\").

**color_low** \<*value*\>

:   trace color when low (\"L\").

**color_ltblue** \<*value*\>

:   color for shadowed traces.

**color_ltgray** \<*value*\>

:   color value for \"light gray\" in signal window.

**color_mark** \<*value*\>

:   color of the named markers.

**color_mdgray** \<*value*\>

:   color value for \"medium gray\" in signal window.

**color_mid** \<*value*\>

:   trace color when floating (\"Z\").

**color_normal** \<*value*\>

:   color value for \"normal\" GTK state in signal window.

**color_time** \<*value*\>

:   text color for timebar.

**color_timeb** \<*value*\>

:   text color for timebar\'s background.

**color_trans** \<*value*\>

:   trace color when transitioning.

**color_u** \<*value*\>

:   trace color when undefined (\"U\").

**color_ufill** \<*value*\>

:   trace color (inside of box) when undefined (\"U\").

**color_umark** \<*value*\>

:   color of the unnamed (primary) marker.

**color_value** \<*value*\>

:   text color for vector values.

**color_vbox** \<*value*\>

:   vector color (horizontal).

**color_vtrans** \<*value*\>

:   vector color (verticals/transitions).

**color_w** \<*value*\>

:   trace color when weak (\"W\").

**color_wfill** \<*value*\>

:   trace color (inside of box) when weak (\"W\").

**color_white** \<*value*\>

:   color value for \"white\" in signal window.

**color_x** \<*value*\>

:   trace color when undefined (\"X\") (collision for VHDL).

**color_xfill** \<*value*\>

:   trace color (inside of box) when undefined (\"X\") (collision for
    VHDL).

**constant_marker_update** \<*value*\>

:   A nonzero value indicates that the values for traces listed in the
    signal window are to be updated constantly when the left mouse
    button is being held down rather than only when it is first pressed
    then when released (which is the default).

**context_tabposition** \<*value*\>

:   Use zero for tabbed viewing with named tabs at the top. Nonzero
    places numerically indexed tabs at the left.

**convert_to_reals** \<*value*\>

:   Converts all integer and parameter VCD declarations to real-valued
    ones when set to a nonzero/yes value. The positive aspect of this is
    that integers and parameters will take up less space in memory and
    will automatically display in decimal format. The negative aspect of
    this is that integers and parameters will only be displayable as
    decimals and can\'t be bit reversed, inverted, etc.

**cursor_snap** \<*value*\>

:   A nonzero value indicates the number of pixels the marker should
    snap to for the nearest signal transition.

**dbl_mant_dig_overrides** \<*value*\>

:   A nonzero value indicates that if a simulation\'s max time value
    exceeds double floating point mantissa size, then do not exit the
    viewer. The early exit occurs to prevent potential GUI problems with
    scrollbars, etc. Default is 0 = exit.

**disable_ae2_alias** \<*value*\>

:   A nonzero value indicates that the AE2 loader is to ignore the
    aliasdb keyword and is not to construct facility aliases.

**disable_auto_comphier** \<*value*\>

:   A nonzero value indicates that the loaders that support compressed
    hierarchies should not automatically turn on compression if the
    threshold count of signals (500000) has been reached.

**disable_empty_gui** \<*value*\>

:   A nonzero value indicates that if gtkwave is invoked without a
    dumpfile name, then an empty gtkwave session is to be suppressed.
    Default is a zero value: to bring up an empty session which needs a
    file loaded or dragged into it.

**disable_mouseover** \<*value*\>

:   A nonzero value indicates that signal/value tooltip pop up bubbles
    on mouse button presses should be disabled in the value window. A
    zero value indicates that value tooltips should be active (default
    is disabled).

**disable_tooltips** \<*value*\>

:   A nonzero value indicates that tooltip pop up bubbles should be
    disabled. A zero value indicates that tooltips should be active
    (default).

**do_initial_zoom_fit** \<*value*\>

:   A nonzero value indicates that the trace should initially be
    crunched to fit the screen. A zero value indicates that the initial
    zoom should be zero (default).

**dragzoom_threshold** \<*value*\>

:   A nonzero value indicates the number of pixels in the x direction
    the marker must move in order for a dragzoom to be triggered. This
    is largely to handle noisy input devices.

**dynamic_resizing** \<*value*\>

:   A nonzero value indicates that dynamic resizing should be initially
    enabled (default). A zero value indicates that dynamic resizing
    should be initially disabled.

**editor** \<*\"value\"*\>

:   This is used to specify a string (quotes mandatory) for when gtkwave
    invokes a text editor (e.g., Open Source Definition). Examples are:
    editor \"vimx -g +%d %s\", editor \"gedit +%d %s\", editor \"emacs
    +%d %s\", and for OSX, editor \"mate -l %d %s\". The %d may be
    combined with other characters in a string such as +, etc. The %s
    argument must stand by itself. Note that if this rc variable is not
    set, then the environment variable GTKWAVE_EDITOR will be consulted
    next, then finally gedit will be used (if found).

**enable_fast_exit** \<*value*\>

:   Allows exit without bringing up a confirmation requester. The
    default is nonzero/yes.

**enable_ghost_marker** \<*value*\>

:   lets the user turn on/off the ghost marker during primary marker
    dragging. Default is enabled.

**enable_horiz_grid** \<*value*\>

:   A nonzero value indicates that when grid drawing is enabled,
    horizontal lines are to be drawn. This is the default.

**enable_vcd_autosave** \<*value*\>

:   causes the vcd loader to automatically generate a .sav file
    (vcd_autosave.sav ) in the cwd if a save file is not specified on
    the command line. Note that this mirrors the VCD \$var defs and no
    attempt is made to coalesce split bitvectors back together.

**enable_vert_grid** \<*value*\>

:   A nonzero value indicates that when grid drawing is enabled,
    vertical lines are to be drawn. This is the default. Note that all
    possible combinations of enable_horiz_grid and enable_vert_grid
    values are acceptable.

**fill_waveform** \<*value*\>

:   A zero value indicates that the waveform should not be filled for
    1/H values. This is the default.

**fontname_logfile** \<*value*\>

:   When followed by an argument, this indicates the name of the X11
    font that you wish to use for the logfile browser. You may generate
    appropriate fontnames using the xfontsel program.

**fontname_signals** \<*value*\>

:   When followed by an argument, this indicates the name of the X11
    font that you wish to use for signals. You may generate appropriate
    fontnames using the xfontsel program.

**fontname_waves** \<*value*\>

:   When followed by an argument, this indicates the name of the X11
    font that you wish to use for waves. You may generate appropriate
    fontnames using the xfontsel program. Note that the signal font must
    be taller than the wave font or the viewer will complain then
    terminate.

**force_toolbars** \<*value*\>

:   When enabled, this forces everything above the signal and wave
    windows to be rendered as toolbars. This allows for them to be
    detached which allows for more usable wave viewer space. By default
    this is off.

**hide_sst** \<*value*\>

:   Hides the Signal Search Tree widget for GTK2.4 and greater such that
    it is not embedded into the main viewer window. It is still
    reachable as an external widget through the menus.

**hier_delimeter** \<*value*\>

:   This allows characters other than \'/\' to be used to delimit levels
    in the hierarchy. Only the first character in the value is
    significant.

**hier_grouping** \<*value*\>

:   For the tree widgets, this allows the hierarchies to be grouped in a
    single place rather than spread among the netnames.

**hier_ignore_escapes** \<*value*\>

:   A nonzero value indicates that the signal pane ignores escapes in
    identifiers when determining the hierarchy maximum depth. Default is
    disabled so that escapes are examined.

**hier_max_level** \<*value*\>

:   Sets the maximum hierarchy depth (from the right side) to display
    for trace names. Note that a value of zero displays the full
    hierarchy name.

**highlight_wavewindow** \<*value*\>

:   When enabled, this causes traces highlighted in the signal window
    also to be highlighted in the wave window.

**hpane_pack** \<*value*\>

:   A nonzero value indicates that the horizontal pane should be
    constructed using the gtk_paned_pack functions (default and
    recommended). A zero value indicates that gtk_paned_add will be used
    instead.

**ignore_savefile_pane_pos** \<*value*\>

:   If nonzero, specifies that the pane position attributes (i.e.,
    signal window width size, SST is expanded, etc.) are to be ignored
    during savefile loading and is to be skipped during saving. Default
    is that the attribute is used.

**ignore_savefile_pos** \<*value*\>

:   If nonzero, specifies that the window position attribute is to be
    ignored during savefile loading and is to be skipped during saving.
    Default is that the position attribute is used.

**ignore_savefile_size** \<*value*\>

:   If nonzero, specifies that the window size attribute is to be
    ignored during savefile loading and is to be skipped during saving.
    Default is that the size attribute is used.

**initial_signal_window_width** \<*value*\>

:   Sets the creation width for the signal pane on GUI initialization.
    Also sets another potential minimum value for dynamic resizing.

**initial_window_x** \<*value*\>

:   Sets the size of the initial width of the wave viewer window. Values
    less than or equal to zero will set the initial width equal to -1
    which will let GTK determine the minimum size.

**initial_window_xpos** \<*value*\>

:   Sets the size of the initial x coordinate of the wave viewer window.
    -1 will let the window manager determine the position.

**initial_window_y** \<*value*\>

:   Sets the size of the initial height of the wave viewer window.
    Values less than or equal to zero will set the initial width equal
    to -1 which will let GTK determine the minimum size.

**initial_window_ypos** \<*value*\>

:   Sets the size of the initial y coordinate of the wave viewer window.
    -1 will let the window manager determine the position.

**keep_xz_colors** \<*value*\>

:   When nonzero, indicates that the original color scheme for non 0/1
    signal values is to be used when Color Format overrides are in
    effect. Default is off.

**left_justify_sigs** \<*value*\>

:   When nonzero, indicates that the signal window signal name
    justification should default to left, else the justification is to
    the right (default).

**lxt_clock_compress_to_z** \<*value*\>

:   For LXT (not LXT2) allows clocks to compress to a \'z\' value so
    that regular/periodic value changes may be noted.

**lz_removal** \<*value*\>

:   When nonzero, suppresses the display of leading zeros on
    non-filtered traces. This has no effect on filtered traces.

**max_fsdb_trees** \<*value*\>

:   sets the maximum number of hierarchy and signal trees to process for
    an FSDB file. Default = 0 = unlimited. The intent of this is to work
    around sim environments that accidentally call fsdbDumpVars multiple
    times.

**page_divisor** \<*value*\>

:   Sets the scroll amount for page left and right operations. (The
    buttons, not the hscrollbar.) Values over 1.0 are taken as 1/x and
    values equal to and less than 1.0 are taken literally. (i.e., 2
    gives a half-page scroll and .67 gives 2/3). The default is 1.0.

**ps_maxveclen** \<*value*\>

:   sets the maximum number of characters that can be printed for a
    value in the signal window portion of a postscript file (not
    including the net name itself). Legal values are 4 through 66
    (default).

**ruler_origin** \<*value*\>

:   sets the zero origin for alternate time tick marks.

**ruler_step** \<*value*\>

:   sets the left/right step value for the alternate time tick marks
    from the origin. When this value is zero, alternate time tick marks
    are disabled.

**scale_to_time_dimension** \<*value*\>

:   The value can be any of the characters m, u, n, f, p, or s, which
    indicates which time dimension to convert the time values to. The
    default for this is \* which means that time dimension conversion is
    disabled.

**show_base_symbols** \<*value*\>

:   A nonzero value (default) indicates that the numeric base symbols
    for hexadecimal (\'\$\'), binary (\'%\'), and octal (\'#\') should
    be rendered. Otherwise they will be omitted.

**show_grid** \<*value*\>

:   A nonzero value (default) indicates that a grid should be drawn
    behind the traces. A zero indicates that no grid should be drawn.

**splash_disable** \<*value*\>

:   Turning this off enables the splash screen with the GTKWave mascot
    when loading a trace. Default is on.

**sst_dbl_action_type** \<*value*\>

:   Allows double-clicking to be active in the SST signals pane with the
    following actions possible: insert (default), replace, append,
    prepend, none. The value specified for the action is case
    insensitive and only the first letter is required. Invalid action
    types default to none.

**sst_dynamic_filter** \<*value*\>

:   When true (default) allows the SST dialog signal filter to filter
    signals while keys are being pressed, otherwise enter must be
    pressed to cause the filter to go active.

**sst_expanded** \<*value*\>

:   When true allows the SST dialog (when not hidden) to come up already
    expanded.

**strace_repeat_count** \<*value*\>

:   Determines how many times that edge search and pattern search will
    iterate on a search. This allows, for example, skipping ahead 10
    clock edges instead of 1.

**use_big_fonts** \<*value*\>

:   A nonzero value indicates that any text rendered into the wave
    window will use fonts that are four points larger in size than
    normal. This can enhance readability. A zero value indicates that
    normal font sizes should be used.

**use_fat_lines** \<*value*\>

:   A nonzero value indicates that any lines rendered into the wave
    window will be two pixels wide instead of a single pixel in width.
    This can enhance readability. A zero value indicates that normal
    line widths should be used.

**use_frequency_delta** \<*value*\>

:   allows you to switch between the delta time and frequency display in
    the upper right corner of the main window when measuring distances
    between markers. Default behavior is that the delta time is
    displayed (off).

**use_gestures** \<*value*\>

:   if supported by the GTK version will enable gestures such as swipe
    in the wave window. The default is that this feature is enabled if a
    touch screen is available (value is \"maybe\"). Values of on or off
    are also permissible.

**use_full_precision** \<*value*\>

:   does not round time values when the number of ticks per pixel
    onscreen is greater than 10 when active. The default is that this
    feature is disabled.

**use_maxtime_display** \<*value*\>

:   A nonzero value indicates that the maximum time will be displayed in
    the upper right corner of the screen. Otherwise, the current primary
    (unnamed) marker time will be displayed. This can be toggled at any
    time with the Toggle Max-Marker menu option.

**use_nonprop_fonts** \<*value*\>

:   Allows accelerated redraws of the signalwindow that can be done
    because the font width is constant. Default is off.

**use_pango_fonts** \<*value*\>

:   Uses anti-aliased pango fonts (GTK2) rather than bitmapped X11 ones.
    Default is on.

**use_roundcaps** \<*value*\>

:   A nonzero value indicates that vector traces should be drawn with
    rounded caps rather than perpendicular ones. The default for this is
    zero.

**use_scrollbar_only** \<*value*\>

:   A nonzero value indicates that the page, shift, fetch, and discard
    buttons should not be drawn (i.e., time manipulations should be
    through the scrollbar only rather than front panel buttons). The
    default for this is zero.

**use_scrollwheel_as_y** \<*value*\>

:   A nonzero value indicates that the scroll wheel on the mouse should
    be used to scroll the signals up and down rather than scrolling the
    time value from left to right.

**use_standard_clicking** \<*value*\>

:   This option no longer has any effect in gtkwave: normal GTK click
    semantics are used in the signalwindow.

**use_standard_trace_select** \<*value*\>

:   A nonzero value keeps the currently selected traces from deselecting
    on mouse button press. This allows drag and drop to function more
    smoothly. As this behavior is not how GTK normally functions, it is
    by default disabled.

**use_toolbutton_interface** \<*value*\>

:   A nonzero value indicates that a toolbar with buttons should be at
    the top of the screen instead of the traditional style gtkwave
    button groups. Default is on.

**vcd_explicit_zero_subscripts** \<*value*\>

:   indicates that signal names should be stored internally as
    name.bitnumber when enabled. When disabled, a more \"normal\"
    ordering of name\[bitnumber\] is used. Note that when disabled, the
    Bundle Up and Bundle Down options are disabled in the Signal Search
    Regexp, Signal Search Hierarchy, and Signal Search Tree options.
    This is necessary as the internal data structures for signals are
    represented with one \"less\" level of hierarchy than when enabled
    and those functions would not work properly. This should not be an
    issue if atomic_vectors are enabled. Default for
    vcd_explicit_zero_subscripts is disabled.

**vcd_preserve_glitches** \<*value*\>

:   indicates that any repeat equal values for a net spanning different
    time values in the VCD/FST file are not to be compressed into a
    single value change but should remain in order to allow glitches to
    be present for this case. Default for vcd_preserve_glitches is
    disabled.

**vcd_preserve_glitches_real** \<*value*\>

:   indicates that any repeat equal values for a real net spanning
    different time values in the VCD/FST file are not to be compressed
    into a single value change but should remain for this case. Default
    for vcd_preserve_glitches is disabled. The intended use is for when
    viewing analog interpolated data such that removing duplicate values
    would incorrectly deform the interpolation.

**vcd_warning_filesize** \<*value*\>

:   produces a warning message if the VCD filesize is greater than the
    argument\'s size in MB. Set to zero to disable this.

**vector_padding** \<*value*\>

:   indicates the number of pixels of extra whitespace that should be
    added to any strings for the purpose of calculating text in vectors.
    Permissible values are 0 to 16 with the default being 4.

**vlist_compression** \<*value*\>

:   indicates the value to pass to zlib during vlist processing (which
    is used in the VCD recoder). -1 disables compression, 0-9 correspond
    to the value zlib expects. 4 is default.

**vlist_prepack** \<*value*\>

:   indicates that the VCD recoder should pre-compress data going into
    the value change vlists in order to reduce memory usage. This is
    done before potential zlib packing. Default is off.

**vlist_spill** \<*value*\>

:   indicates that the VCD recoder should spill all generated vlists to
    a tempfile on disk in order to reduce memory usage. Default is off.

**wave_scrolling** \<*value*\>

:   a nonzero value enables scrolling by dragging the marker off the
    left or right sides of the wave window. A zero value disables it.

**zoom_base** \<*value*\>

:   allows setting of the zoom base with a value between 1.5 and 10.0.
    Default is 2.0.

**zoom_center** \<*value*\>

:   a nonzero value enables center zooming, a zero value disables it.

**zoom_dynamic** \<*value*\>

:   a nonzero value enables dynamic full zooming when using the partial
    VCD (incremental) loader, a zero value disables it.

**zoom_dynamic_end** \<*value*\>

:   a nonzero value enables dynamic zoom to the end when using the
    partial VCD (incremental) loader, a zero value disables it.

**zoom_pow10_snap** \<*value*\>

:   corresponds to the Zoom Pow10 Snap menu option. Default for this is
    disabled (zero).

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*gtkwave*(1)
