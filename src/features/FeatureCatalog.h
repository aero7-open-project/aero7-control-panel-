#pragma once

#include <QList>
#include <QString>
#include <QStringList>

enum class FeatureState {
    NotInstalled,
    Installing,
    Installed,
    PartiallyInstalled,
    Removing,
    Failed,
    RebootRequired,
    Unavailable,
    HardwareNotPresent,
};

struct FeatureDefinition {
    QString id;
    QString name;
    QString description;
    QString category;
    QStringList packages;
    QStringList supportPackages;
    QStringList services;
    QStringList dependencies;
    QStringList conflicts;
    QString route;
    QStringList launch;
    int installSizeMiB = 0;
    bool rebootRequired = false;
    bool logoutRequired = false;
    bool visibleInOptionalFeatures = true;
    QString privilege;
    QString availabilityMode;
    QString availabilityProbe;
    QString unavailableReason;
    QString compatibility;
};

struct FeatureStatus {
    FeatureState state = FeatureState::Unavailable;
    QString detail;
    QStringList installedPackages;
    QStringList missingPackages;
};

class FeatureCatalog {
public:
    static FeatureCatalog load(QString *error = nullptr);
    static QString catalogPath();

    const QList<FeatureDefinition> &features() const { return m_features; }
    const FeatureDefinition *find(const QString &id) const;
    bool isValid() const { return !m_features.isEmpty(); }

    FeatureStatus status(const FeatureDefinition &feature,
                         bool checkRepository = true) const;
    FeatureStatus status(const QString &id, bool checkRepository = true) const;

    static QString stateName(FeatureState state);
    static bool isEnabled(FeatureState state);

private:
    QList<FeatureDefinition> m_features;
};
