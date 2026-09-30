#pragma once

#include <QWidget>
#include <QString>
#include "PageId.h"

class QScrollArea;
class QVBoxLayout;

// The "Ease of Access Center" detail page, a Control-Panel rendering of the
// Windows 7 Ease of Access Center.
//
// Advanced links temporarily open working KDE settings modules until their
// Aero7 property sheets have verified backend behavior.
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
