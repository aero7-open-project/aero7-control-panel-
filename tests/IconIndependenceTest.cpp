#include "IconHelper.h"

#include <QApplication>
#include <QHash>
#include <QImage>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    const QStringList icons = {
        QStringLiteral("preferences-system"), QStringLiteral("dialog-password"),
        QStringLiteral("folder-network"), QStringLiteral("input_devices_settings"),
        QStringLiteral("application-vnd.debian.binary-package"),
        QStringLiteral("preferences-desktop-accessibility"),
        QStringLiteral("system-software-update"), QStringLiteral("dialog-warning"),
        QStringLiteral("preferences-security-firewall"), QStringLiteral("system-users"),
        QStringLiteral("preferences-desktop-display"), QStringLiteral("audio-speakers")
    };
    QHash<QString, QImage> baseline;
    QIcon::setThemeName(QStringLiteral("Aero7-test-theme"));
    for (const QString &name : icons) {
        const QIcon icon = resolveIcon(name);
        if (icon.isNull()) return 1;
        for (const int size : {16, 22, 24, 32, 48, 64, 128, 256})
            if (icon.pixmap(size, size).isNull()) return 2;
        baseline.insert(name, icon.pixmap(32, 32).toImage());
    }
    for (const QString &theme : {QStringLiteral("breeze"), QStringLiteral("breeze-dark"),
                                 QStringLiteral("missing-aero7-theme")}) {
        QIcon::setThemeName(theme);
        for (const QString &name : icons)
            if (resolveIcon(name).pixmap(32, 32).toImage() != baseline.value(name)) return 3;
    }
    if (aero7ResourceIcon(QStringLiteral(":/missing/aero7-icon.svg"),
                          QStringLiteral("control-panel")).isNull()) return 4;
    return 0;
}
