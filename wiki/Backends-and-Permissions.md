# Backends and Permissions

Aero7 Control Panel is a frontend to real Linux and KDE settings. It does not
maintain a second fake settings database. This page explains which component
owns each kind of change and when administrator approval is expected.

## Backend map

| Area | Backend | What Control Panel uses it for |
| --- | --- | --- |
| Packages and updates | `pacman` | Update checks, updates, installed programs, removal, repair, and history |
| Optional features | Shared feature catalog, `pacman`, systemd and Polkit | Real availability/state checks and allowlisted install, repair and removal |
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
- installing, repairing or removing an Aero7 optional feature;
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

Specialized Aero7 capabilities are managed by the standalone **Aero7 Optional
Features** application. It consumes the same signed/static feature catalog as
Control Panel, accepts feature IDs rather than arbitrary package names, and
queries the real package and service state. Parental Controls, backups,
Btrfs-based recovery, advanced accessibility, Sync Center, Aero7 Defender,
Remote Desktop, SMB sharing, modem support and color management remain outside
the focused base package set.

Hardware-specific entries such as modem support report **Hardware not
present**, not a generic failure. Features unavailable for this Aero7 version
are disabled with an explanation. Speech Recognition is currently unavailable
because speech synthesis is not misrepresented as recognition.

The wallet integration is intentionally not installed by Aero7. NetworkManager
remains usable without making a desktop wallet part of the default system.
