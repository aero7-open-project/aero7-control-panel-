#include "PowerOptionsPage.h"
#include "KdeSettingsBridge.h"
#include "Win7Ui.h"
#include "LinkLabel.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFont>
#include <QPalette>
#include <QRadioButton>
#include <QButtonGroup>
#include <QToolButton>
#include <QSignalBlocker>
#include <QMouseEvent>
#include <QMessageBox>
#include <QEvent>
#include <QProgressBar>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusVariant>
#include <QDBusArgument>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

// power-profiles-daemon access//
// PPD lives on the system bus. All access goes through the standard
// org.freedesktop.DBus.Properties interface on its single object, so we never
// need PPD-specific introspection to be present.

namespace {
constexpr auto kService   = "net.hadess.PowerProfiles";
constexpr auto kPath      = "/net/hadess/PowerProfiles";
constexpr auto kInterface = "net.hadess.PowerProfiles";

QDBusInterface ppdProperties()
{
    return QDBusInterface(QString::fromLatin1(kService),
                          QString::fromLatin1(kPath),
                          QStringLiteral("org.freedesktop.DBus.Properties"),
                          QDBusConnection::systemBus());
}

QVariant dbusProperty(const QString &service, const QString &path,
                      const QString &interface, const QString &property)
{
    QDBusInterface properties(
        service, path, QStringLiteral("org.freedesktop.DBus.Properties"),
        QDBusConnection::systemBus());
    if (!properties.isValid())
        return {};
    QDBusReply<QVariant> reply =
        properties.call(QStringLiteral("Get"), interface, property);
    return reply.isValid() ? reply.value() : QVariant();
}
} // namespace

QStringList PowerOptionsPage::availableProfiles(bool &ok) const
{
    ok = false;
    QDBusInterface props = ppdProperties();
    if (!props.isValid())
        return {};

    QDBusReply<QVariant> reply =
        props.call(QStringLiteral("Get"), QString::fromLatin1(kInterface),
                   QStringLiteral("Profiles"));
    if (!reply.isValid())
        return {};

    ok = true;

    // Profiles is aa{sv}: an array of string-to-variant maps. Each map carries
    // a "Profile" key with the profile's name ("balanced", etc); the rest
    // (drivers, etc.) we don't need.
    QStringList ids;
    const QDBusArgument arg = reply.value().value<QDBusArgument>();
    arg.beginArray();
    while (!arg.atEnd()) {
        QVariantMap entry;
        arg >> entry;
        const QString id = entry.value(QStringLiteral("Profile")).toString();
        if (!id.isEmpty())
            ids << id;
    }
    arg.endArray();
    return ids;
}

QString PowerOptionsPage::activeProfile() const
{
    QDBusInterface props = ppdProperties();
    if (!props.isValid())
        return {};
    QDBusReply<QVariant> reply =
        props.call(QStringLiteral("Get"), QString::fromLatin1(kInterface),
                   QStringLiteral("ActiveProfile"));
    return reply.isValid() ? reply.value().toString() : QString();
}

void PowerOptionsPage::setActiveProfile(const QString &profileId)
{
    if (profileId.isEmpty() || m_applying)
        return;
    QDBusInterface props = ppdProperties();
    if (!props.isValid()) {
        m_status->setText(tr("Power profile service is unavailable."));
        syncSelection();
        return;
    }
    if (activeProfile() == profileId) {
        m_status->setText(tr("This power plan is already active."));
        return;
    }
    m_applying = true;
    m_status->setText(tr("Applying power plan..."));
    for (QAbstractButton *button : m_group->buttons())
        button->setEnabled(false);
    auto *watcher = new QDBusPendingCallWatcher(
        props.asyncCall(QStringLiteral("Set"), QString::fromLatin1(kInterface),
                        QStringLiteral("ActiveProfile"),
                        QVariant::fromValue(QDBusVariant(profileId))), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, profileId]() {
                const QDBusPendingReply<> reply = *watcher;
                watcher->deleteLater();
                m_applying = false;
                for (QAbstractButton *button : m_group->buttons())
                    button->setEnabled(true);
                const QString actual = activeProfile();
                if (reply.isError() || actual != profileId) {
                    m_status->setText(reply.isError()
                        ? tr("Could not apply the power plan: %1")
                              .arg(reply.error().message())
                        : tr("The power service did not retain the selected plan."));
                    syncSelection();
                    return;
                }
                m_status->setText(tr("Power plan applied."));
                syncSelection();
            });
}

