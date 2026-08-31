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
        const QString launcher = temp.filePath(QStringLiteral("launcher"));
        QFile script(launcher);
        QVERIFY(script.open(QIODevice::WriteOnly | QIODevice::Truncate));
        script.write(R"SH(#!/bin/sh
if [ "$1" = "--status-json" ]; then
  printf '%s\n' '{"selectedDesktopId":"firefox.desktop","isDefault":false,"policyLocked":false,"backends":[{"desktopId":"firefox.desktop","displayName":"Firefox","icon":"firefox","supportsNewWindow":true,"supportsPrivateMode":true},{"desktopId":"chromium.desktop","displayName":"Chromium","icon":"chromium","supportsNewWindow":true,"supportsPrivateMode":true}]}'
  exit 0
fi
printf '%s\n' "$*" >> "$AERO7_IE_TEST_LOG"
)SH");
        script.close();
        QVERIFY(script.setPermissions(QFileDevice::ReadOwner
                                      | QFileDevice::WriteOwner
                                      | QFileDevice::ExeOwner));
        qputenv("AERO7_IE_EXECUTABLE", launcher.toUtf8());
        qputenv("AERO7_IE_TEST_LOG", log.toUtf8());

        DefaultProgramsPage page(nullptr);
        auto *browser = page.findChild<QComboBox *>(
            QStringLiteral("internetExplorerBackend"));
        auto *makeDefault = page.findChild<QCheckBox *>(
            QStringLiteral("internetExplorerDefault"));
        auto *save = page.findChild<QPushButton *>(
            QStringLiteral("saveInternetExplorerDefaults"));
        auto *details = page.findChild<QLabel *>(QStringLiteral("backendDetails"));
        QVERIFY(browser);
        QVERIFY(makeDefault);
        QVERIFY(save);
        QVERIFY(details);
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
        qunsetenv("AERO7_IE_EXECUTABLE");
        qunsetenv("AERO7_IE_TEST_LOG");
    }
};

QTEST_MAIN(DefaultProgramsPageTest)
#include "DefaultProgramsPageTest.moc"
