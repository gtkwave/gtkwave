libfst_sources = [
    'fastlz.c',
    'fstapi.c',
    'lz4.c',
]

libfst_lib = static_library(
    'libstd',
    libfst_sources,
    dependencies: zlib_dep,
    include_directories: config_inc,
)

libfst_dep = declare_dependency(
    link_with: libfst_lib,
    include_directories: '.',
)