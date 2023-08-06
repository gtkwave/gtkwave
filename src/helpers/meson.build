subdir('fst')

# liblxt

liblxt_sources = [
    'lxt_write.c',
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

# liblxt2

liblxt2_sources = [
    'lxt2_read.c',
    'lxt2_write.c',
]

liblxt2_dependencies = [
    zlib_dep,
]

liblxt2 = static_library(
    'lxt2',
    sources: liblxt2_sources,
    dependencies: liblxt2_dependencies,
    include_directories: [config_inc, '..'],
)

liblxt2_dep = declare_dependency(
    link_with: liblxt2,
    include_directories: '.',
)

# libvzt

libvzt_sources = [
    'vzt_read.c',
    'vzt_write.c',
]

libvzt_dependencies = [
    liblzma_dep,
    zlib_dep,
    bzip2_dep,
]

libvzt = static_library(
    'vzt',
    sources: libvzt_sources,
    dependencies: libvzt_dependencies,
    include_directories: [config_inc, '..'],
)

libvzt_dep = declare_dependency(
    link_with: libvzt,
    dependencies: liblzma_dep,
    include_directories: '.',
)

# Helper executables

helpers = [
    'evcd2vcd',
    'fst2vcd',
    'fstminer',
    'lxt2miner',
    'lxt2vcd',
    'shmidcat',
    'vcd2fst',
    'vcd2lxt',
    'vcd2lxt2',
    'vcd2vzt',
    'vzt2vcd',
    'vztminer',
]

foreach helper : helpers
    sources = [helper + '.c']
    dependencies = []

    if helper in ['evcd2vcd', 'vcd2fst']
        sources += '../../contrib/rtlbrowse/jrb.c'
    endif
    if helper in ['vcd2lxt']
        sources += 'v2l_debug.c'
    endif
    if helper in ['vcd2lxt2', 'vcd2vzt']
        sources += 'v2l_debug.c'
    endif
    if helper in ['lxt2vcd', 'vzt2vcd']
        sources += 'scopenav.c'
    endif

    if helper.contains('lxt')
        dependencies += [liblxt_dep, liblxt2_dep]
    endif
    if helper.contains('fst')
        dependencies += libfst_dep
    endif
    if helper.contains('vzt')
        dependencies += libvzt_dep
    endif

    executable(
        helper,
        sources,
        dependencies: dependencies,
        include_directories: config_inc,
        install: true,
    )
endforeach
