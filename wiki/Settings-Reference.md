# Control Panel Settings Reference

This is the complete, searchable reference for every setting exposed by the
Aero7 Control Panel catalog. Use your browser's **Find** command (Ctrl+F) to
look up a setting by its Windows 7 name, its KDE Plasma name, or its historical
KDE Control Module (KCM) identifier.

The tables are generated from the same authoritative catalog that drives
Control Panel search, group pages, routing, and the Start menu's **Settings**
search results. There are **71 settings** across 17 sections.

## Status and backend meanings

- **Aero7 native** — the main workflow is implemented inside Aero7 Control
  Panel and talks directly to the relevant Linux service or configuration.
- **Aero7 partial** — Aero7 provides the main workflow but uses a separate
  Linux or KDE component for advanced functions.
- **Aero7 native editor** — Aero7 writes established Plasma/KWin/KIO
  configuration through its own property sheet. No KDE settings UI opens.
- **KDE Plasma name** — the upstream Plasma name retained for documentation,
  troubleshooting, and searching.
- Hardware-specific settings report unavailable services or devices clearly
  instead of silently ignoring the click.

See [Control Panel Guide](Control-Panel.md) for native button behavior,
permissions, unavailable features, and troubleshooting.

## Appearance and themes (10)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Personalization** | Choose the Aero7 theme and desktop appearance. | Global Theme; Aero7 Control Panel page; original KDE module <code>kcm_lookandfeel</code> | Aero7 native |
| **Window Color** | Choose window and application colors. | Colors; <code>kcm_colors</code> | Aero7 native editor |
| **Application Appearance** | Change the appearance and behavior of application controls. | Application Style; <code>kcm_style</code> | Aero7 native editor |
| **Desktop Style** | Change the desktop shell style. | Plasma Style; <code>kcm_desktoptheme</code> | Aero7 native editor |
| **Icons** | Choose the icon theme used by applications and the desktop. | Icons; <code>kcm_icons</code> | Aero7 native editor |
| **Mouse Pointers** | Choose the pointer design and size. | Pointers; <code>kcm_cursortheme</code> | Aero7 native editor |
| **Desktop Background** | Choose the desktop background image. | Wallpaper; <code>kcm_wallpaper</code> | Aero7 native editor |
| **Fonts** | Change interface fonts and font rendering. | Fonts; <code>kcm_fonts</code> | Aero7 native editor |
| **Font Management** | Install, remove and preview fonts. | Font Management; <code>kcm_fontinst</code> | Aero7 native editor |
| **Welcome Animation** | Choose the desktop-session loading animation. | Splash Screen; <code>kcm_splashscreen</code> | Aero7 native editor |

## Display (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Screen Resolution** | Arrange displays and change resolution, scale and refresh rate. | Display Configuration; <code>kcm_kscreen</code> | Aero7 native editor |
| **Night Light** | Reduce blue light according to a schedule. | Night Light; <code>kcm_nightlight</code> | Aero7 native editor |
| **Day and Night Schedule** | Set the times used by automatic light and dark behavior. | Day-Night Cycle; <code>kcm_nighttime</code> | Aero7 native editor |

## Taskbar and Start menu (4)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Desktop Behavior** | Choose click behavior, selection markers and desktop navigation. | General Behavior; <code>kcm_workspace</code> | Aero7 native editor |
| **Start Menu Search** | Choose which search providers appear in the Start menu. | Plasma Search; <code>kcm_plasmasearch</code> | Aero7 native editor |
| **Keyboard Shortcuts** | Review and change desktop keyboard shortcuts. | Shortcuts; <code>kcm_keys</code> | Aero7 native editor |
| **Notification Area** | Choose notification behavior and application alerts. | Notifications; <code>kcm_notifications</code> | Aero7 native editor |

