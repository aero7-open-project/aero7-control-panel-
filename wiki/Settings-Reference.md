# Control Panel Settings Reference

This is the complete, searchable reference for every setting exposed by the
Aero7 Control Panel catalog. Use your browser's **Find** command (Ctrl+F) to
look up a setting by its Windows 7 name, its KDE Plasma name, or its historical
KDE Control Module (KCM) identifier.

The tables track the same catalog that drives Control Panel search, group
pages, routing, and the Start menu's **Settings** search results. There are
**72 settings** across 17 sections. For machine-readable current routes use
`control --list-settings-json`, including its `backend` and `status` fields.

## Status and backend meanings

- **Aero7 native** — the main workflow is implemented inside Aero7 Control
  Panel and talks directly to the relevant Linux service or configuration.
- **Aero7 partial** — Aero7 provides the main workflow but uses a separate
  Linux or KDE component for advanced functions.
- **Temporary KDE module** — the advertised action is handled by a working
  installed KDE module until its Aero7 editor has verified backend behavior.
  Aero7 checks the module before opening it and reports a missing one clearly.
- **KDE Plasma name** — the upstream Plasma name retained for documentation,
  troubleshooting, and searching.
- If a hardware-specific or optional KCM is unavailable, Control Panel reports
  that clearly instead of silently ignoring the click.

See [Control Panel Guide](Control-Panel-Guide.md) for native button behavior,
permissions, unavailable features, and troubleshooting.

## Appearance and themes (10)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Personalization** | Choose the Aero7 theme and desktop appearance. | Global Theme; Aero7 Control Panel page; original KDE module <code>kcm_lookandfeel</code> | Aero7 native |
| **Window Color** | Change Aero glass color and transparency. | Aero Glass Color; AeroShell glass-color editor | Aero7 native editor |
| **Aero7 Appearance** | Use the supported Aero7 appearance for programs and windows. | Application Style; <code>kcm_style</code> | Aero7 page |
| **Aero7 Desktop Theme** | Choose an installed Aero7 desktop theme. | Plasma Style; <code>kcm_desktoptheme</code> | Aero7 page |
| **Desktop Icons** | Choose which Aero7 icons appear on the desktop. | Icons; <code>kcm_icons</code> | Aero7 partial page |
| **Mouse Pointers** | Review the Aero7 pointer design used by the desktop. | Pointers; <code>kcm_cursortheme</code> | Aero7 partial page |
| **Desktop Background** | Choose the desktop picture and how it fills the screen. | Wallpaper; <code>kcm_wallpaper</code> | Aero7 page |
| **Fonts** | Preview, install and remove fonts. | Fonts; <code>kcm_fonts</code> | Aero7 page |
| **Font Management** | Preview, install and remove fonts. | Font Management; <code>kcm_fontinst</code> | Aero7 page |
| **Welcome Animation** | Use the Aero7 welcome animation supplied by the selected theme. | Splash Screen; <code>kcm_splashscreen</code> | Aero7 page |

## Display (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Screen Resolution** | Arrange displays and change resolution, scale and refresh rate. | Display Configuration; <code>kcm_kscreen</code> | Aero7 page |
| **Night Light** | Reduce blue light according to a schedule. | Night Light; <code>kcm_nightlight</code> | Temporary KDE module |
| **Day and Night Schedule** | Set the times used by automatic light and dark behavior. | Day-Night Cycle; <code>kcm_nighttime</code> | Temporary KDE module |

## Taskbar and Start menu (4)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Taskbar Appearance** | Lock, resize or automatically hide the Aero7 taskbar. | General Behavior; <code>kcm_workspace</code> | Aero7 page |
| **Start Menu** | Choose recent-program and Jump List behavior in the Start menu. | Plasma Search; <code>kcm_plasmasearch</code> | Aero7 page |
| **Taskbar Buttons** | Choose how taskbar buttons combine and display labels. | Shortcuts; <code>kcm_keys</code> | Aero7 page |
| **Notification Area** | Review notification-area behavior and icon overflow. | Notifications; <code>kcm_notifications</code> | Aero7 partial page |

