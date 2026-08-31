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
- **Aero7 native editor** means Control Panel writes the established
  Plasma/KWin/KIO configuration through its own Windows-style property sheet.
  It does not open KDE System Settings or an individual KCM user interface.
- Aero7 installs `plasma-nm` for Network Management and uses Plasma 6's
  `kcmspellchecking` module with English and Dutch Hunspell dictionaries.
- KDE Wallet is intentionally not installed or exposed by Aero7 Control Panel.

## Appearance and themes

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Personalization | Global Theme | Plasma configuration / Aero7 Personalization | Aero7 native |
| Window Color | Colors | `kcm_colors` | Aero7 native editor |
| Application Appearance | Application Style | `kcm_style` | Aero7 native editor |
| Desktop Style | Plasma Style | `kcm_desktoptheme` | Aero7 native editor |
| Icons | Icons | `kcm_icons` | Aero7 native editor |
| Mouse Pointers | Pointers | `kcm_cursortheme` | Aero7 native editor |
| Desktop Background | Wallpaper | `kcm_wallpaper` | Aero7 native editor |
| Fonts | Fonts | `kcm_fonts` | Aero7 native editor |
| Font Management | Font Management | `kcm_fontinst` | Aero7 native editor |
| Welcome Animation | Splash Screen | `kcm_splashscreen` | Aero7 native editor |

## Display

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Screen Resolution | Display Configuration | `kcm_kscreen` | Aero7 native editor |
| Night Light | Night Light | `kcm_nightlight` | Aero7 native editor |
| Day and Night Schedule | Day-Night Cycle | `kcm_nighttime` | Aero7 native editor |

## Taskbar and Start menu

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Desktop Behavior | General Behavior | `kcm_workspace` | Aero7 native editor |
| Start Menu Search | Plasma Search | `kcm_plasmasearch` | Aero7 native editor |
| Keyboard Shortcuts | Shortcuts | `kcm_keys` | Aero7 native editor |
| Notification Area | Notifications | `kcm_notifications` | Aero7 native editor |

## Window behavior

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Window Borders | Window Decorations | `kcm_kwindecoration` | Aero7 native editor |
| Window Behavior | Window Behavior | `kcm_kwinoptions` | Aero7 native editor |
| Program Window Rules | Window Rules | `kcm_kwinrules` | Aero7 native editor |
| Switch Between Windows | Task Switcher | `kcm_kwintabbox` | Aero7 native editor |
| Visual Effects | Desktop Effects | `kcm_kwin_effects` | Aero7 native editor |
| Animations | Animations | `kcm_animations` | Aero7 native editor |
| Screen Edges | Screen Edges | `kcm_kwinscreenedges` | Aero7 native editor |
| Multiple Desktops | Virtual Desktops | `kcm_kwin_virtualdesktops` | Aero7 native editor |
| Activities | Activities | `kcm_activities` | Aero7 native editor |
| Window Manager Add-ons | KWin Scripts | `kcm_kwin_scripts` | Aero7 native editor |
| Legacy App Keyboard Access | Legacy X11 App Support | `kcm_kwinxwayland` | Aero7 native editor |

## Input devices

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Mouse | Mouse | `kcm_mouse` | Aero7 native editor |
| Keyboard | Keyboard | `kcm_keyboard` | Aero7 native editor |
| Touchpad | Touchpad | `kcm_touchpad` | Aero7 native editor |
| Touchscreen | Touchscreen | `kcm_touchscreen` | Aero7 native editor |
| Touchscreen Gestures | Touchscreen Gestures | `kcm_kwintouchscreen` | Aero7 native editor |
| Pen and Drawing Tablet | Drawing Tablet | `kcm_tablet` | Aero7 native editor |
| Game Controller | Game Controller | `kcm_gamecontroller` | Aero7 native editor |
| On-Screen Keyboard | Virtual Keyboard | `kcm_virtualkeyboard` | Aero7 native editor |