## Window behavior (11)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Window Borders** | Choose title bars, borders and window buttons. | Window Decorations; <code>kcm_kwindecoration</code> | Aero7 native editor |
| **Window Behavior** | Configure focus, movement and title-bar actions. | Window Behavior; <code>kcm_kwinoptions</code> | Aero7 native editor |
| **Program Window Rules** | Remember or force settings for individual program windows. | Window Rules; <code>kcm_kwinrules</code> | Aero7 native editor |
| **Switch Between Windows** | Configure the Alt+Tab window switcher. | Task Switcher; <code>kcm_kwintabbox</code> | Aero7 native editor |
| **Visual Effects** | Enable and configure desktop visual effects. | Desktop Effects; <code>kcm_kwin_effects</code> | Aero7 native editor |
| **Animations** | Change animation speed and style. | Animations; <code>kcm_animations</code> | Aero7 native editor |
| **Screen Edges** | Assign actions to screen corners and edges. | Screen Edges; <code>kcm_kwinscreenedges</code> | Aero7 native editor |
| **Multiple Desktops** | Configure the number and layout of virtual desktops. | Virtual Desktops; <code>kcm_kwin_virtualdesktops</code> | Aero7 native editor |
| **Activities** | Keep separate groups of windows and desktop state. | Activities; <code>kcm_activities</code> | Aero7 native editor |
| **Window Manager Add-ons** | Manage scripts that extend window behavior. | KWin Scripts; <code>kcm_kwin_scripts</code> | Aero7 native editor |
| **Legacy App Keyboard Access** | Choose which keys legacy X11 programs may receive. | Legacy X11 App Support; <code>kcm_kwinxwayland</code> | Aero7 native editor |

## Input devices (8)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Mouse** | Configure buttons, speed and scrolling. | Mouse; <code>kcm_mouse</code> | Aero7 native editor |
| **Keyboard** | Configure keyboard hardware and layouts. | Keyboard; <code>kcm_keyboard</code> | Aero7 native editor |
| **Touchpad** | Configure touchpad taps, gestures and scrolling. | Touchpad; <code>kcm_touchpad</code> | Aero7 native editor |
| **Touchscreen** | Map and configure touchscreen input. | Touchscreen; <code>kcm_touchscreen</code> | Aero7 native editor |
| **Touchscreen Gestures** | Configure touchscreen gestures handled by the window manager. | Touchscreen Gestures; <code>kcm_kwintouchscreen</code> | Aero7 native editor |
| **Pen and Drawing Tablet** | Configure drawing tablets and pens. | Drawing Tablet; <code>kcm_tablet</code> | Aero7 native editor |
| **Game Controller** | Test and configure game controllers. | Game Controller; <code>kcm_gamecontroller</code> | Aero7 native editor |
| **On-Screen Keyboard** | Choose the virtual keyboard used on screen. | Virtual Keyboard; <code>kcm_virtualkeyboard</code> | Aero7 native editor |

## Sound (2)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Sound** | Manage playback, recording, sound schemes and communications. | Sound; Aero7 <code>sound</code> dialog; original KDE module <code>kcm_pulseaudio</code> | Aero7 native |
| **System Sounds** | Choose the notification sound theme. | System Sounds; <code>kcm_soundtheme</code> | Aero7 native editor |

## Network (4)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Network and Sharing Center** | View active connections and network information. | Connections; Aero7 Control Panel page; original KDE module <code>kcm_networkmanagement</code> | Aero7 native |
| **Change Adapter Settings** | Create and edit wired, wireless and VPN connections. | Connections; <code>kcm_networkmanagement</code> | Aero7 native editor |
| **Proxy Settings** | Configure proxy servers used by applications. | Proxy; <code>kcm_proxy</code> | Aero7 native editor |
| **Connection Preferences** | Configure generic connection behavior and timeouts. | Connection Preferences; <code>kcm_netpref</code> | Aero7 native editor |

## Power (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Power Options** | Choose a power plan and energy profile. | Power Management; Aero7 Control Panel page; original KDE module <code>kcm_powerdevilprofilesconfig</code> | Aero7 native |
| **Advanced Power Settings** | Configure sleep, screen energy, lid and power-button behavior. | Power Management; <code>kcm_powerdevilprofilesconfig</code> | Aero7 native editor |
| **Battery and Energy** | Configure energy settings on mobile devices. | Energy; <code>kcm_mobile_power</code> | Aero7 native editor |

## User accounts (2)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **User Accounts** | View and manage local user accounts. | Users; Aero7 Control Panel page; original KDE module <code>kcm_users</code> | Aero7 native |
| **Online Accounts** | Connect supported online services to the desktop. | Online Accounts; <code>kcm_kaccounts</code> | Aero7 native editor |

