#include "SettingsCatalog.h"
#include "Commands.h"

#include <QHash>

namespace {

SettingDefinition nativeEditor(const char *key, const char *aero,
                               const char *original, const char *module,
                               SettingsSection section,
                               const char *description, const char *icon)
{
    return { QString::fromLatin1(key), QString::fromUtf8(aero),
             QString::fromUtf8(original), QString::fromLatin1(module), section,
             QString::fromUtf8(description), QString::fromLatin1(icon),
             SettingsBackend::Aero7NativeEditor,
             ReplacementStatus::Native, PageId::None, {}, {} };
}

SettingDefinition page(const char *key, const char *aero, const char *original,
                       PageId target, SettingsSection section,
                       const char *description, const char *icon,
                       ReplacementStatus status = ReplacementStatus::Native,
                       const char *originalModule = "")
{
    return { QString::fromLatin1(key), QString::fromUtf8(aero),
             QString::fromUtf8(original), QString::fromLatin1(originalModule), section,
             QString::fromUtf8(description), QString::fromLatin1(icon),
             SettingsBackend::Aero7Page, status, target, {}, {} };
}

SettingDefinition applet(const char *key, const char *aero, const char *original,
                         const char *id, SettingsSection section,
                         const char *description, const char *icon,
                         ReplacementStatus status = ReplacementStatus::Native,
                         const char *originalModule = "")
{
    return { QString::fromLatin1(key), QString::fromUtf8(aero),
             QString::fromUtf8(original), QString::fromLatin1(originalModule), section,
             QString::fromUtf8(description), QString::fromLatin1(icon),
             SettingsBackend::Aero7Applet, status, PageId::None,
             QString::fromLatin1(id), {} };
}

LinkTarget toPage(PageId id)
{
    LinkTarget t; t.kind = LinkTarget::Page; t.page = id; return t;
}

LinkTarget toCommand(const QStringList &cmd)
{
    LinkTarget t; t.kind = LinkTarget::Command; t.command = cmd; return t;
}

LinkTarget toApplet(const QString &id)
{
    LinkTarget t; t.kind = LinkTarget::Applet; t.applet = id; return t;
}

} // namespace

