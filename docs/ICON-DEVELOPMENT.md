# Aero7 UI icon development

Control Panel owns its application identity, category, applet, toolbar, status,
dialog, action, tray, and notification icons. Add those through `resolveIcon`,
`themeIcon` (a compatibility name backed by embedded pack resources), or a Qt
resource under `assets/icons`; never add a system-theme lookup.

`QIcon::fromTheme` is allowed only for an external application's declared icon,
including Programs and Features entries and selectable browser backends. Every
exception is enforced by `tests/test_icon_policy.py`.

Launcher icon names must start with `aero7-`. Raster application artwork is
installed in its matching hicolor size directory by CMake. Bundled resources
use `/aero7/icons` and `/aero7/pack`. Test all owned icons at 16, 22, 24, 32,
48, 64, 128, and 256 px. Record the pinned pack commit, upstream filename,
license, modification status, and rename in `assets/icons/LICENSES.md`.
