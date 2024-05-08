# VCD Recoding

## VList Recoding Stategy

VCD files can now be recoded in gtkwave on a per-signal basis using a
modified form of VList. The VList structure used by gtkwave is as
follows:

```c
struct vlist_t {
    struct vlist_t *next;
    unsigned int siz;
    int offs;
    unsigned int elem_siz;
};
```

The elem_siz is always equal to 1 byte. For the first structure, the siz
field is 1. For the next one, it will be 2, then 4, and so forth. Given
this doubling property, this structure allows a dynamically growing
*indexable* array. The offs field is a pointer to the next element to be
written to the array. It starts at zero. When the offs value is equal to
siz, another VList is prepended in front of the existing one. Note that
the siz number of elements are allocated directly after the vlist_t
structure, so the first element can be found by skipping sizeof(vlist_t)
bytes from the start of the vlist_t structure.

When a new vlist_t is prepended in front of an old one, a compaction of
the data elements following the old vlist_t is attempted with zlib when
the number of bytes to compact is 64 or greater. If the compaction
results in a savings of space, the uncompressed vlist_t is discarded and
the compressed one is kept. To signify that a particular vlist_t is
compressed, the offs field is negated. (Thus, when accessing the list
later, a negative offset indicates that the vlist_t structure in
question is compressed.) Note that for a given VList, it is possible
that there will be both compressed and uncompressed vlist_t structures,
and this will have to be taken into account when they are accessed
later.

When a vlist_t is finalized (i.e., when no more elements are to be added
to it), a compression is attempted. If that fails (i.e., not appreciable
size savings), a naive truncation of the unused bytes
(siz-offs)*elem_siz is done. Given the nature of the data in this list,
compression usually succeeds.

These VList structures remain dormant in memory in their (possibly)
compressed form until they are needed to be accessed. At that time they
are decompressed (if required), traversed, and destroyed as they will no
longer be needed. The actual data contained in the memory area following
the vlist_t structures to represent VHDL/Verilog value changes will now
be described.

## Time Encoding

Along with the value changes, an uncompressed VList of 64-bit integers
is also generated as time values are parsed from the VCD file. (i.e.,
lines of the form "#1000") As the time values are added to that VList,
a numerical index (zeroth, first, second) is maintained separately that
indicates what the current time index is.

The reason for maintaining a list of indices is so that value changes
can be encoded as relative distances in this list rather than actual
64-bit integers.

## Single-bit Encoding

Single bit value changes are encoded as a variable length integer of the
format (delta<<2)|(zero_one_bit<<1) when the value is zero or one,
or (delta<<4)|rcv_bit_value for all other bit values.

The "delta" value represents how many timesteps in the VCD file have
taken place since the previous value change for a signal. Look at the
following for an example:

```text
#1000 clk = 0 index = n
#1001 n + 1
#1010 n + 2
#1013 n + 3
#1100 clk = 1 n + 4
```

...so for the value change on clk at time #1100, the delta is 4.

The rcv_bit_value when the bit value is not zero or one is encoded as
the position numbered 0-7 in the string "XZHUWL-?" multiplied by 2
with one added to the result. (i.e., 1, 3, 5, 7, 9, 11, 13, 15)

The variable length integer is generated with the following algorithm.
It shifts the value out seven bits at a time and sets the high bit on
the last byte of the variable length integer:

```c
unsigned int v; // value
char *pnt; // destination pointer
while ((nxt = v>>7)) {
    *(pnt++) = (v&0x7f);
    v = nxt;
}
*pnt = (v&0x7f) | 0x80;
```

Using this scheme, most value changes can be encoded in one or two
(uncompressed) bytes. For the example above, the tuple (4, '1')
encodes into an integer as:

(4<<2) | (1<<1) = 0x12

As a variable length integer, it is stored as a single (uncompressed)
byte 0x92.

## Multi-bit Encoding

Multiple bits are encoded as a variable length integer representing the
time delta (without any left shifting), and immediately after that a
reformatted string is encoded as packed nybbles against the MVL9 string
"0XZ1HUWL-". As multi-bit strings can be of any length, the value of
15 is used to signify the end of string marker. An example:

```text
#1000 val = 1010 index = n
#1001 n + 1
#1010 n + 2
#1013 n + 3
#1100 val = 1zz0 n + 4
```

Thus, the tuple (4, "1ZZ0") at time #1100 is encoded bytewise as:

```text
0x84 [variable length integer for delta of 4]
0x32 ["1Z": "0XZ1HUWL-"[3], "0XZ1HUWL-"[2]]
0x20 ["Z0": "0XZ1HUWL-"[2], "0XZ1HUWL-"[0]]
0xf0 [end of string marker + nybble pad to byte boundary]
```

For longer strings, this provides a 2:1 space compaction prior to calling zlib.

## Reals and String Encoding

They are stored simply as (delta, null terminated string) without any
re-encoding of the real or string from its ASCII representation. So for
the value (4, "3.14159"), it is encoded bytewise as

```text
0x84 [variable length integer for delta of 4]
0x33 0x2e 0x31 0x34 0x31 0x35 0x39 0x00 ["3.14159" with null
termination]
```

## Final Notes on VCD Recoding

Even with zlib compression disabled (which gtkwave allows for
performance), the memory usage savings are substantial. There are
several reasons for this:

- Storing VCD identifiers is completely unnecessary as the value
  change data is routed to its appropriate VList. Hence, the
  identifier implicitly is represented by the VList itself.

- Single-bit changes can be represented in only one or two bytes in
  most cases.

- Multi-bit changes can be represented with slightly less than half
  the amount of memory required normally (as the VCD identifier is no
  longer required).

- The amount of "next" pointers required per-VList is lg(n bytes).
  This allows a low overhead even when having a large number of active
  growable VList "streams" in memory at once.

- VList truncation when the lists are finalized at the end of the VCD
  file read ensure that unused VList space is returned to the
  operating system.