PowerOptionsPage::BatteryInfo PowerOptionsPage::batteryInfo() const
{
    BatteryInfo info;
    constexpr auto service = "org.freedesktop.UPower";
    constexpr auto path = "/org/freedesktop/UPower";
    constexpr auto interface = "org.freedesktop.UPower";
    const QVariant onBattery = dbusProperty(
        QString::fromLatin1(service), QString::fromLatin1(path),
        QString::fromLatin1(interface), QStringLiteral("OnBattery"));
    if (!onBattery.isValid())
        return info;
    info.onBattery = onBattery.toBool();

    QDBusInterface upower(QString::fromLatin1(service), QString::fromLatin1(path),
                          QString::fromLatin1(interface),
                          QDBusConnection::systemBus());
    QDBusReply<QDBusObjectPath> display =
        upower.call(QStringLiteral("GetDisplayDevice"));
    if (!display.isValid() || display.value().path() == QLatin1String("/"))
        return info;

    const QString devicePath = display.value().path();
    const QString deviceInterface = QStringLiteral("org.freedesktop.UPower.Device");
    if (!dbusProperty(QString::fromLatin1(service), devicePath, deviceInterface,
                      QStringLiteral("IsPresent")).toBool())
        return info;
    info.percentage = dbusProperty(
        QString::fromLatin1(service), devicePath, deviceInterface,
        QStringLiteral("Percentage")).toDouble();
    const uint state = dbusProperty(
        QString::fromLatin1(service), devicePath, deviceInterface,
        QStringLiteral("State")).toUInt();
    switch (state) {
    case 1: info.state = QStringLiteral("Charging"); break;
    case 2: info.state = QStringLiteral("Discharging"); break;
    case 4: info.state = QStringLiteral("Fully charged"); break;
    case 5: info.state = QStringLiteral("Pending charge"); break;
    case 6: info.state = QStringLiteral("Pending discharge"); break;
    default: info.state = QStringLiteral("Unknown"); break;
    }
    const qulonglong seconds = dbusProperty(
        QString::fromLatin1(service), devicePath, deviceInterface,
        info.onBattery ? QStringLiteral("TimeToEmpty")
                       : QStringLiteral("TimeToFull")).toULongLong();
    if (seconds > 0) {
        const qulonglong hours = seconds / 3600;
        const qulonglong minutes = (seconds % 3600) / 60;
        info.remaining = QStringLiteral("%1 hr %2 min remaining")
                             .arg(hours).arg(minutes);
    }
    info.model = dbusProperty(
        QString::fromLatin1(service), devicePath, deviceInterface,
        QStringLiteral("Model")).toString();
    info.available = true;
    return info;
}

// Sidebar
QList<SidebarLink> PowerOptionsPage::sidebarLinks()
{
    // "Control Panel Home" is prepended by the sidebar shell itself, so it must
    // not be repeated here.
    return {
        Nav::command("Require a password when the computer wakes",
                     {"kcmshell6", "--caption", "Lock Screen", "kcm_screenlocker"}),
        Nav::command("Choose what the power buttons do",
                     {"kcmshell6", "--caption", "Advanced Power Settings",
                      "kcm_powerdevilprofilesconfig"}),
        Nav::command("Advanced sleep and display settings",
                     {"kcmshell6", "--caption", "Advanced Power Settings",
                      "kcm_powerdevilprofilesconfig"}),
    };
}

QList<SidebarLink> PowerOptionsPage::sidebarSeeAlso()
{
    return {
        Nav::to("Personalization", PageId::Personalization),
        Nav::to("User Accounts", PageId::UserAccounts),
    };
}

