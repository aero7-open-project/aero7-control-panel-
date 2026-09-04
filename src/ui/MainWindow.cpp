#include "MainWindow.h"
#include "CategoryWidget.h"
#include "IconHelper.h"
#include "LinkLabel.h"
#include "Win7Ui.h"

#include <QApplication>
#include <QScrollArea>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QFrame>
#include <QIcon>
#include <QFont>
#include <QStatusBar>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSizeGrip>
#include <QLayoutItem>
#include <QEvent>
#include <QMouseEvent>
#include <QVariantAnimation>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QProcess>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSignalBlocker>
#include <Aero7Qt/glassframe.h>
#include <Aero7Qt/navigationbuttons.h>
#include <algorithm>
#include "Categories.h"
#include "Branding.h"
#include "PageId.h"
#include "PageRegistry.h"
#include "ControlPanelItems.h"
#include "SettingsCatalog.h"
#include "FeatureCatalog.h"
#include "SettingsHubPage.h"
#include "Commands.h"
#include "pages/LinuxUpdatePage.h"
#include "pages/SystemPage.h"
#include "pages/InstalledUpdatesPage.h"
#include "pages/ProgramsFeaturesPage.h"
#include "pages/NetworkSharingPage.h"
#include "pages/FirewallPage.h"
#include "pages/ActionCenterPage.h"
#include "pages/FeatureRequiredPage.h"
#include "pages/ParentalControlsPage.h"
#include "pages/PowerOptionsPage.h"
#include "pages/PerformancePage.h"
#include "dialogs/SoundDialog.h"
#include "pages/PersonalizationPage.h"
#include "pages/TaskbarStartMenuPage.h"
#include "pages/FolderOptionsPage.h"
#include "pages/GettingStartedPage.h"
#include "pages/FontsPage.h"
#include "pages/UserAccountsPage.h"
#include "pages/EaseOfAccessPage.h"
#include "pages/DevicesAndPrintersPage.h"
#include "pages/DisplayPage.h"
#include "pages/DefaultProgramsPage.h"
#include "dialogs/DateTimeDialog.h"
#include "dialogs/LinverConfigDialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Title the main window explicitly rather than via applicationDisplayName,
    // which Qt would otherwise append to every dialog title ("Foo - Control
    // Panel"). With no display name set, dialogs show just their own title.
    setWindowTitle("Control Panel");
    setMinimumSize(700, 520);
    resize(1000, 650);

    m_navSound.setSource(QUrl::fromLocalFile(
        "/usr/share/sounds/Aero7/og/Aero7 Navigation Start.wav"));
    m_navSound.setVolume(1.0f);

    // Hide menu bar (like real Windows 7 Control Panel)
    menuBar()->hide();

    // Crumb bar: Aero glass top inset
    buildCrumbBar();

    // Central container
    auto *central = new QWidget;
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    // The original AeroQt helper inserted this widget into its custom frame.
    // Aero7Qt only styles the region, so keep the real header in our own
    // layout. This also makes the breadcrumb/search bar reliable on Wayland.
    mainLayout->addWidget(m_crumbBar);

    m_scroll = new QScrollArea;
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setStyleSheet("QScrollArea { background: #FFFFFF; }");
    mainLayout->addWidget(m_scroll, 1);

    navigateHome();

    setCentralWidget(central);
    statusBar()->hide();

    // Aero glass on crumb bar: must be after setCentralWidget
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    Aero7::applyGlassFrame(this, central, m_crumbBar, nullptr);

    // Intercept mouse side-button events from any child widget.
    qApp->installEventFilter(this);
}

void MainWindow::openPage(PageId pageId)
{
    if (pageId == PageId::Home) {
        navigateHome();
        return;
    }

    const QString path = PageRegistry::pathFor(pageId);
    if (!path.isEmpty())
        navigateTo(path);
}

void MainWindow::buildCrumbBar()
{
    m_crumbBar = new QWidget;
    m_crumbBar->setFixedHeight(34);
    auto *layout = new QHBoxLayout(m_crumbBar);
    layout->setContentsMargins(4, 3, 6, 3);
    layout->setSpacing(2);

    // Aero-style back/forward nav buttons
    auto *navBtns = new Aero7::NavigationButtons(m_crumbBar);
    m_backBtn = navBtns->back();
    m_forwardBtn = navBtns->forward();
    m_backBtn->setEnabled(false);
    m_forwardBtn->setEnabled(false);
    QObject::connect(m_backBtn, &QPushButton::clicked, this, &MainWindow::goBack);
    QObject::connect(m_forwardBtn, &QPushButton::clicked, this, &MainWindow::goForward);
    layout->addWidget(navBtns);
    layout->addSpacing(4);

    // Address path box: icon + crumb trail. The clean-room Aero7Qt layer does
    // not ship bitmap field textures, so draw the compact glass-like field
    // directly and keep the rule scoped so child labels are unaffected.
    auto *pathBox = new QWidget;
    pathBox->setObjectName("addressBox");
    pathBox->setStyleSheet(
        "#addressBox { background: rgba(255,255,255,235); border: 1px solid #7f9db9; border-radius: 2px; }"
        "#addressBox:hover { border-color: #3c7fb1; }"
    );
    pathBox->setFixedHeight(24);
    m_pathLayout = new QHBoxLayout(pathBox);
    m_pathLayout->setContentsMargins(4, 0, 4, 0);
    m_pathLayout->setSpacing(4);

    layout->addWidget(pathBox, 1);
    layout->addSpacing(6);

    // Search box. The crumb bar already carries _Aero_transpbg=true (set by
    // applyGlassFrame), so Aero7Qt's stylesheet gives plain QLineEdits a
    // translucent glass background that brightens on hover. We must NOT set our
    // own border/background or it overrides that border-image. On focus we swap
    // to a solid white field (Aero7Qt has no focus-state glass image).
    m_searchBox = new QLineEdit;
    m_searchBox->setPlaceholderText("Search Control Panel");
    m_searchBox->setFixedWidth(190);
    m_searchBox->setFixedHeight(24);
    m_searchBox->setClearButtonEnabled(false);
    m_searchBox->addAction(themeIcon({"system-search"}), QLineEdit::TrailingPosition);

    // Italic only while the placeholder shows; upright once the user types.
    QObject::connect(m_searchBox, &QLineEdit::textChanged, m_searchBox,
        [this](const QString &text) {
            QFont f = m_searchBox->font();
            f.setItalic(text.isEmpty());
            m_searchBox->setFont(f);
        });
    QObject::connect(m_searchBox, &QLineEdit::textChanged, this,
                     &MainWindow::updateSearchResults);
    QFont searchFont = m_searchBox->font();
    searchFont.setItalic(true);
    m_searchBox->setFont(searchFont);

    QObject::connect(qApp, &QApplication::focusChanged, m_searchBox,
        [this](QWidget *, QWidget *now) {
            if (now == m_searchBox) {
                // Solid field while typing
                m_searchBox->setStyleSheet(
                    "QLineEdit { border: 1px solid #7EB4EA; border-radius: 2px;"
                    " background: #FFFFFF; color: #000000; }"
                );
            } else {
                // Fall back to Aero7Qt's translucent glass entry
                m_searchBox->setStyleSheet(QString());
            }
        });

    layout->addWidget(m_searchBox);
}

