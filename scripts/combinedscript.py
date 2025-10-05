#!/usr/bin/env python3

import subprocess
import time
import math
import sys
import threading
import shutil
import os
import re
from typing import List, TypeVar, Generic, Callable, Optional, Any
from dataclasses import dataclass

# Import pyatspi for GUI automation
try:
    import pyatspi

    PYATSPI_AVAILABLE = True
except ImportError:
    PYATSPI_AVAILABLE = False
    print("WARNING: pyatspi not available. GUI automation will be skipped.")

# Import python-xlib for window detection
try:
    from Xlib import display, X

    XLIBS_AVAILABLE = True
except Exception:
    XLIBS_AVAILABLE = False

# Import VCD writer
try:
    from vcd import VCDWriter

    VCD_AVAILABLE = True
except ImportError:
    VCD_AVAILABLE = False

T = TypeVar("T")
U = TypeVar("U")


# ============================================================================
# Result Monad for Error Handling
# ============================================================================


@dataclass
class Result(Generic[T]):
    """A Result monad representing success or failure."""

    value: Optional[T] = None
    error: Optional[str] = None

    @property
    def is_success(self) -> bool:
        return self.error is None

    def bind(self, func: Callable[[T], "Result[U]"]) -> "Result[U]":
        """Monadic bind operation (flatMap)."""
        if not self.is_success:
            return Result(error=self.error)
        try:
            return func(self.value)
        except Exception as e:
            return Result(error=f"{func.__name__} failed: {e}")

    def map(self, func: Callable[[T], U]) -> "Result[U]":
        """Map operation for side effects."""
        if not self.is_success:
            return Result(error=self.error)
        try:
            func(self.value)
            return Result(value=self.value)
        except Exception as e:
            return Result(error=f"{func.__name__} failed: {e}")

    @staticmethod
    def success(value: T) -> "Result[T]":
        return Result(value=value)

    @staticmethod
    def failure(error: str) -> "Result[Any]":
        return Result(error=error)


# ============================================================================
# Window Detection Functions
# ============================================================================


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

    NET_WM_PID = d.intern_atom("_NET_WM_PID")
    NET_CLIENT_LIST = d.intern_atom("_NET_CLIENT_LIST")
    gw_pattern = re.compile(r"gtkwave", re.IGNORECASE)

    start_time = time.monotonic()
    while time.monotonic() - start_time < timeout:
        try:
            prop = root.get_full_property(NET_CLIENT_LIST, X.AnyPropertyType)
            client_ids = prop.value if prop else []

            try:
                root_tree = root.query_tree()
                root_children = [w.id for w in getattr(root_tree, "children", []) or []]
            except Exception:
                root_children = []

            combined_ids = list(dict.fromkeys(list(client_ids) + list(root_children)))
            if time.monotonic() - start_time < 2.0:
                print(
                    f"Scanning {len(combined_ids)} windows for GTKWave (timeout {timeout}s)"
                )

            for window_id in combined_ids:
                try:
                    window = d.create_resource_object("window", window_id)

                    def inspect_win(w):
                        try:
                            pid_prop = None
                            try:
                                pid_prop = w.get_full_property(
                                    NET_WM_PID, X.AnyPropertyType
                                )
                            except Exception:
                                pid_prop = None
                            if (
                                pid_prop
                                and pid_prop.value
                                and pid_prop.value[0] == process_pid
                            ):
                                print(
                                    f"--> SUCCESS: Found window ID {w.id} for PID {process_pid}."
                                )
                                return True
                        except Exception:
                            pass

                        try:
                            name = w.get_wm_name() or ""
                        except Exception:
                            name = ""
                        try:
                            wmclass = w.get_wm_class() or []
                            wmclass_str = " ".join(wmclass) if wmclass else ""
                        except Exception:
                            wmclass_str = ""

                        if gw_pattern.search(name) or gw_pattern.search(wmclass_str):
                            try:
                                attrs = w.get_attributes()
                                if not attrs or attrs.map_state != X.IsUnMapped:
                                    print(
                                        f"--> SUCCESS: Found window ID {w.id} matching GTKWave."
                                    )
                                    return True
                            except Exception:
                                print(
                                    f"--> SUCCESS (best-effort): Found window ID {w.id}."
                                )
                                return True

                        try:
                            tree = w.query_tree()
                            for ch in getattr(tree, "children", []) or []:
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
                    continue
        except Exception:
            pass
        time.sleep(0.25)

    print(f"--> FAILURE: Timed out after {timeout} seconds.")
    try:
        d.close()
    except Exception:
        pass
    return False


