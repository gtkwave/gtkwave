# LXT File Format

## LXT Framing

The three most important values in an LXT(interLaced eXtensible Trace)
file are the following:

```c
#define LT_HDRID (0x0138)
#define LT_VERSION (0x0001)
#define LT_TRLID (0xB4)
```

An LXT file starts with a two byte `LT_HDRID` with the two byte version
number LT_VERSION concatenated onto it. The last byte in the file is the
LT_TRLID. These five bytes are the only "absolutes" in an LXT file.

```text
01 38 00 01* ...file body... *B4
```

As one may guess from the example above, *all* integer values
represented in LXT files are stored in big endian order.

Note that all constant definitions found in this appendix may be found
in the header file `src/helpers/lxt_write.h`. Note that LXT2 files use a
completely different file format as well as different constant values.

### LXT Section Pointers

Preceding the trailing ID byte *B4* is a series of tag bytes which
themselves are preceded by 32-bit offsets into the file which indicate
where the sections pointed to by the tags are located. The exception is
tag *00* (LT_SECTION_END) which indicates that no more tags/sections are
specified:

```text
00* ... offset_for_tag_2, tag_2, offset_for_tag_1, tag_1, *B4
```

Currently defined tags are:

```c
#define LT_SECTION_END              (0)
#define LT_SECTION_CHG              (1)
#define LT_SECTION_SYNC_TABLE       (2)
#define LT_SECTION_FACNAME          (3)
#define LT_SECTION_FACNAME_GEOMETRY (4)
#define LT_SECTION_TIMESCALE        (5)
#define LT_SECTION_TIME_TABLE       (6)
#define LT_SECTION_INITIAL_VALUE    (7)
#define LT_SECTION_DOUBLE_TEST      (8)
#define LT_SECTION_TIME_TABLE64     (9)
```

Let's put this all together with an example:

The first tag encountered is *08* (*LT_SECTION_DOUBLE_TEST*) at 339. Its
offset value indicates the position of the double sized floating point
comparison testword. Thus, the section location for the testword is at
0309 from the beginning of the file.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 00 00 00 00 04 01 00 00 02 4b 02 00 00 00 be @.........K.....
00000320: 03 00 00 01 4b 04 00 00 03 08 05 00 00 02 8b 06 ....K...........
00000330: 00 00 03 07 07 <b><u>00 00 03 09</u> 08</b> b4 -- -- -- -- -- ...........*
</code></pre></div>

The next tag encountered is *07* (*LT_SECTION_INITIAL_VALUE*) at 334.
Its offset value indicates the position of the simulation initial value.
Even though this value is a single byte, its own section is defined. The
reasoning behind this is that older versions of LXT readers would be
able to skip unknown sections without needing to know the size of the
section, how it functions, etc.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 00 00 00 00 04 01 00 00 02 4b 02 00 00 00 be @.........K.....
00000320: 03 00 00 01 4b 04 00 00 03 08 05 00 00 02 8b 06 ....K...........
00000330: <b><u>00 00 03</u> 07</b> 07 00 00 03 09 08 b4 -- -- -- -- -- ...........
</code></pre></div>

The next tag encountered is *06* (*LT_SECTION_TIME_TABLE*) at 32F. Its
offset value (the underlined four byte number) indicates the position of
the time table which stores the time value vs positional offset for the
value change data.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 00 00 00 00 04 01 00 00 02 4b 02 00 00 00 be @.........K.....
00000320: 03 00 00 01 4b 04 00 00 03 08 05 <b><u>00 00 02 8b</u> 06</b> ....K...........
00000330: 00 00 03 07 07 00 00 03 09 08 b4 -- -- -- -- -- ...........
</code></pre></div>

The next tag encountered is *05* (*LT_SECTION_TIMESCALE*) at 32A. Its
offset value indicates the position of the timescale byte.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 00 00 00 00 04 01 00 00 02 4b 02 00 00 00 be @.........K.....
00000320: 03 00 00 01 4b 04 <b><u>00 00 03 08</u> 05</b> 00 00 02 8b 06 ....K...........
00000330: 00 00 03 07 07 00 00 03 09 08 b4 -- -- -- -- -- ...........
</code></pre></div>

