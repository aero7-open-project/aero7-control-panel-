#pragma once

#include "ConfFile.h"

#include <QProcess>
#include <QStandardPaths>
#include <QString>

// Fresh Aero7 installations use firewalld. Older UFW installations remain
// supported; never infer "firewall off" just because ufw.conf is absent.
namespace FirewallBackend {

enum class Kind { None, Ufw, Firewalld };

struct Status {
    Kind kind = Kind::None;
    bool active = false;
    QString name;
};

inline Kind choose(bool firewalldInstalled, bool firewalldActive,
                   bool ufwInstalled, bool ufwActive)
{
    if (firewalldActive)
        return Kind::Firewalld;
    if (ufwActive)
        return Kind::Ufw;
    if (firewalldInstalled)
        return Kind::Firewalld;
    if (ufwInstalled)
        return Kind::Ufw;
    return Kind::None;
}

inline bool serviceActive(const QString &name)
{
    QProcess process;
    process.start(QStringLiteral("systemctl"),
                  {QStringLiteral("is-active"), QStringLiteral("--quiet"), name});
    if (!process.waitForStarted(1000))
        return false;
    if (!process.waitForFinished(2000)) {
        process.kill();
        process.waitForFinished(1000);
        return false;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

inline Status detect()
{
    const bool firewalldInstalled = !QStandardPaths::findExecutable(
        QStringLiteral("firewall-cmd")).isEmpty();
    const bool ufwInstalled = !QStandardPaths::findExecutable(
        QStringLiteral("ufw")).isEmpty();
    const bool firewalldActive = firewalldInstalled
        && serviceActive(QStringLiteral("firewalld.service"));
    const bool ufwActive = ufwInstalled
        && readConfField(QStringLiteral("/etc/ufw/ufw.conf"),
                         QStringLiteral("ENABLED")).compare(
               QStringLiteral("yes"), Qt::CaseInsensitive) == 0;
    const Kind kind = choose(firewalldInstalled, firewalldActive,
                             ufwInstalled, ufwActive);
    return {kind, kind == Kind::Firewalld ? firewalldActive : ufwActive,
            kind == Kind::Firewalld ? QStringLiteral("firewalld")
            : kind == Kind::Ufw ? QStringLiteral("UFW")
                                : QStringLiteral("No firewall backend")};
}

} // namespace FirewallBackend
