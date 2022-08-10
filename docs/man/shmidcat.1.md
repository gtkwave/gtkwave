---
date: 3.0.8
myst:
  title_to_header: true
section: 1
title: shmidcat
---

## NAME

shmidcat - Copies stdin/file to a shared memory block for *gtkwave*(1)

## SYNTAX

shmidcat \[*VCDFILE*\]

## DESCRIPTION

Copies either the file specified at the command line or stdin (if no
file specified) line by line to a shared memory block. stdout will
contain a shared memory ID which should be passed on to *gtkwave*(1).

## EXAMPLES

To run this program the standard way type:

cat whatever.vcd \| shmidcat

:   The shared memory ID is emitted to stdout.

shmidcat whatever.vcd \| gtkwave -v -I whatever.sav

:   GTKWave directly grabs the ID from stdin.

## LIMITATIONS

This program is mainly for illustrative and testing purposes only. Its
primary use is for people who wish to write interactive VCD dumpers for
*gtkwave*(1) as its source code may be examined, particularly the
emit_string() function. It can also be used to test if an existing VCD
file will load properly in interactive mode. Note that it can also be
used to redirect VCD files which are written into a pipe to *gtkwave*(1)
in a non-blocking fashion.

## AUTHORS

Anthony Bybell \<bybell@rocketmail.com\>

## SEE ALSO

*gtkwave*(1)
