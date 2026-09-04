#include "FeatureCatalog.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSet>
#include <QTemporaryDir>
#include <iostream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QString error;
    const FeatureCatalog catalog = FeatureCatalog::load(&error);
    if (!catalog.isValid()) {
        std::cerr << error.toStdString() << '\n';
        return 1;
    }

    QSet<QString> ids;
    for (const FeatureDefinition &feature : catalog.features()) {
        if (ids.contains(feature.id)) {
            std::cerr << "duplicate feature id: " << feature.id.toStdString() << '\n';
            return 2;
        }
        ids.insert(feature.id);
        if (feature.category.isEmpty() || feature.description.isEmpty()
            || feature.availabilityMode.isEmpty()) {
            std::cerr << "incomplete feature: " << feature.id.toStdString() << '\n';
            return 3;
        }
        if ((feature.availabilityMode == QStringLiteral("repository")
             || feature.availabilityMode == QStringLiteral("bundled"))
            && feature.packages.isEmpty()) {
            std::cerr << "repository feature without packages: "
                      << feature.id.toStdString() << '\n';
            return 4;
        }
    }

    const FeatureDefinition *core = catalog.find(QStringLiteral("aero7-core"));
    const FeatureDefinition *speech = catalog.find(QStringLiteral("speech-recognition"));
    if (!core || core->availabilityMode != QStringLiteral("core"))
        return 5;
    if (!speech || speech->availabilityMode != QStringLiteral("unavailable")
        || speech->unavailableReason.isEmpty())
        return 6;

    QTemporaryDir temporary;
    if (!temporary.isValid())
        return 7;
    const QString pacmanPath = temporary.filePath(QStringLiteral("pacman"));
    QFile pacman(pacmanPath);
    if (!pacman.open(QIODevice::WriteOnly | QIODevice::Text))
        return 8;
    pacman.write(
        "#!/bin/sh\n"
        "if [ \"$1\" = \"-Q\" ]; then\n"
        "  case \"$2\" in restart-demo|installed-support|installed-anchor) exit 0;; esac\n"
        "  exit 1\n"
        "fi\n"
        "[ \"$1\" = \"-Si\" ]\n");
    pacman.close();
    pacman.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                          | QFileDevice::ExeOwner);
    const QString testCatalogPath = temporary.filePath(QStringLiteral("features.json"));
    QFile testCatalog(testCatalogPath);
    if (!testCatalog.open(QIODevice::WriteOnly | QIODevice::Text))
        return 9;
    testCatalog.write(R"({"features":[{
        "id":"restart-demo","name":"Restart Demo","description":"Test feature",
        "category":"Tests","packages":["restart-demo"],"services":[],
        "dependencies":[],"conflicts":[],"rebootRequired":true,
        "availability":{"mode":"repository"}}, {
        "id":"support-only","name":"Support Only","description":"Test feature",
        "category":"Tests","packages":["missing-anchor"],
        "supportPackages":["installed-support"],"services":[],
        "dependencies":[],"conflicts":[],"availability":{"mode":"repository"}}, {
        "id":"missing-support","name":"Missing Support","description":"Test feature",
        "category":"Tests","packages":["installed-anchor"],
        "supportPackages":["missing-support"],"services":[],
        "dependencies":[],"conflicts":[],"availability":{"mode":"repository"}}]})");
    testCatalog.close();
    const QString markerDirectory = temporary.filePath(QStringLiteral("reboot"));
    QDir().mkpath(markerDirectory);
    QFile marker(QDir(markerDirectory).filePath(QStringLiteral("restart-demo")));
    if (!marker.open(QIODevice::WriteOnly))
        return 10;
    marker.close();
    qputenv("AERO7_FEATURE_CATALOG", testCatalogPath.toUtf8());
    qputenv("AERO7_PACMAN", pacmanPath.toUtf8());
    qputenv("AERO7_FEATURE_REBOOT_DIR", markerDirectory.toUtf8());
    const FeatureCatalog restartCatalog = FeatureCatalog::load(&error);
    if (!restartCatalog.isValid())
        return 11;
    const FeatureStatus restartStatus = restartCatalog.status(QStringLiteral("restart-demo"));
    if (restartStatus.state != FeatureState::RebootRequired)
        return 12;
    const FeatureStatus supportOnlyStatus = restartCatalog.status(
        QStringLiteral("support-only"));
    if (supportOnlyStatus.state != FeatureState::NotInstalled)
        return 13;
    const FeatureStatus missingSupportStatus = restartCatalog.status(
        QStringLiteral("missing-support"));
    if (missingSupportStatus.state != FeatureState::PartiallyInstalled)
        return 14;
    return 0;
}
