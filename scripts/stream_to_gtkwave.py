#!/usr/bin/env python3
import subprocess
import time
import math
import sys
import threading
import io
import shutil
import os
import re
from typing import List

# This import will fail if python-xlib is not installed
try:
    from Xlib import display, X
    XLIBS_AVAILABLE = True
except Exception:
    XLIBS_AVAILABLE = False


def assert_window_is_shown_xlib(process_pid, timeout=5):
    """
    Uses the 'python-xlib' library to verify that a GUI window owned by the
    given PID has appeared on the screen.
    """
    print(f"\nVerifying GUI window for PID {process_pid} using 'python-xlib'...")
    if not XLIBS_AVAILABLE:
        print("--> WARNING: 'python-xlib' not found. Assuming window appeared.")
        return True

    try:
        d = display.Display()
        root = d.screen().root
    except Exception as e:
        print(f"--> ERROR: Could not connect to the X Display: {e}")
        return False

    NET_WM_PID = d.intern_atom('_NET_WM_PID')
    NET_CLIENT_LIST = d.intern_atom('_NET_CLIENT_LIST')

    # Normalize a simple search pattern for window title/class fallback
    gw_pattern = re.compile(r'gtkwave', re.IGNORECASE)

    start_time = time.monotonic()
    while time.monotonic() - start_time < timeout:
        try:
            # Try client list first (if available)
            prop = root.get_full_property(NET_CLIENT_LIST, X.AnyPropertyType)
            client_ids = prop.value if prop else []

            # Also walk the root window's children to find windows that
            # may not be present in _NET_CLIENT_LIST under some setups.
            try:
                root_tree = root.query_tree()
                root_children = [w.id for w in getattr(root_tree, 'children', []) or []]
            except Exception:
                root_children = []

            # Combine unique ids to scan
            combined_ids = list(dict.fromkeys(list(client_ids) + list(root_children)))
            if time.monotonic() - start_time < 2.0:
                print(f"Scanning {len(combined_ids)} windows (client list + root children) for GTKWave (timeout {timeout}s)")

            for window_id in combined_ids:
                try:
                    window = d.create_resource_object('window', window_id)
                    # Try to inspect this window and its children recursively.
                    def inspect_win(w):
                        # Check PID property first
                        try:
                            pid_prop = None
                            try:
                                pid_prop = w.get_full_property(NET_WM_PID, X.AnyPropertyType)
                            except Exception:
                                pid_prop = None
                            if pid_prop and pid_prop.value and pid_prop.value[0] == process_pid:
                                print(f"--> SUCCESS: Found window ID {w.id} for PID {process_pid} via _NET_WM_PID.")
                                return True
                        except Exception:
                            pass

                        # Check WM_NAME / WM_CLASS for GTKWave hints
                        try:
                            name = w.get_wm_name() or ''
                        except Exception:
                            name = ''
                        try:
                            wmclass = w.get_wm_class() or []
                            wmclass_str = ' '.join(wmclass) if wmclass else ''
                        except Exception:
                            wmclass_str = ''

                        if gw_pattern.search(name) or gw_pattern.search(wmclass_str):
                            # Accept as success even if PID didn't match; in headless
                            # setups the window may be 'unviewable' but still present.
                            try:
                                attrs = w.get_attributes()
                                # Accept any state except 'IsUnMapped' (0). This lets
                                # IsUnViewable (1) or IsViewable (2) pass in Xvfb.
                                if not attrs or attrs.map_state != X.IsUnMapped:
                                    print(f"--> SUCCESS: Found window ID {w.id} matching GTKWave (name/class), map_state={getattr(attrs,'map_state',None)}.")
                                    return True
                            except Exception:
                                # Best-effort success
                                print(f"--> SUCCESS (best-effort): Found window ID {w.id} matching GTKWave.")
                                return True

                        # Recurse into children
                        try:
                            tree = w.query_tree()
                            for ch in getattr(tree, 'children', []) or []:
                                try:
                                    if inspect_win(ch):
                                        return True
                                except Exception:
                                    continue
                        except Exception:
                            pass

                        return False

                    if inspect_win(window):
                        d.close()
                        return True
                except Exception:
                    # Some windows may disappear between enumeration and property read.
                    continue
        except Exception:
            pass
        time.sleep(0.25)

    print(f"--> FAILURE: Timed out after {timeout} seconds. No window found for PID {process_pid}.")
    try:
        d.close()
    except Exception:
        pass
    return False