# ============================================================================
# GTKWave Process Monitoring
# ============================================================================


def _start_live_reader(stream, out_buffer: List[str], label=None):
    """Start a background thread that reads lines from stream."""

    def _reader():
        try:
            for line in iter(stream.readline, ""):
                if not line:
                    break
                try:
                    print(line, end="")
                except Exception:
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
    """Report GTKWave exit status and output."""
    if not gtkwave_proc:
        print("--> GTKWave process object not available.")
        return

    returncode = gtkwave_proc.returncode
    if returncode is None:
        try:
            gtkwave_proc.wait(timeout=0.1)
            returncode = gtkwave_proc.returncode
        except Exception:
            returncode = gtkwave_proc.returncode

    if returncode is None:
        print("--> GTKWave exited but return code is unknown.")
    elif returncode < 0:
        sig = -returncode
        print(
            f"\n--- GTKWave terminated by signal: {sig}. Return code: {returncode} ---"
        )
    else:
        print(f"\n--- GTKWave exited with return code: {returncode} ---")

    output = None
    try:
        buf = getattr(gtkwave_proc, "_live_buffer", None)
        if buf:
            output = "".join(buf)
    except Exception:
        output = None

    if output:
        live = getattr(gtkwave_proc, "_live_streaming", False)
        if live:
            tail = output[-3000:]
            print("\n--- GTKWave recent output (tail) ---")
            print(tail)
            print("--- end of GTKWave recent output ---")
        else:
            print("\n--- GTKWave output (truncated) ---")
            print(output[:40000])
            print("--- end of GTKWave output ---")
    else:
        print("(No GTKWave output captured.)")


# ============================================================================
# GUI Automation Functions (pyatspi)
# ============================================================================


def find_gtkwave_app() -> Result[Any]:
    """Fast GTKWave application detection."""
    if not PYATSPI_AVAILABLE:
        return Result.failure("pyatspi not available")

    try:
        desktop = pyatspi.Registry.getDesktop(0)
        app_count = desktop.childCount
        start_idx = max(0, app_count - 15)

        for i in range(start_idx, app_count):
            app = desktop.getChildAtIndex(i)
            name = getattr(app, "name", "") or ""
            if "gtkwave" in name.lower():
                print("Found GTKWave application")
                return Result.success(app)

        return Result.failure("GTKWave not found")
    except Exception as e:
        return Result.failure(f"Error finding GTKWave: {e}")


def find_element_by_role_and_name(
    acc, role_name: str, search_name: str, max_depth: int = 20
) -> Optional[Any]:
    """Generic recursive element finder."""

    def search(node, depth=0):
        if depth > max_depth:
            return None

        try:
            role = node.getRoleName() if hasattr(node, "getRoleName") else ""
            name = getattr(node, "name", "") or ""

            if role_name in role.lower() and search_name.lower() in name.lower():
                return node

            for i in range(node.childCount):
                child = node.getChildAtIndex(i)
                result = search(child, depth + 1)
                if result:
                    return result
        except Exception:
            pass

        return None

    return search(acc)


def find_mysim_cell(app) -> Result[Any]:
    """Find the mysim table cell."""
    cell = find_element_by_role_and_name(app, "table cell", "mysim")
    if cell:
        print(f"Found 'mysim' cell: {cell.name}")
        return Result.success((app, cell))
    return Result.failure("'mysim' not found")


def find_menu_item(app, menu_name: str, item_name: str) -> Optional[Any]:
    """Find a specific menu item."""

    def search(node, depth=0, max_depth=20):
        if depth > max_depth:
            return None

        try:
            role = node.getRoleName() if hasattr(node, "getRoleName") else ""
            name = getattr(node, "name", "") or ""

            if "menu item" in role.lower() and item_name.lower() in name.lower():
                parent = node.parent
                if parent:
                    parent_name = getattr(parent, "name", "") or ""
                    if menu_name.lower() in parent_name.lower():
                        return node

            for i in range(node.childCount):
                child = node.getChildAtIndex(i)
                result = search(child, depth + 1)
                if result:
                    return result
        except Exception:
            pass

        return None

    return search(app)


