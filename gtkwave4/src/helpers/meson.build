subdir('fst')

### liblxt

liblxt_sources = [
    'lxt_write.c',
    'lxt2_read.c',
    'lxt2_write.c',
]

liblxt_lib = static_library(
    'liblxt',
    liblxt_sources,
    dependencies: [zlib_dep, bzip2_dep],
    include_directories: [config_inc, '..'],
)

liblxt_dep = declare_dependency(
    link_with: liblxt_lib,
    include_directories: '.',
)

### libvzt

libvzt_sources = [
    'vzt_read.c',
    'vzt_write.c',
]

libvzt_lib = static_library(
    'libvzt',
    libvzt_sources,
    dependencies: [libgwlzma_dep, bzip2_dep, zlib_dep],
    include_directories: [config_inc, '..'],
)

libvzt_dep = declare_dependency(
    link_with: libvzt_lib,
    dependencies: [libgwlzma_dep, bzip2_dep, zlib_dep],
    include_directories: '.',
)

### executables

executable(
    'evcd2vcd',
    ['evcd2vcd.c', '../../contrib/rtlbrowse/jrb.c'],
    include_directories: config_inc,
    install: true,
)

executable(
    'fst2vcd',
    'fst2vcd.c',
    dependencies: libfst_dep,
    include_directories: config_inc,
    install: true,
)

executable(
    'fstminer',
    'fstminer.c',
    dependencies: libfst_dep,
    include_directories: config_inc,
    install: true,
)

executable(
    'lxt2miner',
    'lxt2miner.c',
    dependencies: liblxt_dep,
    include_directories: config_inc,
    install: true,
)

executable(
    'lxt2vcd',
    ['lxt2vcd.c', 'scopenav.c'],
    dependencies: liblxt_dep,
    include_directories: config_inc,
    install: true,
)

executable(
    'shmidcat',
    'shmidcat.c',
    include_directories: config_inc,
    install: true,
)

executable(
    'vcd2fst',
    ['vcd2fst.c', '../../contrib/rtlbrowse/jrb.c'],
    dependencies: libfst_dep,
    include_directories: config_inc,
    install: true,
)

executable(
    'vcd2lxt',
    ['vcd2lxt.c', 'v2l_debug.c'],
    dependencies: liblxt_dep,
    include_directories: config_inc,
    install: true,
)

executable(
    'vcd2lxt2',
    ['vcd2lxt2.c', 'v2l_debug_lxt2.c'],
    dependencies: liblxt_dep,
    include_directories: [config_inc, '..'],
    install: true,
)

executable(
    'vcd2vzt',
    ['vcd2vzt.c', 'v2l_debug_lxt2.c'],
    dependencies: libvzt_dep,
    include_directories: [config_inc, '..'],
    install: true,
)

executable(
    'vzt2vcd',
    ['vzt2vcd.c', 'scopenav.c'],
    dependencies: libvzt_dep,
    include_directories: config_inc,
    install: true,
)

executable(
    'vztminer',
    'vztminer.c',
    dependencies: libvzt_dep,
    include_directories: config_inc,
    install: true,
)
