libgtkwave_tests = [
    'gw-blackout-regions',
    'gw-color-theme',
    'gw-color',
    'gw-marker',
    'gw-named-markers',
    'gw-project',
    'gw-stems',
]

foreach test : libgtkwave_tests
    test_executable = executable(
        test,
        test + '.c',
        dependencies: libgtkwave_dep,
    )

    test(
        test,
        test_executable,
        workdir: meson.current_source_dir(),
        protocol: 'tap',
    )
endforeach
