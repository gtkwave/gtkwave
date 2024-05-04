# Compiling GTKWave 4

GTKWave 4 is the development version with a lots of 
improvement and changes. Please refer to the
[changelog](https://github.com/gtkwave/gtkwave/blob/master/CHANGELOG.md)
for all notable changes.

## Installing dependencies

Debian, Ubuntu:

```bash
sudo apt install build-essential meson gperf flex desktop-file-utils libgtk-3-dev \
            libbz2-dev libjudy-dev
```

Fedora:

```bash
sudo dnf install meson gperf flex glib2-devel gcc gcc-c++ gtk3-devel \
            gobject-introspection-devel desktop-file-utils tcl
```

## Compiling and Installing

Clone the sources from the git repository:

```bash
git clone https://github.com/gtkwave/gtkwave/ gtkwave
cd gtkwave
```

After doing this, you must decide how you're going
to install GTKWave onto your system. By default, the
software is installed in `/usr/local/`. If you wish to
install into a specific prefix, use the double dash
\--*prefix* option to point to the absolute pathname.

For example, to install in `/opt`

```bash
meson setup build --prefix=/opt
```

Or, to install GTKWave globally

```bash
meson setup build
```

After that

```bash
meson compile -C build
```

Then wait for the compile to finish. This will take some amount of time.

To install GTKWave, run

```bash
sudo meson install -C build
```

GTKWave is now installed on your Unix or Linux system. To use it, make
sure that the *bin/* directory of the install point is in your path.
For example, if the install point is */usr/local*, ensure that
*/usr/local/bin* is in your path. How to do this will vary from shell to
shell.