void MainWindow::setCrumbTrail(const QStringList &trail)
{
    // Clear existing crumb widgets.
    while (QLayoutItem *item = m_pathLayout->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }

    auto addArrow = [this]() {
        m_pathLayout->addWidget(
            Win7::arrowLabel(Qt::RightArrow, QColor(0x66, 0x66, 0x66), 6));
    };

    auto *controlIcon = new QLabel;
    controlIcon->setPixmap(themeIcon({"preferences-system"}).pixmap(16, 16));
    m_pathLayout->addWidget(controlIcon);
    addArrow();

    // "Control Panel" is always a link back to the home view.
    m_homeCrumb = new QLabel("Control Panel");
    m_homeCrumb->setObjectName("crumbHome");
    m_homeCrumb->setCursor(Qt::PointingHandCursor);
    m_homeCrumb->setStyleSheet(
        "QLabel#crumbHome { color: #000000; }"
        "QLabel#crumbHome:hover { color: #003399; }"
    );
    m_homeCrumb->installEventFilter(this);
    m_pathLayout->addWidget(m_homeCrumb);

    // Intermediate segments are clickable links; the last segment is plain text.
    for (int i = 0; i < trail.size(); ++i) {
        addArrow();
        bool isLast = (i == trail.size() - 1);
        // Display the branded segment; navigation still uses the raw path below.
        auto *label = new QLabel(Branding::brand(trail[i]));
        if (isLast) {
            label->setStyleSheet("color: #000000;");
        } else {
            // Clickable intermediate crumb: navigates to that category page.
            label->setCursor(Qt::PointingHandCursor);
            label->setStyleSheet(
                "QLabel { color: #000000; }"
                "QLabel:hover { color: #003399; }"
            );
            label->installEventFilter(this);
            // Build the partial path up to this segment. "All Control Panel
            // Items" is the home/root view rather than a routable category,
            // so store an empty path for that Windows 7-style breadcrumb.
            const QString partialPath = trail.mid(0, i + 1).join('/');
            m_crumbNavLinks.insert(
                label,
                partialPath == QStringLiteral("All Control Panel Items")
                    ? QString()
                    : partialPath);
        }
        m_pathLayout->addWidget(label);
    }

    m_pathLayout->addStretch();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Mouse side buttons: XButton1 = back, XButton2 = forward.
    if (event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::XButton1) { goBack();    return true; }
        if (me->button() == Qt::XButton2) { goForward(); return true; }
    }

    // Qt's QLabel ignores `text-decoration` in style sheets, so underline-on-
    // hover has to be driven by toggling the font's underline flag directly.
    // Only applies to labels explicitly registered as interactive links; the
    // global qApp filter would otherwise underline every QLabel in the app.
    if (auto *label = qobject_cast<QLabel *>(watched)) {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            if (label == m_homeCrumb || label == m_checkUpdatesLabel
                || m_navLinks.contains(watched)
                || m_subpageLinks.contains(watched)
                || m_commandLinks.contains(watched)
                || m_appletLinks.contains(watched)
                || m_featureLinks.contains(watched)
                || m_crumbNavLinks.contains(watched))
            {
                QFont font = label->font();
                font.setUnderline(event->type() == QEvent::Enter);
                label->setFont(font);
            }
        }
    }

    // "Control Panel" crumb: a press returns home. Press (not release) because a
    // plain QLabel doesn't accept the press, so the release can be routed away
    // and the click lost. Queued because navigating rebuilds the crumb trail,
    // deleting this very label while its event is still on the stack.
    if (watched == m_homeCrumb && event->type() == QEvent::MouseButtonPress) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton)
            QMetaObject::invokeMethod(this, [this]() { navigateHome(); },
                                      Qt::QueuedConnection);
        return true;
    }

    // Intermediate crumb segments navigate on press for the same reason as the
    // home crumb above: a plain QLabel doesn't accept the press, so its parent
    // grabs the mouse and the *release* is delivered there instead of to the
    // label; handling release here loses the click intermittently. Queued
    // because navigating rebuilds the crumb trail, deleting this label while its
    // event is still on the stack.
    if (event->type() == QEvent::MouseButtonPress) {
        auto crumbIt = m_crumbNavLinks.constFind(watched);
        if (crumbIt != m_crumbNavLinks.constEnd()) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                const QString path = crumbIt.value();
                QMetaObject::invokeMethod(this,
                    [this, path]() {
                        if (path.isEmpty()) navigateHome();
                        else                navigateTo(path);
                    },
                    Qt::QueuedConnection);
            }
            return true;
        }
    }

    // "Check for updates" sidebar link on the Linux Update page.
    if (watched == m_checkUpdatesLabel &&
        event->type() == QEvent::MouseButtonRelease &&
        m_updatePage)
    {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton)
            QMetaObject::invokeMethod(m_updatePage,
                &LinuxUpdatePage::checkForUpdates, Qt::QueuedConnection);
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            // External-launch links (e.g. Desktop Gadgets -> KDE widget panels).
            auto cmdIt = m_commandLinks.constFind(watched);
            if (cmdIt != m_commandLinks.constEnd()) {
                QStringList cmd = cmdIt.value();
                const QString program = cmd.takeFirst();
                // If the target program isn't on PATH, tell the user it's not
                // installed rather than failing silently.
                if (QStandardPaths::findExecutable(program).isEmpty()) {
                    QMessageBox::warning(this, tr("Control Panel"),
                        tr("\"%1\" is not installed.").arg(program));
                } else {
                    QProcess::startDetached(program, cmd);
                }
                return true;
            }
            auto appletIt = m_appletLinks.constFind(watched);
            if (appletIt != m_appletLinks.constEnd()) {
                const QString id = appletIt.value();
                QMetaObject::invokeMethod(this,
                    [this, id]() { openApplet(id); },
                    Qt::QueuedConnection);
                return true;
            }
            auto featureIt = m_featureLinks.constFind(watched);
            if (featureIt != m_featureLinks.constEnd()) {
                const QString id = featureIt.value();
                const PageId fallback = static_cast<PageId>(
                    m_featureFallbackPages.value(
                        watched, static_cast<int>(PageId::Home)));
                QMetaObject::invokeMethod(this,
                    [this, id, fallback]() { openOptionalFeature(id, fallback); },
                    Qt::QueuedConnection);
                return true;
            }
            auto navIt = m_navLinks.constFind(watched);
            if (navIt != m_navLinks.constEnd()) {
                const QString path = navIt.value();
                QMetaObject::invokeMethod(this,
                    [this, path]() { navigateTo(path); },
                    Qt::QueuedConnection);
                return true;
            }
            auto subIt = m_subpageLinks.constFind(watched);
            if (subIt != m_subpageLinks.constEnd()) {
                const QString path = subIt.value();
                // An empty target means "Control Panel Home".
                auto go = [this, path]() {
                    if (path.isEmpty()) navigateHome();
                    else                navigateTo(path);
                };
                if (m_sidebarTextEffect) {
                    auto *anim = new QPropertyAnimation(m_sidebarTextEffect, "opacity");
                    anim->setStartValue(1.0);
                    anim->setEndValue(0.0);
                    anim->setDuration(300);
                    anim->setEasingCurve(QEasingCurve::InCubic);
                    QObject::connect(anim, &QPropertyAnimation::finished, this, go);
                    anim->start(QAbstractAnimation::DeleteWhenStopped);
                } else {
                    QMetaObject::invokeMethod(this, go, Qt::QueuedConnection);
                }
                return true;
            }
            // (Intermediate crumb links are handled on press, above.)
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

// Breadcrumb paths for the pages MainWindow routes on, derived from each page's
// canonical PageId so the strings live in exactly one place (PageRegistry). The
// select-updates entry is Linux Update's in-window "select updates" sub-view.
static const QString kUpdatePath           = PageRegistry::pathFor(PageId::LinuxUpdate);
static const QString kGettingStartedPath   = PageRegistry::pathFor(PageId::GettingStarted);
static const QString kSelectPath           = PageRegistry::pathFor(PageId::SelectUpdates);
static const QString kProgramsFeaturesPath = PageRegistry::pathFor(PageId::ProgramsFeatures);
static const QString kInstalledUpdatesPath = PageRegistry::pathFor(PageId::InstalledUpdates);
static const QString kNetworkSharingPath   = PageRegistry::pathFor(PageId::NetworkSharing);
static const QString kFirewallPath         = PageRegistry::pathFor(PageId::Firewall);
static const QString kActionCenterPath     = PageRegistry::pathFor(PageId::ActionCenter);
static const QString kPowerOptionsPath     = PageRegistry::pathFor(PageId::PowerOptions);
static const QString kPerformancePath      = PageRegistry::pathFor(PageId::Performance);
static const QString kPersonalizationPath  = PageRegistry::pathFor(PageId::Personalization);
static const QString kFontsPath            = PageRegistry::pathFor(PageId::Fonts);
static const QString kUserAccountsPath     = PageRegistry::pathFor(PageId::UserAccounts);
static const QString kParentalControlsPath = PageRegistry::pathFor(PageId::ParentalControls);
static const QString kEaseOfAccessPath     = PageRegistry::pathFor(PageId::EaseOfAccess);
static const QString kDevicesPrintersPath  = PageRegistry::pathFor(PageId::DevicesPrinters);

// A sub-path (one containing '/') is routable only if showEntry knows how to
// render it. Category-level paths are validated separately via detailGroupsFor.
static bool isRoutableSubPath(const QString &path)
{
    return PageRegistry::idForPath(path) != PageId::None
        || path.startsWith(QStringLiteral("All Control Panel Items/Optional Feature/"));
}

void MainWindow::openOptionalFeature(const QString &featureId, PageId installedPage)
{
    const FeatureCatalog catalog = FeatureCatalog::load();
    const FeatureDefinition *feature = catalog.find(featureId);
    if (feature && FeatureCatalog::isEnabled(catalog.status(*feature).state)) {
        if (!feature->launch.isEmpty()
            && feature->launch.first() != QLatin1String("control")) {
            QProcess::startDetached(feature->launch.first(), feature->launch.mid(1));
        } else {
            openPage(installedPage);
        }
        return;
    }
    navigateTo(QStringLiteral("All Control Panel Items/Optional Feature/") + featureId);
}

