# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Current limitations compared to GTKWave LTS 3.3

Some features have been removed in this version of GTKWave to make the codebase
easier to refactor. These features might be restored in future versions of GTKWave,
if there is enough interest by users. Please notify us on GitHub if you require any
of these features.

- Removed the interactive VCD mode.
- Removed support for LXT, LXT2, VZT, and AET2 formats.

### Changed

- Merged in major GUI widget refactoring.
- Updated the UI design of many dialogs.
- Improved rendering of all 0s/1s states of vectors.
- Replaced hamburger menu with traditional menubar.
- Improved the readability of the cursor and marker time display.
- Migrated from autotools to meson.
- Changed the fallback text editor from gedit to the default editor that is associated with the source filetype.

### Added

- Added support for `namespace import gtkwave::*` in Tcl scripts.
- Added OpenBSD and FreeBSD OS support for unbuffered FST I/O.
- Added `dbl_mant_dig_overrides` rc environment variable.
- Added `disable_antialiasing` rc variable.
- Added `editor_run_in_terminal` rc variable.

### Removed

- Removed support for GTK 2.
- Removed "Signal Search Hierarchy" dialog.
- Removed setting to disable the "Alternate Wheel Mode".
- Removed legacy VCD loader.
- Removed VCD fastload and support for vlist spillfiles.
- Removed rc variables:
    - `alt_wheel_mode`
    - `append_vcd_hier`
    - `atomic_vectors`
    - `disable_ae2_alias`
    - `force_toolbars`
    - `hide_sst`
    - `hier_grouping`
    - `hpane_pack`
    - `lxt_clock_compress_to_z`
    - `use_scrollbar_only`
    - `use_scrollwheel_as_y`
    - `use_standard_clicking`
    - `use_standard_trace_select`
    - `use_toolbutton_interface`
    - `vcd_explicit_zero_subscripts`
    - `vlist_spill`

### Fixed

- Fixed toggle menu item access under Tcl.
- Path fix for `twinwave` on Windows.
- Fixed high CPU usage on Wayland.
- Fixed display update issues on Wayland.
- Fixed slow redrawing of dense waveforms.
- Compiler fix for `__GTK_SOCKET_H__` availability.

[Unreleased]: https://github.com/gtkwave/gtkwave/compare/v3.3.116...HEAD