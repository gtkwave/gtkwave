subdir('lzma')

libvzt_sources = [
    'vzt_read.c',
    'vzt_write.c',
]

libvzt_dependencies = [
    libgwlzma_dep,
    zlib_dep,
    bzip2_dep,
    thread_dep,
]

libvzt = static_library(
    'vzt',
    sources: libvzt_sources,
    dependencies: libvzt_dependencies,
    include_directories: config_inc,
    install: true,
    install_dir: install_libs_rpath,
)

libvzt_dep = declare_dependency(
    link_with: libvzt,
    dependencies: libvzt_dependencies,
    include_directories: '.',
)