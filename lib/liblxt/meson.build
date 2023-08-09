liblxt_sources = [
    'lxt_write.c',
    'lxt2_read.c',
    'lxt2_write.c',
]

liblxt_dependencies = [
    zlib_dep,
    bzip2_dep,
]

liblxt = static_library(
    'lxt',
    sources: liblxt_sources,
    dependencies: liblxt_dependencies,
    include_directories: [config_inc, '..'],
)

liblxt_dep = declare_dependency(
    link_with: liblxt,
    include_directories: '.',
)
