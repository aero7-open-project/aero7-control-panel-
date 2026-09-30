#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QEvent>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include "MainWindow.h"
#include "IconHelper.h"
#include "SettingsCatalog.h"
#include "KdeSettingsBridge.h"
#include "FeatureCatalog.h"
#include <Aero7Qt/stylesheet.h>

// Aero7Qt's application stylesheet skins QScrollBar with the Aero look. We want
// the desktop theme's native scroll bars instead, and they cannot be rescued
// per-widget: while an app-wide stylesheet is active, Qt wraps even an
// explicitly setStyle()'d widget back into the stylesheet engine. The only
// real bypass is to remove the QScrollBar rules from the application sheet
// itself, with no rules matching, the stylesheet engine delegates scroll-bar
// rendering to the real Qt style (Breeze on KDE).
static QString withoutScrollBarRules(QString qss)
{
    // Drop comments first so a brace inside one can't derail the block scan.
    static const QRegularExpression comment(
        QStringLiteral(R"(/\*.*?\*/)"),
        QRegularExpression::DotMatchesEverythingOption);
    qss.remove(comment);

    // QSS has no nested braces: walk "selectors { body }" blocks and drop the
    // selectors mentioning QScrollBar, keeping any others sharing the block.
    QString out;
    out.reserve(qss.size());
    int pos = 0;
    while (pos < qss.size()) {
        const int open = qss.indexOf(QLatin1Char('{'), pos);
        const int close = open < 0 ? -1 : qss.indexOf(QLatin1Char('}'), open);
        if (close < 0) {                       // trailing non-block text
            out += QStringView(qss).mid(pos);
            break;
        }

        QStringList kept;
        const QStringList selectors = qss.mid(pos, open - pos).split(QLatin1Char(','));
        for (const QString &sel : selectors) {
            if (!sel.contains(QLatin1String("QScrollBar")))
                kept << sel;
        }
        if (!kept.isEmpty())
            out += kept.join(QLatin1Char(',')) + qss.mid(open, close - open + 1);
        pos = close + 1;
    }
    return out;
}

