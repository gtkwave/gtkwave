subdir('helpers')

# gtkwave

vcd_keywords_c = custom_target(
    'vcd_keywords.c',
    input: 'vcd_keywords.gperf',
    output: 'vcd_keywords.c',
    command: [
        gperf,
        '-i', '1',
        '-L', 'ANSI-C',
        '-C',
        '-k', '1,\044',
        '-H', 'keyword_hash',
        '-N', 'check_identifier',
        '-tT',
        '--initializer-suffix=,0',
        '--output-file', '@OUTPUT@',
        '@INPUT@',
    ],
)

gtkwave_resources = gnome.compile_resources(
    'gtkwave-resources',
    'io.github.gtkwave.GTKWave.gresource.xml',
    c_name: 'gtkwave',
)

gtkwave_sources = [
    'analyzer.c',
    'baseconvert.c',
    'bitvec.c',
    'bsearch.c',
    'busy.c',
    'clipping.c',
    'currenttime.c',
    'debug.c',
    'edgebuttons.c',
    'entry.c',
    'dump_file_main.c',
    'fetchbuttons.c',
    'fgetdynamic.c',
    'file.c',
    'fonts.c',
    'gconf.c',
    'globals.c',
    'gtk23compat.c',
    'gw-fst-loader.c',
    'gw-mouseover.c',
    'gw-named-marker-dialog.c',
    'gw-time-display.c',
    'gw-vcd-file.c',
    'gw-vcd-loader.c',
    'gw-wave-view-traces.c',
    'gw-wave-view.c',
    'logfile.c',
    'lx2.c',
    'main.c',
    'markerbox.c',
    'menu.c',
    'mouseover.c',
    'pagebuttons.c',
    'pipeio.c',
    'pixmaps.c',
    'print.c',
    'ptranslate.c',
    'rc.c',
    'regex.c',
    'renderopt.c',
    'savefile.c',
    'search.c',
    'shiftbuttons.c',
    'showchange.c',
    'signal_list.c',
    'signalwindow.c',
    'simplereq.c',
    'splash.c',
    'status.c',
    'strace.c',
    'symbol.c',
    'tcl_commands.c',
    'tcl_helper.c',
    'tcl_np.c',
    'tcl_support_commands.c',
    'timeentry.c',
    'translate.c',
    'tree.c',
    'treesearch.c',
    'ttranslate.c',
    'vcd_saver.c',
    'vcd.c',
    'vlist.c',
    'wavewindow.c',
    'zoombuttons.c',
    'cocoa/cocoa_misc.c',
]

gtkwave_dependencies = [
    glib_dep,
    gtk_dep,
    gtk_mac_integration_dep,
    gtk_unix_print_dep,
    m_dep,
    libfst_dep,
    libjrb_dep,
    bzip2_dep,
    judy_dep,
    tcl_dep,
    tk_dep,
    gnu_regex_dep,
    libgtkwave_dep,
]

gtkwave_include_directories = [
    config_inc,
    'cocoa',
]

executable(
    'gtkwave',
    gtkwave_sources,
    gtkwave_resources,
    vcd_keywords_c,
    dependencies: gtkwave_dependencies,
    include_directories: gtkwave_include_directories,
    c_args: warning_args,
    link_args: link_args,
    install: true,
    install_rpath: install_rpath,
)

# twinwave

twinwave_sources = [
    'twinwave.c',
]

twinwave_dependencies = [
    gtk_dep,
    libgtkwave_dep,
]

twinwave_include_directories = [
    config_inc,
    # 'cocoa',
]

executable(
    'twinwave',
    twinwave_sources,
    dependencies: twinwave_dependencies,
    include_directories: twinwave_include_directories,
    install: true,
    install_rpath: install_rpath,
)
