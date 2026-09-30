#include "FirewallBackend.h"

#include <QtTest>

class FirewallBackendTest : public QObject {
    Q_OBJECT

private slots:
    void chooseRunningBackend()
    {
        using namespace FirewallBackend;
        QCOMPARE(choose(true, true, true, true), Kind::Firewalld);
        QCOMPARE(choose(true, false, true, true), Kind::Ufw);
        QCOMPARE(choose(true, true, true, false), Kind::Firewalld);
    }

    void chooseInstalledBackendWithoutClaimingItIsActive()
    {
        using namespace FirewallBackend;
        QCOMPARE(choose(true, false, true, false), Kind::Firewalld);
        QCOMPARE(choose(false, false, true, false), Kind::Ufw);
        QCOMPARE(choose(false, false, false, false), Kind::None);
    }

    void liveStatusIsInternallyConsistent()
    {
        const auto status = FirewallBackend::detect();
        if (!QStandardPaths::findExecutable(QStringLiteral("firewall-cmd")).isEmpty()
            && QStandardPaths::findExecutable(QStringLiteral("ufw")).isEmpty()) {
            QCOMPARE(status.kind, FirewallBackend::Kind::Firewalld);
            QCOMPARE(status.active, FirewallBackend::serviceActive(
                QStringLiteral("firewalld.service")));
        }
        if (status.kind == FirewallBackend::Kind::Firewalld)
            QCOMPARE(status.name, QStringLiteral("firewalld"));
        else if (status.kind == FirewallBackend::Kind::Ufw)
            QCOMPARE(status.name, QStringLiteral("UFW"));
        else {
            QCOMPARE(status.name, QStringLiteral("No firewall backend"));
            QVERIFY(!status.active);
        }
    }
};

QTEST_GUILESS_MAIN(FirewallBackendTest)
#include "FirewallBackendTest.moc"
