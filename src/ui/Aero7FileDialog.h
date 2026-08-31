#pragma once

#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace Aero7FileDialog {

inline QString openFile(QWidget *parent, const QString &applicationId,
                        const QString &filters)
{
    const QString executable = QStandardPaths::findExecutable(
        QStringLiteral("aero7-file-dialog"));
    if (executable.isEmpty()) {
        QMessageBox::warning(parent, QStringLiteral("Open"),
                             QStringLiteral("The Aero7 file dialog service is unavailable."));
        return {};
    }
    QStringList arguments{QStringLiteral("--mode"), QStringLiteral("open"),
                          QStringLiteral("--app-id"), applicationId};
    for (const QString &filter : filters.split(QStringLiteral(";;"), Qt::SkipEmptyParts))
        arguments << QStringLiteral("--filter") << filter;
    QProcess process;
    process.start(executable, arguments);
    if (!process.waitForStarted(3000)) {
        QMessageBox::warning(parent, QStringLiteral("Open"),
                             QStringLiteral("The Aero7 file dialog could not be started."));
        return {};
    }
    process.waitForFinished(-1);
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        return {};
    return QString::fromUtf8(process.readAllStandardOutput()).section(QLatin1Char('\n'), 0, 0).trimmed();
}

}
