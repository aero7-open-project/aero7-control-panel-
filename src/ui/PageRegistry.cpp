#include "PageRegistry.h"

#include <QHash>

namespace {

// Canonical id <-> path table. Keep this the only place these strings live;
// everything else refers to pages by their PageId.
struct Entry {
    PageId  id;
    QString path;
};

const QList<Entry> &table()
{
    static const QList<Entry> kEntries = {
        { PageId::GettingStarted,
          QStringLiteral("All Control Panel Items/Getting Started") },
        { PageId::LinuxUpdate,      QStringLiteral("System and Security/Linux Update") },
        { PageId::SelectUpdates,
          QStringLiteral("System and Security/Linux Update/Select updates to install") },
        { PageId::ProgramsFeatures, QStringLiteral("Programs/Programs and Features") },
        { PageId::InstalledUpdates,
          QStringLiteral("Programs/Programs and Features/Installed Updates") },
        { PageId::NetworkSharing,
          QStringLiteral("Network and Internet/Network and Sharing Center") },
        { PageId::Firewall,         QStringLiteral("System and Security/Linux Firewall") },
        { PageId::ActionCenter,     QStringLiteral("System and Security/Action Center") },
        { PageId::PowerOptions,     QStringLiteral("System and Security/Power Options") },
        { PageId::Performance,
          QStringLiteral("System and Security/Performance Information and Tools") },
        { PageId::Personalization,
          QStringLiteral("Appearance and Personalization/Personalization") },
        { PageId::Fonts,            QStringLiteral("Appearance and Personalization/Fonts") },
        { PageId::UserAccounts,
          QStringLiteral("User Accounts and Family Safety/User Accounts") },
        { PageId::EaseOfAccess,     QStringLiteral("Ease of Access/Ease of Access Center") },
        { PageId::DevicesPrinters,  QStringLiteral("Hardware and Sound/Devices and Printers") },
        { PageId::System,           QStringLiteral("System and Security/System") },
        { PageId::DisplaySettings,
          QStringLiteral("Appearance and Personalization/Display") },
        { PageId::NetworkSettings,
          QStringLiteral("Network and Internet/Network Settings") },
        { PageId::RegionLanguage,
          QStringLiteral("Clock, Language, and Region/Region and Language") },
        { PageId::TaskbarStartMenu,
          QStringLiteral("Appearance and Personalization/Taskbar and Start Menu") },
        { PageId::DefaultPrograms,
          QStringLiteral("Programs/Default Programs") },
        { PageId::InputDevices,
          QStringLiteral("Hardware and Sound/Input Devices") },
        { PageId::StartupShutdown,
          QStringLiteral("System and Security/Startup and Shutdown") },
        { PageId::WindowBehavior,
          QStringLiteral("Appearance and Personalization/Window Behavior") },
        { PageId::WindowSnapping,
          QStringLiteral("Appearance and Personalization/Window Behavior/Windows Snapping") },
        { PageId::SecurityMaintenance,
          QStringLiteral("System and Security/Security and Maintenance") },
        { PageId::StorageAdministration,
          QStringLiteral("System and Security/Storage and Devices") },
        { PageId::InternetOptions,
          QStringLiteral("Network and Internet/Internet Options") },
        { PageId::FolderOptions,
          QStringLiteral("Appearance and Personalization/Folder Options") },
        { PageId::AutoPlay,
          QStringLiteral("Hardware and Sound/AutoPlay") },
        { PageId::BackupRestore,
          QStringLiteral("System and Security/Backup and Restore") },
        { PageId::ParentalControls,
          QStringLiteral("User Accounts and Family Safety/Parental Controls") },
    };
    return kEntries;
}

} // namespace

namespace PageRegistry {

QString pathFor(PageId id)
{
    if (id == PageId::Home || id == PageId::None)
        return QString();
    for (const Entry &e : table())
        if (e.id == id)
            return e.path;
    return QString();
}

PageId idForPath(const QString &path)
{
    if (path.isEmpty())
        return PageId::Home;
    for (const Entry &e : table())
        if (e.path == path)
            return e.id;
    return PageId::None;
}

} // namespace PageRegistry
