# Backends and Permissions

Aero7 Control Panel is a frontend to real Linux and KDE settings. It does not
maintain a second fake settings database. This page explains which component
owns each kind of change and when administrator approval is expected.

## Backend map

| Area | Backend | What Control Panel uses it for |
| --- | --- | --- |
| Packages and updates | `pacman` | Update checks, updates, installed programs, removal, repair, and history |
| Privileged actions | `pkexec` / polkit | Narrow authenticated system changes |
| Firewall | `ufw` and `iptables` | State, rules, logging, enable/disable, reset, and backend health probing |
| Networking | NetworkManager / `nmcli` | Live status, saved connections, activation, addresses, gateways, and diagnostics |
| Audio | PipeWire/PulseAudio via `pactl` and `paplay` | Devices, defaults, volume, mute, and sound preview |
| Accounts | AccountsService, `passwd`, `usermod` and standard account tools | Account discovery, passwords, display names, account type, create, and remove |
| Power | UPower and `power-profiles-daemon` | Battery details and supported power modes |
| Computer name | `hostnamectl` | Validated hostname changes |
| Date and time | authenticated system tools | Clock, time zone, and Internet-time configuration |
| Desktop settings | Plasma/KWin/KIO configuration and D-Bus services | Aero7 property sheets for desktop-owned settings |
| User picture | `~/.face.icon` | Standard account image used by the login screen and desktop |

## When administrator approval appears

Approval is expected for changes that affect the whole system or another user,
including:

- installing updates or removing/repairing packages;
- enabling, disabling, resetting, or changing firewall rules;
- changing the computer name, clock, or system time zone;
- creating or removing users, changing account type, or changing another
  account's details.

Per-user appearance, wallpaper, sound volume, notification sounds, search,
window behavior, shortcuts, and most Plasma settings normally do not need
administrator approval.

## Native Plasma-backed editors

An **Aero7 native editor** presents a Windows-style property sheet and writes
the established Plasma/KWin/KIO configuration. The KDE System Settings shell
and individual KCM user interfaces are not opened. Historical KCM identifiers
remain metadata for upstream documentation and debugging.

KDE Plasma names and module identifiers are listed in the
[Settings Reference](Settings-Reference). They remain stable documentation and
troubleshooting terms even if Aero7 uses friendlier public wording.

## Safety rules

- Destructive actions require confirmation.
- Package removal protects the boot, kernel, hardware, package-manager, and
  desktop foundation.
- Removing an account preserves its home directory.
- Firewall buttons are disabled when the backend probe detects a stale running
  kernel state.
- An unavailable feature remains disabled rather than pretending a change was
  applied.

## Optional and hardware-dependent modules

Touchpad, touchscreen, drawing-tablet, game-controller, Bluetooth, mobile
power, printer, and similar settings depend on detected hardware and installed
components. Their absence does not mean the entire Control Panel installation
is broken.

The wallet integration is intentionally not installed by Aero7. NetworkManager
remains usable without making a desktop wallet part of the default system.
