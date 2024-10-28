# GTKWave

GTKWave is a fully featured GTK+ based wave viewer for Unix and Win32 which reads FST, and GHW files as well as standard Verilog VCD/EVCD files and allows their viewing.

## Building GTKWave from source

### Installing dependencies

Debian, Ubuntu:

```sh
apt install build-essential meson gperf flex desktop-file-utils libgtk-3-dev \
            libbz2-dev libjudy-dev libgirepository1.0-dev
```

Fedora:

```sh
dnf install meson gperf flex glib2-devel gcc gcc-c++ gtk3-devel \
            gobject-introspection-devel desktop-file-utils tcl
```

macOS:

```sh
brew install desktop-file-utils shared-mime-info       \
             gobject-introspection gtk-mac-integration \
             meson ninja pkg-config gtk+3
```

### Building GTKWave


```sh
git clone "https://github.com/gtkwave/gtkwave.git"
cd gtkwave
meson setup build && cd build && meson install
```

### Running GTKWave
```sh
gtkwave [path to a .vcd, .fst, .ghw dump file or a .gtkw savefile]
```
For more information about available command line parameters refer to the built in-help (`gtkwave --help`) or the [`gtkwave` man page](https://gtkwave.github.io/gtkwave/man/gtkwave.1.html).
