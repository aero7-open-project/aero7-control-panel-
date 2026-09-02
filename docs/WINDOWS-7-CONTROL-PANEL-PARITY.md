# Windows 7 Control Panel parity audit

This audit was checked against the project's Windows 7 Professional reference
VM on 31 August 2026. **All Control Panel Items** contains the same 45 public
names, in the same alphabetical five-column order, as Windows 7. Aero7 uses
those Windows 7 names on a fresh account.

The names describe familiar entry points, not Windows components. Each entry
uses an Aero7, Linux, Plasma, NetworkManager, PipeWire, UPower, pacman, UFW or
standalone Aero7 backend. Aero7 does not pretend that a missing Windows service
exists.

## Naming modes

- **Windows 7 names** is the default and preserves the exact 45-item public
  inventory.
- Open **View by > Use KDE Plasma names...** to opt in to the corresponding KDE
  Plasma/Linux names. The choice is saved per user.
- Search always matches both names. In either mode, the alternate name remains
  in the result metadata and tooltips.
- The naming choice does not change the backend or open KDE System Settings.

## The 45 Windows 7 applets

| Windows 7 name | KDE Plasma/Linux name | Aero7 destination | Current parity |
| --- | --- | --- | --- |
| Action Center | Security and Maintenance | Native Action Center status and task page | Working; Linux security and maintenance sources replace Windows services |
| Administrative Tools | System Administration | Aero7 Computer Management (`aero7-compmgmt`) | Working when the companion package is installed |
| AutoPlay | Removable Device Actions | Native removable-device settings editor | Working for Linux device actions; per-media Windows handlers do not apply |
| Backup and Restore | Backups | Optional-feature prompt or Déjà Dup | Optional; installs the supported backup backend on demand and retains archives/configuration on removal |
| Color Management | Colors | Optional-feature prompt or native Personalization/color controls | Optional; installs `colord` for color-managed workflows while retaining user ICC profiles |
| Credential Manager | Credentials | User Accounts | Partial; Aero7 deliberately does not require KDE Wallet and has no credential-vault UI |
| Date and Time | Date & Time | Native Date and Time dialog | Working through authenticated Linux time and time-zone services |
| Default Programs | Default Applications | Native Default Programs page | Working for XDG defaults and file associations |
| Desktop Gadgets | Aero7 Gadgets | Native Aero7 gadget gallery | Working through `aero7-gadget-host --gallery` |
| Device Manager | Device Manager | Standalone `aero7-device-manager` | Working when the companion package is installed |
| Devices and Printers | Printers and Devices | Native devices, Bluetooth and CUPS page | Working; hardware-dependent actions appear only when available |
| Display | Display Configuration | Native Windows 7-style Display page | Working with the live KScreen/Wayland backend |
| Ease of Access Center | Accessibility | Native Ease of Access page and optional advanced services | Basic controls remain available; Orca, Speech Dispatcher and eSpeak NG install on demand |
| Folder Options | File Manager Settings | Native Folder Options page | Working for Aero7 File Explorer/Dolphin, hidden files, trash and Baloo search |
| Fonts | Font Management | Native Fonts page | Working for installed-font browsing and the available font viewer |
| Getting Started | Welcome Center | Native Aero7 Getting Started page | Working |
| HomeGroup | File Sharing | Optional File and Printer Sharing prompt or Network Sharing page | SMB/Samba equivalent installs on demand; Aero7 never claims to implement the retired HomeGroup protocol |
| Indexing Options | File Search | Native Folder Options search tab | Working through Baloo configuration |
| Internet Options | Proxy and Web Settings | Native network/proxy settings hub | Partial; browser history, home page and add-ons stay in each browser |
| Keyboard | Keyboard | Native input-device editor | Working for layouts and basic keyboard configuration |
| Location and Other Sensors | Location Services | Input-device hub | Partial; no system-wide GeoClue permission editor is installed |
| Mouse | Mouse | Native input-device editor | Working for basic pointer, scroll and double-click settings |
| Network and Sharing Center | Connections | Native Network and Sharing Center | Working through NetworkManager and live route/address data |
| Notification Area Icons | Notifications | Native Taskbar and Start Menu page | Working for Aero7 taskbar/notification-area behavior |
| Parental Controls | Parental Controls | Optional-feature prompt or native managed-account page | Optional; installs the official Arch `malcontent` backend and reloads the real page after success |
| Performance Information and Tools | System Performance | Native benchmark and system-performance page | Working |
| Personalization | Global Theme | Native Personalization page | Working for wallpaper, color, sounds, lock settings and installed Aero7 themes |
| Phone and Modem | Modem and Mobile Broadband | Hardware-conditional optional feature | Optional `modemmanager` backend; reports Hardware not present when no compatible modem is detected |
| Power Options | Power Management | Native Power Options page | Working through power-profiles-daemon and UPower; advanced policies use Plasma config files |
| Programs and Features | Software Management | Native Programs and Features page | Working through pacman with authenticated removal |
| Recovery | System Recovery | Hardware-conditional optional feature | Optional Snapper and Btrfs Assistant backend on Btrfs systems; disabled honestly on unsupported root filesystems |
| Region and Language | Region & Language | Native region and language editor | Working for Plasma locale configuration |
| RemoteApp and Desktop Connections | Remote Desktop | Optional-feature prompt or FreeRDP client | Optional client-only FreeRDP support; enabling it does not expose the computer as a server |
| Sound | Sound | Native Sound dialog | Working through PipeWire/PulseAudio |
| Speech Recognition | Speech Recognition | Ease of Access Center | Not implemented; no supported desktop speech-recognition service is installed |
| Sync Center | File Synchronization | Optional-feature prompt or Syncthing interface | Optional Syncthing-backed folder synchronization; user data/configuration is retained on removal |
| System | System Information | Native System page | Working for hardware/OS information, hostname and edition presentation |
| Taskbar and Start Menu | Desktop Behavior | Native Taskbar and Start Menu properties | Working for Aero7 shell settings |
| Troubleshooting | Diagnostics | Security and Maintenance hub | Partial; native network, update and firewall diagnostics exist, but not every Windows wizard |
| User Accounts | Users | Native User Accounts page | Working through Linux account tools and polkit |
| Windows Anytime Upgrade | Aero7 Edition and Updates | Getting Started and Software Update | Equivalent Aero7 edition/package update path; Windows license upgrades do not apply |
| Windows CardSpace | Online Identity | Non-installable legacy explanation | Unavailable; CardSpace is discontinued and no fake Aero7 feature is offered |
| Windows Defender | Malware Protection | Action Center and optional-feature prompt | Optional ClamAV-backed Aero7 Defender; Action Center offers installation and reports real service state |
| Windows Firewall | Firewall | Native Firewall page | Working through UFW, including state, logging, rules and reset controls |
| Windows Update | Software Update | Native Windows Update-style page | Working through pacman and the Aero7 package repository |

