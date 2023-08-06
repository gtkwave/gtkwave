project(
    'gtkwave',
    'c',
    'cpp',
    version: '3.4.0',
    default_options: [
        'buildtype=debugoptimized',
        'warning_level=2',
        'c_std=c99',
    ],
    meson_version: '>= 1.0.0',
    license: 'GPL-2.0-or-later',
)

app_id = 'io.github.gtkwave.GTKWave'

gnome = import('gnome')

cc = meson.get_compiler('c')
add_project_arguments('-D_GNU_SOURCE', language: 'c')

datadir_gtkwave = get_option('datadir') / 'gtkwave3'

# Required dependency versions

glib_req = '>=2.64.0'
gtk_req = '>=3.24.0'
zlib_req = '>=1.2.0'
bzip2_req = '>=1.0.0'
tcl_req = '>=8.6.0'
tk_req = '>=8.6.0'

# Dependencies

glib_dep = dependency('glib-2.0', version: glib_req)
gtk_dep = dependency('gtk+-3.0', version: gtk_req)
gtk_unix_print_dep = dependency('gtk+-unix-print-3.0', version: gtk_req, required: false)
zlib_dep = dependency('zlib', version: zlib_req)
bzip2_dep = dependency('bzip2', version: bzip2_req)
tcl_dep = dependency('tcl', version: tcl_req, required: get_option('tcl'))
tk_dep = dependency('tk', version: tk_req, required: get_option('tcl'))
m_dep = cc.find_library('m', required: false)
judy_dep = cc.find_library('Judy', has_headers: 'Judy.h', required: get_option('judy'))

# External programs

gperf = find_program('gperf')

flex = find_program('flex')
flex_gen = generator(
    flex,
    output: '@BASENAME@.c',
    arguments: [
        '--outfile=@OUTPUT@',
        '@INPUT@',
    ],
)

# Check header files

config = configuration_data()
config.set('HAVE_INTTYPES_H', cc.has_header('inttypes.h'))
config.set('HAVE_SYS_STAT_H', cc.has_header('sys/stat.h'))
config.set('HAVE_ALLOCA_H', cc.has_header('alloca.h'))
config.set('HAVE_GETOPT_H', cc.has_header('getopt.h'))
config.set('_WAVE_HAVE_JUDY', judy_dep.found())
config.set('HAVE_LIBTCL', tcl_dep.found() and tk_dep.found())
config.set('WAVE_GTK_UNIX_PRINT', gtk_unix_print_dep.found())
config.set('WAVE_USE_STRUCT_PACKING', get_option('struct_packing'))
config.set('WAVE_MANYMARKERS_MODE', get_option('manymarkers'))
config.set('WAVE_DISABLE_FAST_TREE', not get_option('fasttree'))

# config.h

config.set_quoted('PACKAGE_BUGREPORT', 'bybell@rocketmail.com')
config.set_quoted('PACKAGE_VERSION', meson.project_version())

configure_file(output: 'config.h', configuration: config)
config_inc = include_directories('.')

subdir('src')
subdir('man')
subdir('share')
subdir('examples')
subdir('doc')
subdir('contrib')

gnome.post_install(
    update_mime_database: get_option('update_mime_database'),
    update_desktop_database: true,
)
