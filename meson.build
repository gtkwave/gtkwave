project(
    'gtkwave',
    'c',
    'cpp',
    version: '4.0.0-prealpha',
    default_options: [
        'buildtype=debugoptimized',
        'warning_level=2',
        'c_std=c99',
    ],
    meson_version: '>= 1.0.0',
    license: 'GPL-2.0-or-later',
)

app_id = 'io.github.gtkwave.GTKWave'

pkgconfig = import('pkgconfig')
gnome = import('gnome')

cc = meson.get_compiler('c')
add_project_arguments('-D_GNU_SOURCE', language: 'c')
link_args = []

datadir_gtkwave = get_option('datadir') / 'gtkwave3'

version_h = vcs_tag(input: 'src/version.h.in', output: 'version.h')

# Required dependency versions

glib_req = '>=2.72.0'
glib_req_macro = 'GLIB_VERSION_2_72'
json_glib_req = '>=1.8.0'
gtk_req = '>=3.24.0'
gtk_mac_integration_req = '>=3.0.0'
gtk4_req = '>=4.6'
zlib_req = '>=1.2.0'
bzip2_req = '>=1.0.0'
gstreamer_req = '>=1.24.0'

# Dependencies

glib_dep = dependency('glib-2.0', version: glib_req)
gobject_dep = dependency('gobject-2.0', version: glib_req)
gio_dep = dependency('gio-2.0', version: glib_req)
gtk_dep = dependency('gtk+-3.0', version: gtk_req)
gtk4_dep = dependency('gtk4', version: gtk4_req)
gtk_unix_print_dep = dependency(
    'gtk+-unix-print-3.0',
    version: gtk_req,
    required: false,
)
gtk_mac_integration_dep = dependency(
    'gtk-mac-integration-gtk3',
    version: gtk_mac_integration_req,
    required: host_machine.system() == 'darwin',
)
json_glib_dep = dependency('json-glib-1.0', version: json_glib_req)
zlib_dep = dependency('zlib', version: zlib_req)
m_dep = cc.find_library('m', required: false)
judy_dep = cc.find_library(
    'Judy',
    has_headers: 'Judy.h',
    required: get_option('judy'),
)
gnu_regex_dep = cc.find_library(
    'regex',
    has_headers: 'regex.h',
    required: host_machine.system() == 'windows',
)
thread_dep = dependency('threads', required: false)

bzip2_dep = dependency('bzip2', version: bzip2_req, required: false)
if not bzip2_dep.found()
    # pkg-config files for bzip2 aren't always installed,
    # try to manually find the library as a fallback
    bzip2_dep = cc.find_library('bz2', has_headers: 'bzlib.h')
endif

gstreamer_dep = dependency('gstreamer-1.0', version: gstreamer_req, required: get_option('audio'))
gstreamer_audio_dep = dependency('gstreamer-audio-1.0', version: gstreamer_req, required: get_option('audio'))
gstreamer_app_dep = dependency('gstreamer-app-1.0', version: gstreamer_req, required: get_option('audio'))
gstreamer_found = gstreamer_dep.found() and gstreamer_audio_dep.found() and gstreamer_app_dep.found()

libfst_dep = dependency('libfst')

# Experimental libpeas based plugin support

if get_option('experimental_plugin_support')
    libpeas_dep = dependency('libpeas-2')
    add_project_arguments('-DEXPERIMENTAL_PLUGIN_SUPPORT', language: 'c')
endif

# Limit used glib functions to minimum required version

add_project_arguments(
    '-DGLIB_VERSION_MIN_REQUIRED=' + glib_req_macro,
    '-DGLIB_VERSION_MAX_ALLOWED=' + glib_req_macro,
    language: 'c',
)

# Add build arguments needed for MacOS

if host_machine.system() == 'darwin'
    add_project_arguments(
        '-xobjective-c',
        '-DWAVE_COCOA_GTK',
        language: 'c')
    link_args = [
        '-Wl,-lobjc',
        '-framework', 'Cocoa',
        '-framework', 'ApplicationServices',
    ]
endif

# External programs

gperf = find_program('gperf')

flex = find_program('flex')
flex_gen = generator(
    flex,
    output: ['@BASENAME@.yy.c', '@BASENAME@.yy.h'],
    arguments: [
        '--outfile=@OUTPUT0@',
        '--header-file=@OUTPUT1@',
        '@INPUT@',
    ],
)

# Check header files

