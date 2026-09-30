#include "TaskbarStartMenuPage.h"

#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>

namespace {
bool writeHelper(const QString &path, const QByteArray &body)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || file.write(body) != body.size())
        return false;
    file.close();
    return file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                               | QFileDevice::ExeOwner);
}
}

int main(int argc, char **argv)
{
    if (qEnvironmentVariableIsSet("AERO7_TASKBAR_REAL_BACKEND")) {
        QApplication app(argc, argv);
        TaskbarStartMenuPage page(nullptr);
        app.processEvents();
        auto *status = page.findChild<QLabel *>(QStringLiteral("taskbarStatus"));
        auto *apply = page.findChild<QPushButton *>(QStringLiteral("taskbarApply"));
        if (!status || !apply
            || status->text() != QStringLiteral("Settings loaded from AeroShell."))
            return 20;
        apply->click();
        return status->text() == QStringLiteral("Taskbar and Start menu settings applied.")
            ? 0 : 21;
    }

    QTemporaryDir sandbox;
    if (!sandbox.isValid())
        return 1;
    qputenv("PATH", sandbox.path().toUtf8());
    QApplication app(argc, argv);
    const QString helper = sandbox.filePath(QStringLiteral("qdbus6"));

    if (!writeHelper(helper,
            "#!/bin/sh\necho '{\"panel\":false,\"tasks\":false,\"start\":false}'\n"))
        return 2;
    TaskbarStartMenuPage absent(nullptr);
    app.processEvents();
    auto *absentStatus = absent.findChild<QLabel *>(QStringLiteral("taskbarStatus"));
    auto *absentApply = absent.findChild<QPushButton *>(QStringLiteral("taskbarApply"));
    if (!absentStatus || !absentApply
        || !absentStatus->text().contains(QStringLiteral("not available")))
        return 3;
    absentApply->click();
    if (!absentStatus->text().contains(QStringLiteral("nothing was applied")))
        return 4;

    if (!writeHelper(helper, "#!/bin/sh\nexit 2\n"))
        return 5;
    absentApply->click();
    if (!absentStatus->text().contains(QStringLiteral("Could not apply")))
        return 6;

    const QByteArray validState =
        "{\"panel\":true,\"tasks\":true,\"start\":true,"
        "\"locked\":false,\"hiding\":\"none\",\"height\":40,"
        "\"grouping\":1,\"onlyWhenFull\":false,\"labels\":false,"
        "\"previews\":true,\"recents\":true,\"jumpLists\":true,\"rows\":8}";
    if (!writeHelper(helper,
            QByteArray("#!/bin/sh\necho '") + validState + "'\n"))
        return 7;
    TaskbarStartMenuPage present(nullptr);
    app.processEvents();
    auto *presentStatus = present.findChild<QLabel *>(QStringLiteral("taskbarStatus"));
    auto *presentApply = present.findChild<QPushButton *>(QStringLiteral("taskbarApply"));
    if (!presentStatus || !presentApply
        || presentStatus->text() != QStringLiteral("Settings loaded from AeroShell."))
        return 8;
    presentApply->click();
    if (presentStatus->text() != QStringLiteral("Taskbar and Start menu settings applied."))
        return 9;

    if (!writeHelper(helper,
            "#!/bin/sh\necho '{\"panel\":true,\"tasks\":true,\"start\":true}'\n"))
        return 10;
    presentApply->click();
    if (!presentStatus->text().contains(QStringLiteral("did not retain")))
        return 11;
    return 0;
}