## Sound

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Sound | Sound | PipeWire/PulseAudio / Aero7 Sound dialog | Aero7 native |
| System Sounds | System Sounds | `kcm_soundtheme` | Aero7 native editor |

## Network

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Network and Sharing Center | Connections | NetworkManager / Aero7 network page | Aero7 native |
| Change Adapter Settings | Connections | `kcm_networkmanagement` | Aero7 native editor |
| Proxy Settings | Proxy | `kcm_proxy` | Aero7 native editor |
| Connection Preferences | Connection Preferences | `kcm_netpref` | Aero7 native editor |

## Power

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Power Options | Power Management | power-profiles-daemon + UPower / Aero7 Power Options | Aero7 native |
| Advanced Power Settings | Power Management | `kcm_powerdevilprofilesconfig` | Aero7 native editor |
| Battery and Energy | Energy | `kcm_mobile_power` | Aero7 native editor |

## User accounts

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| User Accounts | Users | Linux account tools + polkit / Aero7 User Accounts | Aero7 native |
| Online Accounts | Online Accounts | `kcm_kaccounts` | Aero7 native editor |

## Region and language

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Date and Time | Date & Time | `kcm_clock` / Aero7 Date and Time dialog | Aero7 partial |
| Region and Language | Region & Language | `kcm_regionandlang` | Aero7 native editor |
| Spelling | Spell Check | `kcmspellchecking` | Aero7 native editor |

## Default programs and files

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Default Programs | Default Applications | `kcm_componentchooser` | Aero7 native editor |
| File Type Associations | File Associations | `kcm_filetypes` | Aero7 native editor |
| Personal Folder Locations | Locations | `kcm_desktoppaths` | Aero7 native editor |
| Removable Device Actions | Device Actions | `kcm_solid_actions` | Aero7 native editor |

## Search and history

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| File Search and Indexing | File Search | `kcm_baloofile` | Aero7 native editor |
| Recent Items | Recent Files | `kcm_recentFiles` | Aero7 native editor |
| Search Keywords | Web Search Keywords | `kcm_webshortcuts` | Aero7 native editor |

## Ease of access

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Ease of Access Center | Accessibility | `kcm_access` / Aero7 Ease of Access Center | Aero7 partial |
| Advanced Accessibility | Accessibility | `kcm_access` | Aero7 native editor |

## Startup and shutdown

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Startup Programs | Autostart | `kcm_autostart` | Aero7 native editor |
| Sign-in and Sign-out | Desktop Session | `kcm_smserver` | Aero7 native editor |
| Lock Screen | Screen Locking | `kcm_screenlocker` | Aero7 native editor |
| Background Services | Background Services | `kcm_kded` | Aero7 native editor |

## Security and maintenance

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Firewall | Firewall | Aero7 `ufw` page | Aero7 partial |
| Windows Update | Software Update | Aero7 `pacman` update page | Aero7 native |
| Diagnostic Data | User Feedback | `kcm_feedback` | Aero7 native editor |

## Storage and devices

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| Automatic Media Mounting | Device Automounter | `kcm_device_automounter` | Aero7 native editor |

## Advanced system settings

| Windows 7 name | KDE Plasma name | Original KDE module/backend | State |
| --- | --- | --- | --- |
| System Information | Quick Settings | `kcm_landingpage` / Aero7 System page | Aero7 native |
| Desktop Renderer | Plasma Renderer | `kcm_qtquicksettings` | Aero7 native editor |

## Replacement priorities

Plasma and Linux remain the implementation backends for desktop-owned settings,
but their user interfaces are Aero7 Control Panel pages and property sheets.
NetworkManager, PipeWire/PulseAudio, UPower, power-profiles-daemon, Plasma
configuration files, and authenticated Linux account tools provide the real
workflows. The original module IDs remain documentation and audit metadata.

When a bridge becomes native, update its one `SettingDefinition` entry. The
public label, search result, hub row, KDE Plasma trace, and tests then remain
in sync automatically.
