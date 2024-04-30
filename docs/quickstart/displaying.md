# Displaying Waveforms

In the preceding section, the viewer was brought up with a save file so
when the viewer did appear, the main window already had signals present
as seen in on page . All the signals in a model do not appear on their
own as this would be unwieldy for large models. Instead, it is up to the
user to import signals manually. An exception to this exists for VCD files, see
the definition of the [`enable_vcd_autosave` *.gtkwaverc* variable](#anchor-6).
That said, several requesters exist for importing signals into the main window.

## Signal Search

:::{figure-md}

![The Signal Search (regular expression search) Requester](../_static/images/quickstart2.png)

The Signal Search (regular expression search) Requester
:::

The signal search requester accepts a search string as a POSIX regular
expression. Any signals found in the dumpfile that match that regular
expression are listed in the Matches box and may be individually or
multiply selected and imported into the viewer window. The regular
expression can be modified in one of four ways: WRange, WStrand, Range,
and Strand. No modification is possible with None. This optionally
matches the string you enter in the search string above with a Verilog
format range (signal[7:0]), a strand (signal.1, signal.0), or with no
suffix. The "W" modifier for "Range" and "Strand" explicitly matches on
word boundaries. (addr matches unit.freezeaddr[63:0] for "Range" but
only unit.addr[63:0] for "WRange" since addr has to be on a word
boundary.) Note that when "None" is selected, the search string may be
located anywhere in the signal name.

Append will add the selected signals to end of the display on the main
window.

Insert will add selected signals after last highlighted signal on the
main window.

Replace will replace highlighted signals on the main window with signals
selected.

## Hierarchy Search

:::{figure-md}

![The Hierarchy Search Requester](../_static/images/quickstart3.png)

The Hierarchy Search Requester
:::

The hierarchy search requester provides a view of the hierarchy in a
format similar to the current working directory of a file in a file
system on a computer. The Signal Hierarchy box contains the current
hierarchy and the Children box contain all of the signals in that
immediate level of hierarchy and all of the component instantiation
names for that level of hierarchy (denoted by a "(+)" prefix). To
navigate down a level of hierarchy, click on an item with a "(+)"
prefix. To move up a level of hierarchy, click on the ".." line.

Selecting individual items allow you to import traces singly when the
Append, Insert, or Replace buttons are clicked. Not selecting anything
will do a "deep import" such that all the child signals are imported.
Use of that feature is not recommended for very large designs.

Note that is is possible to modify the display order such that
components and signals are intermixed in this gadget rather than being
separated such that all the components for a given level of hierarchy
are listed alphabetically at the top and all signals are listed
alphabetically at the bottom. In order to do this, toggle the Search
submenu item Search Hierarchy Grouping as described on page
[43](#anchor-18).

## Tree Search

The Tree Search Requester is the requester that most users will feel
comfortable using and is also the requester that can optionally be
embedded in the main window on versions of GTK greater than or equal to
2.4. See on page for an example of this.

The Tree Search Requester is composed of a top tree selection box, a
signals box, and a POSIX regular expression filter. The tree selection
box is used to navigate at the hierarchy level. Click on an item in
order to show the signals at that level of hierarchy. In the figure on
page , the "top" level of hierarchy is selected and the signals box
shows what signals are available at that level of hierarchy. Signals may
be individually or multiply selected and can be dragged and dropped into
the signal window. In addition, a POSIX filter can be specified that
allows the selective filtering of signal names at a level of hierarchy
which is handy for finding a specific signal at a level of hierarchy
that is very large (e.g., in a synthesized netlist).

:::{figure-md}

![The Signal Search Tree Requester](../_static/images/quickstart4.png)

The Signal Search Tree Requester
:::

## Signal Save Files

The signals show in the main window can be saved to a file so they can
automatically be imported without reselection the next time the viewer
is started. In order to save signals to a save file, select the File
submenu option Write Save File (As). Save files can also be loaded at
any time by selecting the Read Save File option.

## Pattern Search

Values, not only nets may be searched on and marked in the wave window.
In order to do this, select one or more nets in the signal window and
then click on the Search submenu option Pattern Search. A Pattern Search
Requester will then appear that will allow various types of search
operations for the signals that have been selected.

The following is an example of a Pattern Search Requester being used to
mark the rising edges for the clock signal in a simulation model.

:::{figure-md}

![The Pattern Search Requester](../_static/images/quickstart5.png)

The Pattern Search Requester
:::

The edges as they are marked by the configuration of
the Requester in Figure can be seen in on page .

To remove pattern marks, either select another pattern or select the
View submenu option Remove Pattern Marks. Note that pattern marks save
to the save file and that the actual pattern search criteria is saved,
not the absolute times of the individual marks themselves.

Search criteria for individual nets can be edge or value based. For
"String" searches (the entry box to the right of the search type box
which in the case above is marked "Rising Edge"), note that is is no
longer required that you must press Enter for the string in order to
commit the value to the search.

## Alias Files and Attaching External Disassemblers

The viewer supports signal aliasing through both plaintext filters and
through external program filters. Note that signal aliasing is a strict
one-to-one correspondence so the value represented in the viewer must
exactly represent what format your filter expects. (e.g., binary,
hexadecimal, with leading base markers, etc.) For your convenience, the
comparisons are case insensitive.

For text filters, the viewer looks at an ASCII text file of the
following format:

```text
#
# this is a comment
#
00 Idle
01 Advance
10 Stop
11 Reset
```

The first non-whitespace item is treated as a literal value that would
normally be printed by the viewer and the remaining items on the line
are substitution text. Any time this text is encountered if the filter
is active, it will replace the left-hand side text with the right-hand
side. Leading and trailing whitespaces are removed from the right-hand
side item.

To turn on the filter:

1. Highlight the signals you want filtered
1. Edit->Data Format->Translate Filter File->Enable and Select
1. Add Filter to List
1. Click on filter filename
1. Select filter filename from list
1. OK

To turn off the filter:

1. Highlight the signals you want unfiltered.
1. Edit->Data Format->Translate Filter File->Disable

!!! note
    Filter configurations load and save properly to and from save files.

An external process that accepts one line in from stdin and returns with
data on stdout can be used as a process filter. An example of this are
disassemblers. The following sample code would show how to interface
with a disassembler function in C:

```c
int main(int argc, char **argv)
{
    while (!feof(stdin)) {
        char buf[1025], buf2[1025];
        buf[0] = 0;
        fscanf(stdin, "%s", buf);
        if (buf[0]) {
            int hx;
            sscanf(buf, "%x", &hx);
            ppc_dasm_one(buf2, 0, hx);
            printf("%s\n", buf2);
            fflush(stdout);
        }
     }
    return 0;
}
```

Note that the `#!c fflush(stdout)` is necessary, otherwise *gtkwave* will
hang. Also note that every line of input needs to generate a line of
output or the viewer will hang too.

To turn on the filter:

1. Highlight the signals you want filtered
1. Edit->Data Format->Translate Filter Process->Enable and Select
1. Add Proc Filter to List
1. Click on filter filename
1. Select filter filename from list
1. OK

To turn off the filter:

1. Highlight the signals you want unfiltered.
1. Edit->Data Format->Translate Filter Process->Disable

Note: In order to use the filter to modify the background color of a
trace, you can prefix the return string to stdout with the X11 color
name surrounded by '?' characters as follows:

```text
?CadetBlue?isync
?red?xor r0,r0,r0
?lavender?lwz r2,0(r7)
```

Legal color names may be found in the `rgb.c` file in the source code
distribution.
