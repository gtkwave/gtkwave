#!/usr/bin/env python3

import pyatspi
import time
from typing import TypeVar, Generic, Callable, Optional, Any
from dataclasses import dataclass

T = TypeVar("T")
U = TypeVar("U")


def send_tab_key(app) -> Result[Any]:
    """Send Tab key to the focused window."""
    try:
        focus_window(app)
        time.sleep(0.1)

        print("Sending Tab key...")
        pyatspi.Registry.generateKeyboardEvent(
            pyatspi.keySyms.index("Tab"), None, pyatspi.KEY_SYM
        )

        time.sleep(0.1)
        print("Tab key sent successfully")
        return Result.success(app)
    except Exception as e:
        return Result.failure(f"Failed to send Tab key: {e}")


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


# Core discovery functions
def find_gtkwave_app() -> Result[Any]:
    """Fast GTKWave application detection."""
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
    if cell := find_element_by_role_and_name(app, "table cell", "mysim"):
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


# Action functions
def focus_window(app) -> bool:
    """Focus and raise the GTKWave window."""
    try:
        import subprocess

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
    print("Waiting for GTKWave application...")
    start_time = time.time()

    while time.time() - start_time < timeout:
        result = find_gtkwave_app()
        if result.is_success:
            return result
        time.sleep(0.1)

    return Result.failure(f"GTKWave not found within {timeout} seconds")


def main():
    # Monadic pipeline: chain all operations together
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
        print("✓ All operations completed successfully!")
        return 0
    else:
        print(f"✗ Operation failed: {result.error}")
        return 1


if __name__ == "__main__":
    exit(main())
