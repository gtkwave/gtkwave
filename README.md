# gtkwave
GTKWave is a fully featured GTK+ based wave viewer for Unix and Win32 which reads LXT, LXT2, VZT, FST, and GHW files as well as standard Verilog VCD/EVCD files and allows their viewing.


# Build GTKwave-GTK3 on Mac OS

- install Homebrew
- brew install gtk+3
- brew install adwaita-icon-theme
- brew install gtk-mac-integration
- brew install tcl
- brew install xz
- export PKG_CONFIG_PATH=$(brew --prefix)/lib/pkgconfig 
- autoconf (only when configure.ac was changed, should not be the case)
- CPPFLAGS="-I$(brew --prefix)/include" LDFLAGS="-L$(brew --prefix)/lib" ./configure --enable-gtk3 --with-tcl=$(brew --prefix)/opt/tcl-tk/lib --with-tk=$(brew --prefix)/opt/tcl-tk/lib --prefix=<wherever you like>
- make -j4
