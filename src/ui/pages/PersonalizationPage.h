#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include "PageId.h"
#include <QList>
#include <QHash>
#include <QColor>

class QScrollArea;
class QGridLayout;
class QFrame;
class QPixmap;

// The "Personalization" detail page (Appearance and Personalization), a
// Control-Panel rendering of the Windows 7 theme picker.
//
// Only Aero7-approved colour schemes are exposed. A swatch is painted from the
// scheme's own window / titlebar / selection colours, and clicking one applies
// it live with `plasma-apply-colorscheme`. The bottom row opens Aero7-owned
// wallpaper, window-color, sound, and lock-screen surfaces.
class PersonalizationPage : public QWidget {
    Q_OBJECT

public:
    explicit PersonalizationPage(QScrollArea *sidebar, QWidget *parent = nullptr);

    static QList<SidebarLink> sidebarLinks();
    static QList<SidebarLink> sidebarSeeAlso();

signals:
    void soundRequested();

protected:
    // Clicking a swatch frame applies that scheme.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct Scheme {
        QString id;         // file basename, the id plasma-apply-colorscheme wants
        QString name;       // display name from the scheme's [General] Name
        QColor  window;     // window background
        QColor  view;       // content/view background
        QColor  titlebar;   // active window titlebar
        QColor  accent;     // selection colour
        QColor  text;       // normal foreground
        QStringList aliases; // visually identical installed schemes
    };

    static QList<Scheme> gatherSchemes();
    static QString        currentSchemeId();

    QPixmap swatchPixmap(const Scheme &s) const;
    void    applyScheme(int index);
    void    refreshHighlight();
    void    chooseWallpaper();
    void    openWindowColor();
    void    configureLockScreen();

    QList<Scheme>         m_schemes;
    QString               m_currentId;
    bool                  m_applyInProgress = false;
    QList<QFrame *>       m_frames;
    QHash<QObject *, int> m_frameToIndex;
};
