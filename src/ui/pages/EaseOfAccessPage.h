#pragma once

#include <QWidget>
#include <QString>
#include "PageId.h"

class QScrollArea;
class QVBoxLayout;

// The "Ease of Access Center" detail page, a Control-Panel rendering of the
// Windows 7 Ease of Access Center.
//
// Every link stays inside an Aero7-owned property sheet. Plasma/KWin remain
// implementation backends only.
class EaseOfAccessPage : public QWidget {
    Q_OBJECT

public:
    explicit EaseOfAccessPage(QScrollArea *sidebar, QWidget *parent = nullptr);

    static QList<SidebarLink> sidebarLinks();
    static QList<SidebarLink> sidebarSeeAlso();

private:
    void openSetting(const QString &key);
    void addSettingLink(QVBoxLayout *into, const QString &iconName,
                        const QString &text, const QString &key);
};
