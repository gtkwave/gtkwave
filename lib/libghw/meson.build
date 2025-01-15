libghw_sources = [
    'libghw.c',
]

libghw_dependencies = [
]

libghw = static_library(
    'ghw',
    sources: libghw_sources,
    dependencies: libghw_dependencies,
    include_directories: config_inc,
)

libghw_dep = declare_dependency(
    link_with: libghw,
    dependencies: libghw_dependencies,
    include_directories: '.',
)