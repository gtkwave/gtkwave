# GTKWave

GTKWave is a fully featured GTK+ based wave viewer for Unix and Win32 which reads LXT, LXT2, VZT, FST, and GHW files as well as standard Verilog VCD/EVCD files and allows their viewing.

## Installation

2) If you compile a GIT version: Type `./autogen.sh`
2) Type `./configure --enable-gtk3` (or `./configure` for GTK+ 2)
3) `make`
4) `make install` (as root)

Make sure you copy the `.gtkwaverc` file to your home directory or to your
VCD project directory.  It contains the prefs for a good configuration
that most people find ergonomic.  It is not strictly necessary however.

[Note: for mingw with gtk+-2, you don't need to do anything except have
pkg-config in your PATH however the following note is from Thomas Uhle.]

Important to know is to compile with `CFLAGS=-mms-bitfields` in Windows in order to link correctly
to the GTK+ dlls. This is how I did configure GTKWave with additional optimisation switches:

```sh
./configure CFLAGS='-Wall -O3 -mcpu=i686 -mms-bitfields -ffast-math -fstrict-aliasing'
```

After that you may just call make the usual way.

## Misc. Notes

Note that Ver Structural Verilog Compiler AET files are no longer supported. 
They have been superceded by LXT.  Also note that the AMULET group will be
taking over maintenance of the viewer effective immediately.


Add these flags to your compile for new warnings on AMD64:
`-g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 -mtune=generic`

on i386:
`-g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m32 -march=i386 -mtune=generic -fasynchronous-unwind-tables`

## Note (1) For Ubuntu users:

I had to do the following to get it to install directly. Please
include in INSTALL .txt as an option for ubuntu users. Other linux
distributions might have other things to do.

```sh
sudo apt-get install libgtk2.0-dev
./configure --with-tcl=/usr/lib/tcl8.4 --with-tk=/usr/lib/tk8.4
```


## Note (2) For Ubuntu users:

If your compile fails because gzopen64 cannot be found, you will either have
to fix your Ubuntu install or use the version of libz in gtkwave:

```sh
./configure --enable-local-libz
```

## Note (3) For Ubuntu users (version 11.10):

```sh
sudo apt-get install libjudy-dev
sudo apt-get install libbz2-dev
sudo apt-get install liblzma-dev
sudo apt-get install libgconf2-dev
sudo apt-get install libgtk2.0-dev
sudo apt-get install tcl-dev
sudo apt-get install tk-dev
sudo apt-get install gperf
sudo apt-get install gtk2-engines-pixbuf
```

Configure then as:

```sh
./configure --enable-judy --enable-struct-pack --with-gconf
```


## Notes for Mac OSX users:

Install MacPorts then

```sh
sudo port -v selfupdate
sudo port install Judy tcl tk xz-devel gtk2
```

If Quartz is used:

```sh
sudo port install gtk-osx-application

./configure --prefix=/opt/local --enable-judy --enable-struct-pack "CFLAGS=-I/opt/local/include -O2 -g" LDFLAGS=-L/opt/local/lib --no-create --no-recursion
```

Tcl works in the OSX version of gtkwave starting with version 3.3.26.  

At this point all features working on Linux should be functional on the Mac,
except that twinwave does not render to a single window when Quartz is used
instead of X11.

If you wish to use llvm, also add `CC=llvm-gcc` and change the `-O2` in CFLAGS
to `-O4`.

At the current time Quartz support is experimental.  Please report any bugs
encountered as compared to X11 function.

Note that the preferred environment for Quartz builds is jhbuild.  To build
gtkwave as an app bundle (while in jhbuild shell):

```sh
./configure --enable-judy --enable-struct-pack --prefix=/Users/$USER/gtk/inst
make
make install
cd contrib/bundle_for_osx
./make_bundle.sh
```

This assumes that Judy arrays and XZ were both already compiled and installed.
If Judy arrays are not installed, do not add `--enable-judy`.
If XZ is not installed, add `--disable-xz`.

The current environment used is modulesets.  Bug 664894 has an interim fix in
the binary distribution by applying patches using the
`contrib/bundle_for_osx/gtk_diff_against_modulesets.patch` file.

## MSYS2 notes for creating a working environment for compiling gtkwave:

```sh
pacman -Syuu 
[repeat "pacman -Syuu" multiple times until environment stabilizes]
pacman -S --needed base-devel mingw-w64-i686-toolchain mingw-w64-x86_64-toolchain git subversion mercurial mingw-w64-i686-cmake mingw-w64-x86_64-cmake 
pacman -S base-devel mingw-w64-toolchain mingw-w64-i686-gtk2 
pacman -S mingw-w64-i686-gtk2 
pacman -S mingw-w64-x86_64-gtk2 
pacman -S mingw-w64-i686-gtk3 
pacman -S mingw-w64-x86_64-gtk3 
```


Add `--enable-gtk3` to the `./configure` invocation to build against GTK3.
This causes the GTK+ frontend to be built with gtk3 instead of gtk2.

