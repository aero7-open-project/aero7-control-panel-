#include "LinuxUpdatePage.h"

#include <QFile>
#include <QLabel>
#include <QScrollArea>
#include <QSettings>
#include <QTemporaryDir>
#include <QPushButton>
#include <QTreeWidget>
#include <QtTest>

class LinuxUpdateCheckTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("Aero7UpdateCheckTest"));
        QCoreApplication::setApplicationName(QStringLiteral("Aero7UpdateCheckTest"));
        m_originalPath = qgetenv("PATH");
    }

    void cleanup()
    {
        qputenv("PATH", m_originalPath);
        QSettings settings;
        settings.clear();
    }

    void missingToolDoesNotPretendTheSystemIsUpToDate()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();

        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        QVERIFY(title);
        QCOMPARE(title->text(), QStringLiteral("Updates could not be checked"));
        QSettings settings;
        QVERIFY(!settings.value(QStringLiteral("LinuxUpdate/lastCheck"))
                     .toDateTime().isValid());
    }

    void failedToolShowsErrorAndDoesNotRecordSuccessfulCheck()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        QFile script(tools.filePath(QStringLiteral("checkupdates")));
        QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\nprintf 'repository unavailable\\n' >&2\nexit 1\n");
        script.close();
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                      | QFile::ExeOwner));

        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();

        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        QVERIFY(title);
        QTRY_COMPARE(title->text(), QStringLiteral("Updates could not be checked"));
        QVERIFY(title->toolTip().contains(QStringLiteral("repository unavailable")));
        QSettings settings;
        QVERIFY(!settings.value(QStringLiteral("LinuxUpdate/lastCheck"))
                     .toDateTime().isValid());
    }

    void noUpdatesExitRecordsSuccessfulCheck()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        QFile script(tools.filePath(QStringLiteral("checkupdates")));
        QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\nexit 2\n");
        script.close();
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                      | QFile::ExeOwner));

        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();

        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        QVERIFY(title);
        QTRY_VERIFY(title->text().contains(QStringLiteral("up to date")));
        QSettings settings;
        QVERIFY(settings.value(QStringLiteral("LinuxUpdate/lastCheck"))
                    .toDateTime().isValid());
    }

    void failedAurCheckDoesNotClaimEverythingIsUpToDate()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        QFile repoTool(tools.filePath(QStringLiteral("checkupdates")));
        QVERIFY(repoTool.open(QIODevice::WriteOnly));
        repoTool.write("#!/bin/sh\nexit 2\n");
        repoTool.close();
        QVERIFY(repoTool.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                        | QFile::ExeOwner));
        QFile aurTool(tools.filePath(QStringLiteral("yay")));
        QVERIFY(aurTool.open(QIODevice::WriteOnly));
        aurTool.write("#!/bin/sh\nprintf 'AUR network unavailable\\n' >&2\nexit 1\n");
        aurTool.close();
        QVERIFY(aurTool.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                       | QFile::ExeOwner));

        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();

        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        QVERIFY(title);
        QTRY_COMPARE(title->text(), QStringLiteral("Updates could not be checked"));
        QVERIFY(title->toolTip().contains(QStringLiteral("AUR network unavailable")));
        QSettings settings;
        QVERIFY(!settings.value(QStringLiteral("LinuxUpdate/lastCheck"))
                     .toDateTime().isValid());
    }

    void missingInstallerDoesNotRemainDownloadingOrClaimSuccess()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        QFile script(tools.filePath(QStringLiteral("checkupdates")));
        QVERIFY(script.open(QIODevice::WriteOnly));
        script.write("#!/bin/sh\nprintf 'example-package 1.0 -> 1.1\\n'\nexit 0\n");
        script.close();
        QVERIFY(script.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                      | QFile::ExeOwner));

        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();
        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        auto *install = page.findChild<QPushButton *>(QStringLiteral("updateInstallButton"));
        QVERIFY(title);
        QVERIFY(install);
        QTRY_COMPARE(title->text(), QStringLiteral("Install updates for your computer"));
        QVERIFY(install->isEnabled());
        install->click();
        QTRY_COMPARE(title->text(), QStringLiteral("Some updates failed to install."));
        QSettings settings;
        QCOMPARE(settings.value(QStringLiteral("LinuxUpdate/lastInstallOk"))
                     .toBool(), false);
    }

    void failedInstallerShowsMergedOutput()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        QFile checker(tools.filePath(QStringLiteral("checkupdates")));
        QVERIFY(checker.open(QIODevice::WriteOnly));
        checker.write("#!/bin/sh\nprintf 'example-package 1.0 -> 1.1\\n'\nexit 0\n");
        checker.close();
        QVERIFY(checker.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                       | QFile::ExeOwner));
        QFile installer(tools.filePath(QStringLiteral("pkexec")));
        QVERIFY(installer.open(QIODevice::WriteOnly));
        installer.write("#!/bin/sh\nprintf 'package database unavailable\\n' >&2\nexit 1\n");
        installer.close();
        QVERIFY(installer.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                         | QFile::ExeOwner));

        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();
        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        auto *details = page.findChild<QLabel *>(QStringLiteral("updateErrorDetails"));
        auto *install = page.findChild<QPushButton *>(QStringLiteral("updateInstallButton"));
        QVERIFY(title);
        QVERIFY(details);
        QVERIFY(install);
        QTRY_COMPARE(title->text(), QStringLiteral("Install updates for your computer"));
        install->click();
        QTRY_COMPARE(title->text(), QStringLiteral("Some updates failed to install."));
        QVERIFY(details->text().contains(QStringLiteral("package database unavailable")));
        QSettings settings;
        QCOMPARE(settings.value(QStringLiteral("LinuxUpdate/lastInstallOk"))
                     .toBool(), false);
    }

    void partialRepositorySelectionIsRejectedBeforePacman()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        QFile checker(tools.filePath(QStringLiteral("checkupdates")));
        QVERIFY(checker.open(QIODevice::WriteOnly));
        checker.write("#!/bin/sh\nprintf 'first-package 1.0 -> 1.1\\nsecond-package 2.0 -> 2.1\\n'\nexit 0\n");
        checker.close();
        QVERIFY(checker.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                       | QFile::ExeOwner));

        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();
        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        auto *details = page.findChild<QLabel *>(QStringLiteral("updateErrorDetails"));
        auto *install = page.findChild<QPushButton *>(QStringLiteral("updateInstallButton"));
        QVERIFY(title);
        QVERIFY(details);
        QVERIFY(install);
        QTRY_COMPARE(title->text(), QStringLiteral("Install updates for your computer"));

        page.showSelectView();
        auto *tree = page.findChild<QTreeWidget *>();
        QVERIFY(tree);
        QCOMPARE(tree->topLevelItemCount(), 1);
        QCOMPARE(tree->topLevelItem(0)->childCount(), 2);
        tree->topLevelItem(0)->child(0)->setCheckState(0, Qt::Unchecked);
        QPushButton *ok = nullptr;
        for (auto *button : page.findChildren<QPushButton *>()) {
            if (button->text() == QStringLiteral("OK")) {
                ok = button;
                break;
            }
        }
        QVERIFY(ok);
        ok->click();
        page.showStatusView();
        install->click();

        QCOMPARE(title->text(), QStringLiteral("Install updates for your computer"));
        QVERIFY(details->text().contains(QStringLiteral("full repository upgrade")));
        QSettings settings;
        QVERIFY(!settings.value(QStringLiteral("LinuxUpdate/lastInstall"))
                     .toDateTime().isValid());
    }

    void vanishedAurInstallerDoesNotClaimSuccess()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        QFile checker(tools.filePath(QStringLiteral("checkupdates")));
        QVERIFY(checker.open(QIODevice::WriteOnly));
        checker.write("#!/bin/sh\nexit 2\n");
        checker.close();
        QVERIFY(checker.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                       | QFile::ExeOwner));
        const QString yayPath = tools.filePath(QStringLiteral("yay"));
        QFile aurTool(yayPath);
        QVERIFY(aurTool.open(QIODevice::WriteOnly));
        aurTool.write("#!/bin/sh\nprintf 'example-aur 1.0 -> 1.1\\n'\nexit 0\n");
        aurTool.close();
        QVERIFY(aurTool.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                       | QFile::ExeOwner));

        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();
        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        auto *details = page.findChild<QLabel *>(QStringLiteral("updateErrorDetails"));
        auto *install = page.findChild<QPushButton *>(QStringLiteral("updateInstallButton"));
        QVERIFY(title);
        QVERIFY(details);
        QVERIFY(install);
        QTRY_COMPARE(title->text(), QStringLiteral("Install updates for your computer"));
        QVERIFY(QFile::remove(yayPath));
        install->click();
        QTRY_COMPARE(title->text(), QStringLiteral("Some updates failed to install."));
        QVERIFY(details->text().contains(QStringLiteral("AUR installer could not start")));
        QSettings settings;
        QCOMPARE(settings.value(QStringLiteral("LinuxUpdate/lastInstallOk"))
                     .toBool(), false);
    }

    void fullRepositorySelectionUsesFullUpgrade()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        QFile checker(tools.filePath(QStringLiteral("checkupdates")));
        QVERIFY(checker.open(QIODevice::WriteOnly));
        checker.write("#!/bin/sh\nprintf 'example-package 1.0 -> 1.1\\n'\nexit 0\n");
        checker.close();
        QVERIFY(checker.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                       | QFile::ExeOwner));
        const QString argsPath = tools.filePath(QStringLiteral("install-args"));
        QFile installer(tools.filePath(QStringLiteral("pkexec")));
        QVERIFY(installer.open(QIODevice::WriteOnly));
        installer.write(QStringLiteral(
            "#!/bin/sh\nprintf '%s\\n' \"$@\" > '%1'\nexit 0\n").arg(argsPath).toUtf8());
        installer.close();
        QVERIFY(installer.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                         | QFile::ExeOwner));

        LinuxUpdatePage page(new QScrollArea);
        page.checkForUpdates();
        auto *title = page.findChild<QLabel *>(QStringLiteral("updateStatusTitle"));
        auto *install = page.findChild<QPushButton *>(QStringLiteral("updateInstallButton"));
        QVERIFY(title);
        QVERIFY(install);
        QTRY_COMPARE(title->text(), QStringLiteral("Install updates for your computer"));
        install->click();
        QTRY_COMPARE(title->text(), QStringLiteral("The updates were installed successfully."));
        QFile args(argsPath);
        QVERIFY(args.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(args.readAll()).split(QLatin1Char('\n'),
                    Qt::SkipEmptyParts),
                 QStringList({"pacman", "--color", "never", "-Syu", "--noconfirm"}));
        QSettings settings;
        QVERIFY(settings.value(QStringLiteral("LinuxUpdate/lastInstallOk"))
                    .toBool());
    }

private:
    QByteArray m_originalPath;
};

QTEST_MAIN(LinuxUpdateCheckTest)
#include "LinuxUpdateCheckTest.moc"
