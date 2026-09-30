#include "KdeSettingsBridge.h"

#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>

namespace KdeSettingsBridge {

bool moduleAvailable(const QString &module)
{
    const QString executable = QStandardPaths::findExecutable(QStringLiteral("kcmshell6"));
    if (executable.isEmpty() || module.isEmpty())
        return false;

    QProcess query;
    query.start(executable, {QStringLiteral("--list")});
    if (!query.waitForStarted(3000) || !query.waitForFinished(5000)
        || query.exitStatus() != QProcess::NormalExit || query.exitCode() != 0)
        return false;

    const QStringList lines = QString::fromUtf8(query.readAllStandardOutput()).split('\n');
    for (const QString &line : lines) {
        if (line.section(' ', 0, 0) == module)
            return true;
    }
    return false;
}

bool open(QWidget *parent, const QString &module, const QString &caption)
{
    if (!moduleAvailable(module)) {
        QMessageBox::warning(parent, QStringLiteral("Control Panel"),
            QStringLiteral("This setting is temporarily provided by KDE, but its "
                           "module (%1) is not installed. No settings were changed.")
                .arg(module));
        return false;
    }

    const QString executable = QStandardPaths::findExecutable(QStringLiteral("kcmshell6"));
    if (!QProcess::startDetached(executable,
                                 {QStringLiteral("--caption"), caption, module})) {
        QMessageBox::warning(parent, QStringLiteral("Control Panel"),
            QStringLiteral("Could not open the %1 settings module. No settings were changed.")
                .arg(caption));
        return false;
    }
    return true;
}

} // namespace KdeSettingsBridge
