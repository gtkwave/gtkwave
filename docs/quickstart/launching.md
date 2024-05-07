# Launching GTKWave

We already have a `.fst` file available, but to illustrate the automatic
conversion of VCD files, let's use the `-o` option. The `-t` option is used
to specify the `stems` file. The `.gtkw` file is a "save file" that contains
GTKWave scope state.

```text
gtkwave/examples> gtkwave -o -t des.stems des.vcd des.gtkw
GTKWave Analyzer v3.3.118 (w)1999-2023 BSI
FSTLOAD | Processing 1432 facs.
FSTLOAD | Built 1287 signals and 145 aliases.
FSTLOAD | Building facility hierarchy tree.
FSTLOAD | Sorting facility hierarchy tree.
```

:::{figure-md}

![The main window with viewer state loaded from a save file](../_static/images/quickstart1.png)

The main window with viewer state loaded from a save file
:::

After splash screen, the GTKWave main window and an RTLBrowse hierarchy
window will appear. We are now ready to start experimenting with
various features of the wave viewer and RTLBrowse.
