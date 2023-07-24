## -*- makefile -*-
##

xdgdatadir=@XDGDATADIR@
desktopdir=$(xdgdatadir)/applications
desktop_DATA = gtkwave.desktop

if FDO_MIME
install-data-hook:
	$(UPDATE_DESKTOP_DATABASE)

uninstall-hook:
	$(UPDATE_DESKTOP_DATABASE)
endif

EXTRA_DIST= gtkwave.desktop

