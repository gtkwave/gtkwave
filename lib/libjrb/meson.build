libjrb = static_library(
    'jrb',
    'jrb.c',
    install: true,
    install_dir: install_libs_rpath,
)

libjrb_dep = declare_dependency(
    link_with: libjrb,
    include_directories: '.',
)
