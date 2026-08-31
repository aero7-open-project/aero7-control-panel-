#include "DisplayPage.h"

#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QTemporaryDir>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir temporary;
    if (!temporary.isValid())
        return 1;

    const QString doctorPath = temporary.filePath(QStringLiteral("kscreen-doctor"));
    QFile doctor(doctorPath);
    if (!doctor.open(QIODevice::WriteOnly | QIODevice::Text))
        return 2;
    doctor.write(R"SH(#!/bin/sh
if [ "$1" != "--json" ]; then
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
    return 0;
}
