libgtkwave_tests = [
    'test-gw-blackout-regions',
    'test-gw-color-theme',
    'test-gw-color',
    'test-gw-dump-file',
    'test-gw-enum-filter-list',
    'test-gw-enum-filter',
    'test-gw-facs',
    'test-gw-fst-loader',
    'test-gw-ghw-loader',
    'test-gw-marker',
    'test-gw-named-markers',
    'test-gw-project',
    'test-gw-stems',
    'test-gw-string-table',
    'test-gw-time-range',
    'test-gw-time',
    'test-gw-tree',
    'test-gw-vcd-loader',
    'test-gw-vlist-packer',
    'test-gw-vlist-writer',
    'test-gw-vlist',
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

dump_executable = executable(
    'dump',
    ['dump.c'],
    dependencies: libgtkwave_dep,
    install: false,
)

diff = find_program('diff')

dumpfile_tests = [
    'basic.fst',
    'basic.vcd',
    'basic.ghw',
]

foreach test : dumpfile_tests
    dump_file = meson.current_source_dir() / 'files' / test
    golden_file = dump_file + '.dump'

    dump_target = custom_target(
        'generate-dump-' + test,
        input: dump_file,
        command: [dump_executable, '@INPUT@'],
        output: test + '.dump',
        capture: true,
    )

    test(
        'test-dump-' + test,
        diff,
        args: [golden_file, dump_target],
    )
endforeach
