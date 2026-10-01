#include "FirewallPage.h"

#include <QFile>
#include <QPushButton>
#include <QDialog>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QtTest>

class FirewallBridgeTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { m_originalPath = qgetenv("PATH"); }
    void cleanup() { qputenv("PATH", m_originalPath); }

    void rulesOpenCheckedModule()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        prepareFirewalld(tools);
        const QString marker = tools.filePath("opened");
        writeTool(tools.filePath("kcmshell6"), QStringLiteral(
            "#!/bin/sh\n"
            "if [ \"$1\" = '--list' ]; then\n"
            "  printf 'kcm_firewall - Control your network rules\\n'\n"
            "  exit 0\n"
            "fi\n"
            "printf '%s\\n' \"$@\" > '%1'\n").arg(marker).toUtf8());

        const auto links = FirewallPage::sidebarLinks();
        for (const int index : {0, 1, 4}) {
            QCOMPARE(links[index].target.kind, LinkTarget::Command);
            QVERIFY(links[index].target.command.contains("kcm_firewall"));
        }
        QCOMPARE(links[3].target.kind, LinkTarget::Disabled);

        FirewallPage page(new QScrollArea);
        for (const auto &name : {"firewall-allow-service", "firewall-notification-settings"}) {
            auto *button = page.findChild<QPushButton *>(name);
            QVERIFY(button);
            QVERIFY(button->isEnabled());
            button->click();
            QTRY_VERIFY(QFile::exists(marker));
            QFile result(marker);
            QVERIFY(result.open(QIODevice::ReadOnly));
            const auto arguments = result.readAll();
            QVERIFY(arguments.contains("kcm_firewall"));
            QVERIFY(arguments.contains("Linux Firewall"));
            result.close();
            QVERIFY(QFile::remove(marker));
        }
        QVERIFY(!page.findChild<QPushButton *>("firewall-restore-defaults")->isEnabled());
        QVERIFY(page.findChild<QLabel *>("firewall-editor-limitations"));
    }

    void missingModuleDisablesOnlyUnsupportedActions()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        prepareFirewalld(tools);
        const auto links = FirewallPage::sidebarLinks();
        for (const int index : {0, 1, 3, 4})
            QCOMPARE(links[index].target.kind, LinkTarget::Disabled);
        FirewallPage page(new QScrollArea);
        QVERIFY(page.findChild<QPushButton *>("firewall-toggle")->isEnabled());
        for (const auto &name : {"firewall-allow-service", "firewall-notification-settings"}) {
            auto *button = page.findChild<QPushButton *>(name);
            QVERIFY(button);
            QVERIFY(!button->isEnabled());
            QVERIFY(button->toolTip().contains("plasma-firewall"));
        }
        QVERIFY(!page.findChild<QPushButton *>("firewall-service-log")->isEnabled());
        QVERIFY(page.findChild<QPushButton *>("firewall-service-log")
                    ->toolTip().contains("journalctl"));
    }

    void serviceLogReadsDiagnosticsWithoutModuleOrPrivileges()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        prepareFirewalld(tools);
        const QString marker = tools.filePath("journal-arguments");
        writeTool(tools.filePath("journalctl"), QStringLiteral(
            "#!/bin/sh\nprintf '%s\\n' \"$@\" > '%1'\n"
            "printf '2026-10-01T20:00:00 firewalld: service started\\n'\n")
            .arg(marker).toUtf8());
        FirewallPage page(new QScrollArea);
        auto *button = page.findChild<QPushButton *>("firewall-service-log");
        QVERIFY(button->isEnabled());
        button->click();
        auto *dialog = page.findChild<QDialog *>("firewall-service-log-dialog");
        QVERIFY(dialog);
        auto *output = dialog->findChild<QPlainTextEdit *>("firewall-service-log-output");
        QTRY_VERIFY(output->toPlainText().contains("service started"));
        QVERIFY(output->isReadOnly());
        QFile arguments(marker);
        QVERIFY(arguments.open(QIODevice::ReadOnly));
        QCOMPARE(arguments.readAll(), QByteArray(
            "--unit=firewalld.service\n--lines=100\n--no-pager\n--output=short-iso\n"));
        QVERIFY(dialog->findChild<QLabel *>("firewall-service-log-status")
                    ->text().contains("up to 100"));
        dialog->reject();
    }

    void serviceLogFailureAndPartialPermissionAreVisible_data()
    {
        QTest::addColumn<QByteArray>("script");
        QTest::addColumn<QString>("expected");
        QTest::newRow("failure") << QByteArray(
            "#!/bin/sh\nprintf 'Access denied\\n' >&2\nexit 1\n")
            << QString("could not be read");
        QTest::newRow("partial-permission") << QByteArray(
            "#!/bin/sh\nprintf 'limited output\\n'\n"
            "printf 'Insufficient journal permissions\\n' >&2\nexit 0\n")
            << QString("Journal reader warning");
        QTest::newRow("empty") << QByteArray(
            "#!/bin/sh\nprintf '%s\\n' '-- No entries --'\n")
            << QString("available to this account");
    }

    void serviceLogFailureAndPartialPermissionAreVisible()
    {
        QFETCH(QByteArray, script);
        QFETCH(QString, expected);
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        prepareFirewalld(tools);
        writeTool(tools.filePath("journalctl"), script);
        FirewallPage page(new QScrollArea);
        page.findChild<QPushButton *>("firewall-service-log")->click();
        auto *dialog = page.findChild<QDialog *>("firewall-service-log-dialog");
        QVERIFY(dialog);
        auto *status = dialog->findChild<QLabel *>("firewall-service-log-status");
        QTRY_VERIFY(status->text().contains(expected));
        dialog->reject();
    }

    void serviceLogTimeoutDoesNotLeaveReadingForever()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        prepareFirewalld(tools);
        writeTool(tools.filePath("journalctl"), "#!/bin/sh\nexec /usr/bin/sleep 30\n");
        FirewallPage page(new QScrollArea);
        page.findChild<QPushButton *>("firewall-service-log")->click();
        auto *dialog = page.findChild<QDialog *>("firewall-service-log-dialog");
        QVERIFY(dialog);
        auto *status = dialog->findChild<QLabel *>("firewall-service-log-status");
        QTRY_VERIFY_WITH_TIMEOUT(status->text().contains("timed out"), 10000);
        dialog->reject();
    }

    void serviceLogFailedStartIsVisible()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        prepareFirewalld(tools);
        writeTool(tools.filePath("journalctl"), "#!/no-such-aero7-journal-interpreter\n");
        FirewallPage page(new QScrollArea);
        page.findChild<QPushButton *>("firewall-service-log")->click();
        auto *dialog = page.findChild<QDialog *>("firewall-service-log-dialog");
        QVERIFY(dialog);
        auto *status = dialog->findChild<QLabel *>("firewall-service-log-status");
        QTRY_VERIFY(status->text().contains("could not start"));
        dialog->reject();
    }

private:
    void writeTool(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(contents), qint64(contents.size()));
        file.close();
        QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
    }

    void prepareFirewalld(const QTemporaryDir &tools)
    {
        qputenv("PATH", tools.path().toUtf8());
        writeTool(tools.filePath("firewall-cmd"), "#!/bin/sh\nexit 0\n");
        writeTool(tools.filePath("systemctl"), "#!/bin/sh\nexit 0\n");
    }

    QByteArray m_originalPath;
};

QTEST_MAIN(FirewallBridgeTest)
#include "FirewallBridgeTest.moc"
