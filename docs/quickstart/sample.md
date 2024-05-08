# Creating Example Waveform

In the `examples/` directory of the source code distribution, an example
Verilog design and testbench for a DES encryptor can be found as
`des.v`.

```text
gtkwave> cd examples
gtkwave/examples> ls -al
total 132
drwxrwxr-x 2 bybell bybell 4096 Apr 30 14:12 .
drwxr-xr-x 8 bybell bybell 4096 Apr 29 22:05 ..
-rw-rw-r-- 1 bybell bybell 187 Apr 29 22:09 des.sav
-rw-r--r-- 1 bybell bybell 47995 Apr 29 22:05 des.v
-rw-rw-r-- 1 bybell bybell 68801 Apr 29 22:06 des.vzt
```

If you have a Verilog simulator handy, you can simulate the design to
create a VCD file. To try the example in 
[Icarus Verilog](http://www.icarus.com/), type the following:

```text
gtkwave/examples> iverilog -D GENERATE_VCD des.v && ./a.out
VCD info: dumpfile des.vcd opened for output.
gtkwave/examples> ls -la des.vcd
-rw-rw-r-- 1 bybell bybell 3465481 Apr 30 13:39 des.vcd
```

Or, if you do not have a simulator readily available, you can expand the
*des.fst* file into *des.vcd* by typing the following:

```text
gtkwave/examples> fst2vcd des.fst >des.vcd
gtkwave/examples> ls -la des.vcd
-rw-rw-r-- 1 bybell bybell 3456247 Apr 30 13:42 des.vcd
```

You will notice that the generated `VCD` file is much larger than the
`FST` file. This illustrates the compressibility of `VCD` files and
the space-saving advantages of using the database formats that GTKWave
supports. Normally we would not want to work with `VCD` as GTKWave is
forced to process the whole file rather than access only the data
needed. However, in the next section, we will show how to invoke GTKWave such
that `VCD` files are automatically converted into `FST` ones.

Next, let's create a `stems` file that allows us to bring up RTLBrowse.
Using [Verilator](https://verilator.org/).

```text
gtkwave/examples> verilator -Wno-fatal --no-timing des.v -xml-only \
&& xml2stems obj_dir/Vdes.xml des.stems
gtkwave/examples> ls -la des.stems
-rw-rw-r-- 1 bybell bybell 4044 Apr 30 13:50 des.stems
```

`stems` files only need to be generated when the source code undergoes
file layout and/or hierarchy changes.

Now that we have a `VCD` file and a `stems` file, we can bring up the
viewer.


