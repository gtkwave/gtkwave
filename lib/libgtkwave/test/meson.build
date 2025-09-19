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
    'test-gw-node',
    'test-gw-project',
    'test-gw-stems',
    'test-gw-string-table',
    'test-gw-time-range',
    'test-gw-time',
    'test-gw-tree-builder',
    'test-gw-tree',
    'test-gw-vcd-loader',
    'test-gw-vcd-partial-loader-equivalence',
    'test-gw-vcd-partial-loader-stream',
    'test-gw-vlist-live-reader',
    'test-gw-vlist-packer',
    'test-gw-vlist-writer',
    'test-gw-vlist',
]

foreach test : libgtkwave_tests
    test_sources = [test + '.c', 'test-util.c']
    if test == 'test-gw-vcd-partial-loader-equivalence'
        test_sources += 'test-helpers.c'
    endif

    test_executable = executable(
        test,
        test_sources,
        dependencies: libgtkwave_dep,
    )

    test(
        test,
        test_executable,
        workdir: meson.current_source_dir(),
        protocol: 'tap',
    )
endforeach

# First, define the consumer executable so it gets built
consumer_exe = executable(
    'test-gw-shared-memory-consumer',
    'test-gw-shared-memory-consumer.c',
    dependencies: [libgtkwave_dep],
)

# Next, find our test script
shm_test_script = find_program('test_shm_pipeline.sh', required: true)

# Finally, define the test. Meson will run the script.
test(
    'Shared Memory Pipeline',
    shm_test_script,
    is_parallel: false, # Important: This test uses a system-wide resource
    depends: [consumer_exe] # Ensures the consumer is built before the test runs
)

dump_executable = executable(
    'dump',
    ['dump.c'],
    dependencies: libgtkwave_dep,
    install: false,
)

diff = find_program('diff')

dumpfile_tests = [
    'autocoalesce.fst',
    'autocoalesce.vcd',
    'basic.fst',
    'basic.ghw',
    'basic.vcd',
    'equivalence.vcd',
    'evcd.fst',
    'evcd.vcd',
    'ghdl_basic.fst',
    'hashkill.vcd',
    'left_extension.vcd',
    'names_with_delimiters.fst',
    'names_with_delimiters.vcd',
    'synvec.fst',
    'timescale_100fs_fractional.vcd',
    'timescale_100fs.fst',
    'timescale_100fs.vcd',
    'timescale_1ms.fst',
    'timescale_1ms.vcd',
    'timezero.vcd',
    'timezero.fst',
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
        env: ['G_DEBUG=fatal-warnings'],
        # Without this explicit dependency the generated file isn't update when
        # libgtkwave changes.
        depends: libgtkwave,
    )

    test(
        'test-dump-' + test,
        diff,
        args: ['-u', golden_file, dump_target],
    )
endforeach
