#include "FeatureCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStorageInfo>

namespace {

QStringList strings(const QJsonValue &value)
{
    QStringList result;
    for (const QJsonValue &entry : value.toArray())
        if (entry.isString() && !entry.toString().isEmpty())
            result << entry.toString();
    return result;
}

bool runPacman(const QStringList &arguments)
{
    const QString program = qEnvironmentVariable(
        "AERO7_PACMAN", QStringLiteral("/usr/bin/pacman"));
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(1500))
        return false;
    if (!process.waitForFinished(8000)) {
        process.kill();
        process.waitForFinished();
        return false;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

bool serviceActive(QString unit)
{
    if (unit.contains(QLatin1String("%USER%"))) {
        const QString user = qEnvironmentVariable("USER");
        if (user.isEmpty())
            return false;
        unit.replace(QStringLiteral("%USER%"), user);
    }
    QProcess process;
    process.start(QStringLiteral("/usr/bin/systemctl"),
                  {QStringLiteral("is-active"), QStringLiteral("--quiet"), unit});
    return process.waitForFinished(2500)
        && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

bool probeHardware(const QString &probe)
{
    if (probe == QLatin1String("btrfs-root"))
        return QStorageInfo(QStringLiteral("/")).fileSystemType() == QByteArray("btrfs");
    if (probe == QLatin1String("cellular-modem")) {
        const QDir tty(QStringLiteral("/sys/class/tty"));
        const QStringList names = tty.entryList(
            {QStringLiteral("ttyUSB*"), QStringLiteral("ttyACM*")}, QDir::Dirs);
        if (!names.isEmpty())
            return true;
        const QDir usb(QStringLiteral("/sys/class/usbmisc"));
        return !usb.entryList({QStringLiteral("cdc-wdm*")}, QDir::Dirs).isEmpty();
    }
    return true;
}

bool rebootPending(const QString &featureId)
{
    const QString directory = qEnvironmentVariable(
        "AERO7_FEATURE_REBOOT_DIR",
        QStringLiteral("/run/aero7/optional-features-reboot"));
    return QFileInfo::exists(QDir(directory).absoluteFilePath(featureId));
}

bool bundledPackageAvailable(const QString &package)
{
    const QString directory = qEnvironmentVariable(
        "AERO7_FEATURE_BUNDLE_DIR",
        QStringLiteral("/var/cache/aero7/optional-packages"));
    const QString archive = QDir(directory).absoluteFilePath(
        package + QStringLiteral(".pkg.tar.zst"));
    return QFileInfo(archive).isFile()
        && QFileInfo(archive + QStringLiteral(".sha256")).isFile();
}

} // namespace

QString FeatureCatalog::catalogPath()
{
    const QString overridePath = qEnvironmentVariable("AERO7_FEATURE_CATALOG");
    if (!overridePath.isEmpty())
        return overridePath;

    const QString installed = QStringLiteral("/usr/share/aero7-control-panel/features.json");
    if (QFileInfo::exists(installed))
        return installed;

    const QString besideApp = QDir(QCoreApplication::applicationDirPath())
        .absoluteFilePath(QStringLiteral("../share/aero7-control-panel/features.json"));
    if (QFileInfo::exists(besideApp))
        return besideApp;

#ifdef AERO7_FEATURE_CATALOG_SOURCE
    return QStringLiteral(AERO7_FEATURE_CATALOG_SOURCE);
#else
    return installed;
#endif
}

FeatureCatalog FeatureCatalog::load(QString *error)
{
    FeatureCatalog catalog;
    QFile file(catalogPath());
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = QStringLiteral("Cannot open feature catalog: %1").arg(file.errorString());
        return catalog;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = QStringLiteral("Invalid feature catalog: %1").arg(parseError.errorString());
        return catalog;
    }

    for (const QJsonValue &value : document.object().value(QStringLiteral("features")).toArray()) {
        const QJsonObject object = value.toObject();
        FeatureDefinition feature;
        feature.id = object.value(QStringLiteral("id")).toString();
        feature.name = object.value(QStringLiteral("name")).toString();
        feature.description = object.value(QStringLiteral("description")).toString();
        feature.category = object.value(QStringLiteral("category")).toString();
        feature.packages = strings(object.value(QStringLiteral("packages")));
        feature.supportPackages = strings(object.value(QStringLiteral("supportPackages")));
        feature.services = strings(object.value(QStringLiteral("services")));
        feature.dependencies = strings(object.value(QStringLiteral("dependencies")));
        feature.conflicts = strings(object.value(QStringLiteral("conflicts")));
        feature.route = object.value(QStringLiteral("route")).toString();
        feature.launch = strings(object.value(QStringLiteral("launch")));
        feature.installSizeMiB = object.value(QStringLiteral("installSizeMiB")).toInt();
        feature.rebootRequired = object.value(QStringLiteral("rebootRequired")).toBool();
        feature.logoutRequired = object.value(QStringLiteral("logoutRequired")).toBool();
        feature.visibleInOptionalFeatures =
            object.value(QStringLiteral("visibleInOptionalFeatures")).toBool(true);
        feature.privilege = object.value(QStringLiteral("privilege")).toString();
        const QJsonObject availability = object.value(QStringLiteral("availability")).toObject();
        feature.availabilityMode = availability.value(QStringLiteral("mode")).toString();
        feature.availabilityProbe = availability.value(QStringLiteral("probe")).toString();
        feature.unavailableReason = availability.value(QStringLiteral("reason")).toString();
        feature.compatibility = object.value(QStringLiteral("compatibility")).toString();
        if (feature.id.isEmpty() || feature.name.isEmpty()) {
            if (error)
                *error = QStringLiteral("Feature catalog contains an item without id or name.");
            catalog.m_features.clear();
            return catalog;
        }
        catalog.m_features << feature;
    }
    if (catalog.m_features.isEmpty() && error)
        *error = QStringLiteral("Feature catalog does not contain any features.");
    return catalog;
}

const FeatureDefinition *FeatureCatalog::find(const QString &id) const
{
    for (const FeatureDefinition &feature : m_features)
        if (feature.id == id)
            return &feature;
    return nullptr;
}

FeatureStatus FeatureCatalog::status(const QString &id, bool checkRepository) const
{
    const FeatureDefinition *feature = find(id);
    if (!feature)
        return {FeatureState::Unavailable, QStringLiteral("Unknown feature."), {}, {}};
    return status(*feature, checkRepository);
}

FeatureStatus FeatureCatalog::status(const FeatureDefinition &feature,
                                     bool checkRepository) const
{
    FeatureStatus result;
    if (feature.availabilityMode == QLatin1String("unavailable")) {
        result.state = FeatureState::Unavailable;
        result.detail = feature.unavailableReason;
        return result;
    }
    for (const QString &package : feature.packages) {
        if (runPacman({QStringLiteral("-Q"), package}))
            result.installedPackages << package;
        else
            result.missingPackages << package;
    }
    const bool noAnchorInstalled = result.installedPackages.isEmpty();
    const bool anchorsMissing = !result.missingPackages.isEmpty();
    QStringList missingSupportPackages;
    for (const QString &package : feature.supportPackages) {
        if (runPacman({QStringLiteral("-Q"), package}))
            result.installedPackages << package;
        else {
            result.missingPackages << package;
            missingSupportPackages << package;
        }
    }

    if (feature.availabilityMode == QLatin1String("hardware")
        && !probeHardware(feature.availabilityProbe)) {
        result.state = FeatureState::HardwareNotPresent;
        result.detail = result.missingPackages.isEmpty()
            ? QStringLiteral("Feature support is installed, but no compatible hardware was detected.")
            : QStringLiteral("No compatible hardware was detected. Feature support is not installed.");
        return result;
    }

    if (!anchorsMissing && missingSupportPackages.isEmpty()) {
        QStringList inactiveServices;
        for (const QString &service : feature.services)
            if (!serviceActive(service))
                inactiveServices << service;
        if (!inactiveServices.isEmpty()) {
            result.state = FeatureState::PartiallyInstalled;
            result.detail = QStringLiteral("Installed, but required services are not running. Select the feature to repair it.");
            return result;
        }
        if (feature.rebootRequired && rebootPending(feature.id)) {
            result.state = FeatureState::RebootRequired;
            result.detail = QStringLiteral("Installed; restart required to finish applying this feature.");
        } else {
            result.state = FeatureState::Installed;
            result.detail = feature.availabilityMode == QLatin1String("core")
                ? QStringLiteral("Required Aero7 component")
                : QStringLiteral("Installed");
        }
        return result;
    }
    if (!noAnchorInstalled) {
        result.state = FeatureState::PartiallyInstalled;
        result.detail = QStringLiteral("Some components are missing. Select the feature to repair it.");
        return result;
    }

    if (feature.availabilityMode == QLatin1String("core")) {
        result.state = FeatureState::PartiallyInstalled;
        result.detail = QStringLiteral("A required Aero7 package is missing. Repair the desktop package set.");
        return result;
    }
    if (checkRepository && feature.availabilityMode == QLatin1String("bundled")) {
        for (const QString &package : feature.packages) {
            if (!bundledPackageAvailable(package)) {
                result.state = FeatureState::Unavailable;
                result.detail = QStringLiteral(
                    "The bundled package for %1 is not available on this installation.")
                                    .arg(package);
                return result;
            }
        }
    } else if (checkRepository) {
        QStringList repositoryPackages = feature.packages;
        repositoryPackages << feature.supportPackages;
        repositoryPackages.removeDuplicates();
        for (const QString &package : repositoryPackages) {
            if (!runPacman({QStringLiteral("-Si"), package})) {
                result.state = FeatureState::Unavailable;
                result.detail = QStringLiteral("Package %1 is not available from the configured repositories.")
                                    .arg(package);
                return result;
            }
        }
    }
    result.state = FeatureState::NotInstalled;
    result.detail = QStringLiteral("Not installed");
    return result;
}

QString FeatureCatalog::stateName(FeatureState state)
{
    switch (state) {
    case FeatureState::NotInstalled: return QStringLiteral("Not installed");
    case FeatureState::Installing: return QStringLiteral("Installing");
    case FeatureState::Installed: return QStringLiteral("Installed");
    case FeatureState::PartiallyInstalled: return QStringLiteral("Partially installed");
    case FeatureState::Removing: return QStringLiteral("Removing");
    case FeatureState::Failed: return QStringLiteral("Failed");
    case FeatureState::RebootRequired: return QStringLiteral("Reboot required");
    case FeatureState::Unavailable: return QStringLiteral("Unavailable");
    case FeatureState::HardwareNotPresent: return QStringLiteral("Hardware not present");
    }
    return QStringLiteral("Unknown");
}

bool FeatureCatalog::isEnabled(FeatureState state)
{
    return state == FeatureState::Installed || state == FeatureState::RebootRequired;
}