config = configuration_data()
config.set('STDC_HEADERS', 1)
config.set('HAVE_INTTYPES_H', cc.has_header('inttypes.h'))
config.set('HAVE_SYS_STAT_H', cc.has_header('sys/stat.h'))
config.set('HAVE_ALLOCA_H', cc.has_header('alloca.h'))
config.set('HAVE_GETOPT_H', cc.has_header('getopt.h'))
config.set('HAVE_FCNTL', cc.has_header('fcntl.h'))
config.set10('HAVE_UNISTD_H', cc.has_header('unistd.h'))
config.set('HAVE_LIBPTHREAD', thread_dep.found())
config.set('_WAVE_HAVE_JUDY', judy_dep.found())
config.set('WAVE_GTK_UNIX_PRINT', gtk_unix_print_dep.found())
config.set('WAVE_USE_STRUCT_PACKING', get_option('struct_packing'))
config.set('WAVE_MANYMARKERS_MODE', get_option('manymarkers'))
config.set('WAVE_DISABLE_FAST_TREE', not get_option('fasttree'))
config.set('HAVE_GSTREAMER', gstreamer_found)

# Check functions

functions = [
    'fseeko',
    'realpath',
    'setenv',
    'unsetenv'
]

foreach function : functions
    config.set('HAVE_' + function.to_upper(), cc.has_function(function))
endforeach

# Additional warnings and errors

warning_args = []
test_warning_args = [
    '-Wcast-align',
    '-Werror=address',
    '-Werror=array-bounds',
    '-Werror=empty-body',
    '-Werror=implicit',
    '-Werror=implicit-function-declaration',
    '-Werror=incompatible-pointer-types',
    '-Werror=init-self',
    '-Werror=int-conversion',
    '-Werror=int-to-pointer-cast',
    '-Werror=main',
    '-Werror=misleading-indentation',
    '-Werror=missing-braces',
    '-Werror=missing-include-dirs',
    '-Werror=nonnull',
    '-Werror=overflow',
    '-Werror=parenthesis',
    '-Werror=pointer-arith',
    '-Werror=pointer-to-int-cast',
    '-Werror=return-type',
    '-Werror=sequence-point',
    '-Werror=shadow',
    '-Werror=strict-prototypes',
    '-Werror=trigraphs',
    '-Werror=undef',
    '-Werror=write-strings',
    '-Wformat-nonliteral',
    ['-Werror=format-security', '-Werror=format=2'],
    '-Wignored-qualifiers',
    '-Wimplicit-function-declaration',
    '-Wlogical-op',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wnested-externs',
    '-Wno-cast-function-type',
    '-Wno-missing-field-initializers',
    '-Wno-sign-compare',
    '-Wno-unused-parameter',
    '-Wold-style-definition',
    '-Wpointer-arith',
    '-Wstrict-prototypes',
    '-Wswitch-default',
    # '-Wswitch-enum',
    '-Wundef',
    '-Wuninitialized',
    '-Wunused',
]

foreach arg: test_warning_args
    if cc.has_multi_arguments(arg)
        warning_args += arg
    endif
endforeach

# Set excutable rpaths for non standard prefixes

if get_option('set_rpath').enabled() or (get_option('set_rpath').auto() and get_option('prefix') != '/usr')
    install_rpath = get_option('prefix') / get_option('libdir')
else
    install_rpath = ''
endif

# config.h

config.set_quoted('PACKAGE_BUGREPORT', 'bybell@rocketmail.com')
config.set_quoted('PACKAGE_VERSION', meson.project_version())

configure_file(output: 'config.h', configuration: config)
config_inc = include_directories('.')

subdir('lib')
subdir('src')
subdir('man')
subdir('share')
subdir('examples')
subdir('doc')
subdir('contrib')

gnome.post_install(
    update_mime_database: get_option('update_mime_database'),
    update_desktop_database: true,
    gtk_update_icon_cache: true,
)

if host_machine.system() == 'darwin' and get_option('build_macos_app') == true
    macos_app_path = get_option('prefix')
    macos_app_contents = macos_app_path / 'Contents'

    macos_app_exe = macos_app_contents / 'MacOS'
    macos_app_resources = macos_app_contents / 'Resources'

    install_data(
        'macos/launcher.sh',
        rename: 'gtkwave',
        install_dir: macos_app_exe,
        install_mode: 'rwxr-xr-x'
    )

    install_data(
        'macos/Info.plist',
        install_dir: macos_app_contents
    )

    install_data(
        'macos/AppIcon.icns',
        install_dir: macos_app_resources
    )

    install_data(
        'macos/gtkwave.entitlements',
        install_dir: macos_app_contents
    )

    meson.add_install_script('macos/install_deps.sh')
endif
