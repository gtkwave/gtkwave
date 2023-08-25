fs = import('fs')
windows = import('windows')

makensis = find_program('makensis', required: false)

# Resources

additional_gtkwave_sources += windows.compile_resources('gtkwave.rc')

# Installer

if makensis.found()
    windows_installer_inputs = [
        fs.copyfile('gtkwave.nsi'),
        fs.copyfile('gtkwave_logo.ico'),
        fs.copyfile('gtkwave_files_logo.ico'),
        fs.copyfile('DumpLog.nsh'),
        fs.copyfile('../COPYING'),
        fs.copyfile('../README.md'),
    ]

    windows_installer_output = 'Install-@0@-w64-@1@.exe'.format(
        application_name,
        meson.project_version(),
    )

    custom_target(
        'windows_installer',
        input: windows_installer_inputs,
        output: windows_installer_output,
        build_always_stale: true,
        command: [
            makensis,
            '-DLIBGCCDLL=libgcc_s_seh-1.dll',
            '-DAPPLICATION_NAME='+application_name,
            '-DBUILDROOT=' + meson.project_build_root(),
            '-DBINDIR=' + get_option('prefix') / get_option('bindir'),
            '-DLIBDIR=' + get_option('prefix') / get_option('libdir'),
            '-DDATADIR=' + get_option('prefix') / get_option('datadir'),
            '-DSYSCONFDIR='+ get_option('prefix') / get_option('sysconfdir'),
            '-DPROJECT_NAME=' + meson.project_name(),
            '-DPROJECT_VERSION=' + meson.project_version(),
            '-DOUT_FILE=' + windows_installer_output,
            '@INPUT0@',
        ],
        depends: exe_files,
    )
endif
