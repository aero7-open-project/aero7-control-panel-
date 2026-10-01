#include "UserAccountsPage.h"
#include "LinkLabel.h"

#include <QFile>
#include <QLabel>
#include <QScrollArea>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class UserAccountsBridgeTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { m_originalPath = qgetenv("PATH"); }
    void cleanup() { qputenv("PATH", m_originalPath); }

    void accountTasksOpenTheCheckedUsersModule()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        const QString marker = tools.filePath(QStringLiteral("opened"));
        QFile module(tools.filePath(QStringLiteral("kcmshell6")));
        QVERIFY(module.open(QIODevice::WriteOnly));
        module.write(QStringLiteral(
            "#!/bin/sh\n"
            "if [ \"$1\" = '--list' ]; then\n"
            "  printf 'kcm_users - Manage user accounts\\n'\n"
            "  exit 0\n"
            "fi\n"
            "printf '%s\\n' \"$@\" > '%1'\n").arg(marker).toUtf8());
        module.close();
        QVERIFY(module.setPermissions(QFile::ReadOwner | QFile::WriteOwner
                                      | QFile::ExeOwner));

        const auto sidebar = UserAccountsPage::sidebarLinks();
        QVERIFY(!sidebar.isEmpty());
        QCOMPARE(sidebar.first().target.kind, LinkTarget::Command);
        QVERIFY(sidebar.first().target.command.contains(QStringLiteral("kcm_users")));

        UserAccountsPage page(new QScrollArea);
        const auto *warning = page.findChild<QLabel *>(QStringLiteral("accountCreationWarning"));
        QVERIFY(warning);
        QVERIFY(warning->wordWrap());
        QVERIFY(warning->text().contains(QStringLiteral("before signing out")));
        QVERIFY(warning->text().contains(QStringLiteral("Change Password")));
        QVERIFY(warning->text().contains(QStringLiteral("nothing changed")));
        const QStringList editTasks = {
            QStringLiteral("Change your password"),
            QStringLiteral("Change your picture"),
            QStringLiteral("Change your account name"),
            QStringLiteral("Change your account type"),
            QStringLiteral("Manage another account"),
        };
        for (const QString &task : editTasks) {
            LinkLabel *link = nullptr;
            for (auto *candidate : page.findChildren<LinkLabel *>()) {
                if (candidate->text() == task) {
                    link = candidate;
                    break;
                }
            }
            QVERIFY2(link, qPrintable(task));
            link->clicked();
            QTRY_VERIFY(QFile::exists(marker));
            QFile opened(marker);
            QVERIFY(opened.open(QIODevice::ReadOnly));
            const QString args = QString::fromUtf8(opened.readAll());
            QVERIFY(args.contains(QStringLiteral("--caption")));
            QVERIFY(args.contains(QStringLiteral("User Accounts")));
            QVERIFY(args.contains(QStringLiteral("kcm_users")));
            opened.close();
            QVERIFY(QFile::remove(marker));
        }

        QSignalSpy navigation(&page, &UserAccountsPage::navigateRequested);
        for (auto *candidate : page.findChildren<LinkLabel *>()) {
            if (candidate->text() == QStringLiteral("Review administrator approval")) {
                candidate->clicked();
                break;
            }
        }
        QCOMPARE(navigation.count(), 1);
    }

    void missingModuleDoesNotAdvertiseAdvancedSettings()
    {
        QTemporaryDir tools;
        QVERIFY(tools.isValid());
        qputenv("PATH", tools.path().toUtf8());
        const auto sidebar = UserAccountsPage::sidebarLinks();
        QVERIFY(!sidebar.isEmpty());
        QCOMPARE(sidebar.first().target.kind, LinkTarget::Disabled);
        UserAccountsPage page(new QScrollArea);
        int disabledEditors = 0;
        for (auto *link : page.findChildren<LinkLabel *>()) {
            if (link->text() == QStringLiteral("Review administrator approval"))
                QVERIFY(link->isEnabled());
            else {
                QVERIFY(!link->isEnabled());
                ++disabledEditors;
            }
        }
        QCOMPARE(disabledEditors, 5);
    }

private:
    QByteArray m_originalPath;
};

QTEST_MAIN(UserAccountsBridgeTest)
#include "UserAccountsBridgeTest.moc"
