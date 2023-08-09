libgwlzma_sources = [
    'LzmaLib.c',
]

libgwlzma = static_library(
    'gwlzma',
    sources: libgwlzma_sources,
    include_directories: config_inc,
)

libgwlzma_dep = declare_dependency(
    link_with: libgwlzma,
    include_directories: '.',
)
