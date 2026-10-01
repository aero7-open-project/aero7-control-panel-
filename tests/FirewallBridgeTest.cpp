#include "FirewallPage.h"

#include <QFile>
#include <QPushButton>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QtTest>

class FirewallBridgeTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { m_originalPath = qgetenv("PATH"); }
    void cleanup() { qputenv("PATH", m_originalPath); }

    void rulesAndLogsOpenCheckedModule()
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