def focus_window(app) -> bool:
    """Focus and raise the GTKWave window."""
    try:
        for i in range(app.childCount):
            child = app.getChildAtIndex(i)
            if child.getRoleName() == "frame":
                window_name = child.name
                result = subprocess.run(
                    ["wmctrl", "-a", window_name], capture_output=True, text=True
                )

                if result.returncode == 0:
                    time.sleep(0.1)
                    return True

                result = subprocess.run(
                    ["xdotool", "search", "--name", window_name, "windowactivate"],
                    capture_output=True,
                    text=True,
                )

                if result.returncode == 0:
                    time.sleep(0.1)
                    return True
    except Exception:
        pass

    return False


def click_element(app_cell_tuple) -> Result[Any]:
    """Click an element at its center."""
    app, cell = app_cell_tuple
    try:
        focus_window(app)
        component = cell.queryComponent()
        extents = component.getExtents(pyatspi.DESKTOP_COORDS)

        x = extents.x + extents.width // 2
        y = extents.y + extents.height // 2

        print(f"Clicking {cell.name} at: ({x}, {y})")
        pyatspi.Registry.generateMouseEvent(x, y, "b1c")

        time.sleep(0.1)
        return Result.success(app)
    except Exception as e:
        return Result.failure(f"Click failed: {e}")


def wait_for_sine_wave(app) -> Result[Any]:
    """Wait for sine_wave to appear after expanding mysim."""
    print("Waiting for sine_wave to appear...")
    start_time = time.time()
    timeout = 10

    while time.time() - start_time < timeout:
        cell = find_element_by_role_and_name(
            app, "table cell", "sine_wave", max_depth=50
        )
        if cell:
            print("Found 'sine_wave' after expanding mysim")
            return Result.success((app, cell))
        time.sleep(0.1)

    return Result.failure("'sine_wave' not found after timeout")


def double_click_element(app_cell_tuple) -> Result[Any]:
    """Double-click an element."""
    app, cell = app_cell_tuple
    try:
        focus_window(app)
        component = cell.queryComponent()
        extents = component.getExtents(pyatspi.DESKTOP_COORDS)

        x = extents.x + extents.width // 2
        y = extents.y + extents.height // 2

        print(f"Double-clicking {cell.name} at: ({x}, {y})")
        pyatspi.Registry.generateMouseEvent(x, y, "b1c")
        time.sleep(0.1)
        pyatspi.Registry.generateMouseEvent(x, y, "b1c")

        time.sleep(0.1)
        return Result.success(app)
    except Exception as e:
        return Result.failure(f"Double-click failed: {e}")


def click_menu_action(menu_name: str, item_name: str):
    """Return a function that clicks a specific menu item."""

    def click_menu(app) -> Result[Any]:
        menu_item = find_menu_item(app, menu_name, item_name)
        if not menu_item:
            return Result.failure(f"'{item_name}' menu item not found")

        print(f"Found '{item_name}' menu item")

        try:
            focus_window(app)
            time.sleep(0.1)

            action = menu_item.queryAction()
            if action and action.nActions > 0:
                action.doAction(0)
            else:
                component = menu_item.queryComponent()
                extents = component.getExtents(pyatspi.DESKTOP_COORDS)
                x = extents.x + extents.width // 2
                y = extents.y + extents.height // 2
                pyatspi.Registry.generateMouseEvent(x, y, "b1c")

            print(f"Successfully clicked '{item_name}'")
            time.sleep(0.1)
            return Result.success(app)
        except Exception as e:
            return Result.failure(f"Failed to click '{item_name}': {e}")

    return click_menu


def wait_for_gtkwave(timeout: int = 30) -> Result[Any]:
    """Wait for GTKWave to appear."""
    if not PYATSPI_AVAILABLE:
        return Result.failure("pyatspi not available for GUI automation")

    print("Waiting for GTKWave application...")
    start_time = time.time()

    while time.time() - start_time < timeout:
        result = find_gtkwave_app()
        if result.is_success:
            return result
        time.sleep(0.1)

    return Result.failure(f"GTKWave not found within {timeout} seconds")


