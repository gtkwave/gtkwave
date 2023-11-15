# GTKWave

GTKWave is a fully featured GTK+ based wave viewer for Unix and Win32 which reads FST, and GHW files as well as standard Verilog VCD/EVCD files and allows their viewing.

## Building GTKWave from source

### Installing dependencies

Debian, Ubuntu:

```sh
apt install build-essential meson gperf flex desktop-file-utils libgtk-3-dev \
            libbz2-dev libjudy-dev
```

Fedora:

```sh
dnf install meson gperf flex glib2-devel gcc gcc-c++ gtk3-devel \
            gobject-introspection-devel desktop-file-utils tcl
```

### Building GTKWave


```sh
meson setup build
meson compile -C build
```