void MainWindow::openOptionalFeature(const QString &featureId)
{
    PageId installedPage = PageId::Home;
    for (const ControlPanelItem &item : controlPanelItems()) {
        if (item.optionalFeature == featureId) {
            installedPage = item.target.page;
            break;
        }
    }
    if (installedPage == PageId::Home) {
        const FeatureCatalog catalog = FeatureCatalog::load();
        const FeatureDefinition *feature = catalog.find(featureId);
        const QHash<QString, PageId> routes{
            {QStringLiteral("getting-started"), PageId::GettingStarted},
            {QStringLiteral("backup-restore"), PageId::BackupRestore},
            {QStringLiteral("ease-of-access"), PageId::EaseOfAccess},
            {QStringLiteral("network-settings"), PageId::NetworkSettings},
            {QStringLiteral("network-sharing"), PageId::NetworkSharing},
            {QStringLiteral("security-maintenance"), PageId::SecurityMaintenance},
            {QStringLiteral("personalization"), PageId::Personalization},
            {QStringLiteral("parental-controls"), PageId::ParentalControls},
        };
        if (feature)
            installedPage = routes.value(feature->route, PageId::Home);
    }
    openOptionalFeature(featureId, installedPage);
}

void MainWindow::navigateHome()
{
    pushHistory(QString());
    showEntry(QString());
}

bool MainWindow::openSetting(const QString &key)
{
    const SettingDefinition *setting = SettingsCatalog::findByKey(key);
    if (!setting)
        return false;

    const LinkTarget target = SettingsCatalog::targetForSetting(*setting);
    switch (target.kind) {
    case LinkTarget::Home:
        navigateHome();
        return true;
    case LinkTarget::Page:
        openPage(target.page);
        return true;
    case LinkTarget::Applet:
        openApplet(target.applet);
        return true;
    case LinkTarget::Command:
        if (!target.command.isEmpty()) {
            const QString program = target.command.first();
            const QStringList arguments = target.command.mid(1);
            return QProcess::startDetached(program, arguments);
        }
        return false;
    case LinkTarget::Disabled:
    case LinkTarget::None:
        return false;
    }
    return false;
}

void MainWindow::navigateTo(const QString &path)
{
    if (path.contains('/')) {
        // Sub-path: if we can't render it (e.g. an intermediate crumb like
        // "Programs/Programs and Features" with no page of its own), fall back
        // to its parent category page rather than showing a blank pane.
        if (!isRoutableSubPath(path)) {
            const QString category = path.section('/', 0, 0);
            if (detailGroupsFor(category))
                navigateTo(category);
            return;
        }
    } else if (!detailGroupsFor(path)) {
        // Category-level path with no detail page: nothing to show.
        return;
    }
    m_navSound.play();
    pushHistory(path);
    showEntry(path);
}

void MainWindow::openApplet(const QString &id)
{
    // Applet ids are "base[:view]", e.g. "datetime:additional".
    const QString base = id.section(':', 0, 0);
    const QString view = id.section(':', 1, 1);
    m_navSound.play();

    if (base == QLatin1String("datetime")) {
        DateTimeDialog dlg(this);
        if (view == QLatin1String("additional"))
            dlg.showTab(DateTimeDialog::TabAdditionalClocks);
        else if (view == QLatin1String("internet"))
            dlg.showTab(DateTimeDialog::TabInternetTime);
        dlg.exec();
    } else if (base == QLatin1String("sound")) {
        SoundDialog dlg(this);
        if (view == QLatin1String("recording"))
            dlg.showTab(SoundDialog::TabRecording);
        else if (view == QLatin1String("sounds"))
            dlg.showTab(SoundDialog::TabSounds);
        else if (view == QLatin1String("communications"))
            dlg.showTab(SoundDialog::TabCommunications);
        dlg.exec();
    }
}

