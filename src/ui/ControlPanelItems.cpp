#include "ControlPanelItems.h"
#include "Commands.h"

namespace {

LinkTarget page(PageId id)
{
    LinkTarget target;
    target.kind = LinkTarget::Page;
    target.page = id;
    return target;
}

LinkTarget applet(const char *id)
{
    LinkTarget target;
    target.kind = LinkTarget::Applet;
    target.applet = QString::fromLatin1(id);
    return target;
}

LinkTarget command(const QStringList &value)
{
    LinkTarget target;
    target.kind = LinkTarget::Command;
    target.command = value;
    return target;
}

ControlPanelItem item(const char *windowsName, const char *kdeName,
                      const char *icon, PageId id)
{
    return {QString::fromUtf8(windowsName), QString::fromUtf8(kdeName),
            QString::fromLatin1(icon), page(id), {}};
}

ControlPanelItem featureItem(const char *windowsName, const char *kdeName,
                             const char *icon, PageId id, const char *feature)
{
    return {QString::fromUtf8(windowsName), QString::fromUtf8(kdeName),
            QString::fromLatin1(icon), page(id), QString::fromLatin1(feature)};
}

ControlPanelItem dialogItem(const char *windowsName, const char *kdeName,
                            const char *icon, const char *id)
{
    return {QString::fromUtf8(windowsName), QString::fromUtf8(kdeName),
            QString::fromLatin1(icon), applet(id), {}};
}

ControlPanelItem commandItem(const char *windowsName, const char *kdeName,
                             const char *icon, const QStringList &value)
{
    return {QString::fromUtf8(windowsName), QString::fromUtf8(kdeName),
            QString::fromLatin1(icon), command(value), {}};
}

} // namespace

const QList<ControlPanelItem> &controlPanelItems()
{
    // Keep this in Windows 7's alphabetic order. The five-column view fills
    // down each column, matching the reference VM instead of filling by rows.
    static const QList<ControlPanelItem> entries = {
        item("Action Center", "Security and Maintenance", "dialog-information", PageId::ActionCenter),
        commandItem("Administrative Tools", "System Administration", "applications-system", kComputerManagementCmd),
        item("AutoPlay", "Removable Device Actions", "media-optical-audio", PageId::AutoPlay),
        featureItem("Backup and Restore", "Backups", "document-revert", PageId::BackupRestore, "backup-restore"),
        featureItem("Color Management", "Colors", "preferences-desktop-color", PageId::Personalization, "color-management"),
        item("Credential Manager", "Credentials", "dialog-password", PageId::UserAccounts),
        dialogItem("Date and Time", "Date & Time", "preferences-system-time", "datetime"),
        item("Default Programs", "Default Applications", "preferences-desktop-default-applications", PageId::DefaultPrograms),
        commandItem("Desktop Gadgets", "Aero7 Gadgets", "applications-utilities", kWidgetExplorerCmd),
        commandItem("Device Manager", "Device Manager", "computer", kDeviceManagerCmd),
        item("Devices and Printers", "Printers and Devices", "preferences-system-bluetooth", PageId::DevicesPrinters),
        item("Display", "Display Configuration", "preferences-desktop-display", PageId::DisplaySettings),
        item("Ease of Access Center", "Accessibility", "preferences-desktop-accessibility", PageId::EaseOfAccess),
        item("Folder Options", "File Manager Settings", "folder-open", PageId::FolderOptions),
        item("Fonts", "Font Management", "preferences-desktop-font", PageId::Fonts),
        item("Getting Started", "Welcome Center", "help-about", PageId::GettingStarted),
        featureItem("HomeGroup", "File Sharing", "network-workgroup", PageId::NetworkSettings, "file-printer-sharing"),
        item("Indexing Options", "File Search", "system-search", PageId::FolderOptions),
        item("Internet Options", "Proxy and Web Settings", "internet-web-browser", PageId::InternetOptions),
        item("Keyboard", "Keyboard", "input-keyboard", PageId::InputDevices),
        item("Location and Other Sensors", "Location Services", "mark-location", PageId::InputDevices),
        item("Mouse", "Mouse", "input-mouse", PageId::InputDevices),
        item("Network and Sharing Center", "Connections", "preferences-system-network", PageId::NetworkSharing),
        item("Notification Area Icons", "Notifications", "preferences-desktop-notification", PageId::TaskbarStartMenu),
        featureItem("Parental Controls", "Parental Controls", "preferences-system-parental-controls", PageId::ParentalControls, "parental-controls"),
        item("Performance Information and Tools", "System Performance", "utilities-system-monitor", PageId::Performance),
        item("Personalization", "Global Theme", "preferences-desktop-theme", PageId::Personalization),
        featureItem("Phone and Modem", "Modem and Mobile Broadband", "modem", PageId::NetworkSettings, "mobile-broadband"),
        item("Power Options", "Power Management", "preferences-system-power-management", PageId::PowerOptions),
        item("Programs and Features", "Software Management", "system-software-install", PageId::ProgramsFeatures),
        featureItem("Recovery", "System Recovery", "edit-undo", PageId::BackupRestore, "system-recovery"),
        item("Region and Language", "Region & Language", "preferences-desktop-locale", PageId::RegionLanguage),
        featureItem("RemoteApp and Desktop Connections", "Remote Desktop", "preferences-system-network-server", PageId::NetworkSettings, "remote-desktop"),
        dialogItem("Sound", "Sound", "preferences-desktop-sound", "sound"),
        featureItem("Speech Recognition", "Speech Recognition", "audio-input-microphone", PageId::EaseOfAccess, "speech-recognition"),
        featureItem("Sync Center", "File Synchronization", "folder-sync", PageId::NetworkSettings, "sync-center"),
        item("System", "System Information", "computer", PageId::System),
        item("Taskbar and Start Menu", "Desktop Behavior", "preferences-desktop", PageId::TaskbarStartMenu),
        item("Troubleshooting", "Diagnostics", "tools-report-bug", PageId::SecurityMaintenance),
        item("User Accounts", "Users", "preferences-system-users", PageId::UserAccounts),
        item("Windows Anytime Upgrade", "Aero7 Edition and Updates", "system-upgrade", PageId::GettingStarted),
        featureItem("Windows CardSpace", "Online Identity", "security-high", PageId::UserAccounts, "legacy-cardspace"),
        featureItem("Windows Defender", "Malware Protection", "security-high", PageId::SecurityMaintenance, "aero7-defender"),
        item("Windows Firewall", "Firewall", "preferences-security-firewall", PageId::Firewall),
        item("Windows Update", "Software Update", "system-software-update", PageId::LinuxUpdate),
    };
    return entries;
}
