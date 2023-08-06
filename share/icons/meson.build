icons_dir = get_option('datadir') / 'icons'

install_data(
    'hicolor/scalable/apps/gtkwave.svg',
    install_dir: icons_dir / 'hicolor/scalable/apps',
)

install_data(
    'gtkwave_256x256x32.png',
    'gtkwave_files_256x256x32.png',
    'gtkwave_savefiles_256x256x32.png',
    install_dir: icons_dir,
)

mime_types = [
    'gnome-mime-application-vnd.gtkwave-ae2.png',
    'gnome-mime-application-vnd.gtkwave-aet.png',
    'gnome-mime-application-vnd.gtkwave-evcd.png',
    'gnome-mime-application-vnd.gtkwave-fst.png',
    'gnome-mime-application-vnd.gtkwave-ghw.png',
    'gnome-mime-application-vnd.gtkwave-gtkw.png',
    'gnome-mime-application-vnd.gtkwave-lx2.png',
    'gnome-mime-application-vnd.gtkwave-lxt2.png',
    'gnome-mime-application-vnd.gtkwave-lxt.png',
    'gnome-mime-application-vnd.gtkwave-vcd.png',
    'gnome-mime-application-vnd.gtkwave-vzt.png',
    'gtkwave.png',
]

icon_sizes = ['16x16', '32x32', '48x48']

foreach icon_size : icon_sizes
    foreach mime_type : mime_types
        install_data(
            'gnome/@0@/mimetypes/@1@'.format(icon_size, mime_type),
            install_dir: icons_dir / 'gnome' / icon_size / 'mimetypes',
        )
    endforeach
endforeach