// Render an entry without touching history. Empty string == home page.
void MainWindow::showEntry(const QString &entry)
{
    // Moving between the status view and the select-updates view reuses the
    // live page so the checked-update state survives; only the crumb trail and
    // the page's internal view switch. Recreating it would reset the checks.
    if ((entry == kUpdatePath || entry == kSelectPath)
        && m_updatePage && m_scroll->widget() == m_updatePage) {
        m_crumbNavLinks.clear();
        setCrumbTrail(entry.split('/'));
        if (entry == kSelectPath) m_updatePage->showSelectView();
        else                      m_updatePage->showStatusView();
        return;
    }

    // The page about to be replaced owns the old nav-link labels.
    m_navLinks.clear();
    m_subpageLinks.clear();
    m_commandLinks.clear();
    m_appletLinks.clear();
    m_featureLinks.clear();
    m_featureFallbackPages.clear();
    m_crumbNavLinks.clear();
    m_sidebarTextEffect  = nullptr;
    m_updatePage         = nullptr;
    m_checkUpdatesLabel  = nullptr;

    if (entry.isEmpty()) {
        setCrumbTrail({});
        m_scroll->setWidget(buildHomePage());
    } else if (entry.contains('/')) {
        // Sub-page path: "Category/SubPage[/SubView]"
        QStringList parts = entry.split('/');
        setCrumbTrail(parts);
        if (entry.startsWith(QStringLiteral("All Control Panel Items/Optional Feature/"))) {
            const QString featureId = entry.section(QLatin1Char('/'), -1);
            const FeatureCatalog catalog = FeatureCatalog::load();
            const FeatureDefinition *feature = catalog.find(featureId);
            if (!feature) {
                auto *error = new QLabel(tr("This optional feature is not defined."));
                error->setAlignment(Qt::AlignCenter);
                m_scroll->setWidget(error);
            } else {
                const FeatureStatus status = catalog.status(*feature);
                auto *required = new FeatureRequiredPage(*feature, status);
                PageId fallback = PageId::Home;
                for (const ControlPanelItem &item : controlPanelItems()) {
                    if (item.optionalFeature == featureId) {
                        fallback = item.target.page;
                        break;
                    }
                }
                QObject::connect(required,
                    &FeatureRequiredPage::optionalFeaturesRequested,
                    this, [](const QString &id) {
                        QProcess::startDetached(
                            QStringLiteral("aero7-optional-features"),
                            {QStringLiteral("--feature"), id});
                    });
                QObject::connect(required, &FeatureRequiredPage::installRequested,
                    this, [this, required, entry, fallback](const QString &id) {
                        auto *installer = new QProcess(required);
                        required->setWaitingForInstaller(true);
                        QObject::connect(installer,
                            qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
                            this, [this, entry, fallback, id](int, QProcess::ExitStatus) {
                                const FeatureCatalog refreshed = FeatureCatalog::load();
                                if (FeatureCatalog::isEnabled(
                                        refreshed.status(id, false).state)) {
                                    openOptionalFeature(id, fallback);
                                } else {
                                    showEntry(entry);
                                }
                            });
                        installer->start(QStringLiteral("aero7-optional-features"),
                                         {QStringLiteral("--install"), id});
                    });
                QObject::connect(required, &FeatureRequiredPage::cancelRequested,
                                 this, &MainWindow::goBack);
                m_scroll->setWidget(required);
            }
        } else if (entry == kGettingStartedPath) {
            auto *gettingStarted = new GettingStartedPage;
            QObject::connect(gettingStarted, &GettingStartedPage::navigateRequested,
                             this, [this](PageId id) {
                                 navigateTo(PageRegistry::pathFor(id));
                             }, Qt::QueuedConnection);
            m_scroll->setWidget(gettingStarted);
        } else if (parts.size() >= 2 && parts[0] == "System and Security"
            && parts[1] == "Linux Update") {
            auto *sidebar = buildSubpageSidebar(
                LinuxUpdatePage::sidebarLinks(),
                LinuxUpdatePage::sidebarSeeAlso());
            auto *wuPage = new LinuxUpdatePage(sidebar);
            m_updatePage = wuPage;
            QObject::connect(wuPage, &LinuxUpdatePage::navigateHomeRequested,
                             this, &MainWindow::navigateHome, Qt::QueuedConnection);
            QObject::connect(wuPage, &LinuxUpdatePage::selectUpdatesRequested,
                             this, [this]() { navigateTo(kSelectPath); },
                             Qt::QueuedConnection);
            QObject::connect(wuPage, &LinuxUpdatePage::selectViewClosed,
                             this, &MainWindow::goBack, Qt::QueuedConnection);
            QObject::connect(wuPage, &QObject::destroyed, this,
                             [this]() { m_updatePage = nullptr; });
            m_scroll->setWidget(wuPage);
            if (entry == kSelectPath) m_updatePage->showSelectView();
        } else if (parts.size() == 2 && parts[0] == "System and Security"
                   && parts[1] == "System") {
            auto *sidebar = buildSubpageSidebar(
                SystemPage::sidebarLinks(),
                SystemPage::sidebarSeeAlso());
            auto *sysPage = new SystemPage(sidebar);
            QObject::connect(sysPage, &SystemPage::performanceRequested, this,
                             [this]() { navigateTo(kPerformancePath); },
                             Qt::QueuedConnection);
            // Re-navigate after the branding dialog so the new wording/badge
            // (and every other page reached from here) takes effect live.
            QObject::connect(sysPage, &SystemPage::brandingChanged, this,
                             [this, entry]() { navigateTo(entry); },
                             Qt::QueuedConnection);
            m_scroll->setWidget(sysPage);
        } else if (entry == kPerformancePath) {
            auto *sidebar = buildSubpageSidebar(
                PerformancePage::sidebarLinks(),
                PerformancePage::sidebarSeeAlso());
            m_scroll->setWidget(new PerformancePage(sidebar));
        } else if (entry == kInstalledUpdatesPath) {
            auto *sidebar = buildSubpageSidebar(
                InstalledUpdatesPage::sidebarLinks(),
                InstalledUpdatesPage::sidebarSeeAlso());
            m_scroll->setWidget(new InstalledUpdatesPage(sidebar));
        } else if (entry == kProgramsFeaturesPath) {
            auto *sidebar = buildSubpageSidebar(
                ProgramsFeaturesPage::sidebarLinks(),
                ProgramsFeaturesPage::sidebarSeeAlso());
            m_scroll->setWidget(new ProgramsFeaturesPage(sidebar));
        } else if (entry == kNetworkSharingPath) {
            auto *sidebar = buildSubpageSidebar(
                NetworkSharingPage::sidebarLinks(),
                NetworkSharingPage::sidebarSeeAlso());
            auto *network = new NetworkSharingPage(sidebar);
            QObject::connect(network, &NetworkSharingPage::settingsRequested,
                             this, [this]() {
                                 navigateTo(PageRegistry::pathFor(PageId::NetworkSettings));
                             }, Qt::QueuedConnection);
            m_scroll->setWidget(network);
        } else if (entry == kFirewallPath) {
            auto *sidebar = buildSubpageSidebar(
                FirewallPage::sidebarLinks(),
                FirewallPage::sidebarSeeAlso());
            auto *firewall = new FirewallPage(sidebar);
            QObject::connect(firewall, &FirewallPage::refreshRequested,
                             this, [this, entry]() { showEntry(entry); },
                             Qt::QueuedConnection);
            m_scroll->setWidget(firewall);
        } else if (entry == kActionCenterPath) {
            auto *sidebar = buildSubpageSidebar(
                ActionCenterPage::sidebarLinks(),
                ActionCenterPage::sidebarSeeAlso());
            auto *action = new ActionCenterPage(sidebar);
            QObject::connect(action, &ActionCenterPage::navigateRequested,
                             this, [this](PageId id) {
                                 navigateTo(PageRegistry::pathFor(id));
                             }, Qt::QueuedConnection);
            QObject::connect(action, &ActionCenterPage::optionalFeatureRequested,
                             this, [this](const QString &id, PageId page) {
                                 openOptionalFeature(id, page);
                             }, Qt::QueuedConnection);
            m_scroll->setWidget(action);
        } else if (entry == kPowerOptionsPath) {
            auto *sidebar = buildSubpageSidebar(
                PowerOptionsPage::sidebarLinks(),
                PowerOptionsPage::sidebarSeeAlso());
            m_scroll->setWidget(new PowerOptionsPage(sidebar));
        } else if (entry == kPersonalizationPath) {
            auto *sidebar = buildSubpageSidebar(
                PersonalizationPage::sidebarLinks(),
                PersonalizationPage::sidebarSeeAlso());
            auto *personalization = new PersonalizationPage(sidebar);
            QObject::connect(
                personalization, &PersonalizationPage::soundRequested,
                this, [this]() {
                    SoundDialog dialog(this);
                    dialog.showTab(SoundDialog::TabSounds);
                    dialog.exec();
                });
            m_scroll->setWidget(personalization);
        } else if (entry == kFontsPath) {
            auto *sidebar = buildSubpageSidebar(
                FontsPage::sidebarLinks(),
                FontsPage::sidebarSeeAlso());
            m_scroll->setWidget(new FontsPage(sidebar));
        } else if (entry == kUserAccountsPath) {
            auto *sidebar = buildSubpageSidebar(
                UserAccountsPage::sidebarLinks(),
                UserAccountsPage::sidebarSeeAlso());
            auto *users = new UserAccountsPage(sidebar);
            QObject::connect(users, &UserAccountsPage::navigateRequested,
                             this, [this](PageId id) {
                                 navigateTo(PageRegistry::pathFor(id));
                             }, Qt::QueuedConnection);
            QObject::connect(users, &UserAccountsPage::refreshRequested,
                             this, [this, entry]() { showEntry(entry); },
                             Qt::QueuedConnection);
            m_scroll->setWidget(users);
        } else if (entry == kParentalControlsPath) {
            auto *sidebar = buildSubpageSidebar({
                Nav::to(tr("User Accounts"), PageId::UserAccounts),
            });
            m_scroll->setWidget(new ParentalControlsPage(sidebar));
        } else if (entry == kEaseOfAccessPath) {
            auto *sidebar = buildSubpageSidebar(
                EaseOfAccessPage::sidebarLinks(),
                EaseOfAccessPage::sidebarSeeAlso());
            m_scroll->setWidget(new EaseOfAccessPage(sidebar));
        } else if (entry == kDevicesPrintersPath) {
            // A shell folder, not a Control Panel item: no left-nav sidebar.
            m_scroll->setWidget(new DevicesAndPrintersPage());
        } else {
            // All remaining registered sub-pages are catalog-backed Aero7
            // settings hubs. They replace the System Settings landing page,
            // while individual KDE modules remain compatibility backends until
            // their native Aero7 implementation is complete.
            const PageId pageId = PageRegistry::idForPath(entry);
            switch (pageId) {
            case PageId::DisplaySettings: {
                auto *sidebar = buildSubpageSidebar(
                    DisplayPage::sidebarLinks(), DisplayPage::sidebarSeeAlso());
                m_scroll->setWidget(new DisplayPage(sidebar));
                break;
            }
            case PageId::TaskbarStartMenu: {
                auto *sidebar = buildSubpageSidebar({});
                m_scroll->setWidget(new TaskbarStartMenuPage(sidebar));
                break;
            }
            case PageId::FolderOptions: {
                auto *sidebar = buildSubpageSidebar({});
                m_scroll->setWidget(new FolderOptionsPage(sidebar));
                break;
            }
            case PageId::DefaultPrograms: {
                auto *sidebar = buildSubpageSidebar({});
                m_scroll->setWidget(new DefaultProgramsPage(sidebar));
                break;
            }
            case PageId::NetworkSettings:
            case PageId::RegionLanguage:
            case PageId::InputDevices:
            case PageId::StartupShutdown:
            case PageId::WindowBehavior:
            case PageId::SecurityMaintenance:
            case PageId::StorageAdministration:
            case PageId::InternetOptions:
            case PageId::AutoPlay:
            case PageId::BackupRestore: {
                auto *sidebar = buildSubpageSidebar({});
                QString titleOverride;
                QString descriptionOverride;
                if (pageId == PageId::InternetOptions) {
                    titleOverride = "Internet Options";
                    descriptionOverride =
                        "Configure connections and Internet access. Browser-specific "
                        "home pages, add-ons and history remain in each browser.";
                } else if (pageId == PageId::FolderOptions) {
                    titleOverride = "Folder Options";
                    descriptionOverride =
                        "Configure personal folders, file search, associations and recent items.";
                } else if (pageId == PageId::AutoPlay) {
                    titleOverride = "AutoPlay";
                    descriptionOverride =
                        "Choose default applications and actions for removable media and devices.";
                } else if (pageId == PageId::BackupRestore) {
                    titleOverride = "Backup and Restore";
                    descriptionOverride =
                        "Review storage and automatic mounting. A dedicated backup engine is not installed yet.";
                }
                auto *hub = new SettingsHubPage(
                    SettingsCatalog::sectionForPage(pageId), sidebar,
                    titleOverride, descriptionOverride);
                QObject::connect(hub, &SettingsHubPage::navigateRequested,
                                 this, [this](PageId id) {
                                     navigateTo(PageRegistry::pathFor(id));
                                 }, Qt::QueuedConnection);
                QObject::connect(hub, &SettingsHubPage::appletRequested,
                                 this, [this](const QString &id) {
                                     openApplet(id);
                                 }, Qt::QueuedConnection);
                m_scroll->setWidget(hub);
                break;
            }
            default:
                break;
            }
        }
    } else {
        setCrumbTrail({ entry });
        m_scroll->setWidget(buildCategoryPage(entry));
    }
}

void MainWindow::pushHistory(const QString &entry)
{
    // Re-navigating to the current location shouldn't grow history.
    if (m_historyIndex >= 0 && m_history.at(m_historyIndex) == entry)
        return;
    // Drop any forward entries before branching to a new location.
    while (m_history.size() > m_historyIndex + 1)
        m_history.removeLast();
    m_history.append(entry);
    m_historyIndex = m_history.size() - 1;
    updateNavButtons();
}

void MainWindow::goBack()
{
    if (m_historyIndex <= 0)
        return;
    --m_historyIndex;
    showEntry(m_history.at(m_historyIndex));
    updateNavButtons();
}

void MainWindow::goForward()
{
    if (m_historyIndex >= m_history.size() - 1)
        return;
    ++m_historyIndex;
    showEntry(m_history.at(m_historyIndex));
    updateNavButtons();
}

