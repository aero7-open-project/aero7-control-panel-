#include "WindowSnappingPage.h"

#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
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
    QApplication app(argc, argv);
    if (qEnvironmentVariableIsSet("AERO7_SNAP_REAL_BACKEND")) {
        WindowSnappingPage page(nullptr);
        auto *slider = page.findChild<QSlider *>(QStringLiteral("snapSensitivity"));
        auto *apply = page.findChild<QPushButton *>(QStringLiteral("applySnapSensitivity"));
        auto *status = page.findChild<QLabel *>(QStringLiteral("snapSensitivityStatus"));
        if (!slider || !apply || !status || !slider->isEnabled())
            return 20;
        const int original = slider->value();
        slider->setValue(original == 100 ? 99 : original + 1);
        apply->click();
        if (status->text() != QStringLiteral("Snap sensitivity saved."))
            return 21;
        slider->setValue(original);
        apply->click();
        return status->text() == QStringLiteral("Snap sensitivity saved.") ? 0 : 22;
    }

    QTemporaryDir sandbox;
    if (!sandbox.isValid())
        return 1;
    const QString helper = sandbox.filePath(QStringLiteral("snap-control"));
    const QString state = sandbox.filePath(QStringLiteral("sensitivity"));
    qputenv("AERO7_SNAP_CONTROL_PROGRAM", helper.toUtf8());
    qputenv("AERO7_SNAP_TEST_STATE", state.toUtf8());

    if (!writeHelper(helper,
            "#!/bin/sh\n"
            "case \"$1\" in\n"
            "  sensitivity) echo 35;;\n"
            "  set-sensitivity) exit 0;;\n"
            "esac\n"))
        return 2;
    WindowSnappingPage page(nullptr);
    auto *slider = page.findChild<QSlider *>(QStringLiteral("snapSensitivity"));
    auto *apply = page.findChild<QPushButton *>(QStringLiteral("applySnapSensitivity"));
    auto *status = page.findChild<QLabel *>(QStringLiteral("snapSensitivityStatus"));
    if (!slider || !apply || !status || slider->value() != 35 || apply->isEnabled())
        return 3;
    slider->setValue(40);
    apply->click();
    if (!status->text().contains(QStringLiteral("did not retain"))
        || !apply->isEnabled())
        return 4;

    if (!writeHelper(helper,
            "#!/bin/sh\n"
            "case \"$1\" in\n"
            "  sensitivity) if [ -f \"$AERO7_SNAP_TEST_STATE\" ]; then\n"
            "      read value < \"$AERO7_SNAP_TEST_STATE\"; echo \"$value\";\n"
            "    else echo 35; fi;;\n"
            "  set-sensitivity) echo \"$2\" > \"$AERO7_SNAP_TEST_STATE\";;\n"
            "esac\n"))
        return 5;
    apply->click();
    if (status->text() != QStringLiteral("Snap sensitivity saved.")
        || apply->isEnabled())
        return 6;

    if (!writeHelper(helper,
            "#!/bin/sh\n"
            "case \"$1\" in\n"
            "  sensitivity) echo 40;;\n"
            "  set-sensitivity) exit 3;;\n"
            "esac\n"))
        return 7;
    slider->setValue(41);
    apply->click();
    if (!status->text().contains(QStringLiteral("Could not save")))
        return 8;

    if (!writeHelper(helper, "#!/bin/sh\nexit 3\n"))
        return 9;
    WindowSnappingPage unavailable(nullptr);
    auto *unavailableSlider = unavailable.findChild<QSlider *>(QStringLiteral("snapSensitivity"));
    auto *unavailableStatus = unavailable.findChild<QLabel *>(QStringLiteral("snapSensitivityStatus"));
    if (!unavailableSlider || !unavailableStatus || unavailableSlider->isEnabled()
        || !unavailableStatus->text().contains(QStringLiteral("unavailable")))
        return 10;
    return 0;
}
