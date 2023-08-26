libgtkwave_tests = [
    'gw-marker',
    'gw-named-markers',
    'gw-project',
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