// Strips the QScrollBar rules now and again whenever they reappear: Aero7Qt
// re-applies its sheet when the desktop theme flips between Aero and
// non-Aero, which reaches us as StyleChange events. The re-strip is deferred
// with a queued single-shot: mutating the application stylesheet while a
// StyleChange is still being delivered would re-enter the style engine, and
// is a no-op once the rules are gone, so it cannot loop.
class ScrollBarUnstyler : public QObject {
public:
    explicit ScrollBarUnstyler(QApplication *app) : QObject(app), m_app(app)
    {
        strip();
        app->installEventFilter(this);
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::StyleChange && !m_pending
            && m_app->styleSheet().contains(QLatin1String("QScrollBar"))) {
            m_pending = true;
            QTimer::singleShot(0, this, [this]() {
                m_pending = false;
                strip();
            });
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void strip()
    {
        const QString qss = m_app->styleSheet();
        const QString filtered = withoutScrollBarRules(qss);
        if (filtered != qss)
            m_app->setStyleSheet(filtered);
    }

    QApplication *m_app;
    bool m_pending = false;
};

static void printSettingsCatalog()
{
    QJsonArray catalog;
    for (const SettingDefinition &setting : SettingsCatalog::all()) {
        catalog.append(QJsonObject{
            {QStringLiteral("key"), setting.key},
            {QStringLiteral("name"), setting.aeroName},
            {QStringLiteral("description"), setting.description},
            {QStringLiteral("icon"), setting.iconName},
            {QStringLiteral("section"),
             SettingsCatalog::sectionTitle(setting.section)},
            {QStringLiteral("backend"), SettingsCatalog::backendLabel(setting)},
            {QStringLiteral("status"), SettingsCatalog::statusLabel(setting.status)},
            {QStringLiteral("keywords"),
             QStringList{setting.kdeName, setting.kdeModule, setting.key}
                 .join(QLatin1Char(' '))},
        });
    }
    const FeatureCatalog features = FeatureCatalog::load();
    for (const FeatureDefinition &feature : features.features()) {
        if (!feature.visibleInOptionalFeatures
            || feature.availabilityMode == QLatin1String("core"))
            continue;
        const FeatureStatus status = features.status(feature);
        const QString state = FeatureCatalog::stateName(status.state);
        catalog.append(QJsonObject{
            {QStringLiteral("key"), QStringLiteral("optional-feature:") + feature.id},
            {QStringLiteral("name"), feature.name},
            {QStringLiteral("description"),
             feature.description + QStringLiteral(" Optional feature — ") + state + QLatin1Char('.')},
            {QStringLiteral("icon"), QStringLiteral("system-software-install")},
            {QStringLiteral("section"), feature.category},
            {QStringLiteral("keywords"),
             QStringLiteral("optional feature install remove ") + feature.id},
            {QStringLiteral("optionalFeature"), feature.id},
            {QStringLiteral("featureState"), state},
        });
    }
    QTextStream(stdout) << QJsonDocument(catalog).toJson(QJsonDocument::Compact)
                        << Qt::endl;
}

int main(int argc, char *argv[]) {
    // The Plasma launcher reads this catalog while the desktop is starting and
    // may not have access to a display server yet. Handle this one read-only
    // command before QApplication tries to initialize a GUI platform plugin.
    for (int index = 1; index < argc; ++index) {
        if (QString::fromLocal8Bit(argv[index]) ==
            QLatin1String("--list-settings-json")) {
            QCoreApplication app(argc, argv);
            printSettingsCatalog();
            return 0;
        }
    }

    QApplication app(argc, argv);
    app.setOrganizationName("controlpanel");
    app.setApplicationName("controlpanel");
    // No setApplicationDisplayName: Qt appends it to every window/dialog title.
    app.setWindowIcon(aero7ResourceIcon(
        QStringLiteral(":/aero7/icons/app/aero7-control-panel.png"),
        QStringLiteral("control-panel")));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Aero7 Control Panel"));
    parser.addHelpOption();
    QCommandLineOption pageOption(
        QStringList{QStringLiteral("p"), QStringLiteral("page")},
        QStringLiteral("Open a Control Panel page directly."),
        QStringLiteral("page"));
    parser.addOption(pageOption);
    QCommandLineOption settingOption(
        QStringList{QStringLiteral("s"), QStringLiteral("setting")},
        QStringLiteral("Open a setting by its stable Aero7 catalog key."),
        QStringLiteral("key"));
    parser.addOption(settingOption);
    QCommandLineOption tabOption(
        QStringLiteral("tab"),
        QStringLiteral("Select a tab on a directly opened properties page."),
        QStringLiteral("tab"));
    parser.addOption(tabOption);
    QCommandLineOption listSettingsOption(
        QStringLiteral("list-settings-json"),
        QStringLiteral("Print the searchable settings catalog as JSON and exit."));
    parser.addOption(listSettingsOption);
    parser.process(app);

    if (parser.isSet(listSettingsOption)) {
        printSettingsCatalog();
        return 0;
    }

    const QString requestedSetting = parser.value(settingOption);
    const bool optionalSetting = requestedSetting.startsWith(
        QStringLiteral("optional-feature:"));
    const QString optionalFeatureId = optionalSetting
        ? requestedSetting.mid(QStringLiteral("optional-feature:").size()) : QString();
    const SettingDefinition *setting = requestedSetting.isEmpty() || optionalSetting
        ? nullptr : SettingsCatalog::findByKey(requestedSetting);
    if (optionalSetting) {
        const FeatureCatalog features = FeatureCatalog::load();
        const FeatureDefinition *feature = features.find(optionalFeatureId);
        if (!feature || !feature->visibleInOptionalFeatures) {
            QTextStream(stderr) << "Unknown Aero7 optional feature: "
                                << optionalFeatureId << Qt::endl;
            return 2;
        }
    }
    if (!requestedSetting.isEmpty() && !setting && !optionalSetting) {
        QTextStream(stderr) << "Unknown Aero7 setting: " << requestedSetting
                            << Qt::endl;
        return 2;
    }

    if (setting) {
        if (setting->backend == SettingsBackend::KdeModuleBridge)
            return KdeSettingsBridge::open(nullptr, setting->kdeModule,
                                           setting->aeroName) ? 0 : 3;
        const LinkTarget target = SettingsCatalog::targetForSetting(*setting);
        if (target.kind == LinkTarget::Command && !target.command.isEmpty()) {
            const QString program = target.command.first();
            const QStringList arguments = target.command.mid(1);
            if (QStandardPaths::findExecutable(program).isEmpty()
                || (program == QStringLiteral("aeroshell-kcmloader")
                    && (arguments.isEmpty()
                        || !QFileInfo::exists(arguments.constFirst())))) {
                QMessageBox::warning(nullptr, setting->aeroName,
                    QStringLiteral("The application needed for %1 is not installed.")
                        .arg(setting->aeroName));
                return 3;
            }
            const bool started = QProcess::startDetached(program, arguments);
            if (!started)
                QMessageBox::warning(nullptr, setting->aeroName,
                    QStringLiteral("Could not open %1.").arg(setting->aeroName));
            return started ? 0 : 3;
        }
    }

    Aero7::applyApplicationStyle(&app);
    new ScrollBarUnstyler(&app);   // native scroll bars; owned by the app

    MainWindow w;
    const QString requestedPage = parser.value(pageOption);
    if (requestedPage.compare(QStringLiteral("getting-started"),
                              Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::GettingStarted);
    } else if (requestedPage.compare(QStringLiteral("devices-and-printers"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::DevicesPrinters);
    } else if (requestedPage.compare(QStringLiteral("taskbar-start-menu"),
                                     Qt::CaseInsensitive) == 0) {
        qputenv("AERO7_TASKBAR_PROPERTIES_TAB",
                parser.value(tabOption).toUtf8());
        w.openPage(PageId::TaskbarStartMenu);
    } else if (requestedPage.compare(QStringLiteral("action-center"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::ActionCenter);
    } else if (requestedPage.compare(QStringLiteral("security-maintenance"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::SecurityMaintenance);
    } else if (requestedPage.compare(QStringLiteral("parental-controls"),
                                     Qt::CaseInsensitive) == 0) {
        w.openOptionalFeature(QStringLiteral("parental-controls"),
                              PageId::ParentalControls);
    } else if (requestedPage.compare(QStringLiteral("programs-features"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::ProgramsFeatures);
    } else if (requestedPage.compare(QStringLiteral("installed-updates"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::InstalledUpdates);
    } else if (requestedPage.compare(QStringLiteral("backup-restore"),
                                     Qt::CaseInsensitive) == 0) {
        w.openOptionalFeature(QStringLiteral("backup-restore"),
                              PageId::BackupRestore);
    } else if (requestedPage.compare(QStringLiteral("ease-of-access"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::EaseOfAccess);
    } else if (requestedPage.compare(QStringLiteral("network-settings"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::NetworkSettings);
    } else if (requestedPage.compare(QStringLiteral("network-sharing"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::NetworkSharing);
    } else if (requestedPage.compare(QStringLiteral("personalization"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::Personalization);
    } else if (requestedPage.compare(QStringLiteral("storage-administration"),
                                     Qt::CaseInsensitive) == 0) {
        w.openPage(PageId::StorageAdministration);
    } else if (requestedPage.compare(QStringLiteral("window-snapping"),
                                     Qt::CaseInsensitive) == 0) {
        // The Aero7 glass frame rebuilds the initial view when the window is
        // first shown. Open this optional page once that first frame exists.
        QTimer::singleShot(0, &w, [&w]() {
            w.openPage(PageId::WindowSnapping);
        });
    }
    w.show();
    if (optionalSetting) {
        QTimer::singleShot(0, &w, [&w, optionalFeatureId]() {
            w.openOptionalFeature(optionalFeatureId);
        });
    } else if (setting) {
        const QString settingKey = setting->key;
        QTimer::singleShot(0, &w, [&w, settingKey]() {
            w.openSetting(settingKey);
        });
    }
    return app.exec();
}
