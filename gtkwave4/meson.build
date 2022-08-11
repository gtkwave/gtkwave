project(
    'gtkwave',
    'c', 'cpp',
    version: '4.0.0-alpha.0',
    meson_version: '>=0.62',
    default_options: [
        'buildtype=debugoptimized',
        'warning_level=3',
        'c_std=gnu99',
        'cpp_std=gnu++11',
    ],
)

fs = import('fs')
gnome = import('gnome')

cc = meson.get_compiler('c')

### Paths

datadir = join_paths(get_option('prefix'), get_option('datadir'))

### Dependencies

if get_option('gtk_version') == 'gtk3'
    gtk_dep = dependency('gtk+-3.0', version: '>=3.24')
else
    gtk_dep = dependency('gtk4', version: '>=4.6')
endif
bzip2_dep = dependency('bzip2', version: '>=1.0')
zlib_dep = dependency('zlib', version: '>=1.2')
libm_dep = cc.find_library('m', required: false)

tmp_tcl_dep = dependency('tcl', version: '>=8.5', required: get_option('tcl'))
tmp_tk_dep = dependency('tk', version: '>=8.5', required: get_option('tcl'))

# Only enable TCL support if both TCL and TK were found
tcl_found = tmp_tcl_dep.found() and tmp_tk_dep.found()
if tcl_found
    tcl_dep = declare_dependency(dependencies: [tmp_tcl_dep, tmp_tk_dep])
else
    tcl_dep = dependency('', required: false)
endif

### config.h generation

config = configuration_data()
config.set_quoted('PACKAGE_VERSION', meson.project_version())
config.set_quoted('PACKAGE_BUGREPORT', 'bybell@rocketmail.com')

# inttypes.h and getopt.h are required for a successful build
cc.has_header('inttypes.h', required: true)
cc.has_header('getopt.h', required: true)

config.set('HAVE_INTTYPES_H', 1)
config.set('HAVE_GETOPT_H', 1)

if cc.has_function('realpath')
    config.set('HAVE_REALPATH', 1)
endif

if tcl_found
    config.set('HAVE_LIBTCL', 1)
endif

configure_file(
    configuration: config,
    output: 'config.h',
)

config_inc = include_directories('.')

### Post installation

gnome.post_install(update_desktop_database: true)

### Subdirectories

subdir('src')
subdir('man')
subdir('share')
subdir('contrib')

