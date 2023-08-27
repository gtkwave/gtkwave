icons_dir = get_option('datadir') / 'icons/hicolor'

install_data(
    '@0@.svg'.format(app_id),
    install_dir: icons_dir / 'scalable/apps',
)

install_data(
    'io.github.gtkwave.GTKWave-symbolic.svg',
    rename: '@0@-symbolic.svg'.format(app_id),
    install_dir: icons_dir / 'symbolic/apps',
)