## KDE Plasma settings that Windows 7 did not expose

These settings remain necessary for a modern Plasma/Wayland desktop. They do
not enlarge or rename the Windows 7 45-item inventory. Instead, they live in
the closest Control Panel hub and remain searchable by both their Windows-style
and KDE names.

| Aero7 hub | KDE/Wayland-only settings placed there |
| --- | --- |
| Display | Night Light; Day-Night Cycle |
| Taskbar and Start Menu | Plasma search providers; desktop behavior; Plasma notifications |
| Window Behavior | Window decorations; focus policy; window rules; task switcher; effects; animation speed; screen edges; virtual desktops; Activities; KWin scripts; XWayland keyboard access |
| Input Devices | Touchpad; touchscreen mapping; touchscreen gestures; drawing tablet; game controller; virtual keyboard |
| Sound | Plasma sound theme |
| Network | NetworkManager connection editor equivalents; KIO proxy and connection timeout settings |
| Power Options | PowerDevil-style advanced display, sleep, lid and mobile power settings |
| User Accounts | Online accounts integration |
| Region and Language | Spell-check dictionaries and behavior |
| Default Programs and Files | XDG file associations; XDG personal folders; removable-device actions |
| Folder Options | Baloo indexing; recent-file history; web-search keywords |
| Startup and Shutdown | Autostart; session restore; screen locking; background desktop services |
| Security and Maintenance | Anonymous diagnostic/feedback preference |
| Storage and Devices | Automatic media mounting |
| Advanced System Settings | Plasma/Qt Quick renderer choice |

## Remaining implementation priorities

GeoClue location permissions, a richer ICC profile editor, advanced modem
properties and a more Windows-like remote-desktop connection editor remain
future native UI work. Speech Recognition stays unavailable until Aero7 can
ship a maintained official-repository recognition backend. Optional-feature
prompts and check boxes must continue reflecting real package/service state and
must never claim that an absent protection or service is active.
