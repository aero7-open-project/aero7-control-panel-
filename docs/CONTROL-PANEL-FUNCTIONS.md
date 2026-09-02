# Control Panel function audit

This document records what the main controls do today. It is intended for the
Aero7 wiki and for regression testing. A blue link or enabled button must have
a real destination; unavailable items are grey, carry an explanation, and do
not pretend to be working controls.

## Native and partial Aero7 pages

| Page | Control | Current function |
| --- | --- | --- |
| Getting Started | Nine setup tasks | Routes to an internal Aero7 page or a catalog-backed settings hub |
| Getting Started | Learn more | Opens the official Aero7 website |
| System | Change settings | Validates and changes the real hostname through `hostnamectl` and polkit |
| System | Experience rating | Opens Performance Information and Tools |
| Windows Update | Check/install updates | Uses the existing Aero7 `pacman` update workflow |
| Programs and Features | Uninstall/repair | Uses the existing package-management workflow and confirmations |
| Programs and Features | Turn Aero7 features on or off | Launches the separate catalog-driven `aero7-optional-features` application |
| Installed Updates | Installed update list | Reads package history; it no longer claims a single historical update can be uninstalled safely |
| Network and Sharing Center | Status, connect and diagnostics | Reads live NetworkManager data, activates saved connections with `nmcli`, and shows address, gateway and routing diagnostics; the advanced editor remains available separately |
| Firewall | Turn on/off | Runs the corresponding `ufw` command through polkit; mutation controls are disabled with a restart-required explanation when an updated kernel has left the running firewall backend stale |
| Firewall | Allow a port or service | Validates a user-provided `ufw allow` rule and applies it through polkit |
| Firewall | Notification settings | Configures UFW event logging from Off through Full and explains that UFW does not provide per-program popups |
| Firewall | Restore defaults | Confirms the destructive reset before running `ufw --force reset` |
| Action Center | Security/account/network/backup tasks | Routes to the appropriate internal page or hub |
| Power Options | Plans and battery | Changes real power-profiles-daemon profiles and reads battery, charging and remaining-time data from UPower; advanced lid/sleep policy remains a separate PowerDevil bridge |
| Personalization | Color scheme tiles | Applies an installed color scheme; the UI now calls these color schemes rather than claiming to change a complete theme |
| Personalization | Background, color, sounds, lock screen | Applies wallpaper and color schemes directly, opens the native Aero7 Sound dialog, and writes the real Plasma lock timeout/resume settings |
| User Accounts | Change password | Opens `passwd` in the configured terminal, with a KDE Users fallback |
| User Accounts | Change picture | Writes the user's standard `~/.face.icon` image |
| User Accounts | Change display name | Uses `usermod -c` through polkit after confirmation |
| User Accounts | Manage users/account type | Creates, removes and changes local accounts through authenticated Linux account tools; account removal preserves the home folder |
| Date and Time | Date, time, time zone and Internet time | Uses the existing native dialog and authenticated system tools |
| Sound | Devices/defaults/volume/mute/themes | Uses PipeWire/PulseAudio for live devices and properties, and writes Plasma's real sound-theme configuration; unsupported call ducking is labelled honestly instead of saving a fake preference |
| Performance | Rate/rerun/details | Uses the existing Aero7 benchmark workflow |
| Fonts | Font list/preview | Uses the installed font database and `kfontview` |
| Ease of Access | Accessibility tasks | Uses truthfully named configuration links for magnifier, screen reader, virtual keyboard, contrast and input |
| Optional applet placeholder | Install/Open Optional Features | Detects the real package/service state, starts the allowlisted Polkit install flow directly, or focuses the feature in the standalone manager |

## Catalog-backed settings

The group pages for Display, Network, Region and Language, Taskbar and Start
Menu, Default Programs, Input Devices, Startup and Shutdown, Window Behavior,
Security and Maintenance, Storage, Internet Options, Folder Options, AutoPlay,
and Backup and Restore are generated from the central catalog.

Each row contains:

1. the default Windows 7 name;
2. the KDE Plasma name used by the optional naming mode;
3. the original KCM module ID when one exists;
4. a native/partial/compatibility status;
5. a working action button.

Desktop-owned settings open an Aero7 property sheet and write the established
Plasma/KWin/KIO configuration. Control Panel does not open the System Settings
home page or an individual KDE KCM interface. Historical module identifiers
remain metadata for documentation and debugging.

See [KDE-SETTINGS-MAP.md](KDE-SETTINGS-MAP.md) for the complete name and module
mapping.

## Intentionally unavailable controls

The following controls are deliberately non-clickable until a correct backend
exists:

- generic **Organize** command-bar menus that do not yet have page-specific
  commands;
- help-article links whose Aero7 documentation page has not been written;
- Speech Recognition, because no supported recognition backend is available
  from the configured official repositories;
- Windows CardSpace, because it is discontinued and has no Aero7 equivalent.

These controls are grey and explain their state in a tooltip. They should not
be changed back into blue decorative links.

## Restart-required firewall state

The Firewall page probes the installed `iptables` frontend before offering a
state-changing action. If a system update has installed a new Linux kernel but
the computer is still running the previous kernel and its matching modules are
no longer available, UFW cannot safely initialize its backend. Control Panel
then shows **Restart required** and disables firewall mutation controls. Run
Windows Update if anything remains pending, restart Aero7, and open Firewall
again. This avoids exposing UFW's otherwise cryptic “Couldn't determine
iptables version” failure dialog.

## Adding or renaming a setting

Edit one entry in `src/ui/SettingsCatalog.cpp`. Keep the exact Windows 7 public
wording in `aeroName`, and keep `kdeName` and `kdeModule` as the upstream trace.
Add a native page, property sheet, applet or approved companion target; update the
mapping document, and run all four CTest tests. Navigation must never depend on
matching the visible label; stable `PageId` values are used for internal routes.