void MainWindow::updateNavButtons()
{
    if (m_backBtn)
        m_backBtn->setEnabled(m_historyIndex > 0);
    if (m_forwardBtn)
        m_forwardBtn->setEnabled(m_historyIndex < m_history.size() - 1);
}

QWidget *MainWindow::buildHomePage()
{
    // Outer: full-width white, centers the inner column
    auto *content = new QWidget;
    content->setObjectName("homeContent");
    content->setStyleSheet("#homeContent { background: #FFFFFF; }");
    auto *outerH = new QHBoxLayout(content);
    outerH->setContentsMargins(0, 0, 0, 0);
    outerH->setSpacing(0);

    // Category view is a centred two-column list. "All Control Panel Items"
    // instead fills the available width, as the reference VM does.
    auto *inner = new QWidget;
    inner->setStyleSheet("background: transparent;");
    if (m_viewMode == ViewMode::Category) {
        inner->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        outerH->addStretch(1);
        outerH->addWidget(inner, 0, Qt::AlignTop);
        outerH->addStretch(1);
    } else {
        setCrumbTrail({QStringLiteral("All Control Panel Items")});
        outerH->setContentsMargins(16, 0, 16, 0);
        inner->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        outerH->addWidget(inner, 1, Qt::AlignTop);
    }

    auto *innerV = new QVBoxLayout(inner);
    innerV->setContentsMargins(0, 15, 0, 10);
    innerV->setSpacing(0);

    // Heading row: "Adjust your computer's settings"  |  View by: Category
    auto *headRow = new QHBoxLayout;
    headRow->setContentsMargins(0, 0, 0, 22);

    auto *settingsLabel = new QLabel("Adjust your computer's settings");
    QFont f = settingsLabel->font();
    f.setPointSize(12);
    settingsLabel->setFont(f);
    settingsLabel->setStyleSheet("color: #003399;");
    headRow->addWidget(settingsLabel);
    headRow->addStretch();

    auto *viewByLabel = new QLabel("View by:");
    viewByLabel->setStyleSheet("color: #333333; font-size: 9pt;");
    headRow->addWidget(viewByLabel);

    // The Win7 "View by:" dropdown: blue text ending in the shared Aero arrow,
    // orange on hover, opening its menu on click.
    const QString viewText = m_viewMode == ViewMode::Category
        ? QStringLiteral("Category")
        : m_viewMode == ViewMode::LargeIcons
            ? QStringLiteral("Large icons") : QStringLiteral("Small icons");
    auto *categoryBtn = new Win7::MenuButton(viewText);
    auto *viewMenu = new QMenu(categoryBtn);
    viewMenu->setStyleSheet(
        "QMenu { background: #ffffff; color: #111111; border: 1px solid #8e9dac; padding: 2px; }"
        "QMenu::item { padding: 4px 24px 4px 22px; }"
        "QMenu::item:selected { background: #dcecff; color: #111111; border: 1px solid #84acdd; }"
    );
    auto setView = [this](ViewMode mode) {
        if (m_viewMode == mode)
            return;
        m_viewMode = mode;
        showEntry(QString());
    };
    QObject::connect(viewMenu->addAction("Category"), &QAction::triggered,
                     this, [setView]() { setView(ViewMode::Category); });
    QObject::connect(viewMenu->addAction("Large icons"), &QAction::triggered,
                     this, [setView]() { setView(ViewMode::LargeIcons); });
    QObject::connect(viewMenu->addAction("Small icons"), &QAction::triggered,
                     this, [setView]() { setView(ViewMode::SmallIcons); });
    viewMenu->addSeparator();
    auto *namingAction = viewMenu->addAction(
        Branding::useWindowsNames()
            ? QStringLiteral("Use KDE Plasma names...")
            : QStringLiteral("Use Windows 7 names..."));
    namingAction->setObjectName(QStringLiteral("control-panel-naming-action"));
    QObject::connect(namingAction, &QAction::triggered, this, [this]() {
        LinverConfigDialog dialog(QStringLiteral("Aero7 Linux"), this);
        if (dialog.exec() == QDialog::Accepted)
            showEntry(QString());
    });
    categoryBtn->setMenu(viewMenu);
    headRow->addWidget(categoryBtn);

    innerV->addLayout(headRow);

    if (m_viewMode != ViewMode::Category) {
        // This public inventory mirrors Windows 7's 45 applets. The internal
        // settings catalog remains available to Start/search, but is not the
        // UI presented as "All Control Panel Items".
        auto *gridWidget = new QWidget;
        gridWidget->setStyleSheet("background: transparent;");
        auto *grid = new QGridLayout(gridWidget);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(20);
        grid->setVerticalSpacing(m_viewMode == ViewMode::LargeIcons ? 14 : 8);

        const QList<ControlPanelItem> &entries = controlPanelItems();
        const int columns = 5;
        const int rows = (entries.size() + columns - 1) / columns;
        const int iconSize = m_viewMode == ViewMode::LargeIcons ? 32 : 16;
        for (int i = 0; i < entries.size(); ++i) {
            const ControlPanelItem setting = entries.at(i);
            auto *cell = new QWidget;
            cell->setStyleSheet("background: transparent;");
            cell->setMinimumWidth(155);
            auto *h = new QHBoxLayout(cell);
            h->setContentsMargins(2, 3, 2, 3);
            h->setSpacing(8);
            auto *icon = new QLabel;
            icon->setFixedSize(iconSize, iconSize);
            icon->setPixmap(resolveIcon(setting.iconName).pixmap(iconSize, iconSize));
            icon->setStyleSheet("background: transparent;");
            h->addWidget(icon, 0, Qt::AlignTop);
            const QString displayName = Branding::useWindowsNames()
                ? setting.windowsName : setting.kdeName;
            auto *label = new QLabel(displayName);
            label->setObjectName(QStringLiteral("all-item-") +
                                 QString(setting.windowsName).toLower().replace(' ', '-'));
            label->setProperty("windowsName", setting.windowsName);
            label->setProperty("kdeName", setting.kdeName);
            label->setToolTip(
                Branding::useWindowsNames()
                    ? QStringLiteral("KDE Plasma name: %1").arg(setting.kdeName)
                    : QStringLiteral("Windows 7 name: %1").arg(setting.windowsName));
            label->setWordWrap(true);
            label->setCursor(Qt::PointingHandCursor);
            label->setStyleSheet(
                "QLabel { color: #087f23; background: transparent; }"
                "QLabel:hover { color: #005c16; text-decoration: underline; }");
            label->installEventFilter(this);
            if (!setting.optionalFeature.isEmpty()) {
                m_featureLinks.insert(label, setting.optionalFeature);
                m_featureFallbackPages.insert(
                    label, static_cast<int>(setting.target.page));
                const FeatureCatalog catalog = FeatureCatalog::load();
                const FeatureStatus status = catalog.status(
                    setting.optionalFeature, false);
                if (!FeatureCatalog::isEnabled(status.state)) {
                    label->setToolTip(
                        label->toolTip() + QStringLiteral("\nOptional feature — ")
                        + FeatureCatalog::stateName(status.state));
                }
            } else {
                registerLinkTarget(label, setting.target);
            }
            h->addWidget(label, 1, Qt::AlignVCenter);
            // Windows 7 lays its alphabetic list across each five-item row.
            grid->addWidget(cell, i / columns, i % columns);
        }
        for (int c = 0; c < columns; ++c)
            grid->setColumnStretch(c, 1);
        innerV->addWidget(gridWidget);
        innerV->addStretch(1);
        return content;
    }

    // Two independent columns. The real Control Panel stacks each column as its
    // own list with even gaps; it does NOT align rows across the two columns, so
    // a short item never gets padded to match a tall item beside it.
    auto *columnsWidget = new QWidget;
    columnsWidget->setStyleSheet("background: transparent;");
    auto *columns = new QHBoxLayout(columnsWidget);
    columns->setContentsMargins(0, 0, 0, 0);
    columns->setSpacing(28);

    auto *colLeft = new QVBoxLayout;
    colLeft->setContentsMargins(0, 0, 0, 0);
    colLeft->setSpacing(0);
    auto *colRight = new QVBoxLayout;
    colRight->setContentsMargins(0, 0, 0, 0);
    colRight->setSpacing(0);

    const QList<CategoryItem> &categories = homeCategories();
    for (int i = 0; i < categories.size(); ++i) {
        auto *w = new CategoryWidget(categories[i]);
        // Queued: the swap deletes this widget, so it must not run while the
        // widget's own click event is still on the call stack.
        QObject::connect(w, &CategoryWidget::titleActivated,
                         this, &MainWindow::navigateTo, Qt::QueuedConnection);
        // Task sub-links: known tasks deep-link to their dedicated page; any
        // other task falls back to its parent category page.
        QObject::connect(w, &CategoryWidget::taskActivated, this,
            [this](const QString &category, const QString &task) {
                static const QHash<QString, QString> knownTaskPaths = {
                    { "Uninstall a program",           kProgramsFeaturesPath },
                    { "View network status and tasks", kNetworkSharingPath },
                    { "Add or remove user accounts",   kUserAccountsPath },
                    { "Change the theme",              kPersonalizationPath },
                    { "Let Linux suggest settings",    kEaseOfAccessPath },
                    { "Optimize visual display",       kEaseOfAccessPath },
                };
                const auto it = knownTaskPaths.constFind(task);
                if (it != knownTaskPaths.constEnd()) {
                    navigateTo(it.value());
                    return;
                }
                // This home-page task is an external administrative dialog,
                // not a Control Panel page.  It used to fall through to the
                // legacy settings lookup and silently do nothing.
                if (task == QLatin1String("Turn Aero7 features on or off")
                    || task == QLatin1String("Turn Linux features on or off")) {
                    QProcess::startDetached(kOptionalFeaturesCmd.first(),
                                            kOptionalFeaturesCmd.mid(1));
                    return;
                }
                const LinkTarget target =
                    SettingsCatalog::targetForLegacyLabel(task);
                switch (target.kind) {
                case LinkTarget::Page:
                    navigateTo(PageRegistry::pathFor(target.page));
                    break;
                case LinkTarget::Applet:
                    openApplet(target.applet);
                    break;
                case LinkTarget::Command:
                    launchDetached(this, target.command);
                    break;
                default:
                    navigateTo(category);
                    break;
                }
            }, Qt::QueuedConnection);
        (i % 2 == 0 ? colLeft : colRight)->addWidget(w);
    }
    colLeft->addStretch(1);
    colRight->addStretch(1);

    columns->addLayout(colLeft);
    columns->addLayout(colRight);

    innerV->addWidget(columnsWidget);
    innerV->addStretch(1);

    return content;
}

