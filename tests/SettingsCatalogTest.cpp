#include "PageRegistry.h"
#include "SettingsCatalog.h"

#include <QSet>

int main()
{
    const auto &settings = SettingsCatalog::all();
    if (settings.size() < 50)
        return 1;

    QSet<QString> keys;
    const QSet<QString> nativeReplacementKeys = {
        QStringLiteral("personalization"), QStringLiteral("sound"),
        QStringLiteral("network-status"), QStringLiteral("power"),
        QStringLiteral("accounts"), QStringLiteral("display"),
    };
    QSet<QString> nativeReplacementsFound;
    bool foundNetworkManagement = false;
    bool foundSpelling = false;
    for (const SettingDefinition &setting : settings) {
        if (setting.key.isEmpty() || setting.aeroName.isEmpty()
            || setting.kdeName.isEmpty() || setting.description.isEmpty())
            return 2;
        if (keys.contains(setting.key))
            return 3;
        keys.insert(setting.key);
        if (SettingsCatalog::findByKey(setting.key) != &setting)
            return 13;
        if (SettingsCatalog::findByKey(setting.key.toUpper()) != &setting)
            return 14;

        const LinkTarget target = SettingsCatalog::targetForSetting(setting);
        if (target.kind == LinkTarget::None || target.kind == LinkTarget::Disabled)
            return 4;

        if (setting.backend == SettingsBackend::Aero7NativeEditor) {
            // Keep the original module ID as audit metadata, but never attach
            // a command capable of opening its user interface.
            if (!setting.kdeModule.startsWith(QStringLiteral("kcm"))
                || !setting.command.isEmpty()
                || setting.status != ReplacementStatus::Native
                || target.kind != LinkTarget::Page)
                return 5;
        }
        if (setting.kdeModule == QStringLiteral("kcm_networkmanagement"))
            foundNetworkManagement = true;
        if (setting.kdeModule == QStringLiteral("kcmspellchecking"))
            foundSpelling = true;
        if (setting.kdeModule.contains(QStringLiteral("kwallet"),
                                       Qt::CaseInsensitive))
            return 9;
        if (nativeReplacementKeys.contains(setting.key)) {
            if (setting.status != ReplacementStatus::Native)
                return 11;
            nativeReplacementsFound.insert(setting.key);
        }
        if (setting.backend == SettingsBackend::Aero7Page
            && PageRegistry::pathFor(setting.page).isEmpty())
            return 6;
        if (setting.backend == SettingsBackend::Aero7Applet
            && setting.applet.isEmpty())
            return 7;
    }

    if (!foundNetworkManagement || !foundSpelling)
        return 10;
    if (nativeReplacementsFound != nativeReplacementKeys)
        return 12;
    if (SettingsCatalog::findByKey(QStringLiteral("not-a-real-setting")))
        return 15;

    const SettingDefinition *windowColor =
        SettingsCatalog::findByKey(QStringLiteral("colors"));
    if (!windowColor || windowColor->backend != SettingsBackend::ExternalCommand
        || windowColor->command.size() != 3
        || windowColor->command.at(0) != QStringLiteral("aeroshell-kcmloader")
        || !windowColor->command.at(1).endsWith(
            QStringLiteral("/kwin_aeroglassblur_config.so")))
        return 16;

    const QList<PageId> hubs = {
        PageId::DisplaySettings, PageId::NetworkSettings,
        PageId::RegionLanguage, PageId::TaskbarStartMenu,
        PageId::DefaultPrograms, PageId::InputDevices,
        PageId::StartupShutdown, PageId::WindowBehavior,
        PageId::SecurityMaintenance, PageId::StorageAdministration,
        PageId::InternetOptions, PageId::FolderOptions,
        PageId::AutoPlay, PageId::BackupRestore,
    };
    for (PageId page : hubs) {
        const QString path = PageRegistry::pathFor(page);
        if (path.isEmpty() || PageRegistry::idForPath(path) != page)
            return 8;
    }

    return 0;
}
