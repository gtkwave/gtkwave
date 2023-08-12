libgtkwave_sources = [
    'gw-time.c',
]

libgtkwave_dependencies = [
    glib_dep,
]

libgtkwave = static_library(
    'gtkwave',
    sources: libgtkwave_sources,
    dependencies: libgtkwave_dependencies,
    include_directories: config_inc,
)

libgtkwave_dep = declare_dependency(
    link_with: libgtkwave,
    include_directories: '.',
    dependencies: libgtkwave_dependencies,
)
