## -*- makefile -*-
##

xdgdatadir=@XDGDATADIR@
mimedir=$(xdgdatadir)/mime
xmldir=$(mimedir)/packages
xml_DATA = x-gtkwave-extension-ae2.xml x-gtkwave-extension-aet.xml x-gtkwave-extension-evcd.xml x-gtkwave-extension-fst.xml x-gtkwave-extension-ghw.xml x-gtkwave-extension-gtkw.xml x-gtkwave-extension-lx2.xml x-gtkwave-extension-lxt2.xml x-gtkwave-extension-lxt.xml x-gtkwave-extension-vcd.xml x-gtkwave-extension-vzt.xml

if FDO_MIME 
install-data-hook:
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(mimedir)"

uninstall-hook:
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(mimedir)"
endif

EXTRA_DIST = x-gtkwave-extension-ae2.xml x-gtkwave-extension-aet.xml x-gtkwave-extension-evcd.xml x-gtkwave-extension-fst.xml x-gtkwave-extension-ghw.xml x-gtkwave-extension-gtkw.xml x-gtkwave-extension-lx2.xml x-gtkwave-extension-lxt2.xml x-gtkwave-extension-lxt.xml x-gtkwave-extension-vcd.xml x-gtkwave-extension-vzt.xml