// Page
PowerOptionsPage::PowerOptionsPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    bool ok = false;
    QStringList profiles = availableProfiles(ok);
    m_ppdAvailable = ok && !profiles.isEmpty();

    // Fixed-width content column, left-aligned with the rest filled white,
    // matching the Linux Firewall page so this screen sits at the same width
    // as the app's other detail pages.
    auto *contentV = Win7::pageScaffold(this, sidebar, /*bottomMargin=*/20,
                                        /*fixedWidth=*/700);

    // Page title.
    contentV->addWidget(Win7::pageTitle("Select a power plan"));
    contentV->addSpacing(10);

    // Intro paragraph with the trailing "Tell me more…" link. A real <a> gives
    // the underlined, palette-coloured link Windows shows.
    auto *intro = new QLabel(
        "Power plans can help you maximize your computer's performance or "
        "conserve energy. Make a plan active by selecting it, or choose a plan "
        "and customize it by changing its power settings. "
        "<a href=\"#\">Tell me more about power plans</a>");
    {
        QFont f = intro->font();
        f.setPointSize(9);
        intro->setFont(f);
    }
    intro->setWordWrap(true);
    intro->setOpenExternalLinks(false);
    intro->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(intro, &QLabel::linkActivated, this, [this]() {
        QMessageBox::information(this, tr("About power plans"), tr(
            "Balanced adjusts performance and energy use automatically. "
            "Power saver favors lower energy use, while High performance "
            "favors speed when your hardware offers it. Select an available "
            "plan here; use Change plan settings for sleep, display and "
            "power-button controls."));
    });
    {
        QPalette pal = intro->palette();
        pal.setColor(QPalette::Link, QColor("#1F4E99"));
        intro->setPalette(pal);
    }
    intro->setStyleSheet("color: #1A1A1A; background: transparent;");
    contentV->addWidget(intro);
    m_status = Win7::label(
        m_ppdAvailable ? QString()
                       : tr("Power profile switching is unavailable on this computer."),
        9, "#333333");
    m_status->setObjectName(QStringLiteral("powerPlanStatus"));
    contentV->addWidget(m_status);
    contentV->addSpacing(16);

    const BatteryInfo battery = batteryInfo();
    if (battery.available) {
        contentV->addLayout(Win7::sectionHeading("Battery status"));
        contentV->addSpacing(6);
        auto *batteryRow = new QHBoxLayout;
        batteryRow->setContentsMargins(14, 0, 0, 0);
        auto *meter = new QProgressBar;
        meter->setRange(0, 100);
        meter->setValue(qRound(battery.percentage));
        meter->setFormat(QStringLiteral("%1%").arg(qRound(battery.percentage)));
        meter->setFixedWidth(220);
        batteryRow->addWidget(meter);
        auto *details = Win7::label(
            QStringLiteral("%1%2%3")
                .arg(battery.state,
                     battery.remaining.isEmpty()
                         ? QString() : QStringLiteral(" — ") + battery.remaining,
                     battery.model.isEmpty()
                         ? QString() : QStringLiteral("\n") + battery.model),
            9, "#333333");
        details->setWordWrap(true);
        batteryRow->addWidget(details, 1);
        contentV->addLayout(batteryRow);
        contentV->addSpacing(16);
    }

    m_group = new QButtonGroup(this);
    QObject::connect(m_group, &QButtonGroup::buttonClicked, this,
        [this](QAbstractButton *btn) {
            if (!m_syncing)
                setActiveProfile(btn->property("profileId").toString());
        });

    // Preferred plans
    contentV->addLayout(Win7::sectionHeading("Preferred plans"));
    contentV->addSpacing(6);

    m_planList = new QVBoxLayout;
    // Indent the plan rows under the heading, as Windows does.
    m_planList->setContentsMargins(14, 0, 0, 0);
    m_planList->setSpacing(14);
    contentV->addLayout(m_planList);

    // Additional plans (collapsible). Built unconditionally so the layout
    // slots exist; the whole block is hidden below when there are none.
    m_additionalWrap = new QWidget;
    m_additionalWrap->setStyleSheet("background: transparent;");
    m_additionalBox = new QVBoxLayout(m_additionalWrap);
    m_additionalBox->setContentsMargins(14, 0, 0, 0);
    m_additionalBox->setSpacing(14);

    // The shared round Aero chevron toggles the additional-plans block: arrow
    // up when expanded (click to hide), down when collapsed (click to show).
    auto *chevron = new Win7::ChevronButton;

    QLabel *additionalHeadingLabel = nullptr;   // text set by applyExpanded

    contentV->addSpacing(16);

    // The heading row lives in its own host widget so it can be hidden wholesale
    // (rule included) when there are no additional plans.
    m_additionalHeader = new QWidget;
    m_additionalHeader->setStyleSheet("background: transparent;");
    {
        auto *row = Win7::sectionHeading(QString(), chevron,
                                         &additionalHeadingLabel);
        m_additionalHeader->setLayout(row);
    }
    contentV->addWidget(m_additionalHeader);
    contentV->addSpacing(6);
    contentV->addWidget(m_additionalWrap);

    auto applyExpanded = [additionalHeadingLabel, chevron, this](bool expanded) {
        m_additionalWrap->setVisible(expanded);
        additionalHeadingLabel->setText(expanded ? "Hide additional plans"
                                                  : "Show additional plans");
        QSignalBlocker block(chevron);
        chevron->setChecked(expanded);
    };
    QObject::connect(chevron, &QToolButton::toggled, this, applyExpanded);

    contentV->addStretch(1);

    // Populate the plan rows, then settle the expander state.
    buildPlans(m_ppdAvailable ? profiles : QStringList());

    if (m_additionalBox->count() == 0) {
        // No additional plans: hide the whole collapsible block and its heading.
        m_additionalHeader->hide();
        m_additionalWrap->hide();
    } else {
        // Expand by default when the active plan is one of the additional ones,
        // so the current selection is never hidden; otherwise start collapsed.
        const QString active = activeProfile();
        bool activeIsAdditional = false;
        for (int i = 0; i < m_additionalBox->count(); ++i) {
            if (auto *w = m_additionalBox->itemAt(i)->widget())
                if (auto *r = w->findChild<QRadioButton *>())
                    if (r->property("profileId").toString() == active)
                        activeIsAdditional = true;
        }
        applyExpanded(activeIsAdditional);
    }

    syncSelection();

    // Stay in sync if the profile is changed elsewhere (KDE tray, power events).
    if (m_ppdAvailable) {
        QDBusConnection::systemBus().connect(
            QString::fromLatin1(kService), QString::fromLatin1(kPath),
            QStringLiteral("org.freedesktop.DBus.Properties"),
            QStringLiteral("PropertiesChanged"), this,
            SLOT(onProfilesPropertiesChanged(QString, QVariantMap, QStringList)));
    }
}

