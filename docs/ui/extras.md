# Ergonomic Extras

## Scroll Wheels

Users with a scroll wheel mouse have some extra one-handed operations
options available which correspond to some functions found in the
[Navigation and Status Panel](gtkwave.md#navigation-and-status-panel).

| Function   | Shortcut                         |
|------------|----------------------------------|
| Shift Right| {kbd}`Control+Scroll_Wheel_Down` |
| Shift Left | {kbd}`Control+Scroll_Wheel_Up`   |
| Page Right | {kbd}`Scroll_Wheel_Down`         |
| Page Left  | {kbd}`Scroll_Wheel_Up`           |
| Zoom In    | {kbd}`Alt+Scroll_Wheel_Down`     |
| Zoom Out   | {kbd}`Alt+Scroll_Wheel_Up`       |

Turning the scroll wheel "presses" the shift, page, and zoom options
repeatedly far faster than is possible with the navigation buttons. Zoom
functions are especially smooth this way.

## The Primary Marker

The primary marker has also had function overloaded onto it for user
convenience. Besides being used as a marker, it can also be used to
navigate with respect to time. It can be dropped with the right mouse
button and dragged to "open" up a region for zooming in closer or out
farther in time. It can also be used to scroll by holding down the left
mouse button and dragging the mouse outside the signal subwindow. The
simulation data outside of the window will then scroll into view with
the scrolling being in the opposite direction that the primary marker is
"pulling" outside of the subwindow.

## Interactive VCD

VCD files may be viewed as they are generated provided that they are
written to a fifo (pipe) and are trampolined through shmidcat first
(assume the simulator will normally generate *outfile.vcd*):

```bash
mkfifo outfile.vcd
cver myverilog.v &
shmidcat outfile.vcd | gtkwave -v -I myverilog.sav
```

You can then navigate the file as simulation is running and watch it
update.
