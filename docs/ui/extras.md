# Ergonomic Extras

## Scroll Wheels

Users with a scroll wheel mouse have some extra one-handed operations
options available which correspond to some functions found in the
[Toolbar](toolbar.md#toolbar).

| Function           | Shortcut                         |
|--------------------|----------------------------------|
| Scroll Right       | {kbd}`Scroll_Wheel_Up`           |
| Scroll Left        | {kbd}`Scroll_Wheel_Down`         |
| Find Previous Edge | {kbd}`Alt+Scroll_Wheel_Up`       |
| Find Next Edge     | {kbd}`Alt+Scroll_Wheel_Down`     |
| Zoom In            | {kbd}`Control+Scroll_Wheel_Up`   |
| Zoom Out           | {kbd}`Control+Scroll_Wheel_Down` |

Turning the scroll wheel is far faster than pressing the navigation
buttons. Zoom functions are especially smooth this way.

## The Primary Marker

The primary marker has also had function overloaded onto it for user
convenience. 

Besides being used as a marker, it can also be used to
navigate with respect to time. It can be dropped with the `right mouse`
button and dragged to "open" up a region for zooming in closer or out
farther in time. 

It can also be used to scroll by holding down the `left
mouse` button and dragging the mouse outside the signal subwindow. The
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

:::{attention}
Deprecated and will be removed in GTKWave 4.
:::
