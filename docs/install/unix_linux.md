# Unix and Linux

Compiling GTKWave on Unix or Linux operating systems should be a
relatively straightforward process as GTKWave was developed under both
Linux and AIX. External software packages required are GTK
([http://www.gtk.org](http://www.gtk.org/)) with versions 1.3 or 2.x
(3.x not yet supported), and *gperf* (for RTLBrowse) which can be
downloaded from the GNU website
([http://www.gnu.org](http://www.gnu.org/)). The compression libraries
libz (*zlib*) and libbz2 (*bzip2*) are not required to be installed on a
target system as their source code is already included in the GTKWave
tarball, however the system ones will be used if located.

## Compiling and Installing

Un-tar the source code into any temporary directory then change
directory into it. After doing this, invoke the configure script. Note
that if you wish to change the install point, use the double dash
\--*prefix* option to point to the absolute pathname. For example, to
install in /usr, type ***./configure \--prefix=/usr***.

```bash
1 :/tmp/gtkwave-3.1.3\> ./configure
```

Use the \--*help* flag to see which options are available. Typically,
outside of \--*prefix*, no flags are needed.

```bash
2 :/tmp/gtkwave-3.1.3\> make
```

Wait for the compile to finish. This will take some amount of time. Then
log on as the superuser.

```bash
3 :/tmp/gtkwave-3.1.3\> **su**
Password:
[root@localhost gtkwave-3.1.3]# make install
```

Wait for the install to finish. It should proceed relatively quickly.
When finished, exit as superuser.

```bash
[root@localhost gtkwave-3.1.3]# exit
exit
```

GTKWave is now installed on your Unix or Linux system. To use it, make
sure that the *bin/* directory off the install point is in your path.
For example, if the install point is */usr/local*, ensure that
*/usr/local/bin* is in your path. How to do this will vary from shell to
shell.

:::{figure-md}

![GTKWave running under Linux.](../_static/images/gtkwave-linux.png)

GTKWave running under Linux.
:::
