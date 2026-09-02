# Windows 7 Control Panel parity

Aero7's **All Control Panel Items** view reproduces the Windows 7 Professional
reference VM's 45 public names and five-column order. Windows 7 names are used
on fresh accounts. Open **View by > Use KDE Plasma names...** to switch visible
labels; search continues to match both naming systems.

The public Windows names are compatibility entry points. Their real backends
are Aero7 applications and Linux services such as pacman, UFW, NetworkManager,
PipeWire, UPower, power-profiles-daemon, CUPS and Plasma configuration.

## Working native or companion-backed groups

- Action Center, Date and Time, Default Programs, Display, Folder Options,
  Fonts, Getting Started, Indexing Options, Network and Sharing Center,
  Performance Information and Tools, Personalization, Power Options, Programs
  and Features, Region and Language, Sound, System, Taskbar and Start Menu,
  User Accounts, Windows Firewall and Windows Update.
- Administrative Tools launches Aero7 Computer Management.
- Desktop Gadgets launches the native Aero7 gadget gallery.
- Device Manager launches the standalone Aero7 Device Manager.
- Devices and Printers uses CUPS, Bluetooth and detected Linux hardware.

## Optional and unavailable equivalents

Backup and Restore, HomeGroup/SMB sharing, Parental Controls, Recovery,
RemoteApp, Sync Center, Windows Defender, advanced accessibility, modem support
and color management use the shared Aero7 Optional Features system. Their
Control Panel entries stay discoverable and provide a feature-required page
when the backend is absent. Speech Recognition remains unavailable until a
supported recognition backend exists, and CardSpace remains a non-installable
legacy entry. Credential storage and location permissions are still honest
partial equivalents.

The complete 45-row backend and limitation matrix is maintained in
[`docs/WINDOWS-7-CONTROL-PANEL-PARITY.md`](https://github.com/aero7-open-project/aero7-control-panel-/blob/beta/docs/WINDOWS-7-CONTROL-PANEL-PARITY.md).

## KDE/Wayland-only settings

Modern Plasma settings that Windows 7 did not have stay out of the 45-item
inventory. They are searchable and placed in the closest Aero7 hub: Night
Light under Display; KWin effects, rules, screen edges, Activities and virtual
desktops under Window Behavior; touch/pen/game input under Input Devices;
PowerDevil policies under Power Options; spell checking under Region and
Language; autostart/session/lock services under Startup and Shutdown; and
automount/renderer settings under Storage and Advanced System Settings.