namespace SettingsCatalog {

const QList<SettingDefinition> &all()
{
    static const QList<SettingDefinition> entries = {
        // Appearance
        page("personalization", "Personalization", "Global Theme",
             PageId::Personalization, SettingsSection::Appearance,
             "Choose the Aero7 theme and desktop appearance.", "preferences-desktop-theme",
             ReplacementStatus::Native, "kcm_lookandfeel"),
        {QStringLiteral("colors"), QStringLiteral("Window Color"),
         QStringLiteral("Aero Glass Color"), QString(), SettingsSection::Appearance,
         QStringLiteral("Change Aero glass color, intensity and transparency."),
         QStringLiteral("preferences-desktop-color"),
         SettingsBackend::ExternalCommand, ReplacementStatus::Native,
         PageId::None, QString(), aeroWindowColor()},
        page("application-style", "Aero7 Appearance", "Application Style",
             PageId::Personalization, SettingsSection::Appearance,
             "Use the supported Aero7 appearance for programs and windows.",
             "preferences-desktop-theme", ReplacementStatus::Native,
             "kcm_style"),
        page("desktop-style", "Aero7 Desktop Theme", "Plasma Style",
             PageId::Personalization, SettingsSection::Appearance,
             "Choose an installed Aero7 desktop theme.",
             "preferences-desktop-plasma-theme", ReplacementStatus::Native,
             "kcm_desktoptheme"),
        page("icons", "Desktop Icons", "Icons", PageId::Personalization,
             SettingsSection::Appearance,
             "Choose which Aero7 icons appear on the desktop.",
             "preferences-desktop-icons", ReplacementStatus::Partial,
             "kcm_icons"),
        page("pointers", "Mouse Pointers", "Pointers", PageId::Personalization,
             SettingsSection::Appearance,
             "Review the Aero7 pointer design used by the desktop.",
             "preferences-desktop-cursors", ReplacementStatus::Partial,
             "kcm_cursortheme"),
        page("wallpaper", "Desktop Background", "Wallpaper",
             PageId::Personalization, SettingsSection::Appearance,
             "Choose the desktop picture and how it fills the screen.",
             "preferences-desktop-wallpaper", ReplacementStatus::Native,
             "kcm_wallpaper"),
        page("fonts", "Fonts", "Fonts", PageId::Fonts,
             SettingsSection::Appearance,
             "Preview, install and remove fonts.",
             "preferences-desktop-font", ReplacementStatus::Native,
             "kcm_fonts"),
        page("font-management", "Font Management", "Font Management",
             PageId::Fonts, SettingsSection::Appearance,
             "Preview, install and remove fonts.",
             "preferences-desktop-font-installer", ReplacementStatus::Native,
             "kcm_fontinst"),
        page("splash", "Welcome Animation", "Splash Screen",
             PageId::Personalization, SettingsSection::Appearance,
             "Use the Aero7 welcome animation supplied by the selected theme.",
             "preferences-desktop-splash", ReplacementStatus::Native,
             "kcm_splashscreen"),

        // Display
        page("display", "Screen Resolution", "Display Configuration",
             PageId::DisplaySettings, SettingsSection::Display,
             "Arrange displays and change resolution, scale and refresh rate.",
             "preferences-desktop-display", ReplacementStatus::Native, "kcm_kscreen"),
        nativeEditor("night-light", "Night Light", "Night Light", "kcm_nightlight", SettingsSection::Display,
            "Reduce blue light according to a schedule.", "redshift-status-on"),
        nativeEditor("day-night", "Day and Night Schedule", "Day-Night Cycle", "kcm_nighttime", SettingsSection::Display,
            "Set the times used by automatic light and dark behavior.", "weather-clear-night"),

        // Taskbar, Start menu and general desktop behavior
        page("workspace", "Taskbar Appearance", "General Behavior",
             PageId::TaskbarStartMenu, SettingsSection::TaskbarStartMenu,
             "Lock, resize or automatically hide the Aero7 taskbar.",
             "preferences-desktop", ReplacementStatus::Native,
             "kcm_workspace"),
        page("search", "Start Menu", "Plasma Search",
             PageId::TaskbarStartMenu, SettingsSection::TaskbarStartMenu,
             "Choose recent-program and Jump List behavior in the Start menu.",
             "system-search", ReplacementStatus::Native,
             "kcm_plasmasearch"),
        page("shortcuts", "Taskbar Buttons", "Shortcuts",
             PageId::TaskbarStartMenu, SettingsSection::TaskbarStartMenu,
             "Choose how taskbar buttons combine and display labels.",
             "preferences-desktop-keyboard-shortcuts", ReplacementStatus::Native,
             "kcm_keys"),
        page("notifications", "Notification Area", "Notifications",
             PageId::TaskbarStartMenu, SettingsSection::TaskbarStartMenu,
             "Review notification-area behavior and icon overflow.",
             "preferences-desktop-notification", ReplacementStatus::Partial,
             "kcm_notifications"),

        // Window behavior
        page("window-snapping", "Windows Snapping", "Window Snapping",
             PageId::WindowSnapping, SettingsSection::WindowBehavior,
             "Adjust how easily windows snap to screen edges.",
             "preferences-system-windows-behavior"),
        nativeEditor("window-decoration", "Window Borders", "Window Decorations", "kcm_kwindecoration", SettingsSection::WindowBehavior,
            "Choose title bars, borders and window buttons.", "preferences-system-windows"),
        nativeEditor("window-behavior", "Window Behavior", "Window Behavior", "kcm_kwinoptions", SettingsSection::WindowBehavior,
            "Configure focus, movement and title-bar actions.", "preferences-system-windows-behavior"),
        nativeEditor("window-rules", "Program Window Rules", "Window Rules", "kcm_kwinrules", SettingsSection::WindowBehavior,
            "Remember or force settings for individual program windows.", "preferences-system-windows-actions"),
        nativeEditor("task-switcher", "Switch Between Windows", "Task Switcher", "kcm_kwintabbox", SettingsSection::WindowBehavior,
            "Configure the Alt+Tab window switcher.", "preferences-system-windows"),
        nativeEditor("effects", "Visual Effects", "Desktop Effects", "kcm_kwin_effects", SettingsSection::WindowBehavior,
            "Enable and configure desktop visual effects.", "preferences-desktop-effects"),
        nativeEditor("animations", "Animations", "Animations", "kcm_animations", SettingsSection::WindowBehavior,
            "Change animation speed and style.", "preferences-desktop-effects"),
        nativeEditor("screen-edges", "Screen Edges", "Screen Edges", "kcm_kwinscreenedges", SettingsSection::WindowBehavior,
            "Assign actions to screen corners and edges.", "preferences-desktop-screen-edges"),
        nativeEditor("virtual-desktops", "Multiple Desktops", "Virtual Desktops", "kcm_kwin_virtualdesktops", SettingsSection::WindowBehavior,
            "Configure the number and layout of virtual desktops.", "preferences-desktop-virtual"),
        nativeEditor("activities", "Activities", "Activities", "kcm_activities", SettingsSection::WindowBehavior,
            "Keep separate groups of windows and desktop state.", "preferences-activities"),
        nativeEditor("kwin-scripts", "Window Manager Add-ons", "KWin Scripts", "kcm_kwin_scripts", SettingsSection::WindowBehavior,
            "Manage scripts that extend window behavior.", "preferences-plugin"),
        nativeEditor("x11-support", "Legacy App Keyboard Access", "Legacy X11 App Support", "kcm_kwinxwayland", SettingsSection::WindowBehavior,
            "Choose which keys legacy X11 programs may receive.", "xorg"),

        // Input
        nativeEditor("mouse", "Mouse", "Mouse", "kcm_mouse", SettingsSection::InputDevices,
            "Configure buttons, speed and scrolling.", "input-mouse"),
        nativeEditor("keyboard", "Keyboard", "Keyboard", "kcm_keyboard", SettingsSection::InputDevices,
            "Configure keyboard hardware and layouts.", "input-keyboard"),
        nativeEditor("touchpad", "Touchpad", "Touchpad", "kcm_touchpad", SettingsSection::InputDevices,
            "Configure touchpad taps, gestures and scrolling.", "input-touchpad"),
        nativeEditor("touchscreen", "Touchscreen", "Touchscreen", "kcm_touchscreen", SettingsSection::InputDevices,
            "Map and configure touchscreen input.", "input-touchscreen"),
        nativeEditor("touchscreen-gestures", "Touchscreen Gestures", "Touchscreen Gestures", "kcm_kwintouchscreen", SettingsSection::InputDevices,
            "Configure touchscreen gestures handled by the window manager.", "input-touchscreen"),
        nativeEditor("tablet", "Pen and Drawing Tablet", "Drawing Tablet", "kcm_tablet", SettingsSection::InputDevices,
            "Configure drawing tablets and pens.", "input-tablet"),
        nativeEditor("game-controller", "Game Controller", "Game Controller", "kcm_gamecontroller", SettingsSection::InputDevices,
            "Test and configure game controllers.", "input-gaming"),
        nativeEditor("virtual-keyboard", "On-Screen Keyboard", "Virtual Keyboard", "kcm_virtualkeyboard", SettingsSection::InputDevices,
            "Choose the virtual keyboard used on screen.", "input-keyboard-virtual"),

        // Sound
        applet("sound", "Sound", "Sound", "sound", SettingsSection::Sound,
               "Manage playback, recording, sound schemes and communications.", "preferences-desktop-sound",
               ReplacementStatus::Native, "kcm_pulseaudio"),
        nativeEditor("sound-theme", "System Sounds", "System Sounds", "kcm_soundtheme", SettingsSection::Sound,
            "Choose the notification sound theme.", "preferences-desktop-sound"),

        // Network
        page("network-status", "Network and Sharing Center", "Connections", PageId::NetworkSharing,
             SettingsSection::Network, "View active connections and network information.", "preferences-system-network",
             ReplacementStatus::Native, "kcm_networkmanagement"),
        nativeEditor("network-connections", "Change Adapter Settings", "Connections", "kcm_networkmanagement", SettingsSection::Network,
            "Create and edit wired, wireless and VPN connections.", "network-wired"),
        nativeEditor("proxy", "Proxy Settings", "Proxy", "kcm_proxy", SettingsSection::Network,
            "Configure proxy servers used by applications.", "preferences-system-network-proxy"),
        nativeEditor("network-preferences", "Connection Preferences", "Connection Preferences", "kcm_netpref", SettingsSection::Network,
            "Configure generic connection behavior and timeouts.", "preferences-system-network"),

        // Power
        page("power", "Power Options", "Power Management", PageId::PowerOptions,
             SettingsSection::Power, "Choose a power plan and energy profile.", "preferences-system-power-management",
             ReplacementStatus::Native, "kcm_powerdevilprofilesconfig"),
        nativeEditor("power-details", "Advanced Power Settings", "Power Management", "kcm_powerdevilprofilesconfig", SettingsSection::Power,
            "Configure sleep, screen energy, lid and power-button behavior.", "preferences-system-power-management"),
        nativeEditor("mobile-power", "Battery and Energy", "Energy", "kcm_mobile_power", SettingsSection::Power,
            "Configure energy settings on mobile devices.", "battery"),

        // Accounts
        page("accounts", "User Accounts", "Users", PageId::UserAccounts,
             SettingsSection::Accounts, "View and manage local user accounts.", "system-users",
             ReplacementStatus::Native, "kcm_users"),
        nativeEditor("online-accounts", "Online Accounts", "Online Accounts", "kcm_kaccounts", SettingsSection::Accounts,
            "Connect supported online services to the desktop.", "preferences-online-accounts"),
        // Region and language
        applet("date-time", "Date and Time", "Date & Time", "datetime", SettingsSection::RegionLanguage,
               "Set the clock, time zone, additional clocks and Internet time.", "preferences-system-time",
               ReplacementStatus::Partial, "kcm_clock"),
        nativeEditor("region-language", "Region and Language", "Region & Language", "kcm_regionandlang", SettingsSection::RegionLanguage,
            "Set language, number, currency and time formats.", "preferences-desktop-locale"),
        nativeEditor("spell-check", "Spelling", "Spell Check", "kcmspellchecking", SettingsSection::RegionLanguage,
            "Choose spell-check dictionaries and options.", "tools-check-spelling"),

        // Applications and file behavior
        {QStringLiteral("get-programs"), QStringLiteral("Get Programs"),
         QStringLiteral("Software Management"), QString(),
         SettingsSection::Applications,
         QStringLiteral("Find and install software for Aero7."),
         QStringLiteral("system-software-install"),
         SettingsBackend::ExternalCommand, ReplacementStatus::Native,
         PageId::None, QString(), kProgramsCenterCmd},
        nativeEditor("default-apps", "Default Programs", "Default Applications", "kcm_componentchooser", SettingsSection::Applications,
            "Choose the default browser, mail, terminal and other programs.", "preferences-desktop-default-applications"),
        nativeEditor("file-associations", "File Type Associations", "File Associations", "kcm_filetypes", SettingsSection::Applications,
            "Choose which program opens each file type.", "preferences-desktop-filetype-association"),
        nativeEditor("locations", "Personal Folder Locations", "Locations", "kcm_desktoppaths", SettingsSection::Applications,
            "Choose the locations of Documents, Downloads and other folders.", "folder-home"),
        nativeEditor("device-actions", "Removable Device Actions", "Device Actions", "kcm_solid_actions", SettingsSection::Applications,
            "Choose actions offered when media and devices are connected.", "device-notifier"),

        // Search and history
        page("folder-options", "Folder Options", "Folder Options",
             PageId::FolderOptions, SettingsSection::SearchHistory,
             "Choose how folder windows display, open and search files.",
             "folder-open", ReplacementStatus::Native),
        page("file-search", "File Search and Indexing", "File Search",
             PageId::FolderOptions, SettingsSection::SearchHistory,
             "Choose file-index and content-search behavior.", "baloo",
             ReplacementStatus::Native, "kcm_baloofile"),
        nativeEditor("recent-files", "Recent Items", "Recent Files", "kcm_recentFiles", SettingsSection::SearchHistory,
            "Manage file activity history and exclusions.", "document-open-recent"),
        nativeEditor("web-shortcuts", "Search Keywords", "Web Search Keywords", "kcm_webshortcuts", SettingsSection::SearchHistory,
            "Configure short keywords for web searches.", "preferences-web-browser-shortcuts"),

        // Accessibility
        page("ease", "Ease of Access Center", "Accessibility", PageId::EaseOfAccess,
             SettingsSection::Accessibility, "Make the computer easier to see, hear and operate.", "preferences-desktop-accessibility",
             ReplacementStatus::Partial, "kcm_access"),
        nativeEditor("accessibility-details", "Advanced Accessibility", "Accessibility", "kcm_access", SettingsSection::Accessibility,
            "Configure keyboard, screen-reader and visual accessibility.", "preferences-desktop-accessibility"),

        // Startup and shutdown
        nativeEditor("autostart", "Startup Programs", "Autostart", "kcm_autostart", SettingsSection::StartupShutdown,
            "Choose programs that start when you sign in.", "system-run"),
        nativeEditor("session", "Sign-in and Sign-out", "Desktop Session", "kcm_smserver", SettingsSection::StartupShutdown,
            "Choose session restore, login and logout behavior.", "system-log-out"),
        nativeEditor("screen-lock", "Lock Screen", "Screen Locking", "kcm_screenlocker", SettingsSection::StartupShutdown,
            "Configure automatic screen locking and lock appearance.", "system-lock-screen"),
        nativeEditor("background-services", "Background Services", "Background Services", "kcm_kded", SettingsSection::StartupShutdown,
            "Choose desktop services that run in the background.", "preferences-system-services"),

        // Security and maintenance
        page("firewall", "Windows Firewall", "Firewall", PageId::Firewall,
             SettingsSection::SecurityMaintenance, "View firewall status and network protection.", "preferences-security-firewall",
             ReplacementStatus::Partial),
        page("updates", "Windows Update", "Software Update", PageId::LinuxUpdate,
             SettingsSection::SecurityMaintenance, "Check for and install system updates.", "system-software-update"),
        nativeEditor("feedback", "Diagnostic Data", "User Feedback", "kcm_feedback", SettingsSection::SecurityMaintenance,
            "Choose whether anonymous desktop feedback is sent.", "preferences-desktop-user-feedback"),

        // Storage and system
        nativeEditor("automount", "Automatic Media Mounting", "Device Automounter", "kcm_device_automounter", SettingsSection::StorageAdministration,
            "Choose which disks and volumes mount automatically.", "drive-removable-media"),
        page("system-overview", "System Information", "Quick Settings",
             PageId::System, SettingsSection::System,
             "View the operating system, processor, memory and computer name.",
             "computer", ReplacementStatus::Native, "kcm_landingpage"),
        nativeEditor("renderer", "Desktop Renderer", "Plasma Renderer", "kcm_qtquicksettings", SettingsSection::System,
            "Choose the graphics renderer used by the desktop shell.", "video-display"),
    };
    return entries;
}

const SettingDefinition *findByKey(const QString &key)
{
    const QString normalized = key.trimmed();
    for (const SettingDefinition &entry : all()) {
        if (entry.key.compare(normalized, Qt::CaseInsensitive) == 0)
            return &entry;
    }
    return nullptr;
}

QList<SettingDefinition> forSection(SettingsSection section)
{
    QList<SettingDefinition> out;
    for (const auto &entry : all())
        if (entry.section == section)
            out.append(entry);
    return out;
}

QString sectionTitle(SettingsSection section)
{
    switch (section) {
    case SettingsSection::Appearance:            return "Appearance and Themes";
    case SettingsSection::Display:               return "Display";
    case SettingsSection::TaskbarStartMenu:      return "Taskbar and Start Menu";
    case SettingsSection::WindowBehavior:        return "Window Behavior";
    case SettingsSection::InputDevices:          return "Input Devices";
    case SettingsSection::Sound:                 return "Sound";
    case SettingsSection::Network:               return "Network Settings";
    case SettingsSection::Power:                 return "Power Options";
    case SettingsSection::Accounts:              return "User Accounts";
    case SettingsSection::RegionLanguage:        return "Region and Language";
    case SettingsSection::Applications:          return "Default Programs and Files";
    case SettingsSection::SearchHistory:         return "Search and History";
    case SettingsSection::Accessibility:         return "Ease of Access";
    case SettingsSection::StartupShutdown:       return "Startup and Shutdown";
    case SettingsSection::SecurityMaintenance:   return "Security and Maintenance";
    case SettingsSection::StorageAdministration: return "Storage and Devices";
    case SettingsSection::System:                return "Advanced System Settings";
    }
    return "Settings";
}

QString sectionDescription(SettingsSection section)
{
    switch (section) {
    case SettingsSection::Display: return "Change screen resolution, scaling, layout and display comfort settings.";
    case SettingsSection::Network: return "View connections and configure adapters, proxies and sharing.";
    case SettingsSection::TaskbarStartMenu: return "Customize the taskbar, Start menu, search and notification area.";
    case SettingsSection::WindowBehavior: return "Control window borders, switching, effects and desktop behavior.";
    case SettingsSection::InputDevices: return "Configure mouse, keyboard, touch, pen and game input.";
    case SettingsSection::RegionLanguage: return "Set language, formats, spelling, date, time and time zone.";
    case SettingsSection::Applications: return "Choose default programs, file associations and removable-media actions.";
    case SettingsSection::StartupShutdown: return "Choose startup programs, background services, locking and sign-in behavior.";
    case SettingsSection::SecurityMaintenance: return "Manage updates, firewall protection and diagnostic data.";
    case SettingsSection::StorageAdministration: return "Configure disks, removable media and automatic mounting.";
    default: return "Review and change the settings in this section.";
    }
}

PageId pageForSection(SettingsSection section)
{
    switch (section) {
    case SettingsSection::Display:               return PageId::DisplaySettings;
    case SettingsSection::Network:               return PageId::NetworkSettings;
    case SettingsSection::RegionLanguage:        return PageId::RegionLanguage;
    case SettingsSection::TaskbarStartMenu:      return PageId::TaskbarStartMenu;
    case SettingsSection::Applications:          return PageId::DefaultPrograms;
    case SettingsSection::InputDevices:          return PageId::InputDevices;
    case SettingsSection::StartupShutdown:       return PageId::StartupShutdown;
    case SettingsSection::WindowBehavior:        return PageId::WindowBehavior;
    case SettingsSection::SecurityMaintenance:   return PageId::SecurityMaintenance;
    case SettingsSection::StorageAdministration: return PageId::StorageAdministration;
    case SettingsSection::SearchHistory:         return PageId::FolderOptions;
    case SettingsSection::Power:                 return PageId::PowerOptions;
    case SettingsSection::Accounts:              return PageId::UserAccounts;
    case SettingsSection::Accessibility:         return PageId::EaseOfAccess;
    case SettingsSection::Appearance:            return PageId::Personalization;
    case SettingsSection::Sound:                 return PageId::None;
    case SettingsSection::System:                return PageId::System;
    }
    return PageId::None;
}

SettingsSection sectionForPage(PageId page)
{
    switch (page) {
    case PageId::DisplaySettings:        return SettingsSection::Display;
    case PageId::NetworkSettings:        return SettingsSection::Network;
    case PageId::RegionLanguage:         return SettingsSection::RegionLanguage;
    case PageId::TaskbarStartMenu:       return SettingsSection::TaskbarStartMenu;
    case PageId::DefaultPrograms:        return SettingsSection::Applications;
    case PageId::InputDevices:           return SettingsSection::InputDevices;
    case PageId::StartupShutdown:        return SettingsSection::StartupShutdown;
    case PageId::WindowBehavior:         return SettingsSection::WindowBehavior;
    case PageId::SecurityMaintenance:    return SettingsSection::SecurityMaintenance;
    case PageId::StorageAdministration:  return SettingsSection::StorageAdministration;
    case PageId::InternetOptions:        return SettingsSection::Network;
    case PageId::FolderOptions:          return SettingsSection::SearchHistory;
    case PageId::AutoPlay:               return SettingsSection::Applications;
    case PageId::BackupRestore:          return SettingsSection::StorageAdministration;
    default:                             return SettingsSection::System;
    }
}

LinkTarget targetForLegacyLabel(const QString &text)
{
    static const QHash<QString, PageId> pages = {
        {"Change User Account Control settings", PageId::SecurityMaintenance},
        {"Review administrator approval", PageId::SecurityMaintenance},
        {"Troubleshoot common computer problems", PageId::SecurityMaintenance},
        {"Restore your computer to an earlier time", PageId::BackupRestore},
        {"View amount of RAM and processor speed", PageId::System},
        {"Check the Linux Experience Index", PageId::Performance},
        {"Allow remote access", PageId::SecurityMaintenance},
        {"See the name of this computer", PageId::System},
        {"Turn automatic updating on or off", PageId::LinuxUpdate},
        {"Check for updates", PageId::LinuxUpdate},
        {"Require a password when the computer wakes", PageId::StartupShutdown},
        {"Change what the power buttons do", PageId::PowerOptions},
        {"Change when the computer sleeps", PageId::PowerOptions},
        {"Change power-saving settings", PageId::PowerOptions},
        {"Back up your computer", PageId::BackupRestore},
        {"Restore files from backup", PageId::BackupRestore},
        {"Protect your computer by encrypting data on your disk", PageId::SecurityMaintenance},
        {"Manage BitLocker", PageId::SecurityMaintenance},
        {"Free up disk space", PageId::StorageAdministration},
        {"Defragment your hard drive", PageId::StorageAdministration},
        {"Create and format hard disk partitions", PageId::StorageAdministration},
        {"View event logs", PageId::SecurityMaintenance},
        {"Schedule tasks", PageId::StartupShutdown},
        {"Connect to a network", PageId::NetworkSettings},
        {"View network computers and devices", PageId::NetworkSettings},
        {"Add a wireless device to the network", PageId::NetworkSettings},
        {"Choose homegroup and sharing options", PageId::NetworkSettings},
        {"Change your homepage", PageId::InternetOptions},
        {"Manage browser add-ons", PageId::InternetOptions},
        {"Delete browsing history and cookies", PageId::InternetOptions},
        {"Change default settings for media or devices", PageId::AutoPlay},
        {"Play CDs or other media automatically", PageId::AutoPlay},
        {"How to correct monitor flicker (refresh rate)", PageId::DisplaySettings},
        {"Run programs made for previous versions of Linux", PageId::DefaultPrograms},
        {"How to install a program", PageId::ProgramsFeatures},
        {"Uninstall a gadget", PageId::Personalization},
        {"Restore desktop gadgets installed with Linux", PageId::Personalization},
        {"Customize the Start menu", PageId::TaskbarStartMenu},
        {"Customize icons on the taskbar", PageId::TaskbarStartMenu},
        {"Change the picture on the Start menu", PageId::TaskbarStartMenu},
        {"Accommodate low vision", PageId::EaseOfAccess},
        {"Use screen reader", PageId::EaseOfAccess},
        {"Turn on easy access keys", PageId::EaseOfAccess},
        {"Turn High Contrast on or off", PageId::EaseOfAccess},
        {"Specify single- or double-click to open", PageId::FolderOptions},
        {"Show hidden files and folders", PageId::FolderOptions},
        {"Add the Clock gadget to the desktop", PageId::Personalization},
        {"Replace sounds with visual cues", PageId::EaseOfAccess},
        {"Backup and Restore", PageId::BackupRestore},
        {"BitLocker Drive Encryption", PageId::SecurityMaintenance},
        {"Administrative Tools", PageId::StorageAdministration},
        {"Internet Options", PageId::InternetOptions},
        {"AutoPlay", PageId::AutoPlay},
        {"Default Programs", PageId::DefaultPrograms},
        {"Display", PageId::DisplaySettings},
        {"Taskbar and Start Menu", PageId::TaskbarStartMenu},
        {"Folder Options", PageId::FolderOptions},
        {"Region and Language", PageId::RegionLanguage},
        {"Speech Recognition", PageId::EaseOfAccess},
        {"Parental Controls", PageId::UserAccounts},
        {"Change Action Center settings", PageId::SecurityMaintenance},
        {"Allow a port or service through Linux Firewall", PageId::Firewall},
        {"Turn Linux Firewall on or off", PageId::Firewall},
        {"Restore defaults", PageId::Firewall},
        {"Advanced settings", PageId::Firewall},
        {"Troubleshoot my network", PageId::NetworkSettings},
        {"Get more font information online", PageId::Fonts},
        {"Find a character", PageId::Fonts},
        {"Change font size", PageId::DisplaySettings},
        {"Text Services and Input Language", PageId::RegionLanguage},
        {"View update history", PageId::InstalledUpdates},
        {"Change advanced sharing settings", PageId::NetworkSettings},
        {"Require a password when the computer wakes", PageId::StartupShutdown},
        {"Choose what the power buttons do", PageId::PowerOptions},
        {"Create a power plan", PageId::PowerOptions},
        {"Choose when to turn off the display", PageId::PowerOptions},
    };
    const auto it = pages.constFind(text);
    if (it != pages.constEnd())
        return toPage(it.value());

    static const QHash<QString, PageId> nativePages = {
        {"Change adapter settings", PageId::NetworkSettings},
        {"Adjust visual effects", PageId::WindowBehavior},
        {"Adjust indexing options", PageId::FolderOptions},
        {"Adjust power settings", PageId::PowerOptions},
        {"Change account picture", PageId::UserAccounts},
    };
    const auto nativeIt = nativePages.constFind(text);
    if (nativeIt != nativePages.constEnd())
        return toPage(nativeIt.value());

    if (text == "Date and Time")
        return toApplet("datetime");
    if (text == "Sound")
        return toApplet("sound");
    return {};
}

LinkTarget targetForSetting(const SettingDefinition &setting)
{
    switch (setting.backend) {
    case SettingsBackend::Aero7Page:       return toPage(setting.page);
    case SettingsBackend::Aero7Applet:     return toApplet(setting.applet);
    case SettingsBackend::Aero7NativeEditor:
        return toPage(pageForSection(setting.section));
    case SettingsBackend::ExternalCommand:
        return toCommand(setting.command);
    }
    return {};
}

QString backendLabel(const SettingDefinition &setting)
{
    switch (setting.backend) {
    case SettingsBackend::Aero7Page:       return "Aero7 Control Panel page";
    case SettingsBackend::Aero7Applet:     return "Aero7 Control Panel dialog";
    case SettingsBackend::Aero7NativeEditor: return "Aero7 native editor";
    case SettingsBackend::ExternalCommand: return setting.command.join(' ');
    }
    return {};
}

QString statusLabel(ReplacementStatus status)
{
    switch (status) {
    case ReplacementStatus::Native:              return "Aero7 native";
    case ReplacementStatus::Partial:             return "Aero7 partial";
    case ReplacementStatus::CompatibilityBridge: return "KDE compatibility backend";
    }
    return {};
}

} // namespace SettingsCatalog
