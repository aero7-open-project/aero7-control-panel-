#include "FolderOptionsPage.h"

#include <QApplication>
#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <cstdio>

int main(int argc, char **argv)
{
    QTemporaryDir sandbox;
    if (!sandbox.isValid())
        return 1;
    qputenv("XDG_CONFIG_HOME", sandbox.path().toUtf8());
    qputenv("PATH", sandbox.path().toUtf8());
    QApplication app(argc, argv);
    if (!QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
             .startsWith(sandbox.path()))
        return 2;

    FolderOptionsPage page(nullptr);
    auto *apply = page.findChild<QPushButton *>(QStringLiteral("folderOptionsApply"));
    auto *status = page.findChild<QLabel *>(QStringLiteral("folderOptionsStatus"));
    if (!apply || !status)
        return 3;

    QCheckBox *index = nullptr;
    for (QCheckBox *box : page.findChildren<QCheckBox *>()) {
        if (box->text().contains(QStringLiteral("file index"))) {
            index = box;
            break;
        }
    }
    if (!index)
        return 4;
    index->setChecked(false);

    apply->click();
    if (!status->text().contains(QStringLiteral("not installed")))
        return 5;
    QSettings saved(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                    + QStringLiteral("/baloofilerc"), QSettings::IniFormat);
    if (saved.value(QStringLiteral("Basic Settings/Indexing-Enabled"), true).toBool())
        return 6;

    const QString fakeTool = sandbox.filePath(QStringLiteral("balooctl6"));
    QFile tool(fakeTool);
    if (!tool.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || tool.write("#!/bin/sh\nexit 7\n") < 0)
        return 7;
    tool.close();
    if (!tool.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                             | QFileDevice::ExeOwner))
        return 8;
    apply->click();
    if (!status->text().contains(QStringLiteral("could not be changed"))) {
        std::fprintf(stderr, "Unexpected indexer failure status: %s\n",
                     qPrintable(status->text()));
        return 9;
    }

    if (!tool.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || tool.write("#!/bin/sh\nexit 0\n") < 0)
        return 10;
    tool.close();
    apply->click();
    if (status->text() != QStringLiteral("Folder settings applied."))
        return 11;
    return 0;
}
