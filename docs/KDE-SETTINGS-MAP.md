# Aero7 Control Panel settings map

This page records the default Windows 7-style name, the KDE Plasma name,
the KDE Control Module (KCM) or Aero7 backend, and the current replacement
state for every setting exposed through the catalog-driven Control Panel.

The runtime source of truth is
[`src/ui/SettingsCatalog.cpp`](../src/ui/SettingsCatalog.cpp). The same data
drives Control Panel pages, search results, icon views, routing tests, and the
original-name labels visible beneath each setting. This document is formatted
so it can later be copied directly into the Aero7 GitHub wiki.

## Naming policy

- **Windows 7 name** is the fresh-install public label.
- **KDE Plasma name** can be enabled from **View by > Use KDE Plasma names...**
  and remains the stable trace back to KDE documentation and bug reports.
- **Aero7 native** means the setting is handled inside Control Panel.
- **Aero7 partial** means Control Panel handles part of the workflow and may
  still use a KDE or Linux service for advanced options.
- **Temporary KDE module** means Aero7 launches the installed KDE settings
  module under the Windows-style setting name. This bridge is used where the
  old Aero7 property sheet did not perform its advertised action. The catalog
  checks the module before launch and gives a clear error if it is absent.
- The state cells below describe the current route. `control
  --list-settings-json` also exposes `backend` and `status` for every catalog
  setting, including temporary bridges; optional-feature search entries use
  their separate feature-state fields.
- Aero7 installs `plasma-nm` for Network Management and uses Plasma 6's
  `kcmspellchecking` module with English and Dutch Hunspell dictionaries.
- KDE Wallet is intentionally not installed or exposed by Aero7 Control Panel.

## Appearance and themes

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Personalization | Global Theme | Plasma configuration / Aero7 Personalization | Aero7 native |
| Window Color | Aero Glass Color | AeroShell glass-color editor | Aero7 native editor |
| Aero7 Appearance | Application Style | `kcm_style` | Aero7 page |
| Aero7 Desktop Theme | Plasma Style | `kcm_desktoptheme` | Aero7 page |
| Icon Theme | Icons | `kcm_icons` | Temporary KDE module |
| Mouse Pointers | Pointers | `kcm_cursortheme` | Temporary KDE module |
| Desktop Background | Wallpaper | `kcm_wallpaper` | Aero7 page |
| Fonts | Fonts | `kcm_fonts` | Aero7 page |
| Font Management | Font Management | `kcm_fontinst` | Aero7 page |
| Welcome Animation | Splash Screen | `kcm_splashscreen` | Temporary KDE module |

## Display

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Screen Resolution | Display Configuration | `kcm_kscreen` | Aero7 page |
| Night Light | Night Light | `kcm_nightlight` | Temporary KDE module |
| Day and Night Schedule | Day-Night Cycle | `kcm_nighttime` | Temporary KDE module |

## Taskbar and Start menu

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Taskbar Appearance | General Behavior | `kcm_workspace` | Aero7 page |
| Start Menu | Plasma Search | `kcm_plasmasearch` | Aero7 page |
| Taskbar Buttons | Shortcuts | `kcm_keys` | Aero7 page |
| Notification Area | Notifications | `kcm_notifications` | Temporary KDE module |

## Window behavior

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Windows Snapping | Window Snapping | Aero7 Control Panel page | Aero7 native |
| Window Borders | Window Decorations | `kcm_kwindecoration` | Temporary KDE module |
| Window Behavior | Window Behavior | `kcm_kwinoptions` | Temporary KDE module |
| Program Window Rules | Window Rules | `kcm_kwinrules` | Temporary KDE module |
| Switch Between Windows | Task Switcher | `kcm_kwintabbox` | Temporary KDE module |
| Visual Effects | Desktop Effects | `kcm_kwin_effects` | Temporary KDE module |
| Animations | Animations | `kcm_animations` | Temporary KDE module |
| Screen Edges | Screen Edges | `kcm_kwinscreenedges` | Temporary KDE module |
| Multiple Desktops | Virtual Desktops | `kcm_kwin_virtualdesktops` | Temporary KDE module |
| Activities | Activities | `kcm_activities` | Temporary KDE module |
| Window Manager Add-ons | KWin Scripts | `kcm_kwin_scripts` | Temporary KDE module |
| Legacy App Keyboard Access | Legacy X11 App Support | `kcm_kwinxwayland` | Temporary KDE module |

