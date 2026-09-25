#pragma once

#include <QString>
#include <QStringList>
#include <QLibraryInfo>

// ---------------------------------------------------------------------------
// External launchers shared by MainWindow's task links and the detail pages'
// sidebar links. Task and sidebar links with no dedicated page in this app hand
// off to the real KDE module, the way the Windows Control Panel opens an applet.
// ---------------------------------------------------------------------------

// Command for opening a KDE System Settings module, e.g. kcm("kcm_kscreen").
inline QStringList kcm(const char *module)
{
    return { QStringLiteral("kcmshell6"), QString::fromLatin1(module) };
}

// Open AeroShell's glass-color editor, which is a KWin effect configuration
// plugin rather than a normal kcmshell module. The loader needs both the
// plugin path and an icon argument.
inline QStringList aeroWindowColor()
{
    return {
        QStringLiteral("aeroshell-kcmloader"),
        QLibraryInfo::path(QLibraryInfo::PluginsPath)
            + QStringLiteral("/kwin/effects/configs/kwin_aeroglassblur_config.so"),
        QStringLiteral("preferences-desktop-color")
    };
}

// Desktop Gadgets links hook into KDE Plasma's real widget panels. The
// "Desktop Gadgets" heading and "Add gadgets to the desktop" link toggle
// Plasma's widget explorer (the "Add or Manage widgets" panel) over D-Bus;
// "Get more gadgets online" opens the Get-New-Widgets download dialog.
inline const QStringList kWidgetExplorerCmd = {
    QStringLiteral("qdbus6"), QStringLiteral("org.kde.plasmashell"),
    QStringLiteral("/PlasmaShell"),
    QStringLiteral("org.kde.PlasmaShell.toggleWidgetExplorer")
};
inline const QStringList kGetWidgetsCmd = {
    QStringLiteral("knewstuff-dialog6"),
    QStringLiteral("/usr/share/knsrcfiles/plasmoids.knsrc")
};

// "Device Manager" links launch the standalone devmgmt program.
inline const QStringList kDeviceManagerCmd = {
    QStringLiteral("devmgmt")
};

// Programs Center is independently packaged and optional. Launching by name
// keeps Control Panel usable when the testing package is not installed.
inline const QStringList kProgramsCenterCmd = {
    QStringLiteral("aero7-programs-center")
};