QWidget *MainWindow::buildSearchResultsPage(const QString &query)
{
    auto *page = new QWidget;
    page->setObjectName("searchResultsPage");
    page->setStyleSheet("#searchResultsPage { background: #FFFFFF; }");

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(42, 26, 42, 26);
    layout->setSpacing(18);

    auto *title = Win7::pageTitle(
        tr("Search results for: %1").arg(query.trimmed()), 12, "#003399");
    layout->addWidget(title);
    layout->addWidget(Win7::hairline());

    const QString needle = query.trimmed();
    const QStringList gettingStartedTerms = {
        QStringLiteral("Getting Started"),
        QStringLiteral("Aero7 welcome setup essentials help")
    };
    bool matchesGettingStarted = false;
    for (const QString &term : gettingStartedTerms) {
        if (term.contains(needle, Qt::CaseInsensitive)) {
            matchesGettingStarted = true;
            break;
        }
    }

    bool foundAny = false;
    if (matchesGettingStarted) {
        foundAny = true;
        auto *row = new QWidget;
        row->setStyleSheet("background: transparent;");
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(8, 4, 8, 4);
        rowLayout->setSpacing(14);

        auto *icon = new QLabel;
        icon->setPixmap(themeIcon({"help-about", "dialog-information",
                                   "preferences-system"}).pixmap(40, 40));
        icon->setFixedSize(40, 40);
        rowLayout->addWidget(icon, 0, Qt::AlignTop);

        auto *text = new QVBoxLayout;
        text->setContentsMargins(0, 0, 0, 0);
        text->setSpacing(2);
        auto *link = new LinkLabel(tr("Getting Started"));
        Win7::setPointSize(link, 11);
        QObject::connect(link, &LinkLabel::clicked, this, [this]() {
            QMetaObject::invokeMethod(this, [this]() {
                {
                    const QSignalBlocker blocker(m_searchBox);
                    m_searchBox->clear();
                    QFont font = m_searchBox->font();
                    font.setItalic(true);
                    m_searchBox->setFont(font);
                }
                navigateTo(kGettingStartedPath);
            }, Qt::QueuedConnection);
        });
        text->addWidget(link);
        text->addWidget(
            Win7::label(tr("Discover Aero7 and set up the essentials."),
                        9, "#333333"));
        rowLayout->addLayout(text, 1);
        layout->addWidget(row);
    }

    for (const SettingDefinition &setting : SettingsCatalog::all()) {
        const QString haystack = setting.aeroName + ' ' + setting.kdeName + ' '
            + setting.kdeModule + ' ' + setting.description;
        if (!haystack.contains(needle, Qt::CaseInsensitive))
            continue;
        foundAny = true;
        auto *row = new QWidget;
        row->setStyleSheet("background: transparent;");
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(8, 4, 8, 4);
        rowLayout->setSpacing(14);
        auto *icon = new QLabel;
        icon->setPixmap(resolveIcon(setting.iconName).pixmap(32, 32));
        icon->setFixedSize(32, 32);
        rowLayout->addWidget(icon, 0, Qt::AlignTop);
        auto *text = new QVBoxLayout;
        text->setContentsMargins(0, 0, 0, 0);
        text->setSpacing(1);
        const QString displayName = Branding::useWindowsNames()
            ? setting.aeroName : setting.kdeName;
        auto *link = new QLabel(displayName);
        link->setObjectName(QStringLiteral("search-result-") + setting.key);
        link->setCursor(Qt::PointingHandCursor);
        link->setStyleSheet(
            "QLabel { color: #1F4E99; background: transparent; }"
            "QLabel:hover { color: #0033AA; text-decoration: underline; }");
        link->installEventFilter(this);
        registerLinkTarget(link, SettingsCatalog::targetForSetting(setting));
        text->addWidget(link);
        const QString alternateName = Branding::useWindowsNames()
            ? QStringLiteral("KDE Plasma name: %1").arg(setting.kdeName)
            : QStringLiteral("Windows 7 name: %1").arg(setting.aeroName);
        text->addWidget(Win7::label(
            QStringLiteral("%1  ·  %2")
                .arg(setting.description, alternateName), 8, "#555555"));
        rowLayout->addLayout(text, 1);
        layout->addWidget(row);
    }

    const FeatureCatalog featureCatalog = FeatureCatalog::load();
    for (const FeatureDefinition &feature : featureCatalog.features()) {
        if (!feature.visibleInOptionalFeatures
            || feature.availabilityMode == QLatin1String("core"))
            continue;
        const QString haystack = feature.name + QLatin1Char(' ')
            + feature.description + QLatin1Char(' ') + feature.category
            + QStringLiteral(" optional feature install remove ") + feature.id;
        if (!haystack.contains(needle, Qt::CaseInsensitive))
            continue;

        foundAny = true;
        auto *row = new QWidget;
        row->setStyleSheet(QStringLiteral("background: transparent;"));
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(8, 4, 8, 4);
        rowLayout->setSpacing(14);
        auto *icon = new QLabel;
        icon->setPixmap(resolveIcon(QStringLiteral("system-software-install")).pixmap(32, 32));
        icon->setFixedSize(32, 32);
        rowLayout->addWidget(icon, 0, Qt::AlignTop);
        auto *text = new QVBoxLayout;
        text->setContentsMargins(0, 0, 0, 0);
        text->setSpacing(1);
        auto *link = new QLabel(feature.name);
        link->setObjectName(QStringLiteral("search-result-optional-") + feature.id);
        link->setCursor(Qt::PointingHandCursor);
        link->setStyleSheet(
            "QLabel { color: #1F4E99; background: transparent; }"
            "QLabel:hover { color: #0033AA; text-decoration: underline; }");
        link->installEventFilter(this);
        PageId fallback = PageId::Home;
        for (const ControlPanelItem &item : controlPanelItems()) {
            if (item.optionalFeature == feature.id) {
                fallback = item.target.page;
                break;
            }
        }
        m_featureLinks.insert(link, feature.id);
        m_featureFallbackPages.insert(link, static_cast<int>(fallback));
        text->addWidget(link);
        const FeatureStatus status = featureCatalog.status(feature, false);
        text->addWidget(Win7::label(
            feature.description + QStringLiteral("  ·  Optional feature — ")
                + FeatureCatalog::stateName(status.state), 8, "#555555"));
        rowLayout->addLayout(text, 1);
        layout->addWidget(row);
    }

    if (!foundAny) {
        layout->addWidget(
            Win7::label(tr("No Control Panel items match your search."),
                        9, "#333333"));
    }

    layout->addStretch();
    return page;
}

void MainWindow::updateSearchResults(const QString &query)
{
    if (query.trimmed().isEmpty()) {
        const QString current = m_historyIndex >= 0
            ? m_history.at(m_historyIndex)
            : QString();
        showEntry(current);
        return;
    }

    m_navLinks.clear();
    m_subpageLinks.clear();
    m_commandLinks.clear();
    m_appletLinks.clear();
    m_featureLinks.clear();
    m_featureFallbackPages.clear();
    m_crumbNavLinks.clear();
    setCrumbTrail({tr("Search Results")});
    m_scroll->setWidget(buildSearchResultsPage(query));
}

