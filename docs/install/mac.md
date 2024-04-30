# Apple Macintosh

## OSX / Macports

All functionality of the Linux/UNIX version is present in the OSX
version when GDK/GTK is compiled for X11. If GDK/GTK is compiled for
Quartz (i.e., */opt/local/etc/macports/variants.conf* has a line of the
form *+no_x11 +quartz*) and the package gtk-osx-application is also
installed, GTKWave will behave more like a Mac application with native
menus, an icon on the dock, etc. as shown below.

:::{figure-md}

![Demonstrating application integration with Mac OSX / Quartz](../_static/images/gtkwave-mac.png)

Demonstrating application integration with Mac OSX / Quartz
:::

Note that if running GTKWave on the command line out of a precompiled
bundle *gtkwave.app*, it is required that the Perl script
*gtkwave.app/Contents/Resources/bin/gtkwave* is invoked to start the
program. Please see the gtkwave(1) man page for more information.
