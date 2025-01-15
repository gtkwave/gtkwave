libfst_sources = [
    'fastlz.c',
    'fstapi.c',
    'lz4.c',
]

libfst_dependencies = [
    zlib_dep,
    bzip2_dep,
    judy_dep,
    thread_dep,
]

libfst = static_library(
    'fst',
    sources: libfst_sources,
    dependencies: libfst_dependencies,
    include_directories: config_inc,
)

libfst_dep = declare_dependency(
    link_with: libfst,
    include_directories: '.',
)
