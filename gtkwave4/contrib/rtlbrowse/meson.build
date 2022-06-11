rtlbrowse_sources = [
    'definehash.c',
    'fgetdynamic.c',
    'jrb.c',
    'logfile.c',
    'splay.c',
    'tcl_helper.c',
    'tree_widget.c',
    'stem_recurse.c',
    'tcl_helper.c',
    'tree_widget.c',
    'vlex.c',
]

executable(
    'rtlbrowse',
    rtlbrowse_sources,
    dependencies: [
        gtk_dep,
        libgwlzma_dep,
        libfst_dep,
        liblxt_dep,
        libvzt_dep,
    ],
    include_directories: [config_inc, '../../src'],
    install: true,
)