void PowerOptionsPage::buildPlans(const QStringList &profiles)
{
    // Map a raw PPD profile id onto its Windows-flavoured presentation.
    auto meta = [](const QString &id) -> Plan {
        if (id == QLatin1String("balanced"))
            return { id, QStringLiteral("Balanced (recommended)"),
                     QStringLiteral("Automatically balances performance with "
                                    "energy consumption on capable hardware."),
                     false };
        if (id == QLatin1String("performance"))
            return { id, QStringLiteral("High performance"),
                     QStringLiteral("Favors performance, but may use more "
                                    "energy."),
                     false };
        if (id == QLatin1String("power-saver"))
            return { id, QStringLiteral("Power saver"),
                     QStringLiteral("Saves energy by reducing your computer's "
                                    "performance where possible."),
                     true };
        // Any other profile a custom PPD setup might expose.
        QString pretty = id;
        pretty.replace(QLatin1Char('-'), QLatin1Char(' '));
        if (!pretty.isEmpty())
            pretty[0] = pretty[0].toUpper();
        return { id, pretty,
                 QStringLiteral("A custom power plan provided by your system."),
                 true };
    };

    QList<Plan> plans;

    if (profiles.isEmpty()) {
        // Fall-back: no PPD. Show a single, selected Balanced plan with no
        // profile id (selecting it is a no-op), matching what Windows shows on
        // hardware that exposes no alternative plans.
        plans << Plan{ QString(), QStringLiteral("Balanced (recommended)"),
                       QStringLiteral("Automatically balances performance with "
                                      "energy consumption on capable hardware."),
                       false };
    } else {
        // Preferred plans first, in Windows' order, then the additional ones.
        for (const QString &id : { QStringLiteral("balanced"),
                                   QStringLiteral("performance") })
            if (profiles.contains(id))
                plans << meta(id);
        if (profiles.contains(QLatin1String("power-saver")))
            plans << meta(QStringLiteral("power-saver"));
        for (const QString &id : profiles)
            if (id != QLatin1String("balanced")
                && id != QLatin1String("performance")
                && id != QLatin1String("power-saver"))
                plans << meta(id);
    }

    for (const Plan &plan : plans) {
        QWidget *row = buildPlanRow(plan);
        (plan.additional ? m_additionalBox : m_planList)->addWidget(row);
    }

    // Show the unavailable fall-back plan without presenting it as selectable.
    if (!m_ppdAvailable && !m_group->buttons().isEmpty()) {
        m_group->buttons().first()->setChecked(true);
        m_group->buttons().first()->setEnabled(false);
    }
}

