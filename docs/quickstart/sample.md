# Sample Design

In the *examples/* directory of the source code distribition a sample
Verilog design and testbench for a DES encryptor can be found as
*des.v*.

```text
10 :/home/bybell/gtkwave-3.0.0pre21/examples> ls -al
total 132
drwxrwxr-x 2 bybell bybell 4096 Apr 30 14:12 .
drwxr-xr-x 8 bybell bybell 4096 Apr 29 22:05 ..
-rw-rw-r-- 1 bybell bybell 187 Apr 29 22:09 des.sav
-rw-r--r-- 1 bybell bybell 47995 Apr 29 22:05 des.v
-rw-rw-r-- 1 bybell bybell 68801 Apr 29 22:06 des.vzt
```

If you have a Verilog simulator handy, you can simulate the design to
create a VCD file. To try the example in Icarus Verilog
([http://www.icarus.com](http://www.icarus.com/)), type the following:

```text
/tmp/gtkwave-3.0.0/examples> **iverilog des.v && a.out**
VCD info: dumpfile des.vcd opened for output.
/tmp/gtkwave-3.0.0/examples> **ls -la des.vcd**
-rw-rw-r-- 1 bybell bybell 3465481 Apr 30 13:39 des.vcd
```

If you do not have a simulator readily available, you can expand the
*des.vzt* file into *des.vcd* by typing the following:

```text
/tmp/gtkwave-3.0.0/examples> vzt2vcd des.vzt >des.vcd
VZTLOAD | 1432 facilities
VZTLOAD | Total value bits: 22921
VZTLOAD | Read 1 block header OK
VZTLOAD | [0] start time
VZTLOAD | [704] end time
VZTLOAD |
VZTLOAD | block [0] processing 0 / 704
/tmp/gtkwave-3.0.0/examples> ls -la des.vcd
-rw-rw-r-- 1 bybell bybell 3456247 Apr 30 13:42 des.vcd
```

You will notice that the generated VCD file is about fifty times larger
than the VZT file. This illustrates the compressibility of VCD files and
the space saving advantages of using the database formats that GTKWave
supports. Normally we would not want to work with VCD as GTKWave is
forced to process the whole file rather than access only the data
needed, but in the next section we will show how to invoke GTKWave such
that VCD files are automatically converted into LXT2 ones.

Next, let's create a stems file that allows us to bring up RTLBrowse.

```text
/tmp/gtkwave-3.0.0/examples> verilator -Wno-fatal des.v -xml-only
--bbox-sys && xml2stems obj_dir/Vdes.xml des.stems
/tmp/gtkwave-3.0.0/examples> ls -la des.stems
-rw-rw-r-- 1 bybell bybell 4044 Apr 30 13:50 des.stems
```

Stems files only need to be generated when the source code undergoes
file layout and/or hierarchy changes.

Now that we have a VCD file and a stems file, we can bring up the
viewer.