## Window behavior (12)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Windows Snapping** | Adjust how easily windows snap to screen edges. | Window Snapping; Aero7 Control Panel page | Aero7 native |
| **Window Borders** | Choose title bars, borders and window buttons. | Window Decorations; <code>kcm_kwindecoration</code> | Temporary KDE module |
| **Window Behavior** | Configure focus, movement and title-bar actions. | Window Behavior; <code>kcm_kwinoptions</code> | Temporary KDE module |
| **Program Window Rules** | Remember or force settings for individual program windows. | Window Rules; <code>kcm_kwinrules</code> | Temporary KDE module |
| **Switch Between Windows** | Configure the Alt+Tab window switcher. | Task Switcher; <code>kcm_kwintabbox</code> | Temporary KDE module |
| **Visual Effects** | Enable and configure desktop visual effects. | Desktop Effects; <code>kcm_kwin_effects</code> | Temporary KDE module |
| **Animations** | Change animation speed and style. | Animations; <code>kcm_animations</code> | Temporary KDE module |
| **Screen Edges** | Assign actions to screen corners and edges. | Screen Edges; <code>kcm_kwinscreenedges</code> | Temporary KDE module |
| **Multiple Desktops** | Configure the number and layout of virtual desktops. | Virtual Desktops; <code>kcm_kwin_virtualdesktops</code> | Temporary KDE module |
| **Activities** | Keep separate groups of windows and desktop state. | Activities; <code>kcm_activities</code> | Temporary KDE module |
| **Window Manager Add-ons** | Manage scripts that extend window behavior. | KWin Scripts; <code>kcm_kwin_scripts</code> | Temporary KDE module |
| **Legacy App Keyboard Access** | Choose which keys legacy X11 programs may receive. | Legacy X11 App Support; <code>kcm_kwinxwayland</code> | Temporary KDE module |

## Input devices (8)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Mouse** | Configure buttons, speed and scrolling. | Mouse; <code>kcm_mouse</code> | Temporary KDE module |
| **Keyboard** | Configure keyboard hardware and layouts. | Keyboard; <code>kcm_keyboard</code> | Temporary KDE module |
| **Touchpad** | Configure touchpad taps, gestures and scrolling. | Touchpad; <code>kcm_touchpad</code> | Temporary KDE module |
| **Touchscreen** | Map and configure touchscreen input. | Touchscreen; <code>kcm_touchscreen</code> | Temporary KDE module |
| **Touchscreen Gestures** | Configure touchscreen gestures handled by the window manager. | Touchscreen Gestures; <code>kcm_kwintouchscreen</code> | Temporary KDE module |
| **Pen and Drawing Tablet** | Configure drawing tablets and pens. | Drawing Tablet; <code>kcm_tablet</code> | Temporary KDE module |
| **Game Controller** | Test and configure game controllers. | Game Controller; <code>kcm_gamecontroller</code> | Temporary KDE module |
| **On-Screen Keyboard** | Choose the virtual keyboard used on screen. | Virtual Keyboard; <code>kcm_virtualkeyboard</code> | Temporary KDE module |

## Sound (2)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Sound** | Manage playback, recording, sound schemes and communications. | Sound; Aero7 <code>sound</code> dialog; original KDE module <code>kcm_pulseaudio</code> | Aero7 native |
| **System Sounds** | Choose the notification sound theme. | System Sounds; <code>kcm_soundtheme</code> | Temporary KDE module |

## Network (4)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Network and Sharing Center** | View active connections and network information. | Connections; Aero7 Control Panel page; original KDE module <code>kcm_networkmanagement</code> | Aero7 native |
| **Change Adapter Settings** | Create and edit wired, wireless and VPN connections. | Connections; <code>kcm_networkmanagement</code> | Temporary KDE module |
| **Proxy Settings** | Configure proxy servers used by applications. | Proxy; <code>kcm_proxy</code> | Temporary KDE module |
| **Connection Preferences** | Configure generic connection behavior and timeouts. | Connection Preferences; <code>kcm_netpref</code> | Temporary KDE module |

## Power (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Power Options** | Choose a power plan and energy profile. | Power Management; Aero7 Control Panel page; original KDE module <code>kcm_powerdevilprofilesconfig</code> | Aero7 native |
| **Advanced Power Settings** | Configure sleep, screen energy, lid and power-button behavior. | Power Management; <code>kcm_powerdevilprofilesconfig</code> | Temporary KDE module |
| **Battery and Energy** | Configure energy settings on mobile devices. | Energy; <code>kcm_mobile_power</code> | Temporary KDE module |

## User accounts (2)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **User Accounts** | View and manage local user accounts. | Users; Aero7 Control Panel page; original KDE module <code>kcm_users</code> | Aero7 native |
| **Online Accounts** | Connect supported online services to the desktop. | Online Accounts; <code>kcm_kaccounts</code> | Temporary KDE module |