The next tag encountered is *04* (*LT_SECTION_FACNAME_GEOMETRY*) at 325.
Its offset value indicates the geometry (array/msb/lsb/type/etc) of the
dumped facilities (signals) in the file.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 00 00 00 00 04 01 00 00 02 4b 02 00 00 00 be @.........K.....
00000320: 03 <b><u>00 00 01 4b</u> 04</b> 00 00 03 08 05 00 00 02 8b 06 ....K...........
00000330: 00 00 03 07 07 00 00 03 09 08 b4 -- -- -- -- -- ...........
</code></pre></div>

The next tag encountered is *03* (*LT_SECTION_FACNAME*) at 320. Its
offset value indicates where the compressed facility names are stored.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 00 00 00 00 04 01 00 00 02 4b 02 <b><u>00 00 00 be</u></b> @.........K.....
00000320: <b>03</b> 00 00 01 4b 04 00 00 03 08 05 00 00 02 8b 06 ....K...........
00000330: 00 00 03 07 07 00 00 03 09 08 b4 -- -- -- -- -- ...........
</code></pre></div>

The next tag encountered is *02* (*LT_SECTION_SYNC_TABLE*) at 31B. Its
offset value points to a table where the final value changes for each
facility may be found.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 00 00 00 00 04 01 <b><u>00 00 02 4b</u> 02</b> 00 00 00 be @.........K.....
00000320: 03 00 00 01 4b 04 00 00 03 08 05 00 00 02 8b 06 ....K...........
00000330: 00 00 03 07 07 00 00 03 09 08 b4 -- -- -- -- -- ...........
</code></pre></div>

The next tag encountered is *01* (*LT_SECTION_CHG*) at 316. Its offset
value points to the actual value changes in the file.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 00 <b><u>00 00 00 04</u> 01</b> 00 00 02 4b 02 00 00 00 be @.........K.....
00000320: 03 00 00 01 4b 04 00 00 03 08 05 00 00 02 8b 06 ....K...........
00000330: 00 00 03 07 07 00 00 03 09 08 b4 -- -- -- -- -- ...........
</code></pre></div>

The final tag encountered is *00* at 311. It signifies that there are no
more tags.

<div class="highlight"><pre><code>
00000300: XX XX XX XX XX XX XX XX XX 6e 86 1b f0 f9 21 09 .........n....!.
00000310: 40 <b>00</b> 00 00 00 04 01 00 00 02 4b 02 00 00 00 be @.........K.....
00000320: 03 00 00 01 4b 04 00 00 03 08 05 00 00 02 8b 06 ....K...........
00000330: 00 00 03 07 07 00 00 03 09 08 b4 -- -- -- -- -- ...........
</code></pre></div>

Note that with the exception of the termination tag *00*, tags may be
encountered in any order. The fact that they are encountered in
monotonically decreasing order in the example above is an implementation
detail of the lxt_write dumper. Code which processes LXT files should be
able to handle tags which appear in any order. For tags which are
defined multiple times, it is to be assumed that the tag instance
closest to the termination tag is the one to be used unless each unique
instantiation possesses a special meaning. Currently, repeated tags have
no special semantics.

## LXT Section Definitions

The body of each section (as currently defined) will now be explained in
detail.

### 08: LT_SECTION_DOUBLE_TEST

This section is only present if double precision floating point data is
to be found in the file. In order to resolve byte ordering issues across
various platforms, a rounded version of pi (3.14159) is stored in eight
consecutive bytes. This value was picked because each of its eight bytes
are unique. It is the responsibility of an LXT reader to compare the
byte ordering found in the LT_SECTION_DOUBLE_TEST section to that of the
same rounded version of pi as represented by reader\'s processor. By
comparing the position on the host and in the file, it may be determined
how the values stored in the LXT file need to be rearranged. The
following bit of code shows one possible implementation for this:

