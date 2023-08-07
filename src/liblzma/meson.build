liblzma_sources = [
    'LzmaLib.c',
]

liblzma = static_library(
    'lzma',
    sources: liblzma_sources,
    include_directories: config_inc,
)

liblzma_dep = declare_dependency(
    link_with: liblzma,
    include_directories: '.',
)
