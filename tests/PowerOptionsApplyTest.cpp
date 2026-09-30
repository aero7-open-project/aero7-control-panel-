#include "PowerOptionsPage.h"

#include <QAbstractButton>
#include <QApplication>
#include <QButtonGroup>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QLabel>
#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include <cstdio>

namespace {
bool waitForResult(QLabel *status)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 10000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        if (status->text() == QStringLiteral("Power plan applied."))
            return true;
        if (status->text().startsWith(QStringLiteral("Could not apply"))
            || status->text().startsWith(QStringLiteral("The power service did not")))
            return false;
        QThread::msleep(20);
    }
    return false;
}
}

int main(int argc, char **argv)
{
    const bool realBackend = qEnvironmentVariableIsSet("AERO7_POWER_REAL_BACKEND");
    if (!realBackend)
        qputenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/tmp/aero7-no-system-bus");
    QApplication app(argc, argv);
    PowerOptionsPage page(nullptr);
    auto *status = page.findChild<QLabel *>(QStringLiteral("powerPlanStatus"));
    auto *group = page.findChild<QButtonGroup *>();
    if (!status || !group)
        return 1;

    const QList<SidebarLink> links = PowerOptionsPage::sidebarLinks();
    if (links.size() != 3
        || links.at(0).target.kind != LinkTarget::Command
        || links.at(0).target.command.last() != QStringLiteral("kcm_screenlocker")
        || links.at(1).target.kind != LinkTarget::Command
        || links.at(1).target.command.last() != QStringLiteral("kcm_powerdevilprofilesconfig")
        || links.at(2).target.kind != LinkTarget::Command)
        return 2;

    if (!realBackend) {
        if (!status->text().contains(QStringLiteral("unavailable"))
            || group->buttons().size() != 1
            || group->buttons().first()->isEnabled())
            return 3;
        QLabel *helpLink = nullptr;
        for (QLabel *label : page.findChildren<QLabel *>()) {
            if (label->text().contains(QStringLiteral("Tell me more about power plans"))) {
                helpLink = label;
                break;
            }
        }
        if (!helpLink)
            return 4;
        bool helpOpened = false;
        QTimer::singleShot(0, &app, [&helpOpened]() {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                auto *dialog = qobject_cast<QMessageBox *>(widget);
                if (dialog && dialog->windowTitle() == QStringLiteral("About power plans")) {
                    helpOpened = true;
                    dialog->accept();
                }
            }
        });
        QMetaObject::invokeMethod(helpLink, "linkActivated", Qt::DirectConnection,
                                  Q_ARG(QString, QStringLiteral("#")));
        if (!helpOpened)
            return 5;
        return 0;
    }

    QAbstractButton *original = nullptr;
    QAbstractButton *alternative = nullptr;
    for (QAbstractButton *button : group->buttons()) {
        if (button->isChecked())
            original = button;
        else if (!button->property("profileId").toString().isEmpty())
            alternative = button;
    }
    if (!original || !alternative || !original->isEnabled())
        return 20;
    alternative->click();
    if (!waitForResult(status) || !alternative->isChecked()) {
        std::fprintf(stderr, "Power plan switch failed: %s\n",
                     status->text().toUtf8().constData());
        return 21;
    }
    original->click();
    if (!waitForResult(status) || !original->isChecked()) {
        std::fprintf(stderr, "Power plan restore failed: %s\n",
                     status->text().toUtf8().constData());
        return 22;
    }
    return 0;
}
