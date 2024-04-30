# TwinWave

TwinWave is a front end to GTKWave that allows two sessions to be open
at one time in a single window. The horizontal scrolling, zoom factor,
primary marker, and secondary marker are synchronized between the two
sessions.

:::{figure-md}

![TwinWave managing two GTKWave sessions in a single window](../_static/images/twinwave.png)

TwinWave managing two GTKWave sessions in a single window
:::

Starting a TwinWave session is easy: simply invoke
twinwave with the arguments for each gtkwave session listed fully
separating them with a plus sign.

```bash
twinwave a.vcd a.sav + b.vcd b.sav
```
