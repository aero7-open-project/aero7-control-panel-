#pragma once

#include <QString>

class QWidget;

// Temporary, explicit bridge for advanced settings without a complete Aero7
// editor. Never silently route to an unrelated Control Panel page.
namespace KdeSettingsBridge {
bool moduleAvailable(const QString &module);
bool open(QWidget *parent, const QString &module, const QString &caption);
}
