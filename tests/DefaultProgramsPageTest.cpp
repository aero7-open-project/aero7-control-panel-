#include "DefaultProgramsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QPushButton>
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

        DefaultProgramsPage page(nullptr);
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
        DefaultProgramsPage page(nullptr);
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
        DefaultProgramsPage invalid(nullptr);
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
        DefaultProgramsPage locked(nullptr);
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
};

QTEST_MAIN(DefaultProgramsPageTest)
#include "DefaultProgramsPageTest.moc"