MainWindow::Sidebar MainWindow::buildSidebarShell(int initialWidth)
{
    auto *clip = new QScrollArea;
    clip->setFixedWidth(initialWidth);
    clip->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    clip->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    clip->setFrameShape(QFrame::NoFrame);
    clip->setWidgetResizable(true);
    clip->setStyleSheet("QScrollArea { background: transparent; border: none; }");

    auto *pane = new QFrame;
    pane->setObjectName("navPane");
    pane->setFixedWidth(195);
    pane->setStyleSheet(
        "#navPane { background: #F1F4F9; border-right: 1px solid #DCE0E8; }"
    );
    auto *outerV = new QVBoxLayout(pane);
    outerV->setContentsMargins(0, 0, 0, 0);
    outerV->setSpacing(0);

    // Text lives in a child widget so a fade effect touches only the text,
    // never the pane background.
    auto *textWrap = new QWidget;
    textWrap->setStyleSheet("background: transparent;");
    auto *navV = new QVBoxLayout(textWrap);
    navV->setContentsMargins(12, 14, 8, 10);
    navV->setSpacing(0);
    outerV->addWidget(textWrap);

    auto *controlHome = new QPushButton("Control Panel Home");
    controlHome->setCursor(Qt::PointingHandCursor);
    controlHome->setFlat(true);
    controlHome->setStyleSheet(
        // padding-left 12 matches the category links' contentsMargins(12, …) so
        // "Control Panel Home" lines up with the rest of the sidebar options.
        "QPushButton { border: none; background: transparent; color: #000000;"
        " text-align: left; padding: 0 0 0 12; font-size: 9pt; }"
        "QPushButton:hover { color: #0033AA; }"
    );
    QObject::connect(controlHome, &QPushButton::clicked, this,
                     &MainWindow::navigateHome, Qt::QueuedConnection);
    navV->addWidget(controlHome);
    navV->addSpacing(16);

    clip->setWidget(pane);
    return { clip, textWrap, navV };
}

QScrollArea *MainWindow::buildNavSidebar(const QString &currentCategory)
{
    // Width 0: buildCategoryPage animates the clip open to 195px.
    Sidebar bar = buildSidebarShell(0);

    for (const QString &cat : navOrder()) {
        if (cat == currentCategory) {
            auto *row = new QHBoxLayout;
            row->setContentsMargins(0, 3, 0, 3);
            row->setSpacing(4);
            auto *bullet = new QLabel("•");
            bullet->setStyleSheet("color: #000000; font-size: 9pt; background: transparent;");
            row->addWidget(bullet, 0, Qt::AlignTop);
            auto *label = new QLabel(cat);
            QFont bf = label->font();
            bf.setPointSize(9);
            bf.setBold(true);
            label->setFont(bf);
            label->setWordWrap(true);
            label->setStyleSheet("color: #000000; background: transparent;");
            row->addWidget(label, 1);
            bar.navV->addLayout(row);
        } else {
            auto *link = new QLabel(cat);
            QFont lf = link->font();
            lf.setPointSize(9);
            link->setFont(lf);
            link->setWordWrap(true);
            link->setCursor(Qt::PointingHandCursor);
            link->setContentsMargins(12, 3, 0, 3);
            link->setStyleSheet(
                "QLabel { color: #000000; background: transparent; }"
                "QLabel:hover { color: #0033AA; }"
            );
            link->installEventFilter(this);
            m_navLinks.insert(link, cat);
            bar.navV->addWidget(link);
        }
    }
    bar.navV->addStretch(1);

    // Static sidebar (shown at full opacity). The effect is kept so that
    // navigating into a subpage can fade this text out first.
    m_sidebarTextEffect = new QGraphicsOpacityEffect(bar.textWrap);
    bar.textWrap->setGraphicsEffect(m_sidebarTextEffect);
    m_sidebarTextEffect->setOpacity(1.0);

    return bar.clip;
}

// Wire a sidebar label to the destination its link declares. Home and page
// targets share m_subpageLinks (an empty path means "Control Panel Home"),
// commands and applets go to their own maps. Unknown legacy links are visibly
// disabled so the UI never advertises a clickable control that does nothing.
void MainWindow::registerLinkTarget(QLabel *label, const LinkTarget &target)
{
    // Old pages used Nav::plain for unfinished links. Resolve those labels
    // through the central catalog so they cannot remain false affordances.
    const LinkTarget resolved = target.kind == LinkTarget::None
        ? SettingsCatalog::targetForLegacyLabel(label->text()) : target;
    switch (resolved.kind) {
    case LinkTarget::Disabled:
        label->setCursor(Qt::ArrowCursor);
        label->setStyleSheet(
            "QLabel { color: #888888; background: transparent; }");
        label->setToolTip("This setting is not available yet.");
        break;
    case LinkTarget::Home:
        m_subpageLinks.insert(label, QString());
        break;
    case LinkTarget::Page:
        m_subpageLinks.insert(label, PageRegistry::pathFor(resolved.page));
        break;
    case LinkTarget::Command:
        m_commandLinks.insert(label, resolved.command);
        break;
    case LinkTarget::Applet:
        m_appletLinks.insert(label, resolved.applet);
        break;
    case LinkTarget::None:
        label->setCursor(Qt::ArrowCursor);
        label->setStyleSheet(
            "QLabel { color: #888888; background: transparent; }");
        label->setToolTip("This setting has not been connected yet.");
        break;
    }
}

QScrollArea *MainWindow::buildSubpageSidebar(const QList<SidebarLink> &links,
                                              const QList<SidebarLink> &seeAlso)
{
    Sidebar bar = buildSidebarShell(195);

    auto addLink = [&](const SidebarLink &sl) {
        // Show the branded wording, but match routing on the canonical text.
        auto *l = new QLabel(Branding::brand(sl.text));
        QFont f = l->font();
        f.setPointSize(9);
        l->setFont(f);
        l->setWordWrap(true);
        l->setCursor(Qt::PointingHandCursor);
        l->setContentsMargins(12, 3, 0, 3);
        l->setStyleSheet(
            "QLabel { color: #000000; background: transparent; }"
            "QLabel:hover { color: #0033AA; }"
        );
        l->installEventFilter(this);
        if (sl.text == "Check for updates")
            m_checkUpdatesLabel = l;
        // The link already carries its own destination, so there is no title
        // string to reverse-map: just wire the label to the target it declares.
        registerLinkTarget(
            l, sl.target.kind == LinkTarget::None
                   ? SettingsCatalog::targetForLegacyLabel(sl.text)
                   : sl.target);
        bar.navV->addWidget(l);
    };

    for (const SidebarLink &sl : links)
        addLink(sl);

    bar.navV->addStretch(1);

    if (!seeAlso.isEmpty()) {
        auto *sep = new QFrame;
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("color: #B8C4D8;");
        bar.navV->addWidget(sep);
        bar.navV->addSpacing(4);

        auto *seeAlsoLabel = new QLabel("See also");
        QFont f = seeAlsoLabel->font();
        f.setPointSize(8);
        seeAlsoLabel->setFont(f);
        seeAlsoLabel->setContentsMargins(12, 0, 0, 0);
        seeAlsoLabel->setStyleSheet("color: #666666; background: transparent;");
        bar.navV->addWidget(seeAlsoLabel);

        for (const SidebarLink &sl : seeAlso)
            addLink(sl);
    }

    // Windows 7 renders the task links immediately.  Delaying them behind a
    // two-second opacity animation made short sidebars look empty on slower
    // machines, including Programs and Features.
    bar.textWrap->setGraphicsEffect(nullptr);

    return bar.clip;
}

