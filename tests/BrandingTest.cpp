#include "Branding.h"

#include <QCoreApplication>
#include <QSettings>
#include <QStandardPaths>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("aero7-tests"));
    QCoreApplication::setApplicationName(QStringLiteral("control-panel-branding"));
    QStandardPaths::setTestModeEnabled(true);

    QSettings settings;
    settings.clear();
    if (!Branding::useWindowsNames())
        return 1;
    if (Branding::brand(QStringLiteral("Linux Update"))
        != QStringLiteral("Windows Update"))
        return 2;

    Branding::setUseWindowsNames(false);
    if (Branding::useWindowsNames()
        || Branding::brand(QStringLiteral("Linux Update"))
            != QStringLiteral("Linux Update"))
        return 3;

    settings.clear();
    return 0;
}
