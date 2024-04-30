# Microsoft Windows

## Cygwin

The best way to run GTKWave under Windows is to compile it to run under
Cygwin. This will provide the same functionality as compared to the
Unix/Linux version and better graphical performance than the native
binary version. Follow the directions for Unix compiles in the preceding
section. Note that launching RTLBrowse requires Cygserver to be enabled.
Please see the Cygwin documentation for information on how to enable
Cygserver for your version of Cygwin.
(<http://www.cygwin.com/cygwin-ug-net/using-cygserver.html>)

## MinGW versus VC++ for Native Binaries

It is recommended that Windows compiles and installs are done in the
MinGW environment in order to mimic the Unix shell environment as well
as produce binaries that are natively usable on Windows. Producing
native binaries with VisualC++ has not been attempted for some time so
it is currently untested.

## MinGW with GTK-1.2

If you are missing a working version of *gtk-config*, you will need a
fake *gtk-config* file in order to compile under GTK-1.2. It will look
like this with the include and linker search directories modified
accordingly:

```bash
#!/bin/sh

if [ "$1" == "--libs" ]
then
    echo -L/home/bybell/libs -lgck -lgdk-1.3 -lgimp-1.2 -lgimpi -lgimpui-1.2 \
    -lglib-1.3 -lgmodule-1.3 -lgnu-intl -lgobject-1.3 -lgthr ead-1.3 -lgtk-1.3 \
    -liconv-1.3 -ljpeg -llibgplugin_a -llibgplugin_b -lpng -lpthread32 \
    -ltiff-lzw -ltiff-nolzw -ltiff
fi

if [ "$1" == "--cflags" ]
then
    echo " -mms-bitfields -I/home/bybell/src/glib
    -I/home/bybell/src/gtk+/gtk -I/home/bybell/src/gtk+/gdk
    -I/home/bybell/src/gtk+ "
fi
```

Compiling as under Unix/Linux is the same.

## MinGW with GTK-2.0

You do not need to do anything special except ensure that *pkg-config*
is pointed to by your PATH environment variable. Proceed as with
GTK-1.2. Pre-made binaries can be found at the
<http://www.dspia.com/gtkwave.html> website.
