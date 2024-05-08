# Toolbar

The toolbar, also known as "navigation and status panel"
occupies the top part of the main window.

:::{figure-md}

![The Navigation and Status Panel](../_static/images/navigation.png)

The Navigation and Status Panel
:::

The Navigation Panel contains useful tools like `Zoom In/Out` and
`Find Edge`.

`Menu`
: _Menu_ will open the [Menu](menu.md#menu) of GTKWave.

`Cut Traces`
: _Cut Traces_ removes highlighted signals from the display and places
them in an offscreen cut/copy buffer for later Paste operations.
It also implicitly destroys the previous contents of the cut/copy buffer.

`Copy Traces`
: _Copy Traces_ copies highlighted signals from the display
and places them in an offscreen cut/copy buffer for later Paste operations.
It also implicitly destroys the previous contents of the cut/copy buffer.

`Paste Traces`
: _Paste Traces_ pastes signals from an offscreen cut/copy buffer and
places them in a group after the last highlighted signal, or at the
end of the display if no signal is highlighted.

`Zoom Fit`
: _Zoom Fit_ zoom out to display the full range of simulation time.
If the baseline marker is set, zooms between the baseline marker
and the primary marker.

`Zoom In`
: _Zoom In_ is used to increase the zoom factor around the marker.

`Zoom Out`
: _Zoom Out_ is used to decrease the zoom factor around the marker.

`Zoom Undo`
: _Zoom Undo_ is used to revert to the previous zoom value used.
Undo only works one level deep.

`Zoom to Start`
: _Zoom to Start_ is used to jump to the trace's beginning.

`Zoom to End`
: _Zoom to End_ is used to jump to the trace's end.

`Find Previous Edge`
: _Find Previous Edge_ moves the marker to the nearest transition
on the left side of the primary marker of the last highlighted trace.
If the primary marker is not located, it starts from max time.

`Find Next Edge`
: _Find Next Edge_ moves the marker to the nearest transition
on the right side of the primary marker of the last highlighted trace.
If the primary marker is not located, it starts from min time.

`From / To boxes`
: _The "From" and "To" boxes_ indicate the start and end times for what part
of the simulation run shall be visible and can be navigated inside the
wave frame. Values can directly be entered into these boxes, and
units (e.g., ns, ps, fs) can also be affixed to values. Named marker
is also supported, use `MX` for named marker `X`.

`Reload`
: _Reload_ will reload the currently displayed waveform.
Only available with some dumpfile types.

The Marker Time label indicates where the primary marker is located. If
it is not present, a double-dash ("\--") is displayed. The Current Time
label indicates where the mouse is pointing. Its function is to
determine the time under the cursor without having to activate or move
the primary marker. Note that when the primary marker is being
click-dragged, the Marker Time label will indicate the delta time off
the initial marker click.

When the baseline marker is set. The Current Time label is replaced
with the Base Time label that indicates the value of the baseline
marker. Then the Marker time label indicates the delta time between
the baseline marker and the primary marker.