## Input devices

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Mouse | Mouse | `kcm_mouse` | Temporary KDE module |
| Keyboard | Keyboard | `kcm_keyboard` | Temporary KDE module |
| Touchpad | Touchpad | `kcm_touchpad` | Temporary KDE module |
| Touchscreen | Touchscreen | `kcm_touchscreen` | Temporary KDE module |
| Touchscreen Gestures | Touchscreen Gestures | `kcm_kwintouchscreen` | Temporary KDE module |
| Pen and Drawing Tablet | Drawing Tablet | `kcm_tablet` | Temporary KDE module |
| Game Controller | Game Controller | `kcm_gamecontroller` | Temporary KDE module |
| On-Screen Keyboard | Virtual Keyboard | `kcm_virtualkeyboard` | Temporary KDE module |

## Sound

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Sound | Sound | PipeWire/PulseAudio / Aero7 Sound dialog | Aero7 native |
| System Sounds | System Sounds | `kcm_soundtheme` | Temporary KDE module |

## Network

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Network and Sharing Center | Connections | NetworkManager / Aero7 network page | Aero7 native |
| Change Adapter Settings | Connections | `kcm_networkmanagement` | Temporary KDE module |
| Proxy Settings | Proxy | `kcm_proxy` | Temporary KDE module |
| Connection Preferences | Connection Preferences | `kcm_netpref` | Temporary KDE module |

## Power

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Power Options | Power Management | power-profiles-daemon + UPower / Aero7 Power Options | Aero7 native |
| Advanced Power Settings | Power Management | `kcm_powerdevilprofilesconfig` | Temporary KDE module |
| Battery and Energy | Energy | `kcm_mobile_power` | Temporary KDE module |

## User accounts

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| User Accounts | Users | Linux account tools + polkit / Aero7 User Accounts | Aero7 native |
| Online Accounts | Online Accounts | `kcm_kaccounts` | Temporary KDE module |

## Region and language

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Date and Time | Date & Time | `kcm_clock` / Aero7 Date and Time dialog | Aero7 partial |
| Region and Language | Region & Language | `kcm_regionandlang` | Temporary KDE module |
| Spelling | Spell Check | `kcmspellchecking` | Temporary KDE module |

## Default programs and files

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Get Programs | Software Management | Programs Center | Aero7 external app |
| Default Programs | Default Applications | `kcm_componentchooser` | Temporary KDE module |
| File Type Associations | File Associations | `kcm_filetypes` | Temporary KDE module |
| Personal Folder Locations | Locations | `kcm_desktoppaths` | Temporary KDE module |
| Removable Device Actions | Device Actions | `kcm_solid_actions` | Temporary KDE module |

## Search and history

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Folder Options | Folder Options | Aero7 Control Panel page | Aero7 native |
| File Search and Indexing | File Search | `kcm_baloofile` | Temporary KDE module |
| Recent Items | Recent Files | `kcm_recentFiles` | Temporary KDE module |
| Search Keywords | Web Search Keywords | `kcm_webshortcuts` | Temporary KDE module |

## Ease of access

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Ease of Access Center | Accessibility | `kcm_access` / Aero7 Ease of Access Center | Aero7 partial |
| Advanced Accessibility | Accessibility | `kcm_access` | Temporary KDE module |

## Startup and shutdown

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Startup Programs | Autostart | `kcm_autostart` | Temporary KDE module |
| Sign-in and Sign-out | Desktop Session | `kcm_smserver` | Temporary KDE module |
| Lock Screen | Screen Locking | `kcm_screenlocker` | Temporary KDE module |
| Background Services | Background Services | `kcm_kded` | Temporary KDE module |

## Security and maintenance

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Windows Firewall | Firewall | Aero7 firewall page | Aero7 partial |
| Windows Update | Software Update | Aero7 `pacman` update page | Aero7 native |
| Diagnostic Data | User Feedback | `kcm_feedback` | Temporary KDE module |

## Storage and devices

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Automatic Media Mounting | Device Automounter | `kcm_device_automounter` | Temporary KDE module |

## Advanced system settings

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| System Information | Quick Settings | `kcm_landingpage` / Aero7 System page | Aero7 native |
| Desktop Renderer | Plasma Renderer | `kcm_qtquicksettings` | Temporary KDE module |

## Replacement priorities

Plasma and Linux remain the implementation backends for desktop-owned settings,
but their user interfaces are Aero7 Control Panel pages and property sheets.
NetworkManager, PipeWire/PulseAudio, UPower, power-profiles-daemon, Plasma
configuration files, and authenticated Linux account tools provide the real
workflows. The original module IDs remain documentation and audit metadata.

When a bridge becomes native, update its one `SettingDefinition` entry. The
public label, search result, hub row, KDE Plasma trace, and tests then remain
in sync automatically.
