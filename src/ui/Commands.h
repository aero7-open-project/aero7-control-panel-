#pragma once

#include <QString>
#include <QStringList>

// ---------------------------------------------------------------------------
// Approved standalone launchers shared by MainWindow and detail pages.
// ---------------------------------------------------------------------------

// Desktop Gadgets uses Aero7's native gadget host and gallery. This avoids
// exposing Plasma edit mode and keeps the Control Panel entry aligned with the
// gadget package shipped by Aero7.
inline const QStringList kWidgetExplorerCmd = {
    QStringLiteral("aero7-gadget-host"), QStringLiteral("--gallery")
};
inline const QStringList kGetWidgetsCmd = {
    QStringLiteral("aero7-gadget-host"), QStringLiteral("--gallery")
};

// "Device Manager" links launch the standalone devmgmt program.
inline const QStringList kDeviceManagerCmd = {
    QStringLiteral("aero7-device-manager")
};

// Administrative Tools opens Aero7's Computer Management console.
inline const QStringList kComputerManagementCmd = {
    QStringLiteral("aero7-compmgmt")
};

// Programs Center is independently packaged and optional. Launching by name
// keeps Control Panel usable when the testing package is not installed.
inline const QStringList kProgramsCenterCmd = {
    QStringLiteral("aero7-programs-center")
};

inline const QStringList kOptionalFeaturesCmd = {
    QStringLiteral("aero7-optional-features")
};
