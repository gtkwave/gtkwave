libgtkwave_public_sources = [
    'gw-blackout-regions.c',
    'gw-color-theme.c',
    'gw-color.c',
    'gw-dump-file-builder.c',
    'gw-dump-file.c',
    'gw-facs.c',
    'gw-ghw-file.c',
    'gw-ghw-loader.c',
    'gw-hist-ent-factory.c',
    'gw-loader.c',
    'gw-marker.c',
    'gw-named-markers.c',
    'gw-project.c',
    'gw-stems.c',
    'gw-time-range.c',
    'gw-time.c',
    'gw-tree.c',
    'gw-var-enums.c',
]

libgtkwave_public_headers = [
    'gtkwave.h',
    'gw-types.h',
    'gw-bit-vector.h',
    'gw-bits.h',
    'gw-blackout-regions.h',
    'gw-color-theme.h',
    'gw-color.h',
    'gw-dump-file-builder.h',
    'gw-dump-file.h',
    'gw-fac.h',
    'gw-facs.h',
    'gw-ghw-file.h',
    'gw-ghw-loader.h',
    'gw-hist-ent-factory.h',
    'gw-hist-ent.h',
    'gw-loader.h',
    'gw-marker.h',
    'gw-named-markers.h',
    'gw-project.h',
    'gw-stems.h',
    'gw-symbol.h',
    'gw-time-range.h',
    'gw-time.h',
    'gw-tree.h',
    'gw-var-enums.h',
    'gw-vector-ent.h',
]

libgtkwave_private_sources = [
    'gw-util.c',
]

libgtkwave_dependencies = [
    glib_dep,
    gobject_dep,
    libghw_dep,
]

if get_option('experimental_plugin_support')
    libgtkwave_public_sources += [
        'gw-plugin-manager.c',
        'gw-plugin.c',
    ]

    libgtkwave_public_headers += [
        'gw-plugin-manager.h',
        'gw-plugin.h',
    ]

    libgtkwave_dependencies += libpeas_dep
endif

libgtkwave_enums = gnome.mkenums_simple('gw-enums',
    sources: libgtkwave_public_headers
)

libgtkwave = shared_library(
    'gtkwave',
    sources: libgtkwave_public_sources + libgtkwave_private_sources + libgtkwave_enums,
    dependencies: libgtkwave_dependencies,
    include_directories: config_inc,
    install: true,
)

install_headers(libgtkwave_public_headers, subdir: 'libgtkwave')


pkgconfig.generate(
    libgtkwave,
    name: 'gtkwave',
    description: 'GTKWave library',
    version: meson.project_version(),
    subdirs: 'libgtkwave',
    requires: [
        'gobject-2.0',
        'libpeas-2',
    ],
)

libgtkwave_dep_sources = []

if get_option('introspection')
    libgtkwave_gir_includes = ['GObject-2.0']
    if get_option('experimental_plugin_support')
        libgtkwave_gir_includes += 'Peas-2'
    endif

    libgtkwave_dep_sources += gnome.generate_gir(
        libgtkwave,
        sources: libgtkwave_public_sources + libgtkwave_public_headers,
        namespace: 'Gw',
        nsversion: '1',
        identifier_prefix: 'Gw',
        symbol_prefix: 'gw',
        export_packages: 'gtkwave-1',
        dependencies: libgtkwave_dependencies,
        includes: libgtkwave_gir_includes,
        header: 'gtkwave.h',
        install: true,
    )
endif

libgtkwave_dep = declare_dependency(
    link_with: libgtkwave,
    include_directories: '.',
    dependencies: libgtkwave_dependencies,
    sources: libgtkwave_dep_sources,
)
