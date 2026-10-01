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
| Firewall | Turn on/off | Uses Polkit with the detected backend: UFW enable/disable or firewalld service enable/disable; verifies resulting state before reporting success. Stale UFW kernels disable mutations with a restart explanation. |
| Firewall | Allow a port or service | Native UFW rule entry; firewalld opens the checked KDE Firewall module (`kcm_firewall`, from `plasma-firewall`) temporarily. The sidebar rule link opens that module for either backend. |
| Firewall | Notification settings / Rules | Native UFW event logging; firewalld opens KDE Firewall for rules. Its firewalld policy selectors and traffic-log viewer are not functional in the tested upstream version; the page explains these limitations. This is not a per-program approval-popup feature. |
| Firewall | Service log | Reads up to 100 firewalld service-journal entries without privileges or changes to firewall settings. Permission warnings, command failures and an eight-second timeout remain visible. This is service diagnostics, not packet logging. |
| Firewall | Restore defaults | Confirms a destructive UFW reset; remains unavailable for firewalld because the temporary editor does not offer an equivalent reset. |
| Firewall | Advanced settings | Opens the checked KDE Firewall module; unavailable when `plasma-firewall` is absent. |
| Action Center | Security/account/network/backup tasks | Routes to the appropriate internal page or hub |
| Power Options | Plans and battery | Changes real power-profiles-daemon profiles and reads battery, charging and remaining-time data from UPower; advanced lid/sleep policy remains a separate PowerDevil bridge |
| Personalization | Color scheme tiles | Applies an installed color scheme; the UI now calls these color schemes rather than claiming to change a complete theme |
| Personalization | Background, color, sounds, lock screen | Applies wallpaper and color schemes directly, opens the native Aero7 Sound dialog, and writes the real Plasma lock timeout/resume settings |
| User Accounts | Password, picture, display name, account type and account management | Opens the checked KDE Users module temporarily. The native overview reads the current account and avatar. Creation/removal, password and account-type changes still require VM acceptance; clearing an existing display name is a known upstream defect. |
| Folder Options | Single/double click | Writes KDE's real `SingleClick` preference using KConfig notifications |
| Folder Options | Hidden files / previews | Reads File Explorer's global view metadata, preserving xattr-backed view choices, and saves the actual `Settings/HiddenFilesShown` and `Dolphin/PreviewsShown` defaults. Reopen File Explorer; custom per-folder views are preserved. |
| Folder Options | Trash confirmation | Saves both KIO's `Confirmations/ConfirmTrash` and Aero7 File Explorer's `Aero7/ConfirmDelete` Recycle Bin preference. Unavailable when the bin is configured for immediate permanent deletion; that mode and its separate confirmation are not changed. |
| Folder Options | File indexing | Saves Baloo's actual `[Basic Settings]` group and invokes `balooctl6 enable/disable`, checking its result and reporting missing/failed helpers |
| Date and Time | Date, time, time zone and Internet time | Uses the existing native dialog and authenticated system tools |
| Sound | Devices/defaults/volume/mute/themes | Uses PipeWire/PulseAudio for live devices and properties, and writes Plasma's real sound-theme configuration; unsupported call ducking is labelled honestly instead of saving a fake preference |
| Performance | Rate/rerun/details | Uses the existing Aero7 benchmark workflow |
| Fonts | Font list/preview | Uses the installed font database and `kfontview` |
| Ease of Access | Accessibility tasks | Uses truthfully named configuration links for magnifier, screen reader, virtual keyboard, contrast and input |
| Optional applet placeholder | Install/Open Optional Features | Detects the real package/service state, starts the allowlisted Polkit install flow directly, or focuses the feature in the standalone manager |

## Catalog-backed settings

Settings destinations and the advanced group pages are registered in the
central catalog. Native pages include Display, Taskbar and Start Menu,
Default Programs, Folder Options, Power Options and Window Snapping.

Each row contains:

1. the default Windows 7 name;
2. the KDE Plasma name used by the optional naming mode;
3. the original KCM module ID when one exists;
4. a native/partial/compatibility status;
5. a working action button.

Verified native controls use Aero7 pages and the established Linux backend.
The 49 advanced or incomplete catalog entries temporarily open their checked
KDE module, as approved for this audit, rather than an Aero7 editor that merely
writes an ineffective preference. Missing modules produce an installation
explanation. Module availability and successful launch are not proof that
every upstream control works; see the QA report for acceptance and defects.

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
mapping document, and run the complete CTest suite (currently 24 tests).
Navigation must never depend on
matching the visible label; stable `PageId` values are used for internal routes.