## Region and language (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Date and Time** | Set the clock, time zone, additional clocks and Internet time. | Date & Time; Aero7 <code>datetime</code> dialog; original KDE module <code>kcm_clock</code> | Aero7 partial |
| **Region and Language** | Set language, number, currency and time formats. | Region & Language; <code>kcm_regionandlang</code> | Aero7 native editor |
| **Spelling** | Choose spell-check dictionaries and options. | Spell Check; <code>kcmspellchecking</code> | Aero7 native editor |

## Default programs and files (4)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Default Programs** | Choose the default browser, mail, terminal and other programs. | Default Applications; <code>kcm_componentchooser</code> | Aero7 native editor |
| **File Type Associations** | Choose which program opens each file type. | File Associations; <code>kcm_filetypes</code> | Aero7 native editor |
| **Personal Folder Locations** | Choose the locations of Documents, Downloads and other folders. | Locations; <code>kcm_desktoppaths</code> | Aero7 native editor |
| **Removable Device Actions** | Choose actions offered when media and devices are connected. | Device Actions; <code>kcm_solid_actions</code> | Aero7 native editor |

## Search and history (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **File Search and Indexing** | Choose indexed locations and file-search behavior. | File Search; <code>kcm_baloofile</code> | Aero7 native editor |
| **Recent Items** | Manage file activity history and exclusions. | Recent Files; <code>kcm_recentFiles</code> | Aero7 native editor |
| **Search Keywords** | Configure short keywords for web searches. | Web Search Keywords; <code>kcm_webshortcuts</code> | Aero7 native editor |

## Ease of access (2)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Ease of Access Center** | Make the computer easier to see, hear and operate. | Accessibility; Aero7 Control Panel page; original KDE module <code>kcm_access</code> | Aero7 partial |
| **Advanced Accessibility** | Configure keyboard, screen-reader and visual accessibility. | Accessibility; <code>kcm_access</code> | Aero7 native editor |

## Startup and shutdown (4)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Startup Programs** | Choose programs that start when you sign in. | Autostart; <code>kcm_autostart</code> | Aero7 native editor |
| **Sign-in and Sign-out** | Choose session restore, login and logout behavior. | Desktop Session; <code>kcm_smserver</code> | Aero7 native editor |
| **Lock Screen** | Configure automatic screen locking and lock appearance. | Screen Locking; <code>kcm_screenlocker</code> | Aero7 native editor |
| **Background Services** | Choose desktop services that run in the background. | Background Services; <code>kcm_kded</code> | Aero7 native editor |

## Security and maintenance (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Firewall** | View firewall status and network protection. | Firewall; Aero7 Control Panel page | Aero7 partial |
| **Windows Update** | Check for and install system updates. | Software Update; Aero7 Control Panel page | Aero7 native |
| **Diagnostic Data** | Choose whether anonymous desktop feedback is sent. | User Feedback; <code>kcm_feedback</code> | Aero7 native editor |

## Storage and devices (1)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Automatic Media Mounting** | Choose which disks and volumes mount automatically. | Device Automounter; <code>kcm_device_automounter</code> | Aero7 native editor |

## Advanced system settings (2)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **System Information** | View the operating system, processor, memory and computer name. | Quick Settings; Aero7 Control Panel page; original KDE module <code>kcm_landingpage</code> | Aero7 native |
| **Desktop Renderer** | Choose the graphics renderer used by the desktop shell. | Plasma Renderer; <code>kcm_qtquicksettings</code> | Aero7 native editor |

## When a setting does not open

1. Install all pending updates and restart Aero7.
2. Search for the setting again and note any missing-module message.
3. Hardware-specific pages such as Touchpad, Touchscreen, Drawing Tablet, Game
   Controller, and mobile power settings can be unavailable when the matching
   hardware or optional KDE module is absent.
4. Report the Windows 7 name, KDE Plasma name, and KCM metadata identifier from
   this page when filing an issue.

The runtime source of truth is
[SettingsCatalog.cpp](https://github.com/memegeko/aero7-control-panel-/blob/main/src/ui/SettingsCatalog.cpp).
