helpers = [
    'evcd2vcd',
    'fst2vcd',
    'fstminer',
    'lxt2miner',
    'lxt2vcd',
    'vcd2fst',
    'vcd2lxt',
    'vcd2lxt2',
    'vcd2vzt',
    'vzt2vcd',
    'vztminer',
]

foreach helper : helpers
    sources = [helper + '.c']
    dependencies = [
        libgtkwave_dep,
    ]

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
        dependencies += liblxt_dep
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
        c_args: warning_args,
        include_directories: config_inc,
        install: true,
        install_rpath: install_rpath,
    )
endforeach
