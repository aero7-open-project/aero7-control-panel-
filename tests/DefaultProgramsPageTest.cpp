#include "DefaultProgramsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

class DefaultProgramsPageTest : public QObject {
    Q_OBJECT

private slots:
    void loadsSharedLauncherStatusAndSavesSelection()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString log = temp.filePath(QStringLiteral("calls.log"));
        const QString backendState = temp.filePath(QStringLiteral("backend-state"));
        const QString defaultState = temp.filePath(QStringLiteral("default-state"));
        const QString launcher = temp.filePath(QStringLiteral("launcher"));
        QFile script(launcher);
        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
        script.write(R"SH(#!/bin/sh
if [ "$1" = "--status-json" ]; then
  if [ -f "$AERO7_IE_TEST_BACKEND" ]; then
    read backend < "$AERO7_IE_TEST_BACKEND"
  else
    backend=firefox.desktop
  fi
  if [ -f "$AERO7_IE_TEST_DEFAULT" ]; then
    read is_default < "$AERO7_IE_TEST_DEFAULT"
  else
    is_default=false
  fi
  printf '{"selectedDesktopId":"%s","isDefault":%s,"policyLocked":false,"backends":[{"desktopId":"firefox.desktop","displayName":"Firefox","icon":"firefox","supportsNewWindow":true,"supportsPrivateMode":true},{"desktopId":"chromium.desktop","displayName":"Chromium","icon":"chromium","supportsNewWindow":true,"supportsPrivateMode":true}]}\n' "$backend" "$is_default"
  exit 0
fi
printf '%s\n' "$*" >> "$AERO7_IE_TEST_LOG"
case "$1" in
  --set-backend) printf '%s\n' "$2" > "$AERO7_IE_TEST_BACKEND";;
  --set-default) printf 'true\n' > "$AERO7_IE_TEST_DEFAULT";;
  --restore-defaults) printf 'false\n' > "$AERO7_IE_TEST_DEFAULT";;
