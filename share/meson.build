subdir('icons')
subdir('mime')

install_data(
    '@0@.desktop'.format(app_id),
    install_dir: get_option('datadir') / 'applications',
)

install_data(
    '@0@.metainfo.xml'.format(app_id),
    install_dir: get_option('datadir') / 'metainfo',
)