## Region and language (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Date and Time** | Set the clock, time zone, additional clocks and Internet time. | Date & Time; Aero7 <code>datetime</code> dialog; original KDE module <code>kcm_clock</code> | Aero7 partial |
| **Region and Language** | Set language, number, currency and time formats. | Region & Language; <code>kcm_regionandlang</code> | Temporary KDE module |
| **Spelling** | Choose spell-check dictionaries and options. | Spell Check; <code>kcmspellchecking</code> | Temporary KDE module |

## Default programs and files (5)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Get Programs** | Find and install software for Aero7. | Software Management; Programs Center | Aero7 native external app |
| **Default Programs** | Choose the default browser, mail, terminal and other programs. | Default Applications; <code>kcm_componentchooser</code> | Temporary KDE module |
| **File Type Associations** | Choose which program opens each file type. | File Associations; <code>kcm_filetypes</code> | Temporary KDE module |
| **Personal Folder Locations** | Choose the locations of Documents, Downloads and other folders. | Locations; <code>kcm_desktoppaths</code> | Temporary KDE module |
| **Removable Device Actions** | Choose actions offered when media and devices are connected. | Device Actions; <code>kcm_solid_actions</code> | Temporary KDE module |

## Search and history (4)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Folder Options** | Choose how folder windows display, open and search files. | Folder Options; Aero7 Control Panel page | Aero7 native |
| **File Search and Indexing** | Choose file-index and content-search behavior. | File Search; <code>kcm_baloofile</code> | Aero7 page |
| **Recent Items** | Manage file activity history and exclusions. | Recent Files; <code>kcm_recentFiles</code> | Temporary KDE module |
| **Search Keywords** | Configure short keywords for web searches. | Web Search Keywords; <code>kcm_webshortcuts</code> | Temporary KDE module |

## Ease of access (2)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Ease of Access Center** | Make the computer easier to see, hear and operate. | Accessibility; Aero7 Control Panel page; original KDE module <code>kcm_access</code> | Aero7 partial |
| **Advanced Accessibility** | Configure keyboard, screen-reader and visual accessibility. | Accessibility; <code>kcm_access</code> | Temporary KDE module |

## Startup and shutdown (4)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Startup Programs** | Choose programs that start when you sign in. | Autostart; <code>kcm_autostart</code> | Temporary KDE module |
| **Sign-in and Sign-out** | Choose session restore, login and logout behavior. | Desktop Session; <code>kcm_smserver</code> | Temporary KDE module |
| **Lock Screen** | Configure automatic screen locking and lock appearance. | Screen Locking; <code>kcm_screenlocker</code> | Temporary KDE module |
| **Background Services** | Choose desktop services that run in the background. | Background Services; <code>kcm_kded</code> | Temporary KDE module |

## Security and maintenance (3)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Windows Firewall** | View firewall status and network protection. | Firewall; Aero7 Control Panel page | Aero7 partial |
| **Windows Update** | Check for and install system updates. | Software Update; Aero7 Control Panel page | Aero7 native |
| **Diagnostic Data** | Choose whether anonymous desktop feedback is sent. | User Feedback; <code>kcm_feedback</code> | Temporary KDE module |

## Storage and devices (1)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **Automatic Media Mounting** | Choose which disks and volumes mount automatically. | Device Automounter; <code>kcm_device_automounter</code> | Temporary KDE module |

## Advanced system settings (2)

| Setting | What it does | KDE Plasma name and implementation | Status |
| --- | --- | --- | --- |
| **System Information** | View the operating system, processor, memory and computer name. | Quick Settings; Aero7 Control Panel page; original KDE module <code>kcm_landingpage</code> | Aero7 native |
| **Desktop Renderer** | Choose the graphics renderer used by the desktop shell. | Plasma Renderer; <code>kcm_qtquicksettings</code> | Temporary KDE module |

## When a setting does not open

1. Install all pending updates and restart Aero7.
2. Search for the setting again and note its status or permission message.
3. Hardware-specific pages such as Touchpad, Touchscreen, Drawing Tablet, Game
   Controller, and mobile power settings can be unavailable when the matching
   hardware or Linux service is absent.
4. Report the Windows 7 name, KDE Plasma name, and KCM metadata identifier from
   this page when filing an issue.

The runtime source of truth is
[SettingsCatalog.cpp](https://github.com/memegeko/aero7-control-panel-/blob/main/src/ui/SettingsCatalog.cpp).
