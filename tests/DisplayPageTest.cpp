#include "DisplayPage.h"

#include <QApplication>
#include <QComboBox>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <cstdio>

namespace {
QString currentMode(const QString &name)
{
    QProcess doctor;
    doctor.start(QStringLiteral("kscreen-doctor"), {QStringLiteral("--json")});
    if (!doctor.waitForFinished(10000) || doctor.exitCode() != 0)
        return {};
    const auto outputs = QJsonDocument::fromJson(doctor.readAllStandardOutput())
                             .object().value(QStringLiteral("outputs")).toArray();
    for (const auto &value : outputs) {
        const auto output = value.toObject();
        if (output.value(QStringLiteral("name")).toString() == name)
            return output.value(QStringLiteral("currentModeId")).toString();
    }
    return {};
}

int runRealBackend(QApplication &app)
{
    DisplayPage page(nullptr);
    page.resize(900, 720);
    page.show();
    app.processEvents();
    auto *display = page.findChild<QComboBox *>(QStringLiteral("displaySelector"));
    auto *resolution = page.findChild<QComboBox *>(QStringLiteral("resolutionSelector"));
    auto *apply = page.findChild<QPushButton *>(QStringLiteral("displayApply"));
    auto *status = page.findChild<QLabel *>(QStringLiteral("displayStatus"));
    if (!display || !resolution || !apply || !status || display->count() < 1)
        return 20;
    const QString name = display->currentData().toString();
    const QString original = currentMode(name);
    if (original.isEmpty() || resolution->currentData().toString() != original)
        return 21;
    int alternateIndex = resolution->findText(QStringLiteral("1280 x 768"));
    if (alternateIndex < 0)
        alternateIndex = resolution->findText(QStringLiteral("1024 x 768"));
    if (alternateIndex < 0 || alternateIndex == resolution->currentIndex())
        return 22;
    const QString alternate = resolution->itemData(alternateIndex).toString();

    auto transition = [&](const QString &mode, QMessageBox::StandardButton answer,
                          const QString &expected, bool timed) {
        const int index = resolution->findData(mode);
        if (index < 0)
            return false;
        resolution->setCurrentIndex(index);
        bool observed = false;
        QTimer::singleShot(2000, &page, [&]() {
            auto *dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            observed = dialog && currentMode(name) == mode;
            if (dialog && !timed)
                dialog->button(answer)->click();
        });
        QElapsedTimer elapsed;
        elapsed.start();
        apply->click();
        const QString actual = currentMode(name);
        const bool passed = observed && actual == expected
            && (!timed || elapsed.elapsed() >= 14000);
        std::fprintf(stderr, "Display %s: requested=%s observed=%s result=%s expected=%s status=%s\n",
                     timed ? "timeout" : answer == QMessageBox::Yes ? "keep" : "reject",
                     mode.toUtf8().constData(), observed ? "yes" : "no",
                     actual.toUtf8().constData(), expected.toUtf8().constData(),
                     status->text().toUtf8().constData());
        return passed;
    };
    const bool passed = transition(alternate, QMessageBox::No, original, false)
        && transition(alternate, QMessageBox::No, original, true)
        && transition(alternate, QMessageBox::Yes, alternate, false)
        && transition(original, QMessageBox::Yes, original, false);
    if (!passed) {
        // A failed assertion must still leave the disposable guest usable.
        QProcess restore;
        restore.start(QStringLiteral("kscreen-doctor"),
                      {QStringLiteral("output.%1.mode.%2").arg(name, original)});
        restore.waitForFinished(10000);
        return 23;
    }
    return 0;
}
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (qEnvironmentVariableIsSet("AERO7_DISPLAY_REAL_BACKEND"))
        return runRealBackend(app);
    QTemporaryDir temporary;
    if (!temporary.isValid())
        return 1;

