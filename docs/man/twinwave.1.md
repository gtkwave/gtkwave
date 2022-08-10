---
date: 3.3.79
myst:
  title_to_header: true
section: 1
title: twinwave
---

## NAME

twinwave - Wraps multiple GTKWave sessions in one window or two
synchronized windows

## SYNTAX

twinwave \<*arglist1*\> \<*separator*\> \<*arglist2*\>

## DESCRIPTION

Wraps multiple GTKWave sessions with synchronized markers, horizontal
scrolling, and zooming.

## EXAMPLES

To run this program the standard way type:

twinwave filename1.vcd filename1.sav + filename2.vcd filename2.sav

:   Two synchronized viewers are then opened in one window. Note that
    accelerator keys for menus will be disabled as some window managers
    have focus issues and route keystrokes to the wrong part of the
    window.

twinwave filename1.vcd filename1.sav ++ filename2.vcd filename2.sav

:   Two synchronized viewers are then opened in two windows.

## LIMITATIONS

*twinwave* uses the GtkSocket/GtkPlug mechanism to embed two
*gtkwave*(1) sessions into one window. The amount of coupling is
currently limited to communication of temporal information. Other than
that, the two gtkwave processes are isolated from each other as if the
viewers were spawned separately. Keep in mind that using the same save
file for each session may cause unintended behavior problems if the save
file is written back to disk: only the session written last will be
saved. (i.e., the save file isn\'t cloned and made unique to each
session.) Note that *twinwave* compiled against Quartz (not X11) on OSX
as well as MinGW does not place both sessions in a single window.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*gtkwave*(1)
