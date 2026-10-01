#include "FolderOptionsPage.h"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QRadioButton>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <cstdio>
#include <KConfig>
#include <KConfigGroup>
#ifdef Q_OS_LINUX
#include <sys/xattr.h>
#endif

int main(int argc, char **argv)
{
    QTemporaryDir sandbox;
    if (!sandbox.isValid())
        return 1;
    qputenv("XDG_CONFIG_HOME", sandbox.path().toUtf8());
    qputenv("XDG_DATA_HOME", sandbox.filePath("data").toUtf8());
    qputenv("PATH", sandbox.path().toUtf8());
    QApplication app(argc, argv);
    if (!QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
             .startsWith(sandbox.path()))
        return 2;
    if (!QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
             .startsWith(sandbox.filePath("data")))
        return 12;

    const QString viewDirectory = sandbox.filePath(
        "data/aero7-file-explorer/view_properties/global");
    if (!QDir().mkpath(viewDirectory))
        return 13;
    const QByteArray viewData =
        "[Dolphin]\nVersion=4\nViewMode=1\nPreviewsShown=false\n"
        "\n[Settings]\nHiddenFilesShown=true\n";
    bool hasMetadata = false;
#ifdef Q_OS_LINUX
    hasMetadata = setxattr(QFile::encodeName(viewDirectory).constData(),
        "user.kde.fm.viewproperties#1", viewData.constData(), viewData.size(), 0) == 0;
#endif
    QFile viewFile(viewDirectory + "/.directory");
    if (!viewFile.open(QIODevice::WriteOnly))
        return 14;
    viewFile.write("[Desktop Entry]\nIcon=keep-this-icon\n");
    if (!hasMetadata)
        viewFile.write(viewData);
    viewFile.close();
    if (!viewFile.open(QIODevice::ReadOnly))
        return 20;
    const QByteArray originalViewFile = viewFile.readAll();
    viewFile.close();

    // Reproduce the ineffective keys from the previous implementation.
    {
        QSettings legacy(sandbox.filePath("dolphinrc"), QSettings::IniFormat);
        legacy.setValue("General/ShowHiddenFiles", false);
        legacy.setValue("General/ShowPreview", true);
        legacy.setValue("General/UnrelatedChoice", "keep");
        QSettings oldBaloo(sandbox.filePath("baloofilerc"), QSettings::IniFormat);
        oldBaloo.setValue("Basic Settings/Indexing-Enabled", true);
        QSettings recycleBin(sandbox.filePath("trashrc"), QSettings::IniFormat);
        recycleBin.setValue("Aero7/ConfirmDelete", false);
        recycleBin.setValue("Aero7/MaximumSizeMiB", 512);
    }

    FolderOptionsPage page(nullptr);
    if (!viewFile.open(QIODevice::ReadOnly) || viewFile.readAll() != originalViewFile)
        return 21;
    viewFile.close();
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
    auto *hidden = page.findChild<QCheckBox *>("folderShowHidden");
    auto *previews = page.findChild<QCheckBox *>("folderShowPreviews");
    auto *single = page.findChild<QRadioButton *>("folderSingleClick");
    auto *confirm = page.findChild<QCheckBox *>("folderConfirmTrash");
    if (!hidden || !previews || !single || !confirm
        || !hidden->isChecked() || previews->isChecked() || confirm->isChecked())
        return 15;
    hidden->setChecked(false);
    previews->setChecked(true);
    single->setChecked(true);
    confirm->setChecked(false);

    apply->click();
    if (!status->text().contains(QStringLiteral("not installed")))
        return 5;
    KConfig saved(sandbox.filePath("baloofilerc"), KConfig::SimpleConfig);
    if (KConfigGroup(&saved, "Basic Settings").readEntry("Indexing-Enabled", true)
        || KConfigGroup(&saved, "Basic%20Settings").hasKey("Indexing-Enabled"))
        return 6;
    KConfig views(viewFile.fileName(), KConfig::SimpleConfig);
    if (KConfigGroup(&views, "Settings").readEntry("HiddenFilesShown", true)
        || !KConfigGroup(&views, "Dolphin").readEntry("PreviewsShown", false)
        || KConfigGroup(&views, "Dolphin").readEntry("ViewMode", -1) != 1
        || KConfigGroup(&views, "Desktop Entry").readEntry("Icon", QString()) != "keep-this-icon")
        return 16;
    KConfig oldDolphin(sandbox.filePath("dolphinrc"), KConfig::SimpleConfig);
    if (KConfigGroup(&oldDolphin, "%General").hasKey("ShowHiddenFiles")
        || KConfigGroup(&oldDolphin, "%General").hasKey("ShowPreview")
        || KConfigGroup(&oldDolphin, "%General").readEntry("UnrelatedChoice", QString()) != "keep")
        return 17;
    KConfig global(sandbox.filePath("kdeglobals"), KConfig::SimpleConfig);
    KConfig trash(sandbox.filePath("kiorc"), KConfig::SimpleConfig);
    // Check with the same reader used by File Explorer, not just our writer.
    QSettings recycleBin(sandbox.filePath("trashrc"), QSettings::IniFormat);
    if (!KConfigGroup(&global, "KDE").readEntry("SingleClick", false)
        || KConfigGroup(&trash, "Confirmations").readEntry("ConfirmTrash", true)
        || recycleBin.value("Aero7/ConfirmDelete", true).toBool()
        || recycleBin.value("Aero7/MaximumSizeMiB").toInt() != 512)
        return 18;
    FolderOptionsPage reread(nullptr);
    if (reread.findChild<QCheckBox *>("folderShowHidden")->isChecked()
        || !reread.findChild<QCheckBox *>("folderShowPreviews")->isChecked()
        || reread.findChild<QCheckBox *>("folderIndexContent")->isChecked())
        return 19;

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

    recycleBin.setValue("Aero7/DeleteImmediately", true);
    recycleBin.sync();
    FolderOptionsPage immediate(nullptr);
    auto *immediateConfirm = immediate.findChild<QCheckBox *>("folderConfirmTrash");
    if (immediateConfirm->isEnabled()
        || !immediateConfirm->toolTip().contains("Permanent deletion"))
        return 22;
    immediateConfirm->setChecked(true);
    immediate.findChild<QPushButton *>("folderOptionsApply")->click();
    recycleBin.sync();
    if (recycleBin.value("Aero7/ConfirmDelete", true).toBool()
        || !recycleBin.value("Aero7/DeleteImmediately", false).toBool()
        || recycleBin.value("Aero7/MaximumSizeMiB").toInt() != 512)
        return 23;
    return 0;
}
