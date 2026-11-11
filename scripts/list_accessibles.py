#!/usr/bin/env python3
"""
gtkwave/scripts/list_accessibles.py

Enumerate accessibility objects for a specific application (default: 'gtkwave').

This script prefers AT-SPI (pyatspi). If AT-SPI bindings are missing it falls
back to a best-effort X11 enumeration using python-xlib. The main difference is
that AT-SPI exposes widgets, roles, actions and text; X11 listing only exposes
top-level windows (title, WM_CLASS, PID).

Usage:
    python3 gtkwave/scripts/list_accessibles.py [--app-name NAME] [--max-depth N] [--actions] [--text]

Options:
    --app-name NAME   Filter by application/accessibile name substring (default: 'gtkwave').
    --max-depth N     Depth to recurse into accessible tree (default: 4).
    --actions         Try to show available actions on accessibles.
    --text            Try to show Text interface contents where available.

Notes:
    - Running this script requires a desktop session (DISPLAY) and appropriate
      accessibility support enabled on the session.
    - On Wayland, AT-SPI is the right way to inspect UI; Xlib fallback will not
      work there.
"""
from __future__ import annotations

import argparse
import sys
from typing import Any, Iterable, Optional

# Try to import AT-SPI (pyatspi). If missing, we will fall back to Xlib.
try:
    import pyatspi  # type: ignore
    HAS_ATSPI = True
except Exception:
    pyatspi = None  # type: ignore
    HAS_ATSPI = False

# Try to import python-xlib for a basic fallback.
try:
    from Xlib import display as xlib_display  # type: ignore
    from Xlib import X as xlib_X  # type: ignore
    HAS_XLIB = True
except Exception:
    xlib_display = None  # type: ignore
    xlib_X = None  # type: ignore
    HAS_XLIB = False


def safe_attr(obj: Any, *names: str):
    """Return the first attribute or callable result available among names, or None."""
    for name in names:
        if hasattr(obj, name):
            try:
                attr = getattr(obj, name)
                if callable(attr):
                    return attr()
                return attr
            except Exception:
                try:
                    # If calling failed, return the attribute itself
                    return getattr(obj, name)
                except Exception:
                    continue
    return None


def iter_children(acc: Any) -> Iterable[Any]:
    """Yield children of an accessible object via several common patterns."""
    # pyatspi-style: childCount + getChildAtIndex
    try:
        child_count = getattr(acc, "childCount", None)
        if isinstance(child_count, int) and child_count > 0:
            get_child = getattr(acc, "getChildAtIndex", None)
            if callable(get_child):
                for i in range(child_count):
                    try:
                        yield get_child(i)
                    except Exception:
                        continue
                return
    except Exception:
        pass

    # .children attribute (sequence-like)
    try:
        children = getattr(acc, "children", None)
        if children:
            for c in children:
                yield c
            return
    except Exception:
        pass

    # Generic fallback: keep trying getChildAtIndex until it fails
    try:
        get_child = getattr(acc, "getChildAtIndex", None)
        if callable(get_child):
            i = 0
            while True:
                try:
                    child = get_child(i)
                    if not child:
                        break
                    yield child
                    i += 1
                except Exception:
                    break
    except Exception:
        pass


def format_role(acc: Any) -> str:
    """Try to get role name from accessible."""
    for name in ("getRoleName", "roleName", "role"):
        if hasattr(acc, name):
            try:
                val = getattr(acc, name)
                return val() if callable(val) else str(val)
            except Exception:
                continue
    return "<role?>"


def format_states(acc: Any) -> str:
    """Attempt to get states representation."""
    for name in ("getState", "get_states", "states", "state"):
        if hasattr(acc, name):
            try:
                st = getattr(acc, name)
                return str(st() if callable(st) else st)
            except Exception:
                continue
    return ""


def list_actions(acc: Any) -> Optional[str]:
    """Return a compact description of available actions for an accessible, if any."""
    try:
        if hasattr(acc, "queryAction"):
            try:
                action_iface = acc.queryAction()
            except Exception:
                action_iface = None
            if action_iface:
                parts = []
                n = getattr(action_iface, "nActions", None)
                if n is None:
                    n = getattr(action_iface, "actionCount", None)
                if n is None:
                    # try a callable that reports count
                    try:
                        get_count = getattr(action_iface, "getActionCount", None)
                        if callable(get_count):
                            n = int(get_count())
                    except Exception:
                        n = None
                if isinstance(n, int):
                    for i in range(n):
                        a_name = None
                        a_desc = None
                        try:
                            if hasattr(action_iface, "getName"):
                                a_name = action_iface.getName(i)
                        except Exception:
                            a_name = None
                        try:
                            if hasattr(action_iface, "getDescription"):
                                a_desc = action_iface.getDescription(i)
                        except Exception:
                            a_desc = None
                        if a_name:
                            parts.append(f"{i}:{a_name}" + (f" ({a_desc})" if a_desc else ""))
                        elif a_desc:
                            parts.append(f"{i}:( {a_desc} )")
                        else:
                            parts.append(str(i))
                    return ", ".join(parts)
    except Exception:
        pass
    return None


