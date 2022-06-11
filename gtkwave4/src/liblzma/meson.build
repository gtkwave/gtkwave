libgwlzma_sources = [
    'LzmaLib.c',
]

libgwlzma_lib = static_library(
    'libgwlzma',
    libgwlzma_sources,
)

libgwlzma_dep = declare_dependency(
    link_with: libgwlzma_lib,
    include_directories: '.',
)