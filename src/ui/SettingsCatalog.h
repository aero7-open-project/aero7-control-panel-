#pragma once

#include "PageId.h"

#include <QList>
#include <QString>
#include <QStringList>

// One source of truth for the public Aero7 wording and the original KDE name.
// The same catalog drives the Control Panel hubs, routing tests and the
// wiki-ready documentation in docs/KDE-SETTINGS-MAP.md.
enum class SettingsSection {
    Appearance,
    Display,
    TaskbarStartMenu,
    WindowBehavior,
    InputDevices,
    Sound,
    Network,
    Power,
    Accounts,
    RegionLanguage,
    Applications,
    SearchHistory,
    Accessibility,
    StartupShutdown,
    SecurityMaintenance,
    StorageAdministration,
    System,
};

enum class SettingsBackend {
    Aero7Page,
    Aero7Applet,
    KdeModuleBridge,
    ExternalCommand,
};

enum class ReplacementStatus {
    Native,
    Partial,
    CompatibilityBridge,
};

struct SettingDefinition {
    QString key;
    QString aeroName;
    QString kdeName;
    QString kdeModule;
    SettingsSection section;
    QString description;
    QString iconName;
    SettingsBackend backend = SettingsBackend::KdeModuleBridge;
    ReplacementStatus status = ReplacementStatus::Native;
    PageId page = PageId::None;
    QString applet;
    QStringList command;
};

namespace SettingsCatalog {

const QList<SettingDefinition> &all();
const SettingDefinition *findByKey(const QString &key);
QList<SettingDefinition> forSection(SettingsSection section);
QString sectionTitle(SettingsSection section);
QString sectionDescription(SettingsSection section);
PageId pageForSection(SettingsSection section);
SettingsSection sectionForPage(PageId page);

// Converts old visible task/sidebar labels into a real destination. This is
// deliberately catalog-owned so a wording change cannot silently make a link
// inert again.
LinkTarget targetForLegacyLabel(const QString &text);
LinkTarget targetForSetting(const SettingDefinition &setting);

QString backendLabel(const SettingDefinition &setting);
QString statusLabel(ReplacementStatus status);

} // namespace SettingsCatalog