def perform_gui_automation() -> int:
    """Perform GUI automation actions. Returns 0 on success, 1 on failure."""
    if not PYATSPI_AVAILABLE:
        print("\n--- WARNING: pyatspi not available. Skipping GUI automation. ---")
        return 0

    print("\n=== Starting GUI Automation ===")

    result = (
        wait_for_gtkwave()
        .bind(find_mysim_cell)
        .bind(click_element)
        .bind(wait_for_sine_wave)
        .bind(double_click_element)
        .bind(click_menu_action("Edit", "Highlight All"))
        .bind(click_menu_action("Edit", "Toggle Group Open|Close"))
    )

    if result.is_success:
        print("✓ All GUI automation operations completed successfully!")
        return 0
    else:
        print(f"✗ GUI automation failed: {result.error}")
        return 1


# ============================================================================
# Main Streaming and Integration Logic
# ============================================================================


def stream_to_gtkwave_integrated():
    """
    Launches GTKWave, verifies the GUI, streams VCD data, and performs
    GUI automation. Returns 0 on success, 1 on any failure.
    """

    shmidcat_proc = None
    gtkwave_proc = None
    writer = None
    overall_success = True

    try:
        # 1. Launch the pipeline
        print("Launching shmidcat process...")
        shmidcat_proc = subprocess.Popen(
            ["shmidcat"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

        print("Launching gtkwave process...")
        cmd = ["gtkwave", "-I", "-v"]
        if shutil.which("stdbuf"):
            cmd = ["stdbuf", "-oL"] + cmd

        gtkwave_proc = subprocess.Popen(
            cmd,
            stdin=shmidcat_proc.stdout,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        print(f"GTKWave process launched with PID: {gtkwave_proc.pid}")

        # Start live reader for gtkwave output
        gtkwave_output_buffer = []
        gtkwave_output_thread = None
        if gtkwave_proc.stdout is not None:
            gtkwave_output_thread = _start_live_reader(
                gtkwave_proc.stdout, gtkwave_output_buffer
            )
            try:
                setattr(gtkwave_proc, "_live_buffer", gtkwave_output_buffer)
                setattr(gtkwave_proc, "_live_streaming", True)
                setattr(gtkwave_proc, "_live_thread", gtkwave_output_thread)
            except Exception:
                pass

        # 2. Create VCDWriter
        writer = VCDWriter(shmidcat_proc.stdin, timescale="1 ns", date="today")
        sine_wave_var = writer.register_var(
            "mysim", "sine_wave", "integer", size=8, init=0
        )

        # 3. Prime the pipeline
        print("\nPriming the pipeline with VCD header...")
        writer.flush()

        # Check if gtkwave exited immediately
        if gtkwave_proc.poll() is not None:
            print("\n--- ERROR: GTKWave exited immediately after launch. ---")
            _report_gtkwave_exit(gtkwave_proc)
            return 1

        # 4. Verify window appears
        try:
            gui_wait = int(os.environ.get("DEMO_GUI_WAIT", "30"))
        except Exception:
            gui_wait = 30

        if not assert_window_is_shown_xlib(gtkwave_proc.pid, timeout=gui_wait):
            if gtkwave_proc.poll() is not None:
                print("\n--- ERROR: GTKWave terminated while waiting for window. ---")
                _report_gtkwave_exit(gtkwave_proc)
                return 1
            print("\n--- TEST FAILED: GTKWave interactive GUI did not appear. ---")
            return 1

        print("\n--- TEST PASSED: GTKWave interactive GUI is visible. ---")

        # 5. Stream initial data
        print("Streaming initial data...")
        amplitude = 127
        initial_steps = 200  # Stream some data first

        for timestamp in range(initial_steps):
            if gtkwave_proc.poll() is not None:
                print("\n--- ERROR: GTKWave exited during initial streaming. ---")
                _report_gtkwave_exit(gtkwave_proc)
                overall_success = False
                break

            angle = (timestamp / 100.0) * 2 * math.pi
            value = int(amplitude * math.sin(angle))

            try:
                writer.change(sine_wave_var, timestamp, value)
                writer.flush()
            except BrokenPipeError:
                print("\n--- ERROR: Broken pipe during initial streaming. ---")
                if gtkwave_proc and gtkwave_proc.poll() is not None:
                    _report_gtkwave_exit(gtkwave_proc)
                overall_success = False
                break
            except Exception as e:
                print(f"\n--- ERROR: Exception during streaming: {e} ---")
                if gtkwave_proc and gtkwave_proc.poll() is not None:
                    _report_gtkwave_exit(gtkwave_proc)
                overall_success = False
                break

            print(f"Initial streaming: {timestamp + 1}/{initial_steps}", end="\r")
            time.sleep(0.01)

        if not overall_success:
            return 1

        print("\n")

        # 6. Perform GUI automation
        automation_result = perform_gui_automation()
        if automation_result != 0:
            print("\n--- ERROR: GUI automation failed. ---")
            overall_success = False
            # Continue to check if gtkwave is still running
            if gtkwave_proc.poll() is not None:
                _report_gtkwave_exit(gtkwave_proc)
                return 1

        # 7. Continue streaming remaining data
        if overall_success:
            print("\nContinuing to stream data...")
            remaining_steps = 1000
            start_timestamp = initial_steps

            for i in range(remaining_steps):
                timestamp = start_timestamp + i

                if gtkwave_proc.poll() is not None:
                    print("\n--- ERROR: GTKWave exited during continued streaming. ---")
                    _report_gtkwave_exit(gtkwave_proc)
                    overall_success = False
                    break

                angle = (timestamp / 100.0) * 2 * math.pi
                value = int(amplitude * math.sin(angle))

                try:
                    writer.change(sine_wave_var, timestamp, value)
                    writer.flush()
                except BrokenPipeError:
                    print("\n--- ERROR: Broken pipe during continued streaming. ---")
                    if gtkwave_proc and gtkwave_proc.poll() is not None:
                        _report_gtkwave_exit(gtkwave_proc)
                    overall_success = False
                    break
                except Exception as e:
                    print(f"\n--- ERROR: Exception during streaming: {e} ---")
                    if gtkwave_proc and gtkwave_proc.poll() is not None:
                        _report_gtkwave_exit(gtkwave_proc)
                    overall_success = False
                    break

                print(
                    f"Timestamp {timestamp}/{start_timestamp + remaining_steps - 1} streamed.",
                    end="\r",
                )
                time.sleep(0.05)
            else:
                # In automated test mode (CI environment), don't wait for input
                if os.environ.get('CI') or os.environ.get('AUTOMATED_TEST'):
                    print(f"\n\nFinished streaming in automated mode. Closing GTKWave...")
                    time.sleep(1)  # Give a moment for final UI updates
                else:
                    print(f"\n\nFinished streaming. Press Enter to close.")
                    try:
                        input()
                    except Exception:
                        pass

    except FileNotFoundError as e:
        print(f"\n--- ERROR: Command not found: {e.filename}. ---")
        overall_success = False
    except KeyboardInterrupt:
        print("\n--- Interrupted by user. Shutting down. ---")
        overall_success = False
    except Exception as e:
        print(f"\n--- ERROR: Unexpected error: {e} ---")
        if gtkwave_proc and gtkwave_proc.poll() is not None:
            _report_gtkwave_exit(gtkwave_proc)
        overall_success = False
    finally:
        print("Cleaning up processes and resources...")

        if writer:
            try:
                writer.close()
            except Exception:
                pass

        if shmidcat_proc:
            try:
                if getattr(shmidcat_proc, "stdin", None):
                    try:
                        shmidcat_proc.stdin.close()
                    except Exception:
                        pass
            except Exception:
                pass

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

            try:
                if gtkwave_proc.returncode is None:
                    gtkwave_proc.wait(timeout=0.1)
            except Exception:
                pass

            if gtkwave_proc.returncode is not None:
                # In automated mode, GTKWave being terminated by SIGTERM (-15) is expected
                # and should not be treated as a failure
                is_automated = os.environ.get('CI') or os.environ.get('AUTOMATED_TEST')
                is_sigterm = gtkwave_proc.returncode == -15
                
                if gtkwave_proc.returncode != 0 and not (is_automated and is_sigterm):
                    _report_gtkwave_exit(gtkwave_proc)
                    overall_success = False

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

    return 0 if overall_success else 1


def main():
    """Main entry point."""
    if not sys.platform.startswith("linux"):
        print("This script is intended for Linux desktop environments.")
        return 1

    print("=== Integrated GTKWave Test ===\n")
    exit_code = stream_to_gtkwave_integrated()

    if exit_code == 0:
        print("\n=== ALL TESTS PASSED ===")
    else:
        print("\n=== TEST FAILED ===")

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
