# Microsoft Windows

## Cygwin

The best way to run GTKWave under Windows is to compile it to run under
Cygwin. This will provide the same functionality as compared to the
Unix/Linux version and better graphical performance than the native
binary version. Note that launching RTLBrowse requires Cygserver to
be enabled. Please see the Cygwin documentation for information on
how to enable Cygserver for your version of Cygwin.
(<http://www.cygwin.com/cygwin-ug-net/using-cygserver.html>)

### Installing dependencies

First, download Cygwin installer from its
[official website](https://www.cygwin.com/install.html).

After installing Cygwin, use the following commands to
install the build dependencies for GTKWave:

```bat
.\setup-x86_64.exe -q -P gcc-g++,gperf,libbz2-devel
.\setup-x86_64.exe -q -P liblzma-devel,zlib-devel,libgtk3-devel
.\setup-x86_64.exe -q -P make,git,xinit,tcl-tk-devel
```

### Compiling and Installing

For compilation and installation instructions, please refer
to the section on [Compiling and Installing for Unix and
Linux](./unix_linux.md#compiling-and-installing).

### Launching

To launch the GTKWave within the Cygwin environment, begin
by initiating an X session using:

```bash
startxwin
```

Then, right-click the X11 icon in system tray to open another Cygwin
terminal and type `gtkwave` to launch the application.

## MSYS2

For users perferring native Windows binaries. MSYS2 is an
excellent alternative.

### Installing dependencies

Follow the installation guide for MSYS2 on its [official website](https://www.msys2.org/).

After installing MSYS2. Open the `MSYS2 MINGW64` shell from the start menu.
And run the following commands to install the necessary build dependencies
for GTKWave:

```bash
pacman -Syuu
pacman -Syuu  # Please run this twice as recommended.
# Or mingw-w64-ucrt-x86_64 for UCRT64
pacman -S mingw-w64-x86_64-gcc base-devel mingw-w64-x86_64-tk
pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gperf git
```

### Compiling and Installing

For compilation and installation instructions, please refer
to the section on [Compiling and Installing for Unix and
Linux](./unix_linux.md#compiling-and-installing).