```c
static char double_mas[8] = {0,0,0,0,0,0,0,0};
static char double_is_native = 0;

static void create_double_endian_mask(double *pnt)
{
    static double p = 3.14159;
    double d = *pnt
    if (p == d) {
        double_is_native = 1;
    } else {
        char *remote = (char *)&d;
        char *here = (char *)&p;
        for (int i = 0; i < 8; i++) {
            for(int j = 0; j < 8; j++) {
                if (here[i] == remote[j]) {
                    double_mask[i] = j;
                    break;
                }
            }
        }
    }
}
```

If *double_is_native* is zero, the following function will then be
needed to be called to rearrange the file byte ordering to match the
host every time a double is encountered in the value change data:

```c
static char *swab_double_via_mask(double *d) {
    char swapbuf[8];
    char *pnt = malloc(8 * sizeof(char));
    memcpy(swapbuf, (char *)d, 8);

    for (int i = 0; i < 8; i++) {
        pnt[i] = swapbuf[double_mask[i]];
    }

    return pnt;
}
```

### 07: LT_SECTION_INITIAL_VALUE

This section is used as a "shortcut" representation to flash all
facilities in a dumpfile to a specific value at the initial time.
Permissible values are '0', '1', 'Z', 'X', 'H', 'U', 'W',
'L', and '-' stored as the byte values 00 through 08 in the LXT
file.

### 06: LT_SECTION_TIME_TABLE / 08: LT_SECTION_TIME_TABLE64

This section marks the time vs positional data for the LXT file. It is
represented in the following format:

4 bytes: *number of entries (n)*  
4 bytes: *min time in dump* (8 bytes for *LT_SECTION_TIME_TABLE64*  
4 bytes: *max time in dump* (8 bytes for *LT_SECTION_TIME_TABLE64*  

*n* 4-byte positional delta entries follow  
*n* 4-byte time delta entries follow (8 byte entries for *LT_SECTION_TIME_TABLE64*)  

It is assumed that the delta values are represented as *current_value -
previous_value*, which means that deltas should always be positive. In
addition, the *previous_value* for delta number zero for both position
and time is zero. This will allow for sanity checking between the time
table and the min/max time fields if it is so desired or if the min/max
fields are needed before the delta entries are traversed.

Example:

*00000005* 5 entries are in the table  
*00000000* Min time of simulation is 0  
*00000004* Max time of simulation is 4.  

*00000000* time[0]=0  
*00000001* time[1]=1  
*00000001* time[2]=2  
*00000001* time[3]=3  
*00000001* time[4]=4  

*00000004* pos[0]=0x4  
*00000010* pos[1]=0x14  
*00000020* pos[2]=0x34  
*00000002* pos[3]=0x36  
*00000300* pos[4]=0x336  

### 05: LT_SECTION_TIMESCALE

This section consists of a single signed byte. Its value (*x*) is the
exponent of a base-10 timescale. Thus, each increment of '1' in the
time value table represented in the previous section represents
10^x seconds. Use -9 for nanoseconds, -12 for picoseconds, etc. Any
eight-bit signed value (-128 to +127) is permissible, but in actual
practice only a handful are useful.

### 03: LT_SECTION_FACNAME

No, section *04: LT_SECTION_FACNAME_GEOMETRY* hasn't been forgotten.
It's more logical to cover the facilities themselves before their
geometries.

4 bytes: *number of facilities (n)*  
4 bytes: *amount of memory required in bytes for the decompressed facilities*  

*n* compressed facilities follow, where a compressed facility consists
of two values:

2 bytes: *number of prefix bytes* (min=0, max=65535)  
zero terminated string: *suffix bytes*  

An example should clarify things (prefix lengths are in bold):

<div class="highlight"><pre><code>
00000020: <u>00 00 00 04</u> <u>00 00 00 1d</u> <b>00 00</b> 61 6c 70 68 61 00 ..........alpha.
00000030: <b>00 01</b> 70 70 6c 65 00 <b>00 04</b> 69 63 61 74 69 6f 6e ..pple...ication
00000040: 00 <b>00 00</b> 7a 65 72 6f 00 00 00 00 01 00 00 00 00 ...zero.........
</code></pre></div>

Four facilities (underlined) are defined and they occupy 0x0000001d
bytes (second underlined value).

This first prefix length is 0000 (offset 28).  
The first suffix is "alpha", therefore the first facility is "alpha". This requires six bytes.

The second prefix length is 0001 (offset 30).  
The second suffix is "pple", therefore the second facility is "apple". This requires six bytes.

The third prefix length is 0004 (offset 37).  
The third suffix is "ication", therefore the third facility is "application". This requires twelve bytes.

The fourth prefix length is 0000 (offset 41).  
The fourth suffix is "zero", therefore the fourth facility is "zero". This requires five bytes.

6 + 6 + 12 + 5 = 29 which indeed is 0x1d.

It is suggested that the facilities are dumped out in alphabetically
sorted order in order to increase the compression ratio of this section.

### 04: LT_SECTION_FACNAME_GEOMETRY

This section consists of a repeating series of sixteen byte entries.
Each entry corresponds in order with a facility as defined in *03:
LT_SECTION_FACNAME*. As such there is a 1:1 in-order correspondence
between the two sections.

4 bytes: *rows* (typically zero, only used when defining arrays: this indicates the max row value+1)  
4 bytes: *msb*  
4 bytes: *lsb*  
4 bytes: *flags*  

*flags* are defined as follows:

```c
#define LT_SYM_F_BITS    (0)
#define LT_SYM_F_INTEGER (1<<0)
#define LT_SYM_F_DOUBLE  (1<<1)
#define LT_SYM_F_STRING  (1<<2)
#define LT_SYM_F_ALIAS   (1<<3)
```

When an *LT_SYM_F_ALIAS* is encountered, it indicates that the rows
field instead means "alias this to facility number *rows*", there the
facility number corresponds to the definition order in *03: LT_SECTION_FACNAME*
and starts from zero.

### 02: LT_SECTION_SYNC_TABLE

This section indicates where the final value change (as a four-byte
offset from the beginning of the file) for every facility is to be
found. Facilities which do not change value contain a value of zero for
their final value change. This section is necessary as value changes are
stored as a linked list of backward directed pointers. There is a 1:1
in-order correspondence between this section and the definitions found
in *LT_SECTION_FACNAME*.

4 bytes: *final offset for facility* (repeated for each facility in order as they were defined)

### 01: LT_SECTION_CHG

This section is usually the largest section in a file as is composed of
value changes, however its structure is set up such that individual
facilities can be quickly accessed without the necessity of reading in
the entire file. In spite of this format, this does not prevent one from
stepping through the entire section *backwards* in order to process it
in one pass. The method to achieve this will be described later.

The final offset for a value change for a specific facility is found in
*02: LT_SECTION_SYNC_TABLE*. Since value changes for a facility are
linked together, one may follow pointers backward from the sync value
offset for a facility in order to read in an entire trace. This is used
to accelerate sparse reads of an LXT file's change data such as those
that a visualization tool such as a wave viewer would perform.

The format for a value change is as follows:

`command_byte, delta_offset_of_previous_change[, [row_changed,] change_data ]`

#### Command Bytes

The *command_byte* is broken into two (as currently defined) bitfields:
bits [5:4] contain a byte count (minus one) required for the
*delta_offset_of_previous_change* (thus a value from one to four bytes),
and bits [3:0] contain the command used to determine the format of the
change data (if any change data is necessary as dictated by the command
byte).

Bits [3:0] of the *command_byte* are defined as follows. Note that
this portion of the *command_byte* is ignored for strings and doubles
and typically x0 is placed in the dumpfile in those cases:

| Value | Description                                            |
|-------|--------------------------------------------------------|
| *0*   | MVL_2 [or default (for datatype) value change follows] |
| *1*   | MVL_4                                                  |
| *2*   | MVL_9                                                  |
| *3*   | flash whole value to 0s                                |
| *4*   | flash whole value to 1s                                |
| *5*   | flash whole value to Zs                                |
| *6*   | flash whole value to Xs                                |
| *7*   | flash whole value to Hs                                |
| *8*   | flash whole value to Us                                |
| *9*   | flash whole value to Ws                                |
| *A*   | flash whole value to Ls                                |
| *B*   | flash whole value to -s                                |
| *C*   | clock compress use 1 byte repeat count                 |
| *D*   | clock compress use 2 byte repeat count                 |
| *E*   | clock compress use 3 byte repeat count                 |
| *F*   | clock compress use 4 byte repeat count                 |

Commands x3-xB only make sense for MVL_2/4/9 (and integer in the case
for x3 and x4 when an integer is 0 or ~0) facilities. They are provided
as a space saving device which obviates the need for dumping value
change data when all the bits in a facility are set to the same exact
value. For single bit facilities, these commands suffice in all cases.

Command x0 is used when *change_data* can be stored as MVL_2. Bits
defined in MVL_2 are '0' and '1' and as such, one bit of storage in
an LXT file corresponds to one bit in the facility value.

Command x1 is used when *change_data* can't be stored as MVL_2 but can
stored as MVL_4. Bits defined in MVL_4 are '0', '1', 'Z', and
'X' are stored in an LXT file as the two-bit values 00, 01, 10, and
11.

Command x2 is used when change_data can't be stored as either MVL_2 or
MVL_4 but can be stored as MVL_9. Bits defined in MVL_9 are '0',
'1', 'Z', 'X', 'H', 'U', 'W', 'L', and '-' corresponding
to the four-bit values 0000 through 1000.

Commands xC-xF are used to repeat a clock. It is assumed that at least
two clock phase changes are present before the current command. Their
time values are subtracted in order to obtain a delta. The delta is used
as the change time between future phase changes with respect to the time
value of the previous command which is used as a "base time" and
"base value" for repeat_count+1 phase changes.

Note that these repeat command nybbles *also* are applicable to
multi-bit facilities which are 32-bits or less and MVL_2 in nature. In
this case, the preceding two deltas are subtracted such that a
recurrence equation can reconstruct any specific item of the compressed
data:

```c
unsigned int j = item_in_series_to_create + 1;
unsigned int res = base + (j / 2) * rle_delta[1] +
                   ((j / 2) + (j & 1)) * rle_delta[0];
```

For a sequence of: *7B 7C 7D 7E 7F 80 81 82* ...

```text
base = 82
rle_delta[1] = 82 - 81 == 01
rle_delta[0] = 81 - 80 == 01
```

Two deltas are used in order to handle the case where a vector which
changes value by a constant XOR. In that case, the *rle_delta* values
will be different. In this way, one command encoding can handle both XOR
and incrementer/decrementer type compression ops.

#### Delta Offsets

Delta offsets indicate where the preceding change may be found with
respect to the beginning of the LXT file. In order to calculate where
the preceding change for a facility is, take the offset of the
*command_byte*, subtract the *delta_offset_of_previous_change* from it,
then subtract 2 bytes more. As an example:

*00001000: 13 02 10* ...

The command byte is 13. Since bits [5:4] are "01", this means that
the *delta_offset_of_previous_change* is two bytes since 1 + 1 = 2.

The next two bytes are 0210, so 1000 - 0210 - 2 = 0DEE. Hence, the
preceding value change can be found at 0DEE. This process is to be
continued until a value change offset of 0 is obtained. This is
impossible because of the existence of the LXT header bytes.

#### Row Changed

This field is *only* present in value changes for arrays. The value is
2, 3, or 4 bytes depending on the magnitude of the array size: greater
than 16777215 rows requires 4 bytes, greater than 65535 requires 3
bytes, and so on down to one byte. Note that any value type can be part
of an array.

#### Change Data

This is only present for *command_byte*s x0-x2 for MVL_2, MVL_4, and
MVL_9 data, and any *command_byte* for strings and doubles. Strings are
stored in zero terminated format and doubles are stored as eight bytes
in machine-native format with *08: LT_SECTION_DOUBLE_TEST* being used to
resolve possible differences in endianness on the machine reading the
LXT file.

Values are stored left justified in big endian order and unused bits are
zeroed out. Examples with "_" used to represent the boundary between
consecutive bytes:

MVL_2: "0101010110101010" (16 bits) is stored as 01010101_10101010  
MVL_2: "011" (3 bits) is stored as 01100000  
MVL_2: "11111110011" (11 bits) is stored as 11111110_01100000  

MVL_4: "01ZX01ZX" (8 bits) is stored as 00011011_00011011  
MVL_4: "ZX1" (3 bits) is stored as 10110100  
MVL_4: "XXXXZ" (5 bits) is stored as 11111111_10000000  

MVL_9: "01XZHUWL-" (9 bits) is stored as 00000001_00100011_01000101_01100111_10000000  

#### Correlating Time Values to Offsets

This is what the purpose of *06: LT_SECTION_TIME_TABLE* is. Given the
offset of a *command_byte*, *bsearch(3)* an array of ascending position
values (not deltas) and pick out the maximum position value which is
less than or equal to the offset of the *command_byte*. The following
code sequence illustrates this given two arrays
*positional_information[]* and *time_information[]*. Note that
*total_cycles* corresponds to *number_of_entries* as defined in *06:
LT_SECTION_TIME_TABLE*.

```c
static int max_compare_time_tc;
static int max_compare_pos_tc;

static int compar_mvl_timechain(const void *s1, const void *s2) {
    int rv;

    int key = *((int *)s1);
    int obj = *((int *)s2);

    if (obj <= key && obj > max_compare_time_tc) {
        max_compare_time_tc = obj;
        max_compare_pos_tc = (int *)s2 - positional_information;
    }

    int rv;
    int delta = key - obj;
    if (delta < 0) {
        rv = -1;
    } else if (delta > 0) {
        rv = 1;
    } else {
        rv = 0;
    }

    return rv;
}

static int bsearch_position_versus_time(int key) {
    max_compare_time_tc = -1;
    max_compare_pos_tc = -1;

    bsearch(&key, positional_information, total_cycles,
            sizeof(int), compar_mvl_timechain);
    
    if (max_compare_pos_tc <= 0 || max_compare_time_tc < 0) {
        max_compare_pos_tc=0; /* aix bsearch fix */
    }

    return time_information[max_compare_pos_tc];
}
```

#### Reading All Value Changes in One Pass

This requires a little bit more work but it can be done. Basically what
you have to do is the following:

1. Read in all the sync offsets from *02: LT_SECTION_SYNC_TABLE* and
   put each in a structure which contains the sync offset and the
   facility index. All of these structures will compose an array that
   is as large as the number of facilities which exist.
1. Heapify the array such that the topmost element of the heap has the
   largest positional offset.
1. Change the topmost element\'s offset to its preceding offset (as
   determined by examining the *command_byte*, bits \[5:4\] and
   calculating the preceding offset by subtracting the
   *delta_offset_of_previous_change* then subtracting 2 bytes.
1. Continue with step 2 until the topmost element\'s offset is zero
   after performing a heapify().

### 00: LT_SECTION_END

As a section pointer doesn't exist for this, there's no section body either.

## The lxt_write API

In order to facilitate the writing of LXT files, an API has been
provided which does most of the hard work.

`struct lt_trace *lt_init(const char *name)`{l=c}
:   This opens an LXT file. The pointer returned by this function is NULL
    if unsuccessful. If successful, the pointer is to be used as a
    "context" for all the remaining functions in the API. In this way,
    multiple LXT files may be generated at once.

`void lt_close(struct lt_trace *lt)`{l=c}
:   This fixates and closes an LXT file. This is extremely important
    because if the file is not fixated, it will be impossible to use the
    value change data in it! For this reason, it is recommended that the
    function be placed in an *atexit(3)* handler in environments where trace
    generation can be interrupted through program crashes or external
    signals such as control-C.

`struct lt_symbol *lt_symbol_add(struct lt_trace *lt, const char *name, unsigned int rows, int msb, int lsb, int flags)`{l=c}
:   This creates a facility. Since the facility and related tables are
    written out during fixation, one may arbitrarily add facilities up until
    the very moment that lt_close() is called. For facilities which are not
    arrays, a value of 0 or 1 for rows. As such, only values 2 and greater
    are used to signify arrays. Flags are defined above as in 04:
    LT_SECTION_FACNAME_GEOMETRY.

`struct lt_symbol *lt_symbol_find(struct lt_trace *lt, const char *name)`{l=c}
:   This finds if a symbol has been previously defined. If returns non-NULL
    on success. It actually returns a symbol pointer, but you shouldn't be
    dereferencing the fields inside it unless you know what you are doing.

`struct lt_symbol *lt_symbol_alias(struct lt_trace *lt, const char *existing_name, const char *alias, int msb, int lsb)`{l=c}
:   This assigns an alias to an existing facility. This is to create
    signals which traverse multiple levels of hierarchy, but are the same
    net, possibly with different MSB and LSB values (though the distance
    between them will be the same).

`void lt_symbol_bracket_stripping(struct lt_trace *lt, int doit)`{l=c}
:   This is to be used when facilities are defined in Verilog format such
    that exploded bitvectors are dumped as x[0], x[1], x[2], etc. If
    doit is set to a nonzero value, the bracketed values will be stripped
    off. In order to keep visualization and other tools from becoming
    confused, the MSB/LSB values must be unique for every bit. The tool
    *vcd2lxt* shows how this works and should be used. If vectors are dumped
    atomically, this function need not be called.

`void lt_set_timescale(struct lt_trace *lt, int timescale)`{l=c}
:   This sets the simulation timescale to 10^timescale^ seconds where
    timescale is an 8-bit signed value. As such, negative values are the
    only useful ones.

`void lt_set_initial_value(struct lt_trace *lt, char value)`{l=c}
:   This sets the initial value of every MVL (bitwise) facility to whatever
    the value is. Permissible values are '0', '1', 'Z', 'X', 'H',
    'U', 'W', 'L', and '-'.

`int lt_set_time(struct lt_trace *lt, unsigned int timeval)`{l=c}, `int lt_inc_time_by_delta(struct lt_trace *lt, unsigned int timeval)`{l=c}, `int lt_set_time64(struct lt_trace *lt, lxttime_t timeval)`{l=c}, `int lt_inc_time_by_delta64(struct lt_trace *lt, lxttime_t timeval)`{l=c}
:   This is how time is dynamically updated in the LXT file. Note that for
    the non-delta functions, timeval changes are expected to be
    monotonically increasing. In addition, time values dumped to the LXT
    file are coalesced if there are no value changes for a specific time
    value. (Note: lxttime_t is defined as an unsigned long long.)

`void lt_set_clock_compress(struct lt_trace *lt)`{l=c}
:   Enables clock compression heuristics for the current trace.
    This cannot be turned off once it is on.

`int lt_emit_value_int(struct lt_trace *lt, struct lt_symbol *s, unsigned int row, int value)`{l=c}
:   This dumps an MVL_2 value for a specific facility which is 32-bits or less.
    Note that this does not work for strings or doubles.

`int lt_emit_value_double(struct lt_trace *lt, struct lt_symbol *s, unsigned int row, double value`{l=c}
:   This dumps a double value for a specific facility.
    Note that this only works for doubles.

`int lt_emit_value_string(struct lt_trace *lt, struct lt_symbol *s, unsigned int row, char *value)`{l=c}
:   This dumps a string value for a specific facility.
    Note that this only works for strings.

`int lt_emit_value_bit_string(struct lt_trace *lt, struct lt_symbol *s, unsigned int row, char *value)`{l=c}
:   This dumps an MVL_2, MVL_4, or MVL_9 value out to the LXT file for a
    specific facility. Note that the value is parsed in order to determine
    how to optimally represent it in the file. In addition, note that if the
    value's string length is shorter than the facility length, it will be
    left justified with the rightmost character will be propagated to the
    right in order to pad the value string out to the correct length.
    Therefore, "10x" for 8-bits becomes "10xxxxxx" and "z" for 8-bits
    becomes "zzzzzzzz".
