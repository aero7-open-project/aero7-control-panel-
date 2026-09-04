#include "OptionalFeaturesWindow.h"

#include "IconHelper.h"

#include <Aero7Qt/stylesheet.h>
#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QIcon>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("aero7"));
    app.setApplicationName(QStringLiteral("optional-features"));
    app.setApplicationDisplayName(QStringLiteral("Aero7 Features"));
    app.setWindowIcon(aero7ResourceIcon(
        QStringLiteral(":/aero7/icons/app/aero7-optional-features.png"),
        QStringLiteral("optional-features")));
    Aero7::applyApplicationStyle(&app);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Turn Aero7 optional features on or off"));
    parser.addHelpOption();
    QCommandLineOption featureOption(QStringLiteral("feature"),
        QStringLiteral("Focus a feature by catalog id."), QStringLiteral("id"));
    QCommandLineOption installOption(QStringLiteral("install"),
        QStringLiteral("Focus and request installation of a feature."), QStringLiteral("id"));
    parser.addOption(featureOption);
    parser.addOption(installOption);
    QCommandLineOption removeOption(QStringLiteral("remove"),
        QStringLiteral("Focus and request removal of a feature."), QStringLiteral("id"));
    parser.addOption(removeOption);
    parser.process(app);

    OptionalFeaturesWindow window;
    window.show();
    const QString installId = parser.value(installOption);
    const QString removeId = parser.value(removeOption);
    if (!installId.isEmpty() && !removeId.isEmpty()) {
        QTextStream(stderr) << "Choose either --install or --remove." << Qt::endl;
        return 2;
    }
    const QString focusId = !installId.isEmpty() ? installId
        : !removeId.isEmpty() ? removeId : parser.value(featureOption);
    if (!focusId.isEmpty() && !window.focusFeature(focusId)) {
        QTextStream(stderr) << "Unknown or unavailable Aero7 feature: " << focusId << Qt::endl;
        return 2;
    }
    if (!installId.isEmpty() && !window.requestInstall(installId)) {
        QTextStream(stderr) << "Aero7 feature cannot be installed: " << installId << Qt::endl;
        return 3;
    }
    if (!removeId.isEmpty() && !window.requestRemove(removeId)) {
        QTextStream(stderr) << "Aero7 feature cannot be removed: " << removeId << Qt::endl;
        return 3;
    }
    return app.exec();
}