    const QString doctorPath = temporary.filePath(QStringLiteral("kscreen-doctor"));
    QFile doctor(doctorPath);
    if (!doctor.open(QIODevice::WriteOnly | QIODevice::Text))
        return 2;
    doctor.write(R"SH(#!/bin/sh
if [ "$1" != "--json" ]; then
    if [ -n "$AERO7_DOCTOR_LOG" ]; then
        printf '%s\n' "$@" > "$AERO7_DOCTOR_LOG"
    fi
    if [ "$AERO7_DOCTOR_FAIL_RESTORE" = "1" ]; then
        for argument in "$@"; do
            if [ "$argument" = "output.Virtual-1.mode.1" ]; then
                echo 'simulated rollback failure' >&2
                exit 1
            fi
        done
    fi
    exit 0
fi
cat <<'EOF'
{"outputs":[
 {"name":"Virtual-1","connected":true,"enabled":true,"priority":1,"currentModeId":"1","scale":1,"rotation":1,"pos":{"x":0,"y":0},"modes":[{"id":"1","name":"1920x1080","size":{"width":1920,"height":1080},"refreshRate":60},{"id":"1-high","name":"1920x1080","size":{"width":1920,"height":1080},"refreshRate":75},{"id":"3","name":"1280x720","size":{"width":1280,"height":720},"refreshRate":60}]},
 {"name":"Virtual-2","connected":true,"enabled":true,"priority":2,"currentModeId":"2","scale":1,"rotation":1,"pos":{"x":1920,"y":0},"modes":[{"id":"2","name":"1920x1080","size":{"width":1920,"height":1080},"refreshRate":60}]}
]}
EOF
)SH");
    doctor.close();
    if (!doctor.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                               | QFileDevice::ExeOwner))
        return 3;
    qputenv("AERO7_KSCREEN_DOCTOR", doctorPath.toUtf8());

    DisplayPage page(nullptr);
    page.resize(900, 720);
    page.show();
    app.processEvents();

    auto *layout = page.findChild<QWidget *>(QStringLiteral("displayLayout"));
    auto *status = page.findChild<QLabel *>(QStringLiteral("displayStatus"));
    const auto combos = page.findChildren<QComboBox *>();
    if (!layout || !status || combos.isEmpty())
        return 4;

    bool foundTwoDisplays = false;
    for (QComboBox *combo : combos) {
        if (combo->count() == 2
            && combo->itemData(0).toString() == QLatin1String("Virtual-1")
            && combo->itemData(1).toString() == QLatin1String("Virtual-2")) {
            foundTwoDisplays = true;
            break;
        }
    }
    if (!foundTwoDisplays)
        return 5;

    auto *resolution = page.findChild<QComboBox *>(QStringLiteral("resolutionSelector"));
    if (!resolution || resolution->count() != 2
        || resolution->itemText(0) != QLatin1String("1920 x 1080")
        || resolution->itemText(0).contains(QLatin1String("Hz")))
        return 6;
    if (!page.findChild<QWidget *>(QStringLiteral("advancedDisplaySettings"))
        || !page.findChild<QPushButton *>(QStringLiteral("identifyDisplays"))
        || !page.findChild<QPushButton *>(QStringLiteral("displayApply")))
        return 7;

    auto *apply = page.findChild<QPushButton *>(QStringLiteral("displayApply"));
    if (apply->isEnabled())
        return 8;
    const QPointF start(layout->width() / 2.0 - 50, 45);
    const QPointF end = start + QPointF(20, 0);
    QMouseEvent press(QEvent::MouseButtonPress, start, layout->mapToGlobal(start.toPoint()), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(layout, &press);
    QMouseEvent move(QEvent::MouseMove, end, layout->mapToGlobal(end.toPoint()), Qt::NoButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(layout, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, end, layout->mapToGlobal(end.toPoint()), Qt::LeftButton,
                        Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(layout, &release);
    if (!apply->isEnabled() || !status->text().contains(QLatin1String("moved")))
        return 9;

    const QString doctorLogPath = temporary.filePath(QStringLiteral("doctor-arguments.log"));
    qputenv("AERO7_DOCTOR_LOG", doctorLogPath.toUtf8());
    QTimer::singleShot(0, []() {
        auto *dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (dialog)
            dialog->button(QMessageBox::No)->click();
    });
    apply->click();
    QFile doctorLog(doctorLogPath);
    if (!doctorLog.open(QIODevice::ReadOnly)
        || !doctorLog.readAll().contains("output.Virtual-1.position.0,0"))
        return 10;
    doctorLog.close();

    page.findChild<QPushButton *>(QStringLiteral("displayCancel"))->click();
    if (apply->isEnabled())
        return 11;
    resolution->setCurrentIndex(1);
    if (!apply->isEnabled())
        return 12;
    qputenv("AERO7_DOCTOR_FAIL_RESTORE", "1");
    QTimer::singleShot(0, []() {
        auto *dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (dialog)
            dialog->button(QMessageBox::No)->click();
    });
    apply->click();
    qunsetenv("AERO7_DOCTOR_FAIL_RESTORE");
    if (!status->text().contains(QLatin1String("Could not restore"))
        || !status->text().contains(QLatin1String("simulated rollback failure")))
        return 13;

    page.findChild<QPushButton *>(QStringLiteral("displayCancel"))->click();
    auto *display = page.findChild<QComboBox *>(QStringLiteral("displaySelector"));
    resolution->setCurrentIndex(1);
    display->setCurrentIndex(1);
    display->setCurrentIndex(0);
    if (resolution->currentData().toString() != QLatin1String("3"))
        return 14;
    display->setCurrentIndex(1);
    if (!apply->isEnabled())
        return 15;
    QTimer::singleShot(0, []() {
        auto *dialog = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (dialog)
            dialog->button(QMessageBox::Yes)->click();
    });
    apply->click();
    if (!doctorLog.open(QIODevice::ReadOnly)
        || !doctorLog.readAll().contains("output.Virtual-1.mode.3"))
        return 16;
    return 0;
}
