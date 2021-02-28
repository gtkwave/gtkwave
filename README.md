# gtkwave
GTKWave is a fully featured GTK+ based wave viewer for Unix and Win32 which reads LXT, LXT2, VZT, FST, and GHW files as well as standard Verilog VCD/EVCD files and allows their viewing.


# Build GTK3 on Mac OS

- install Homebrew
- brew install gtk+3
- brew install adwaita-icon-theme
- brew install tcl (there are some configure messages regarding tcl, there might be something more)
- export PKG_CONFIG_PATH=/opt/homebrew/lib/pkgconfig  (ATTENTION: This is for Big Sur, under older OS X revisions this might be under /usr/local)
- autoconf
- ./configure --enable-gtk3 --disable-xz --prefix=<wherever you like> (--disable-xz should not be necessary, I had issues for it to pick it up somehow)