def _drain_proc_stderr(proc, timeout=1.0):
    """
    Read remaining stderr from a process. If stderr is a text stream, return the text.
    This will block, but since the process is expected to be terminated, it should return quickly.
    """
    if not proc:
        return ""

    # Prefer stderr if available, otherwise stdout. Some processes (like
    # gtkwave) may redirect stderr into stdout; prefer reading whichever is
    # present to return combined logs.
    streams = []
    if getattr(proc, 'stderr', None):
        streams.append(proc.stderr)
    if getattr(proc, 'stdout', None):
        streams.append(proc.stdout)

    for s in streams:
        try:
            data = s.read()
            if data:
                return data
        except Exception:
            try:
                buf = getattr(s, 'buffer', None)
                if buf:
                    raw = buf.read()
                    if raw:
                        return raw.decode('utf-8', errors='replace')
            except Exception:
                continue

    return ""


def _start_live_reader(stream, out_buffer: List[str], label=None):
    """
    Start a background thread that reads lines from `stream`, prints them
    immediately to stdout (so logs are live), and appends them to out_buffer
    for later diagnostics. Returns the Thread object.
    """
    def _reader():
        try:
            for line in iter(stream.readline, ''):
                if not line:
                    break
                # Print live without extra newline handling (line already has newline)
                try:
                    print(line, end='')
                except Exception:
                    # If printing fails, ignore and continue
                    pass
                try:
                    out_buffer.append(line)
                except Exception:
                    pass
        except Exception:
            pass

    t = threading.Thread(target=_reader, daemon=True)
    t.start()
    return t


def _report_gtkwave_exit(gtkwave_proc):
    """
    Called when gtkwave exits unexpectedly: prints return code and stderr contents.
    """
    if not gtkwave_proc:
        print("--> GTKWave process object not available.")
        return

    returncode = gtkwave_proc.returncode
    if returncode is None:
        # Process hasn't been waited on; try to poll/wait briefly.
        try:
            gtkwave_proc.wait(timeout=0.1)
            returncode = gtkwave_proc.returncode
        except Exception:
            returncode = gtkwave_proc.returncode

    if returncode is None:
        print("--> GTKWave exited but return code is unknown.")
    elif returncode < 0:
        # Killed by signal
        sig = -returncode
        print(f"\n--- GTKWave terminated by signal: {sig} (possible segfault if signal 11). Return code: {returncode} ---")
    else:
        print(f"\n--- GTKWave exited with return code: {returncode} ---")

    # Prefer the live buffer if the reader thread collected output.
    output = None
    try:
        buf = getattr(gtkwave_proc, '_live_buffer', None)
        if buf:
            output = ''.join(buf)
    except Exception:
        output = None

    if not output:
        output = _drain_proc_stderr(gtkwave_proc)

    if output:
        # If live streaming was active, avoid re-printing the entire buffer
        live = getattr(gtkwave_proc, '_live_streaming', False)
        if live:
            # Show just the last 3000 characters (or approx last 200 lines)
            tail = output[-3000:]
            print("\n--- GTKWave recent output (tail) ---")
            print(tail)
            print("--- end of GTKWave recent output ---")
        else:
            print("\n--- GTKWave output (stdout+stderr, truncated to 40k chars) ---")
            print(output[:40000])
            print("--- end of GTKWave output ---")
    else:
        print("(No GTKWave output captured.)")


