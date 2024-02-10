libgtkwave_tests = [
    'gw-blackout-regions',
    'gw-color-theme',
    'gw-color',
    'gw-dump-file',
    'gw-enum-filter-list',
    'gw-enum-filter',
    'gw-facs',
    'gw-ghw-loader',
    'gw-marker',
    'gw-named-markers',
    'gw-project',
    'gw-stems',
    'gw-string-table',
    'gw-time-range',
    'gw-time',
    'gw-tree',
]

foreach test : libgtkwave_tests
    test_executable = executable(
        test,
        [test + '.c', 'test-util.c'],
        dependencies: libgtkwave_dep,
    )

    test(
        test,
        test_executable,
        workdir: meson.current_source_dir(),
        protocol: 'tap',
    )
endforeach
