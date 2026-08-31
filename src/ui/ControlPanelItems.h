#pragma once

#include "PageId.h"

#include <QList>
#include <QString>

// The public "All Control Panel Items" surface deliberately follows the
// Windows 7 inventory.  The larger SettingsCatalog still documents every KDE
// replacement and powers search, but exposing that implementation catalog in
// this view made the desktop look and behave like System Settings.
struct ControlPanelItem {
    QString windowsName;
    QString kdeName;
    QString iconName;
    LinkTarget target;
};

const QList<ControlPanelItem> &controlPanelItems();
