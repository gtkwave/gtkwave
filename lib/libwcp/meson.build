libwcp_sources = [
    'wcp_protocol.c',
    'wcp_server.c',
]

libwcp_headers = [
    'wcp_protocol.h',
    'wcp_server.h',
]

json_glib_dep = dependency('json-glib-1.0')
gio_dep = dependency('gio-2.0')

libwcp = static_library(
    'wcp',
    sources: libwcp_sources,
    dependencies: [
        glib_dep,
        gio_dep,
        json_glib_dep,
    ],
    install: false,
)

libwcp_dep = declare_dependency(
    link_with: libwcp,
    include_directories: '.',
    dependencies: [
        glib_dep,
        gio_dep,
        json_glib_dep,
    ],
)