QWidget *PowerOptionsPage::buildPlanRow(const Plan &plan)
{
    // Layout: [radio] [ name-line over description ]. Built from nested box
    // layouts rather than a QGridLayout: a word-wrapped label in a grid cell
    // (especially a column-spanning one) is mis-sized by Qt's height-for-width
    // handling and gets clipped to a single line, which is what truncated the
    // descriptions to "Favors performance, but".
    auto *row = new QWidget;
    row->setStyleSheet("background: transparent;");
    auto *rowH = new QHBoxLayout(row);
    rowH->setContentsMargins(0, 0, 0, 0);
    rowH->setSpacing(6);

    auto *radio = new QRadioButton;
    radio->setProperty("profileId", plan.profileId);
    radio->setCursor(Qt::PointingHandCursor);
    radio->setStyleSheet("QRadioButton { background: transparent; }");
    m_group->addButton(radio);
    rowH->addWidget(radio, 0, Qt::AlignTop);

    // The text block fills the rest of the row width.
    auto *textV = new QVBoxLayout;
    textV->setContentsMargins(0, 0, 0, 0);
    textV->setSpacing(2);

    // Name line: the plan name on the left, "Change plan settings" pinned right.
    auto *nameLine = new QHBoxLayout;
    nameLine->setContentsMargins(0, 0, 0, 0);
    nameLine->setSpacing(6);

    auto *name = new QLabel(plan.name);
    {
        QFont f = name->font();
        f.setPointSize(9);
        // The recommended plan's name is shown bold (a fixed emphasis in
        // Windows, independent of which plan is currently selected).
        f.setBold(plan.name.contains(QLatin1String("(recommended)")));
        name->setFont(f);
    }
    name->setStyleSheet("color: #1A1A1A; background: transparent;");
    name->setCursor(Qt::PointingHandCursor);
    // Clicking the name selects the plan, like the label in Windows.
    m_nameToRadio.insert(name, radio);
    name->installEventFilter(this);
    nameLine->addWidget(name, 0, Qt::AlignVCenter);
    nameLine->addStretch(1);

    auto *change = new LinkLabel("Change plan settings");
    connect(change, &LinkLabel::clicked, this, [this]() {
        KdeSettingsBridge::open(this, QStringLiteral("kcm_powerdevilprofilesconfig"),
                                QStringLiteral("Advanced Power Settings"));
    });
    nameLine->addWidget(change, 0, Qt::AlignVCenter);

    textV->addLayout(nameLine);

    auto *desc = new QLabel(plan.description);
    {
        QFont f = desc->font();
        f.setPointSize(9);
        desc->setFont(f);
    }
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #333333; background: transparent;");
    textV->addWidget(desc);

    rowH->addLayout(textV, 1);

    return row;
}

void PowerOptionsPage::syncSelection()
{
    if (!m_ppdAvailable)
        return;
    const QString active = activeProfile();
    m_syncing = true;
    m_group->setExclusive(false);
    for (QAbstractButton *button : m_group->buttons())
        button->setChecked(false);
    m_group->setExclusive(true);
    if (active.isEmpty()) {
        m_syncing = false;
        return;
    }
    for (QAbstractButton *btn : m_group->buttons()) {
        if (btn->property("profileId").toString() == active) {
            btn->setChecked(true);
            break;
        }
    }
    m_syncing = false;
}

bool PowerOptionsPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        auto it = m_nameToRadio.constFind(watched);
        if (it != m_nameToRadio.constEnd()) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                QRadioButton *radio = it.value();
                if (!radio->isChecked()) {
                    radio->setChecked(true);
                    if (!m_syncing)
                        setActiveProfile(radio->property("profileId").toString());
                }
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void PowerOptionsPage::onProfilesPropertiesChanged(const QString &interface,
                                                   const QVariantMap &changed,
                                                   const QStringList &invalidated)
{
    if (interface != QLatin1String(kInterface))
        return;
    if (changed.contains(QStringLiteral("ActiveProfile"))
        || invalidated.contains(QStringLiteral("ActiveProfile")))
        syncSelection();
}
