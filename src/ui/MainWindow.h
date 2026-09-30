#pragma once

#include <QMainWindow>
#include <QStringList>
#include <QHash>
#include <QSoundEffect>
#include <QGraphicsOpacityEffect>
#include "PageId.h"

class QScrollArea;
class QLineEdit;
class QToolButton;
class QPushButton;
class QMenu;
class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class CategoryWidget;
class LinuxUpdatePage;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    // Opens a registered page through the same history-aware route used by
    // in-window links. This is used by Start-menu launchers such as
    // `control --page getting-started`.
    void openPage(PageId pageId);

    // Opens a catalog setting from an external surface such as the Aero7
    // Start-menu Settings search. Returns false for an unknown key.
    bool openSetting(const QString &key);

    // Opens an installed feature's page, or the in-Control-Panel feature-
    // required surface when its backend is absent.
    void openOptionalFeature(const QString &featureId);
    void openOptionalFeature(const QString &featureId, PageId installedPage);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class ViewMode { Category, LargeIcons, SmallIcons };
    // The shared chrome of a left-nav pane: a width-clipped scroll area, the
    // grey navPane frame, and the text layout (already seeded with the
    // "Control Panel Home" link) that callers fill with their own entries.
    struct Sidebar {
        QScrollArea *clip;
        QWidget     *textWrap;
        QVBoxLayout *navV;
    };
    Sidebar buildSidebarShell(int initialWidth);

    void buildCrumbBar();
    QWidget *buildHomePage();
    QWidget *buildSearchResultsPage(const QString &query);
    QWidget *buildCategoryPage(const QString &category);
    QScrollArea *buildNavSidebar(const QString &currentCategory);
    QScrollArea *buildSubpageSidebar(const QList<SidebarLink> &links,
                                     const QList<SidebarLink> &seeAlso = {});

    // Point a freshly built sidebar label at the destination its link declares
    // (another page, home, an external command, or an in-app applet).
    void registerLinkTarget(QLabel *label, const LinkTarget &target);

    void setCrumbTrail(const QStringList &trail);
    void navigateHome();
    void navigateTo(const QString &path);
    void updateSearchResults(const QString &query);

    // Open an in-app applet dialog (e.g. "datetime", "datetime:additional"),
    // the Windows-style modal popup for a Control Panel item.
    void openApplet(const QString &id);

    // History-aware rendering. An empty string represents the home page.
    void pushHistory(const QString &entry);
    void showEntry(const QString &entry);
    void goBack();
    void goForward();
    void updateNavButtons();

    QWidget     *m_crumbBar;
    QHBoxLayout *m_pathLayout = nullptr;
    QLineEdit   *m_searchBox;
    QScrollArea *m_scroll;

    QPushButton *m_backBtn = nullptr;
    QPushButton *m_forwardBtn = nullptr;
    QMenu *m_historyMenu = nullptr;
    QToolButton *m_historyButton = nullptr;

    QStringList m_history;
    int         m_historyIndex = -1;

    // Maps each left-nav link label to the category it navigates to.
    QHash<QObject *, QString> m_navLinks;

    // Maps group-title labels to full subpage paths (e.g. "System and Security/Linux Update").
    QHash<QObject *, QString> m_subpageLinks;

    // Maps task/title labels that launch an external program to the command to
    // run, stored as {program, arg, arg, ...} (e.g. the Desktop Gadgets links
    // that open KDE Plasma's widget panels).
    QHash<QObject *, QStringList> m_commandLinks;

    // Maps task/title labels to an in-app applet id opened as a modal dialog
    // (e.g. "datetime"). See openApplet.
    QHash<QObject *, QString> m_appletLinks;

    QHash<QObject *, QString> m_featureLinks;
    QHash<QObject *, int> m_featureFallbackPages;

    // Maps intermediate crumb labels to their navigation paths.
    QHash<QObject *, QString> m_crumbNavLinks;

    // "Control Panel" crumb; a QLabel (not a button) so Aero7Qt's glass-bar
    // QAbstractButton texture never applies. Clicks go to the home view.
    QLabel *m_homeCrumb = nullptr;

    ViewMode m_viewMode = ViewMode::Category;

    QSoundEffect m_navSound;

    // Opacity effect on the current category sidebar's text wrap; used to fade
    // out the text before navigating to a subcategory.
    QGraphicsOpacityEffect *m_sidebarTextEffect = nullptr;

    // Active Linux Update page and its "Check for updates" sidebar label.
    LinuxUpdatePage *m_updatePage         = nullptr;
    QLabel            *m_checkUpdatesLabel  = nullptr;
};