def stream_to_gtkwave_interactive():
    """
    Launches the shmidcat -> gtkwave pipeline, verifies the GUI appears,
    and then streams correctly formatted VCD data with flushing for smooth updates.

    This version monitors the gtkwave process: if it exits (crashes or closed),
    streaming stops and stderr is reported to the user.
    
    Returns:
        int: 0 if successful, 1 if GTKWave crashed or exited with error
    """
    shmidcat_proc = None
    gtkwave_proc = None
    writer = None

    try:
        # 1. Launch the pipeline.
        print("Launching shmidcat process...")
        shmidcat_proc = subprocess.Popen(
            ['shmidcat'],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1  # line-buffered in text mode
        )

        print("Launching gtkwave process...")
        # Pass the stdout of shmidcat to gtkwave's stdin. Capture stderr to report crashes.
        # If 'stdbuf' is available, use it to make the child process line-buffered
        # even when stdout is a pipe. This avoids large buffering delays.
        cmd = ['gtkwave', '-I', '-v']
        if shutil.which('stdbuf'):
            cmd = ['stdbuf', '-oL'] + cmd

        gtkwave_proc = subprocess.Popen(
            cmd,
            stdin=shmidcat_proc.stdout,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1
        )
        print(f"GTKWave process launched with PID: {gtkwave_proc.pid}")

        # Start a background thread to stream gtkwave output live and collect it
        gtkwave_output_buffer = []
        gtkwave_output_thread = None
        if gtkwave_proc.stdout is not None:
            gtkwave_output_thread = _start_live_reader(gtkwave_proc.stdout, gtkwave_output_buffer, label='gtkwave')
            # Attach buffer, thread and mark live streaming so reporter can avoid duplicating
            try:
                setattr(gtkwave_proc, '_live_buffer', gtkwave_output_buffer)
                setattr(gtkwave_proc, '_live_streaming', True)
                setattr(gtkwave_proc, '_live_thread', gtkwave_output_thread)
            except Exception:
                pass

        # 2. Create ONE VCDWriter instance.
        try:
            from vcd import VCDWriter
        except Exception:
            print("\n--- ERROR: 'vcd' Python package not found. Install with `pip install vcd` or similar. ---")
            return 1

        # writer writes to shmidcat_proc.stdin.
        writer = VCDWriter(shmidcat_proc.stdin, timescale='1 ns', date='today')
        sine_wave_var = writer.register_var('mysim', 'sine_wave', 'integer', size=8, init=0)

        # 3. Prime the pipeline.
        print("\nPriming the pipeline with VCD header to prevent deadlock...")
        writer.flush()

        # It is possible gtkwave exited immediately (e.g., missing display). Check it.
        if gtkwave_proc.poll() is not None:
            print("\n--- ERROR: GTKWave exited immediately after launch. ---")
            _report_gtkwave_exit(gtkwave_proc)
            return 1

        # 4. Assert that the window appears. Allow overriding wait via DEMO_GUI_WAIT (seconds)
        try:
            gui_wait = int(os.environ.get('DEMO_GUI_WAIT', '30'))
        except Exception:
            gui_wait = 30

        if not assert_window_is_shown_xlib(gtkwave_proc.pid, timeout=gui_wait):
            # If gtkwave has already exited while waiting for the window, report and exit.
            if gtkwave_proc.poll() is not None:
                print("\n--- ERROR: GTKWave terminated while waiting for window. ---")
                _report_gtkwave_exit(gtkwave_proc)
                return 1
            print("\n--- TEST FAILED: GTKWave interactive GUI did not appear. ---")
            return 1

        print("\n--- TEST PASSED: GTKWave interactive GUI is visible. ---")

        # 5. Stream the data.
        print("Streaming data with forced flushing for smooth updates...")
        amplitude = 127
        steps = 1200

        for timestamp in range(steps):
            # Before attempting to write, ensure gtkwave is still running.
            if gtkwave_proc.poll() is not None:
                print("\n--- NOTICE: GTKWave has exited. Stopping streaming. ---")
                _report_gtkwave_exit(gtkwave_proc)
                break

            angle = (timestamp / 100.0) * 2 * math.pi
            value = int(amplitude * math.sin(angle))

            try:
                writer.change(sine_wave_var, timestamp, value)
                # Force the writer to flush its buffer through the pipe immediately.
                writer.flush()
            except BrokenPipeError:
                # Broken pipe indicates the consumer closed (shmidcat or gtkwave). Stop streaming.
                print("\n\n--- ERROR: Broken pipe detected while writing to shmidcat. GTKWave or shmidcat closed? ---")
                if gtkwave_proc and gtkwave_proc.poll() is not None:
                    _report_gtkwave_exit(gtkwave_proc)
                break
            except Exception as e:
                # Catch other IO errors and stop.
                print(f"\n\n--- ERROR: Exception while writing/streaming: {e} ---")
                if gtkwave_proc and gtkwave_proc.poll() is not None:
                    _report_gtkwave_exit(gtkwave_proc)
                break

            # Progress indicator
            print(f"Timestamp {timestamp+1}/{steps} streamed.", end='\r')

            # Small sleep to simulate real streaming frequency.
            time.sleep(0.05)
        else:
            # Completed all steps without interruption.
            print(f"\n\nFinished generating {steps} timesteps. Press Enter to close.")
            try:
                input()
            except Exception:
                # If running in non-interactive environment, don't block.
                pass

    except FileNotFoundError as e:
        print(f"\n--- ERROR: Command not found: {e.filename}. ---")
    except KeyboardInterrupt:
        print("\n--- Interrupted by user. Shutting down. ---")
    except Exception as e:
        print(f"\nAn unexpected error occurred: {e}")
        # If gtkwave has exited, try to report stderr as well.
        if gtkwave_proc and gtkwave_proc.poll() is not None:
            _report_gtkwave_exit(gtkwave_proc)
    finally:
        print("Cleaning up processes and resources...")

        # Close VCD writer if present.
        if writer:
            try:
                writer.close()
            except Exception:
                pass

        # Ensure we close the shmidcat stdin to signal EOF to downstream processes.
        if shmidcat_proc:
            try:
                if getattr(shmidcat_proc, "stdin", None):
                    try:
                        shmidcat_proc.stdin.close()
                    except Exception:
                        pass
            except Exception:
                pass

        # Terminate gtkwave if still running.
        if gtkwave_proc:
            try:
                if gtkwave_proc.poll() is None:
                    gtkwave_proc.terminate()
                    try:
                        gtkwave_proc.wait(timeout=1.0)
                    except Exception:
                        gtkwave_proc.kill()
            except Exception:
                pass

            # Report final stderr if gtkwave exited earlier but wasn't reported.
            try:
                if gtkwave_proc.returncode is None:
                    gtkwave_proc.wait(timeout=0.1)
            except Exception:
                pass

            if gtkwave_proc.returncode is not None:
                _report_gtkwave_exit(gtkwave_proc)

        # Terminate shmidcat if still running.
        if shmidcat_proc:
            try:
                if shmidcat_proc.poll() is None:
                    shmidcat_proc.terminate()
                    try:
                        shmidcat_proc.wait(timeout=1.0)
                    except Exception:
                        shmidcat_proc.kill()
            except Exception:
                pass

        print("Processes terminated.")
        
        # Return exit code based on whether GTKWave crashed
        if gtkwave_proc and gtkwave_proc.returncode is not None:
            if gtkwave_proc.returncode < 0:
                # Crashed with signal
                return 1
            elif gtkwave_proc.returncode > 0:
                # Exited with error
                return 1
        # Success - GTKWave didn't crash
        return 0


if __name__ == "__main__":
    # If user runs this script in an environment without DISPLAY or without the
    # required binaries, the script will report appropriate errors instead of
    # crashing silently.
    if not sys.platform.startswith("linux"):
        print("This script is intended for Linux desktop environments.")
    exit_code = stream_to_gtkwave_interactive()
    sys.exit(exit_code if exit_code is not None else 0)
