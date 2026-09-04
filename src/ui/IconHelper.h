#pragma once

#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QSize>
#include <QString>
#include <initializer_list>

namespace Aero7PackIcons {

inline QString normalized(QString value)
{
    value = value.toLower();
    value.replace(QLatin1Char('_'), QLatin1Char('-'));
    return value;
}

inline QIcon exact(const QString &name)
{
    QIcon icon;
    for (const int size : {8, 16, 22, 24, 32, 36, 48, 64, 72, 96, 128, 192, 256}) {
        const QString path = QStringLiteral(":/aero7/pack/%1/%2.png").arg(size).arg(name);
        if (QFile::exists(path)) icon.addFile(path, QSize(size, size));
    }
    return icon;
}

inline QString fallbackName(const QString &raw)
{
    const QString name = normalized(raw);
    if (name.contains(QStringLiteral("search"))) return QStringLiteral("edit-find");
    if (name.contains(QStringLiteral("help"))) return QStringLiteral("help-contents");
    if (name.contains(QStringLiteral("next")) || name.contains(QStringLiteral("right"))) return QStringLiteral("go-next");
    if (name.contains(QStringLiteral("previous")) || name.contains(QStringLiteral("back"))) return QStringLiteral("go-previous");
    if (name.contains(QStringLiteral("refresh")) || name.contains(QStringLiteral("update"))) return QStringLiteral("view-refresh");
    if (name.contains(QStringLiteral("warning")) || name.contains(QStringLiteral("medium"))) return QStringLiteral("dialog-warning");
    if (name.contains(QStringLiteral("error")) || name.contains(QStringLiteral("low"))) return QStringLiteral("dialog-error");
    if (name.contains(QStringLiteral("security")) || name.contains(QStringLiteral("firewall"))) return QStringLiteral("security-high");
    if (name.contains(QStringLiteral("network")) || name.contains(QStringLiteral("internet"))) return QStringLiteral("network-workgroup");
    if (name.contains(QStringLiteral("wireless"))) return QStringLiteral("network-wireless");
    if (name.contains(QStringLiteral("user")) || name.contains(QStringLiteral("account")) || name.contains(QStringLiteral("parental"))) return QStringLiteral("preferences-system-users");
    if (name.contains(QStringLiteral("sound")) || name.contains(QStringLiteral("audio"))) return QStringLiteral("preferences-desktop-sound");
    if (name.contains(QStringLiteral("time")) || name.contains(QStringLiteral("clock")) || name.contains(QStringLiteral("date"))) return QStringLiteral("preferences-system-time");
    if (name.contains(QStringLiteral("display")) || name.contains(QStringLiteral("monitor"))) return QStringLiteral("display");
    if (name.contains(QStringLiteral("printer"))) return QStringLiteral("gnome-dev-printer");
    if (name.contains(QStringLiteral("bluetooth"))) return QStringLiteral("bluetooth");
    if (name.contains(QStringLiteral("battery")) || name.contains(QStringLiteral("power"))) return QStringLiteral("battery-full");
    if (name.contains(QStringLiteral("drive")) || name.contains(QStringLiteral("disk")) || name.contains(QStringLiteral("storage"))) return QStringLiteral("drive-harddisk");
    if (name.contains(QStringLiteral("computer")) || name.contains(QStringLiteral("device")) || name.contains(QStringLiteral("hardware"))) return QStringLiteral("computer");
    if (name.contains(QStringLiteral("folder"))) return QStringLiteral("folder");
    if (name.contains(QStringLiteral("program")) || name.contains(QStringLiteral("software")) || name.contains(QStringLiteral("package"))) return QStringLiteral("system-software-install");
    if (name.contains(QStringLiteral("game"))) return QStringLiteral("applications-games");
    if (name.contains(QStringLiteral("graphic")) || name.contains(QStringLiteral("photo")) || name.contains(QStringLiteral("personal"))) return QStringLiteral("applications-graphics");
    if (name.contains(QStringLiteral("office")) || name.contains(QStringLiteral("document"))) return QStringLiteral("applications-office");
    if (name.contains(QStringLiteral("access"))) return QStringLiteral("preferences-system");
    if (name.contains(QStringLiteral("propert")) || name.contains(QStringLiteral("setting")) || name.contains(QStringLiteral("control"))) return QStringLiteral("preferences-system");
    return QStringLiteral("applications-system");
}

inline QIcon resolve(const QString &raw)
{
    const QString name = normalized(raw);
    QIcon icon = exact(name);
    if (icon.isNull()) icon = exact(fallbackName(name));
    if (icon.isNull()) icon = exact(QStringLiteral("applications-system"));
    if (icon.isNull()) qWarning().noquote() << "[Aero7 Icons] Missing pack icon:" << raw;
    return icon;
}

} // namespace Aero7PackIcons

inline QIcon resolveIcon(const QString &name) { return Aero7PackIcons::resolve(name); }
inline QIcon tryIconName(const QString &name) { return Aero7PackIcons::resolve(name); }

inline QIcon themeIcon(std::initializer_list<const char *> names)
{
    for (const char *candidate : names) {
        const QIcon icon = Aero7PackIcons::exact(Aero7PackIcons::normalized(QString::fromLatin1(candidate)));
        if (!icon.isNull()) return icon;
    }
    return Aero7PackIcons::resolve(names.size() ? QString::fromLatin1(*names.begin()) : QStringLiteral("applications-system"));
}

inline QIcon aero7ResourceIcon(const QString &path, const QString &fallback)
{
    const QIcon icon(path);
    if (!icon.isNull()) return icon;
    qWarning().noquote() << "[Aero7 Icons] Missing resource:" << path;
    return Aero7PackIcons::resolve(fallback);
}
