#!/usr/bin/env python3
"""Apply Bruce Shark firmware desktop replacement to official Flipper firmware source.

Run from this project's root: python3 apply_overlay.py /path/to/flipperzero-firmware
This transforms the firmware's own DesktopSrv GUI, not an external .fap.
"""
from pathlib import Path
import shutil
import sys

PROJECT = Path(__file__).resolve().parent

def replace_once(path: Path, needle: str, replacement: str) -> None:
    text = path.read_text()
    if needle not in text:
        if replacement in text:
            print(f"Already patched: {path}")
            return
        raise RuntimeError(f"Expected firmware pattern not found: {path} -> {needle[:90]!r}")
    count = text.count(needle)
    if count != 1:
        raise RuntimeError(f"Ambiguous firmware pattern ({count} matches): {path}")
    path.write_text(text.replace(needle, replacement, 1))
    print(f"Patched {path}")

def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("Usage: python3 apply_overlay.py /path/to/flipperzero-firmware")
    root = Path(sys.argv[1]).resolve()
    desktop = root / 'applications' / 'services' / 'desktop'
    if not (desktop / 'desktop.c').is_file():
        raise SystemExit(f"Not a Flipper Zero firmware checkout: {root}")

    # Modify the native desktop home view, replacing the stock dolphin animation
    # in the *main* stack. Leave the lock-screen animation and all services intact.
    desktop_c = desktop / 'desktop.c'
    replace_once(desktop_c,
        '    view_stack_add_view(desktop->main_view_stack, dolphin_view);\n',
        '    /* Bruce Shark firmware: replace dolphin main UI with native Bruce shell. */\n')

    event_h = desktop / 'views' / 'desktop_events.h'
    replace_once(event_h,
        '    DesktopGlobalReloadSettings,\n} DesktopEvent;',
        '    DesktopGlobalReloadSettings,\n\n    /* Native firmware home shell uses one event to launch selected native app. */\n'
        '    DesktopMainEventBruceLaunch,\n} DesktopEvent;')

    main_h = desktop / 'views' / 'desktop_view_main.h'
    if 'desktop_main_get_launch_target' not in main_h.read_text():
        replace_once(main_h,
            'void desktop_main_set_dummy_mode_state(DesktopMainView* main_view, bool dummy_mode);',
            'void desktop_main_set_dummy_mode_state(DesktopMainView* main_view, bool dummy_mode);\n'
            'const char* desktop_main_get_launch_target(DesktopMainView* main_view);')

    scene = desktop / 'scenes' / 'desktop_scene_main.c'
    if 'case DesktopMainEventBruceLaunch' not in scene.read_text():
        replace_once(scene,
            '        case DesktopMainEventOpenMenu: {\n',
        '        case DesktopMainEventBruceLaunch: {\n'
        '            const char* target = desktop_main_get_launch_target(desktop->main_view);\n'
        '            if(target && target[0]) {\n'
        '                loader_start_detached_with_gui_error(desktop->loader, target, NULL);\n'
        '            }\n'
        '            consumed = true;\n'
        '        } break;\n\n'
        '        case DesktopMainEventOpenMenu: {\n')

    source = PROJECT / 'overlay' / 'applications' / 'services' / 'desktop' / 'views' / 'desktop_view_main.c'
    target = desktop / 'views' / 'desktop_view_main.c'
    shutil.copy2(source, target)
    print(f"Replaced firmware home UI: {target}")
    print('Done. This changes the desktop service inside the firmware build.')

if __name__ == '__main__':
    main()
