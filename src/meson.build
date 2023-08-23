subdir('helpers')

# gtkwave

vcd_keywords_c = custom_target(
    'vcd_keywords.c',
    input: 'vcd_keywords.gperf',
    output: 'vcd_keywords.c',
    command: [
        gperf,
        '-i', '1',
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

additional_gtkwave_sources += gnome.compile_resources(
    'gtkwave-resources',
    'io.github.gtkwave.GTKWave.gresource.xml',
    c_name: 'gtkwave',
)

gtkwave_sources = [
    'ae2.c',
    'analyzer.c',
    'baseconvert.c',
    'bitvec.c',
    'bsearch.c',
    'busy.c',
    'clipping.c',
    'color.c',
    'currenttime.c',
    'debug.c',
    'edgebuttons.c',
    'entry.c',
    'extload.c',
    'fetchbuttons.c',
    'fgetdynamic.c',
    'file.c',
    'fonts.c',
    'fst.c',
    'gconf.c',
    'getopt.c',
    'getopt1.c',
    'ghw.c',
    'globals.c',
    'gtk23compat.c',
    'gw-time-display.c',
    'hierpack.c',
    'logfile.c',
    'lx2.c',
    'lxt.c',
    'main.c',
    'markerbox.c',
    'menu.c',
    'mouseover_sigs.c',
    'mouseover.c',
    'pagebuttons.c',
    'pipeio.c',
    'pixmaps.c',
    'print.c',
    'ptranslate.c',
    'rc.c',
    'regex.c',
    'renderopt.c',
    'rgb.c',
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
    'tree_component.c',
    'tree.c',
    'treesearch.c',
    'ttranslate.c',
    'vcd_partial.c',
    'vcd_recoder.c',
    'vcd_saver.c',
    'vcd.c',
    'vlist.c',
    'vzt.c',
    'wave_view_traces.c',
    'wave_view.c',
    'wavewindow.c',
    'zoombuttons.c',
]

gtkwave_dependencies = [
    glib_dep,
    gtk_dep,
    gtk_unix_print_dep,
    m_dep,
    libfst_dep,
    liblxt_dep,
    libvzt_dep,
    libghw_dep,
    libjrb_dep,
    bzip2_dep,
    judy_dep,
    tcl_dep,
    tk_dep,
    gnu_regex_dep,
]

gtkwave_include_directories = [
    config_inc,
    'cocoa',
]

gtkwave_exe = executable(
    'gtkwave',
    gtkwave_sources,
    additional_gtkwave_sources,
    vcd_keywords_c,
    win_subsystem: 'windows', # true to hide console
    dependencies: gtkwave_dependencies,
    include_directories: gtkwave_include_directories,
    c_args: warning_args,
    install: true,
)
exe_files += [gtkwave_exe]

# twinwave

twinwave_sources = [
    'twinwave.c',
]

twinwave_dependencies = [
    gtk_dep,
]

twinwave_include_directories = [
    config_inc,
    # 'cocoa',
]

twinwave_exe = executable(
    'twinwave',
    twinwave_sources,
    dependencies: twinwave_dependencies,
    include_directories: twinwave_include_directories,
    install: true,
)
exe_files += [twinwave_exe]