esac
)SH");
        script.close();
        QVERIFY(script.setPermissions(QFileDevice::ReadOwner
                                      | QFileDevice::WriteOwner
                                      | QFileDevice::ExeOwner));
        qputenv("AERO7_IE_EXECUTABLE", launcher.toUtf8());
        qputenv("AERO7_IE_TEST_LOG", log.toUtf8());
        qputenv("AERO7_IE_TEST_BACKEND", backendState.toUtf8());
        qputenv("AERO7_IE_TEST_DEFAULT", defaultState.toUtf8());

        DefaultProgramsPage page(new QScrollArea);
        auto *browser = page.findChild<QComboBox *>(
            QStringLiteral("internetExplorerBackend"));
        auto *makeDefault = page.findChild<QCheckBox *>(
            QStringLiteral("internetExplorerDefault"));
        auto *save = page.findChild<QPushButton *>(
            QStringLiteral("saveInternetExplorerDefaults"));
        auto *details = page.findChild<QLabel *>(QStringLiteral("backendDetails"));
        auto *icon = page.findChild<QLabel *>(QStringLiteral("internetExplorerIcon"));
        QVERIFY(browser);
        QVERIFY(makeDefault);
        QVERIFY(save);
        QVERIFY(details);
        QVERIFY(icon);
        QVERIFY(!icon->pixmap().isNull());
        QVERIFY(!browser->isEnabled());
        QVERIFY(!makeDefault->isEnabled());
        QVERIFY(!save->isEnabled());
        QTRY_COMPARE(browser->count(), 2);
        QCOMPARE(browser->currentData().toString(), QStringLiteral("firefox.desktop"));
        QVERIFY(details->text().contains(QStringLiteral("InPrivate browsing: available")));

        browser->setCurrentIndex(1);
        makeDefault->setChecked(true);
        save->click();
        QFile calls(log);
        QVERIFY(calls.open(QIODevice::ReadOnly));
        const QByteArray output = calls.readAll();
        QVERIFY(output.contains("--set-backend chromium.desktop"));
        QVERIFY(output.contains("--set-default"));
        QCOMPARE(browser->currentData().toString(), QStringLiteral("chromium.desktop"));
        QVERIFY(makeDefault->isChecked());
        auto *status = page.findChild<QLabel *>(QStringLiteral("internetExplorerStatus"));
        QVERIFY(status);
        QCOMPARE(status->text(), QStringLiteral("Internet Explorer defaults were updated."));
        qunsetenv("AERO7_IE_EXECUTABLE");
        qunsetenv("AERO7_IE_TEST_LOG");
        qunsetenv("AERO7_IE_TEST_BACKEND");
        qunsetenv("AERO7_IE_TEST_DEFAULT");
    }

    void rejectsUnretainedAndInvalidStatus()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString launcher = temp.filePath(QStringLiteral("launcher"));
        QFile script(launcher);
        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
        script.write(R"SH(#!/bin/sh
if [ "$1" = "--status-json" ]; then
  printf '%s\n' '{"selectedDesktopId":"firefox.desktop","isDefault":false,"policyLocked":false,"backends":[{"desktopId":"firefox.desktop","displayName":"Firefox"},{"desktopId":"chromium.desktop","displayName":"Chromium"}]}'
fi
)SH");
        script.close();
        QVERIFY(script.setPermissions(QFileDevice::ReadOwner
                                      | QFileDevice::WriteOwner
                                      | QFileDevice::ExeOwner));
        qputenv("AERO7_IE_EXECUTABLE", launcher.toUtf8());
        DefaultProgramsPage page(new QScrollArea);
        auto *browser = page.findChild<QComboBox *>(
            QStringLiteral("internetExplorerBackend"));
        auto *save = page.findChild<QPushButton *>(
            QStringLiteral("saveInternetExplorerDefaults"));
        auto *status = page.findChild<QLabel *>(QStringLiteral("internetExplorerStatus"));
        QVERIFY(browser);
        QVERIFY(save);
        QVERIFY(status);
        QTRY_COMPARE(browser->count(), 2);
        browser->setCurrentIndex(1);
        save->click();
        QVERIFY(status->text().contains(QStringLiteral("did not retain")));

        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
        script.write("#!/bin/sh\nprintf 'invalid JSON\\n'\n");
        script.close();
        DefaultProgramsPage invalid(new QScrollArea);
        auto *invalidSave = invalid.findChild<QPushButton *>(
            QStringLiteral("saveInternetExplorerDefaults"));
        auto *invalidStatus = invalid.findChild<QLabel *>(
            QStringLiteral("internetExplorerStatus"));
        QVERIFY(invalidSave);
        QVERIFY(invalidStatus);
        QTRY_VERIFY(invalidStatus->text().contains(QStringLiteral("invalid status")));
        QVERIFY(!invalidSave->isEnabled());

        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
        script.write(R"SH(#!/bin/sh
printf '%s\n' '{"selectedDesktopId":"firefox.desktop","isDefault":false,"policyLocked":true,"backends":[{"desktopId":"firefox.desktop","displayName":"Firefox"}]}'
)SH");
        script.close();
        DefaultProgramsPage locked(new QScrollArea);
        auto *lockedBrowser = locked.findChild<QComboBox *>(
            QStringLiteral("internetExplorerBackend"));
        auto *lockedDefault = locked.findChild<QCheckBox *>(
            QStringLiteral("internetExplorerDefault"));
        auto *lockedSave = locked.findChild<QPushButton *>(
            QStringLiteral("saveInternetExplorerDefaults"));
        QVERIFY(lockedBrowser);
        QVERIFY(lockedDefault);
        QVERIFY(lockedSave);
        QTRY_COMPARE(lockedBrowser->count(), 1);
        QVERIFY(!lockedBrowser->isEnabled());
        QVERIFY(!lockedDefault->isEnabled());
        QVERIFY(!lockedSave->isEnabled());
        qunsetenv("AERO7_IE_EXECUTABLE");
    }

    void savesInstalledBackendInVm()
    {
        if (!qEnvironmentVariableIsSet("AERO7_IE_REAL_BACKEND"))
            QSKIP("Real browser-backend changes run only in an isolated Aero7 VM.");
        qunsetenv("AERO7_IE_EXECUTABLE");
        DefaultProgramsPage page(new QScrollArea);
        auto *browser = page.findChild<QComboBox *>(
            QStringLiteral("internetExplorerBackend"));
        auto *save = page.findChild<QPushButton *>(
            QStringLiteral("saveInternetExplorerDefaults"));
        auto *status = page.findChild<QLabel *>(QStringLiteral("internetExplorerStatus"));
        QVERIFY(browser);
        QVERIFY(save);
        QVERIFY(status);
        QTRY_VERIFY(browser->count() > 0);
        QVERIFY(save->isEnabled());
        const QString requested = browser->currentData().toString();
        QVERIFY(!requested.isEmpty());
        save->click();
        QCOMPARE(status->text(), QStringLiteral("Internet Explorer defaults were updated."));

        QProcess launcher;
        launcher.start(QStringLiteral("aero7-internet-explorer"),
                       {QStringLiteral("--status-json")});
        QVERIFY(launcher.waitForStarted(3000));
        QVERIFY(launcher.waitForFinished(10000));
        QCOMPARE(launcher.exitCode(), 0);
        const QJsonDocument document = QJsonDocument::fromJson(
            launcher.readAllStandardOutput());
        QVERIFY(document.isObject());
        QCOMPARE(document.object().value(QStringLiteral("selectedDesktopId")).toString(),
                 requested);
    }

    void changesAndRestoresAssociationsInVm()
    {
        if (!qEnvironmentVariableIsSet("AERO7_IE_REAL_BACKEND"))
            QSKIP("Real MIME changes run only in an isolated Aero7 VM with Falkon installed.");
        qunsetenv("AERO7_IE_EXECUTABLE");
        QVERIFY2(!QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                    "applications/org.kde.falkon.desktop").isEmpty(),
                 "Install signed Falkon in the disposable VM before this test.");
        const auto mimeCommand = [](const QStringList &arguments) {
            QProcess command;
            command.start("xdg-mime", arguments);
            if (!command.waitForStarted(3000) || !command.waitForFinished(10000)
                || command.exitStatus() != QProcess::NormalExit || command.exitCode() != 0)
                return QByteArray();
            return arguments.constFirst() == "query"
                ? command.readAllStandardOutput().trimmed() : QByteArray("saved");
        };
        const QStringList types{"x-scheme-handler/http", "x-scheme-handler/https", "text/html"};
        QStringList previous;
        for (const QString &type : types) {
            const QByteArray desktop = mimeCommand({"query", "default", type});
            QVERIFY2(!desktop.isEmpty(), "A valid original default is required for safe rollback.");
            previous.append(QString::fromUtf8(desktop));
        }
        const QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
            + "/aero7/internet-explorer.conf";
        QFile config(configPath);
        const bool existed = config.exists();
        QByteArray originalConfig;
        if (existed) {
            QVERIFY(config.open(QIODevice::ReadOnly));
            originalConfig = config.readAll();
            config.close();
        }
        const auto rollback = qScopeGuard([&] {
            bool restoredAll = true;
            for (int i = 0; i < types.size(); ++i)
                if (mimeCommand({"default", previous.at(i), types.at(i)}).isEmpty())
                    restoredAll = false;
            if (existed) {
                QSaveFile restored(configPath);
                if (!restored.open(QIODevice::WriteOnly)
                    || restored.write(originalConfig) != originalConfig.size()
                    || !restored.commit())
                    restoredAll = false;
            } else if (QFile::exists(configPath)) {
                if (!QFile::remove(configPath))
                    restoredAll = false;
            }
            if (!restoredAll)
                QTest::qFail("Could not restore every VM browser association/configuration.",
                             __FILE__, __LINE__);
        });
        for (const QString &type : types)
            QVERIFY(!mimeCommand({"default", "org.kde.falkon.desktop", type}).isEmpty());

        DefaultProgramsPage page(new QScrollArea);
        page.resize(1000, 680);
        page.show();
        auto *browser = page.findChild<QComboBox *>("internetExplorerBackend");
        auto *makeDefault = page.findChild<QCheckBox *>("internetExplorerDefault");
        auto *save = page.findChild<QPushButton *>("saveInternetExplorerDefaults");
        auto *status = page.findChild<QLabel *>("internetExplorerStatus");
        QVERIFY(browser && makeDefault && save && status);
        QTRY_VERIFY(browser->count() > 0);
        const int falkon = browser->findData("org.kde.falkon.desktop");
        QVERIFY2(falkon >= 0, "Install signed Falkon in the disposable VM before this test.");
        browser->setCurrentIndex(falkon);
        QVERIFY(!makeDefault->isChecked());
        makeDefault->setChecked(true);
        save->click();
        QCOMPARE(status->text(), "Internet Explorer defaults were updated.");
        for (const QString &type : types)
            QCOMPARE(mimeCommand({"query", "default", type}), QByteArray("aero7-internet-explorer.desktop"));

        makeDefault->setChecked(false);
        save->click();
        QCOMPARE(status->text(), "Internet Explorer defaults were updated.");
        QVERIFY(!makeDefault->isChecked());
        for (const QString &type : types)
            QCOMPARE(mimeCommand({"query", "default", type}), QByteArray("org.kde.falkon.desktop"));

        // Fresh Aero7 installs can already use the wrapper without any older
        // browser recorded. Clearing the checkbox must show that limitation,
        // never claim to have restored associations that do not exist.
        QVERIFY(QFile::remove(configPath));
        for (const QString &type : types)
            QVERIFY(!mimeCommand({"default", "aero7-internet-explorer.desktop", type}).isEmpty());
        DefaultProgramsPage noHistory(new QScrollArea);
        auto *noHistoryDefault = noHistory.findChild<QCheckBox *>("internetExplorerDefault");
        auto *noHistorySave = noHistory.findChild<QPushButton *>("saveInternetExplorerDefaults");
        auto *noHistoryStatus = noHistory.findChild<QLabel *>("internetExplorerStatus");
        QVERIFY(noHistoryDefault && noHistorySave && noHistoryStatus);
        QTRY_VERIFY(noHistorySave->isEnabled());
        QVERIFY(noHistoryDefault->isChecked());
        noHistoryDefault->setChecked(false);
        noHistorySave->click();
        QVERIFY(noHistoryStatus->text().contains("No previous browser default"));
        for (const QString &type : types)
            QCOMPARE(mimeCommand({"query", "default", type}), QByteArray("aero7-internet-explorer.desktop"));
    }
};

QTEST_MAIN(DefaultProgramsPageTest)
#include "DefaultProgramsPageTest.moc"
