### Desktop files

desktop_files = [
    'applications/gtkwave.desktop',
]

install_data(
    desktop_files,
    install_dir: datadir / 'applications'
)

### Icons

icon_files = [
    'icons/gtkwave_256x256x32.png',
    'icons/gtkwave_files_256x256x32.png',
    'icons/gtkwave_savefiles_256x256x32.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-ae2.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-aet.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-evcd.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-fst.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-ghw.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-gtkw.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-lx2.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-lxt.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-lxt2.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-vcd.png',
    'icons/gnome/16x16/mimetypes/gnome-mime-application-vnd.gtkwave-vzt.png',
    'icons/gnome/16x16/mimetypes/gtkwave.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-ae2.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-aet.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-evcd.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-fst.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-ghw.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-gtkw.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-lx2.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-lxt.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-lxt2.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-vcd.png',
    'icons/gnome/32x32/mimetypes/gnome-mime-application-vnd.gtkwave-vzt.png',
    'icons/gnome/32x32/mimetypes/gtkwave.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-ae2.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-aet.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-evcd.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-fst.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-ghw.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-gtkw.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-lx2.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-lxt.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-lxt2.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-vcd.png',
    'icons/gnome/48x48/mimetypes/gnome-mime-application-vnd.gtkwave-vzt.png',
    'icons/gnome/48x48/mimetypes/gtkwave.png',
    'icons/hicolor/scalable/apps/gtkwave.svg',
]

foreach icon_file : icon_files
    install_data(
        icon_file,
        install_dir: datadir / fs.parent(icon_file),
    )
endforeach

### MIME types

mime_files = [
    'mime/packages/x-gtkwave-extension-ae2.xml',
    'mime/packages/x-gtkwave-extension-aet.xml',
    'mime/packages/x-gtkwave-extension-evcd.xml',
    'mime/packages/x-gtkwave-extension-fst.xml',
    'mime/packages/x-gtkwave-extension-ghw.xml',
    'mime/packages/x-gtkwave-extension-gtkw.xml',
    'mime/packages/x-gtkwave-extension-lx2.xml',
    'mime/packages/x-gtkwave-extension-lxt.xml',
    'mime/packages/x-gtkwave-extension-lxt2.xml',
    'mime/packages/x-gtkwave-extension-vcd.xml',
    'mime/packages/x-gtkwave-extension-vzt.xml',
]

install_data(
    mime_files,
    install_dir: datadir / 'mime' / 'packages'
)