QWidget *MainWindow::buildCategoryPage(const QString &currentCategory)
{
    auto *page = new QWidget;
    page->setObjectName("categoryPage");
    page->setStyleSheet("#categoryPage { background: #FFFFFF; }");
    auto *root = new QHBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *sidebarClip = buildNavSidebar(currentCategory);
    root->addWidget(sidebarClip);

    // ---- Right content pane ----------------------------------------------
    auto *contentWrap = new QWidget;
    contentWrap->setObjectName("categoryContent");
    contentWrap->setStyleSheet("#categoryContent { background: #FFFFFF; }");
    auto *contentV = new QVBoxLayout(contentWrap);
    contentV->setContentsMargins(18, 14, 18, 14);
    contentV->setSpacing(0);

    auto makeSep = []() {
        auto *s = new QLabel("|");
        QFont f = s->font();
        f.setPointSize(9);
        s->setFont(f);
        s->setStyleSheet("color: #B0B0B0;");
        return s;
    };
    auto makeTask = [this](const QString &text) {
        // Show the branded wording, but match routing on the canonical text.
        auto *l = new QLabel(Branding::brand(text));
        QFont f = l->font();
        f.setPointSize(9);
        l->setFont(f);
        l->setCursor(Qt::PointingHandCursor);
        l->setStyleSheet(
            "QLabel { color: #1F4E99; }"
            "QLabel:hover { color: #0033AA; }"
        );
        l->installEventFilter(this);
        // Known task links navigate to their page (others are decorative).
        if (text == "View installed updates")
            m_subpageLinks.insert(l, kInstalledUpdatesPath);
        else if (text == "Review your computer's status and resolve issues")
            m_subpageLinks.insert(l, kActionCenterPath);
        else if (text == "Check firewall status"
                 || text == "Allow a program through Linux Firewall")
            m_subpageLinks.insert(l, kFirewallPath);
        else if (text == "View network status and tasks")
            m_subpageLinks.insert(l, kNetworkSharingPath);
        else if (text == "Choose a power plan")
            m_subpageLinks.insert(l, kPowerOptionsPath);
        else if (text == "Uninstall a program")
            m_subpageLinks.insert(l, kProgramsFeaturesPath);
        else if (text == "Turn Aero7 features on or off"
                 || text == "Turn Linux features on or off")
            m_commandLinks.insert(l, kOptionalFeaturesCmd);
        else if (text == "Add gadgets to the desktop")
            m_commandLinks.insert(l, kWidgetExplorerCmd);
        else if (text == "Get more gadgets online")
            m_commandLinks.insert(l, kGetWidgetsCmd);
        else if (text == "Device Manager")
            m_commandLinks.insert(l, kDeviceManagerCmd);

        // Remaining task links: deep-links into the new detail pages, and
        // launchers that open the matching KDE settings module. Consulted only
        // when the explicit cases above didn't already claim this label.
        // Task links that open an in-app applet dialog (mirroring Windows,
        // which opens a dialog rather than a page for these).
        static const QHash<QString, QString> taskApplet = {
            { "Set the time and date",                       "datetime" },
            { "Change the time zone",                        "datetime" },
            { "Add clocks for different time zones",         "datetime:additional" },
            { "Manage audio devices",                        "sound" },
            { "Adjust system volume",                        "sound" },
            { "Change system sounds",                        "sound:sounds" },
            { "Change sound effects",                        "sound:sounds" },
        };
        static const QHash<QString, QString> taskNav = {
            { "Add a device",                                kDevicesPrintersPath },
            { "Add a printer",                               kDevicesPrintersPath },
            { "Change the theme",                            kPersonalizationPath },
            { "Preview, delete, or show and hide fonts",     kFontsPath },
            { "Change your account picture",                 kUserAccountsPath },
            { "Add or remove user accounts",                 kUserAccountsPath },
            { "Change your Linux Password",                  kUserAccountsPath },
            { "Let Linux suggest settings",                  kEaseOfAccessPath },
            { "Optimize visual display",                     kEaseOfAccessPath },
            { "Adjust screen resolution",                    PageRegistry::pathFor(PageId::DisplaySettings) },
            { "Make text and other items larger or smaller", PageRegistry::pathFor(PageId::DisplaySettings) },
            { "Connect to an external display",              PageRegistry::pathFor(PageId::DisplaySettings) },
            { "Mouse",                                       PageRegistry::pathFor(PageId::InputDevices) },
            { "Change desktop background",                   kPersonalizationPath },
            { "Change window glass colors",                  kPersonalizationPath },
            { "Change screen saver",                         PageRegistry::pathFor(PageId::StartupShutdown) },
            { "Change Font Settings",                        kFontsPath },
            { "Adjust ClearType text",                       kFontsPath },
            { "Set your default programs",                   PageRegistry::pathFor(PageId::DefaultPrograms) },
            { "Make a file type always open in a specific program", PageRegistry::pathFor(PageId::DefaultPrograms) },
            { "Change keyboards or other input methods",     PageRegistry::pathFor(PageId::InputDevices) },
            { "Change display language",                     PageRegistry::pathFor(PageId::RegionLanguage) },
            { "Install or uninstall display languages",      PageRegistry::pathFor(PageId::RegionLanguage) },
            { "Change the date, time, or number format",     PageRegistry::pathFor(PageId::RegionLanguage) },
            { "Change location",                             PageRegistry::pathFor(PageId::RegionLanguage) },
            { "Change how your mouse works",                 PageRegistry::pathFor(PageId::InputDevices) },
            { "Change how your keyboard works",              kEaseOfAccessPath },
            { "Start speech recognition",                    kEaseOfAccessPath },
        };
        if (!m_subpageLinks.contains(l) && !m_commandLinks.contains(l)) {
            const auto appIt = taskApplet.constFind(text);
            if (appIt != taskApplet.constEnd())
                m_appletLinks.insert(l, appIt.value());
            else {
                const auto navIt = taskNav.constFind(text);
                if (navIt != taskNav.constEnd())
                    m_subpageLinks.insert(l, navIt.value());
            }
        }
        if (!m_subpageLinks.contains(l) && !m_commandLinks.contains(l)
            && !m_appletLinks.contains(l)) {
            registerLinkTarget(l, SettingsCatalog::targetForLegacyLabel(text));
        }
        return l;
    };

    for (const DetailGroup &group : *detailGroupsFor(currentCategory)) {
        auto *groupRow = new QHBoxLayout;
        groupRow->setContentsMargins(0, 0, 0, 12);
        groupRow->setSpacing(12);

        auto *iconLabel = new QLabel;
        iconLabel->setPixmap(resolveIcon(group.iconName).pixmap(32, 32));
        iconLabel->setFixedSize(32, 32);
        groupRow->addWidget(iconLabel, 0, Qt::AlignTop);

        auto *textBlock = new QVBoxLayout;
        textBlock->setContentsMargins(0, 0, 0, 0);
        textBlock->setSpacing(2);

        auto *title = new QLabel(Branding::brand(group.title));
        QFont tf = title->font();
        tf.setPointSize(11);
        title->setFont(tf);
        title->setCursor(Qt::PointingHandCursor);
        title->setStyleSheet(
            "QLabel { color: #1E7B1E; }"
            "QLabel:hover { color: #145214; }"
        );
        title->installEventFilter(this);

        // Known subpages: register the title label so clicking navigates there.
        static const QHash<QString, QString> knownSubpages = {
            { "Linux Update",              "System and Security/Linux Update" },
            { "Action Center",             "System and Security/Action Center" },
            { "Linux Firewall",            "System and Security/Linux Firewall" },
            { "System",                    "System and Security/System" },
            { "Programs and Features",     "Programs/Programs and Features" },
            { "Power Options",             "System and Security/Power Options" },
            { "Network and Sharing Center",
              "Network and Internet/Network and Sharing Center" },
            { "Personalization",           kPersonalizationPath },
            { "Fonts",                     kFontsPath },
            { "User Accounts",             kUserAccountsPath },
            { "Ease of Access Center",     kEaseOfAccessPath },
            { "Devices and Printers",      kDevicesPrintersPath },
        };
        if (knownSubpages.contains(group.title))
            m_subpageLinks.insert(title, knownSubpages.value(group.title));
        else if (group.title == "Desktop Gadgets")
            m_commandLinks.insert(title, kWidgetExplorerCmd);
        else if (group.title == "Date and Time")
            m_appletLinks.insert(title, QStringLiteral("datetime"));
        else if (group.title == "Sound")
            m_appletLinks.insert(title, QStringLiteral("sound"));
        else
            registerLinkTarget(
                title, SettingsCatalog::targetForLegacyLabel(group.title));

        textBlock->addWidget(title);

        for (const QStringList &line : group.lines) {
            auto *lineRow = new QHBoxLayout;
            lineRow->setContentsMargins(0, 0, 0, 0);
            lineRow->setSpacing(6);
            for (int i = 0; i < line.size(); ++i) {
                if (i > 0)
                    lineRow->addWidget(makeSep());
                lineRow->addWidget(makeTask(line[i]));
            }
            lineRow->addStretch(1);
            textBlock->addLayout(lineRow);
        }

        groupRow->addLayout(textBlock, 1);
        contentV->addLayout(groupRow);
    }

    contentV->addStretch(1);
    root->addWidget(contentWrap, 1);

    // sidebarClip slides from 0 to 195; sidebar inside stays full-width so text
    // never reflows. The content pane rides rightward naturally as the clip grows.
    auto *sidebarAnim = new QVariantAnimation(page);
    sidebarAnim->setStartValue(0);
    sidebarAnim->setEndValue(195);
    sidebarAnim->setDuration(250);
    sidebarAnim->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(sidebarAnim, &QVariantAnimation::valueChanged, sidebarClip,
        [sidebarClip](const QVariant &v) { sidebarClip->setFixedWidth(v.toInt()); });

    // Content fades in over the same span.
    auto *fadeEffect = new QGraphicsOpacityEffect(contentWrap);
    contentWrap->setGraphicsEffect(fadeEffect);
    fadeEffect->setOpacity(0.0);
    auto *contentAnim = new QPropertyAnimation(fadeEffect, "opacity", page);
    contentAnim->setStartValue(0.0);
    contentAnim->setEndValue(1.0);
    contentAnim->setDuration(260);
    contentAnim->setEasingCurve(QEasingCurve::OutCubic);

    QTimer::singleShot(0, page, [sidebarAnim, contentAnim]() {
        sidebarAnim->start();
        contentAnim->start();
    });

    return page;
}
