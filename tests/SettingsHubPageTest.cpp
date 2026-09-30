#include "SettingsHubPage.h"
#include "Branding.h"
#include "KdeSettingsBridge.h"

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSet>
#include <QStandardPaths>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("aero7-tests"));
    QCoreApplication::setApplicationName(QStringLiteral("control-panel-naming"));
    QStandardPaths::setTestModeEnabled(true);
    QSettings().clear();
    Branding::setUseWindowsNames(true);

    const QList<SettingsSection> sections = {
        SettingsSection::Appearance, SettingsSection::Display,
        SettingsSection::TaskbarStartMenu, SettingsSection::WindowBehavior,
        SettingsSection::InputDevices, SettingsSection::Sound,
        SettingsSection::Network, SettingsSection::Power,
        SettingsSection::Accounts, SettingsSection::RegionLanguage,
        SettingsSection::Applications, SettingsSection::SearchHistory,
        SettingsSection::Accessibility, SettingsSection::StartupShutdown,
        SettingsSection::SecurityMaintenance,
        SettingsSection::StorageAdministration, SettingsSection::System,
    };

    int rows = 0;
    QSet<QString> renderedKeys;
    for (SettingsSection section : sections) {
        SettingsHubPage page(section, nullptr);
        page.resize(1000, 720);
        page.show();
        app.processEvents();

        const auto buttons = page.findChildren<QPushButton *>();
        for (QPushButton *button : buttons) {
            if (!button->objectName().startsWith(QStringLiteral("setting-action-")))
                continue;
            if (!button->isEnabled()
                || button->property("settingKey").toString().isEmpty()
                || button->property("originalKdeName").toString().isEmpty())
                return 1;
            renderedKeys.insert(button->property("settingKey").toString());
            ++rows;
        }
        if (buttons.isEmpty())
            return 2;
    }

    if (rows != SettingsCatalog::all().size()
        || renderedKeys.size() != SettingsCatalog::all().size())
        return 3;

    // Unit builds do not necessarily contain Plasma's runtime modules. The
    // installed-module check is an opt-in integration gate for Aero7 hosts.
    for (const SettingDefinition &setting : SettingsCatalog::all()) {
        if (setting.backend != SettingsBackend::KdeModuleBridge)
            continue;
        if (setting.kdeModule.isEmpty()
            || (qEnvironmentVariableIsSet("AERO7_CHECK_INSTALLED_KCMS")
                && !KdeSettingsBridge::moduleAvailable(setting.kdeModule)))
            return 6;
    }

    SettingsHubPage windowsPage(SettingsSection::SecurityMaintenance, nullptr);
    auto *windowsUpdate = windowsPage.findChild<QLabel *>(
        QStringLiteral("setting-link-updates"));
    if (!windowsUpdate || windowsUpdate->text() != QStringLiteral("Windows Update"))
        return 4;

    Branding::setUseWindowsNames(false);
    SettingsHubPage kdePage(SettingsSection::SecurityMaintenance, nullptr);
    auto *softwareUpdate = kdePage.findChild<QLabel *>(
        QStringLiteral("setting-link-updates"));
    if (!softwareUpdate || softwareUpdate->text() != QStringLiteral("Software Update"))
        return 5;

    QSettings().clear();
    return 0;
}
