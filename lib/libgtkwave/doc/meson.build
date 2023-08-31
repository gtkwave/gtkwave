gi_docgen = find_program('gi-docgen')

libgtkwave_toml_conf = configuration_data()
libgtkwave_toml_conf.set('version', meson.project_version())

libgtkwave_toml = configure_file(
    input: 'libgtkwave.toml.in',
    output: 'libgtkwave.toml',
    configuration: libgtkwave_toml_conf,
)

custom_target(
    'libgtkwave-docs',
    input: [libgtkwave_gir[0], libgtkwave_toml],
    output: 'libgtkwave-docs',
    command: [
        gi_docgen,
        'generate',
        '--config=@INPUT1@',
        '--output-dir=@OUTPUT@', '@INPUT0@',
    ],
    build_by_default: true,
)
