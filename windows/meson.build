fs = import('fs')
windows = import('windows')

makensis = find_program('makensis', required: false)

# Resources

additional_gtkwave_sources += windows.compile_resources('gtkwave.rc')

# Installer

if makensis.found()
    windows_installer_additional_inputs = [
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

    windows_installer_config = configuration_data()
    windows_installer_config.set('buildroot', meson.project_build_root())
    windows_installer_config.set('bindir', get_option('prefix') / get_option('bindir'))
    windows_installer_config.set('libdir', get_option('prefix') / get_option('libdir'))
    windows_installer_config.set('datadir', get_option('prefix') / get_option('datadir'))
    windows_installer_config.set('sysconfdir', get_option('prefix') / get_option('sysconfdir'))
    windows_installer_config.set('project_name', meson.project_name())
    windows_installer_config.set('project_version', meson.project_version())
    windows_installer_config.set('out_file', meson.current_build_dir() / windows_installer_output)

    gtkwave_nsi = configure_file(
        configuration: windows_installer_config,
        input: 'gtkwave.nsi.in',
        output: 'gtkwave.nsi',
    )

    custom_target(
        'windows_installer',
        input: [gtkwave_nsi] + windows_installer_additional_inputs,
        output: windows_installer_output,
        build_always_stale: true,
        command: [
            makensis,
            '-DLIBGCCDLL=libgcc_s_seh-1.dll',
            '-DAPPLICATION_NAME='+application_name,
            '@INPUT0@',
            
        ],
        depends: exe_files,
    )
endif
