# Control Panel Guide

Aero7 Control Panel groups desktop and system settings into a familiar
category view. An enabled link or button has a real destination. A feature
without a safe backend is disabled and explains why.

## Opening and navigating Control Panel

Open **Control Panel** from the Start menu. Use the breadcrumb bar to return to
a category or **Control Panel Home**. The view menu switches between category,
large-icon, and small-icon layouts. It also opens the persistent Windows 7/KDE
Plasma naming preference without changing which settings exist or their backend.

Use **Search Control Panel** to search by Windows 7 wording or a KDE Plasma
setting name. For a complete alphabetical lookup, see the
[Settings Reference](Settings-Reference).

## System and Security

### System

- Shows the Aero7 version, processor, installed memory, system type, computer
  name, and operating-system details.
- **Change settings** validates a new computer name and changes the real
  hostname through `hostnamectl` after an administrator prompt.
- **Performance Information and Tools** opens the Aero7 performance benchmark
  and detailed component scores.

### Windows Update

- Checks the configured Arch Linux and Aero7 repositories using `pacman`.
- Installs the complete dependency-consistent update transaction after an
  administrator prompt.
- Shows installed update history from `/var/log/pacman.log`.
- Restart Aero7 when a kernel or core desktop update says a restart is needed.

### Windows Firewall

- Shows UFW state, active network context, rules, and logging level.
- **Turn firewall on/off** runs the corresponding `ufw` action after an
  administrator prompt.
- **Allow a port or service** validates and adds a UFW allow rule.
- **Notification settings** maps to UFW event logging from Off through Full.
  UFW does not provide per-program desktop popups.
- **Restore defaults** warns first, then resets UFW rules.
- **Advanced settings** opens the focused firewall administration component
  when it is installed.

If Control Panel reports **Restart required**, a kernel update has left the
currently running firewall backend without matching modules. Finish Windows
Update, restart Aero7, and return to the page. Firewall mutation buttons remain
disabled until the backend probe succeeds.

### Action Center

The security, update, account, network, backup, and troubleshooting tasks link
to their corresponding Control Panel pages. Unsupported help articles remain
disabled rather than opening an unrelated page.

### Power Options

- Reads battery state, charging state, and remaining-time estimates through
  UPower.
- Changes supported power modes through `power-profiles-daemon`.
- Advanced lid, sleep, and session policies open the focused PowerDevil KDE
  module where necessary.

### Backup and Restore

Backup and Restore is an optional Aero7 feature backed by Déjà Dup. When it is
absent, Control Panel offers direct installation instead of opening an empty
page. Control Panel does not claim that a backup exists unless the backend is
installed and configured.

### Aero7 Optional Features

Open **Programs and Features > Turn Aero7 features on or off** to launch the
separate Optional Features application. Checked, unchecked, partial, disabled
and restart-required states come from real package, service, hardware and
restart-marker checks.

The [Aero7 Optional Features reference](Optional-Features) explains what every
listed feature does, the components it installs, its requirements and what is
retained when it is removed. Programs Center Beta is an optional feature and
is not forced onto fresh Aero7 installations.

Before a change, Aero7 lists the affected features, required packages,
approximate size and any sign-out or restart requirement. Installation and
removal require administrator approval. Progress covers repository checks,
download/install, configuration and final verification. Aero7 never restarts
automatically, and removing a feature keeps user configuration and data.

## Network and Internet

### Network and Sharing Center

- Reads live connection, address, gateway, and route information from
  NetworkManager.
- Connects saved wired, wireless, or VPN profiles through `nmcli`.
- Provides network diagnostics and a separate advanced connection editor.
- A missing adapter or saved profile is reported clearly; Control Panel does
  not create imaginary connections.

Proxy, connection, Bluetooth, and sharing options use their focused KDE
modules. Hardware-specific pages can be absent when the corresponding device
or optional module is not installed.

## Hardware and Sound

### Sound

- Lists playback and recording devices through PipeWire/PulseAudio.
- Changes default devices, volume, and mute state.
- Chooses and previews the installed Plasma notification sound theme.
- Unsupported communications-call ducking is labelled honestly rather than
  saving a preference that has no effect.

### Devices, input, display, and printing

Mouse, keyboard, touchpad, touchscreen, tablet, controller, display, printer,
removable-media, and automatic-mounting settings open the matching focused KDE
module. A hardware-specific page may be unavailable when that hardware is not
present.

## Programs

### Programs and Features

- Lists installed packages through `pacman`.
- Removes a selected removable package only after confirmation and an
  administrator prompt.
- Protects boot, kernel, hardware, package-manager, and desktop-foundation
  packages from casual removal.
- Repair uses the supported package transaction rather than editing files by
  hand.

### Default Programs

The Aero7 Default Programs and Folder Options pages handle their supported
controls. Advanced default-app, file-association, folder-location,
removable-device, recent-item, and web-search settings temporarily open their
working KDE modules.

## User Accounts

- **Change password** launches `passwd` in the configured terminal, with a KDE
  Users fallback.
- **Change picture** writes the standard `~/.face.icon` account image.
- **Change display name** uses `usermod -c` after confirmation and an
  administrator prompt.
- Account management can create users, remove users, and change account type
  through authenticated Linux account tools.
- Removing an account preserves its home folder.

Parental Controls is an optional Aero7 feature backed by `malcontent`. If it is
not installed, the applet shows a direct **Install Parental Controls** action
and a link that opens Optional Features with the correct item selected.

## Appearance and Personalization

- Applies installed color schemes and the selected desktop background.
- Uses Aero7 pages for the desktop theme, wallpaper, and fonts. Icon themes,
  pointers, welcome animation, title bars, effects, and window behavior
  temporarily open their working KDE modules.
- Sound configuration uses the native Aero7 Sound page.
- Lock-screen timeout and resume behavior use the installed KDE lock-screen
  module until an Aero7 editor is verified.

## Clock, Language, and Region

### Date and Time

- Date, time, time zone, and Internet-time changes use authenticated system
  tools.
- Region, language, number, currency, keyboard-layout, and spell-check options
  use their corresponding focused KDE modules.
- English and Dutch dictionaries are installed by Aero7; other dictionaries
  depend on installed language packages.

## Ease of Access

The Ease of Access page provides truthfully named links for screen reader,
magnifier, virtual keyboard, contrast, keyboard, mouse, and visual assistance.
Availability depends on the matching accessibility component being installed.

## Getting Started

The separate **Getting Started** Start-menu shortcut opens an internal Control
Panel page. Its setup tasks route to real pages or catalog-backed settings
hubs. It is not inserted above the main Control Panel home categories.

## Controls intentionally unavailable

- Generic **Organize** menus without page-specific commands.
- Help links whose Aero7 article has not been written.
- Speech Recognition, because no supported recognition backend is currently
  available from the configured official repositories.
- Windows CardSpace, because it is discontinued and has no Aero7 equivalent.

Unavailable controls are grey and include an explanation. Report an enabled
control that does nothing as a bug.

## Troubleshooting

1. Run Windows Update, install all pending updates, and restart Aero7.
2. Search for the setting again and read any missing-module message.
3. For networking, confirm NetworkManager is running and the device appears in
   Network and Sharing Center.
4. For firewall problems, open Windows Firewall and follow any restart-required
   notice before changing UFW manually.
5. For a hardware-specific page, confirm the device is detected and the
   optional KDE module is installed.
6. When filing an issue, include the Windows 7 name, KDE Plasma name,
   backend identifier, Aero7 version, and the exact error message.

Use the [Settings Reference](Settings-Reference) to find the KDE Plasma name
and module identifier needed for a report.