def get_text(acc: Any) -> Optional[str]:
    """Try to extract text from an accessible that supports a Text interface."""
    try:
        if hasattr(acc, "queryText"):
            try:
                text_iface = acc.queryText()
            except Exception:
                text_iface = None
            if text_iface:
                for method in ("getText", "get_text", "getTextRange", "getTextAtOffset"):
                    if hasattr(text_iface, method):
                        try:
                            fn = getattr(text_iface, method)
                            if callable(fn):
                                # Prefer getText(0, -1) when available
                                try:
                                    return str(fn(0, -1))
                                except TypeError:
                                    try:
                                        return str(fn(0))
                                    except Exception:
                                        continue
                        except Exception:
                            continue
                return str(text_iface)
        if hasattr(acc, "text"):
            t = getattr(acc, "text")
            return t() if callable(t) else str(t)
    except Exception:
        pass
    return None


def print_accessible(
    acc: Any,
    indent: int = 0,
    max_depth: int = 12,
    show_actions: bool = False,
    show_text: bool = False,
    depth: int = 0,
) -> None:
    """Print info for an accessible and recursively for its children up to max_depth.
    Enhancements:
      - default max_depth increased to 12 for deeper inspection
      - prints available interfaces (when exposed)
      - uses an explicit depth counter (depth) for recursion limit checks
    """
    prefix = " " * indent
    try:
        name = safe_attr(acc, "name", "getName", "get_name") or ""
    except Exception:
        name = ""
    try:
        role = format_role(acc)
    except Exception:
        role = "<role?>"
    try:
        states = format_states(acc)
    except Exception:
        states = ""

    header = f"{prefix}- {role}"
    if name:
        header += f' : "{name}"'
    if states:
        header += f" [{states}]"
    print(header)

    # Print available AT-SPI interfaces if the object exposes them (best-effort)
    try:
        interfaces = None
        if hasattr(acc, "getInterfaces"):
            try:
                interfaces = acc.getInterfaces()
            except Exception:
                interfaces = None
        if interfaces is None:
            interfaces = safe_attr(acc, "interfaces", "get_interfaces", "getInterfaces")
        if interfaces:
            try:
                if isinstance(interfaces, (list, tuple)):
                    ints = ", ".join(str(x) for x in interfaces)
                else:
                    ints = str(interfaces)
                print(f"{prefix}  interfaces: {ints}")
            except Exception:
                pass
    except Exception:
        pass

    if show_actions:
        try:
            actions_desc = list_actions(acc)
            if actions_desc:
                print(f"{prefix}  actions: {actions_desc}")
        except Exception:
            pass

    if show_text:
        try:
            txt = get_text(acc)
            if txt:
                preview = txt.replace("\n", "\\n")
                if len(preview) > 300:
                    preview = preview[:300] + "...(truncated)"
                print(f"{prefix}  text: \"{preview}\"")
        except Exception:
            pass

    # Recurse by depth counter (more intuitive than deriving from indent)
    if depth >= max_depth:
        return
    try:
        for child in iter_children(acc):
            try:
                print_accessible(child, indent=indent + 2, max_depth=max_depth, show_actions=show_actions, show_text=show_text, depth=depth + 1)
            except Exception:
                continue
    except Exception:
        pass


def enumerate_atspi(app_name_substr: str, max_depth: int = 4, show_actions: bool = False, show_text: bool = False) -> bool:
    """
    Enumerate desktop children via AT-SPI and print accessibles matching app_name_substr.
    Returns True if enumeration was attempted (even if empty), False if AT-SPI not available.
    """
    if not HAS_ATSPI or pyatspi is None:
        return False

    try:
        desktop = pyatspi.Registry.getDesktop(0)
    except Exception as e:
        print(f"Could not access AT-SPI desktop: {e}", file=sys.stderr)
        return True  # AT-SPI present but failed to get desktop

    matched_any = False
    # Iterate top-level children (applications)
    try:
        for i in range(desktop.childCount):
            try:
                app = desktop.getChildAtIndex(i)
            except Exception:
                continue
            try:
                app_name = (app.name or "") if hasattr(app, "name") else safe_attr(app, "getName") or ""
                if app_name is None:
                    app_name = ""
            except Exception:
                app_name = ""

            if app_name_substr.lower() in app_name.lower():
                matched_any = True
                print("=" * 70)
                print(f"Application [{i}] name={app_name}")
                print_accessible(app, indent=0, max_depth=max_depth, show_actions=show_actions, show_text=show_text)
                print("")
    except Exception as e:
        print(f"Error while enumerating desktop children: {e}", file=sys.stderr)

    if not matched_any:
        print(f"No AT-SPI application accessibles found with name containing: '{app_name_substr}'")
    return True


def enumerate_xlib(app_name_substr: str) -> bool:
    """
    Basic X11 fallback: list top-level windows and print those whose WM_NAME or WM_CLASS
    contains the app_name_substr. Returns True if attempted, False if xlib not available.
    """
    if not HAS_XLIB or xlib_display is None:
        return False

    try:
        d = xlib_display.Display()
        root = d.screen().root
    except Exception as e:
        print(f"Could not open X display: {e}", file=sys.stderr)
        return True

    NET_CLIENT_LIST = d.intern_atom("_NET_CLIENT_LIST")
    NET_WM_NAME = d.intern_atom("_NET_WM_NAME")
    WM_NAME = d.intern_atom("WM_NAME")
    WM_CLASS = d.intern_atom("WM_CLASS")
    NET_WM_PID = d.intern_atom("_NET_WM_PID")

    windows = []
    try:
        prop = root.get_full_property(NET_CLIENT_LIST, xlib_X.AnyPropertyType)
        if prop and getattr(prop, "value", None):
            for wid in prop.value:
                try:
                    w = d.create_resource_object("window", wid)
                    windows.append(w)
                except Exception:
                    continue
        else:
            # fallback: use query_tree
            try:
                windows = root.query_tree().children
            except Exception:
                windows = []
    except Exception:
        # fallback to query_tree
        try:
            windows = root.query_tree().children
        except Exception:
            windows = []

    printed_any = False
    for w in windows:
        try:
            name = None
            try:
                prop = w.get_full_property(NET_WM_NAME, xlib_X.AnyPropertyType)
                if prop and getattr(prop, "value", None):
                    raw = prop.value
                    name = raw.decode("utf-8", "ignore") if isinstance(raw, (bytes, bytearray)) else str(raw)
            except Exception:
                pass
            if not name:
                try:
                    prop = w.get_full_property(WM_NAME, xlib_X.AnyPropertyType)
                    if prop and getattr(prop, "value", None):
                        raw = prop.value
                        name = raw.decode("utf-8", "ignore") if isinstance(raw, (bytes, bytearray)) else str(raw)
                except Exception:
                    pass

            cls = None
            try:
                prop = w.get_full_property(WM_CLASS, xlib_X.AnyPropertyType)
                if prop and getattr(prop, "value", None):
                    raw = prop.value
                    cls = raw.decode("utf-8", "ignore") if isinstance(raw, (bytes, bytearray)) else str(raw)
            except Exception:
                pass

            pid = None
            try:
                prop = w.get_full_property(NET_WM_PID, xlib_X.AnyPropertyType)
                if prop and getattr(prop, "value", None):
                    raw = prop.value
                    if isinstance(raw, (list, tuple)) and raw:
                        pid = int(raw[0])
                    else:
                        try:
                            pid = int(raw)
                        except Exception:
                            pid = str(raw)
            except Exception:
                pass

            name_lower = (name or "").lower()
            cls_lower = (cls or "").lower()
            if app_name_substr.lower() in name_lower or app_name_substr.lower() in cls_lower:
                printed_any = True
                print("-" * 60)
                print(f"Window id: {getattr(w, 'id', '<id?>')}")
                if name:
                    print(f"    title: {name}")
                if cls:
                    print(f"    class: {cls}")
                if pid:
                    print(f"    pid: {pid}")
                print("")
        except Exception:
            continue

    if not printed_any:
        print(f"No X11 top-level windows matched name/class containing: '{app_name_substr}'")
    try:
        d.close()
    except Exception:
        pass
    return True


def main(argv: Optional[Iterable[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="List accessibles for a specific application (default 'gtkwave').")
    parser.add_argument("--app-name", default="gtkwave", help="Application name substring to match (default: 'gtkwave').")
    parser.add_argument("--max-depth", type=int, default=12, help="Recursion depth into accessible tree (default: 12).")
    parser.add_argument("--all", action="store_true", help="List all applications (disable name filtering).")
    parser.add_argument("--actions", action="store_true", help="Show available actions when supported.")
    parser.add_argument("--text", action="store_true", help="Show text content when supported.")
    args = parser.parse_args(list(argv) if argv is not None else None)

    # If the user requested --all, use an empty substring which will match all apps.
    app_name_substr = "" if getattr(args, "all", False) else args.app_name

    # Prefer AT-SPI (recommended for accessibility)
    attempted = False
    if HAS_ATSPI:
        attempted = True
        if ok := enumerate_atspi(
            app_name_substr,
            max_depth=args.max_depth,
            show_actions=args.actions,
            show_text=args.text,
        ):
            # We attempted AT-SPI enumeration; do not proceed to Xlib unless no matches.
            return 0

    # If AT-SPI not available / not attempted then try Xlib fallback
    if not attempted and HAS_XLIB:
        enumerate_xlib(app_name_substr)
        return 0

    # If neither mechanism is available, print guidance
    if not HAS_ATSPI and not HAS_XLIB:
        print("Neither AT-SPI (pyatspi) nor python-xlib are available in this environment.", file=sys.stderr)
        print("Install 'pyatspi' for full accessibility enumeration, or 'python-xlib' for a basic X11 fallback.", file=sys.stderr)
        return 2

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
