#include "ActionCenterPage.h"
#include "Commands.h"
#include "ConfFile.h"
#include "IconHelper.h"
#include "Win7Ui.h"
#include "Branding.h"
#include "LinkLabel.h"
#include "FeatureCatalog.h"
#include "FirewallBackend.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFont>
#include <QFile>
#include <QPushButton>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QNetworkInterface>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

using Win7::ClickableWidget;

// Data gathering
ActionCenterPage::AcInfo ActionCenterPage::gatherInfo()
{
    AcInfo ac;

    const auto firewall = FirewallBackend::detect();
    ac.firewallOn = firewall.active;
    ac.firewallName = firewall.name;

    // "Spyware and unwanted software protection" maps to an on-demand scanner.
    // ClamAV is the ubiquitous Linux one; report it when its binary is present.
    if (!QStandardPaths::findExecutable(QStringLiteral("clamscan")).isEmpty()) {
        ac.avPresent = true;
        ac.avName = QStringLiteral("ClamAV");
    }
    const FeatureCatalog features = FeatureCatalog::load();
    ac.avPresent = FeatureCatalog::isEnabled(
        features.status(QStringLiteral("aero7-defender"), false).state);
    if (ac.avPresent)
        ac.avName = QStringLiteral("Aero7 Defender (ClamAV)");
    ac.backupFeaturePresent = FeatureCatalog::isEnabled(
        features.status(QStringLiteral("backup-restore"), false).state);

    // UAC maps to polkit: administrative actions prompt for authentication when
    // a polkit authority is registered on the system bus (it always is on KDE).
    if (auto *iface = QDBusConnection::systemBus().interface())
        ac.uacOn = iface->isServiceRegistered(
            QStringLiteral("org.freedesktop.PolicyKit1"));

    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        const auto flags = interface.flags();
        if (!flags.testFlag(QNetworkInterface::IsLoopBack)
            && flags.testFlag(QNetworkInterface::IsUp)
            && flags.testFlag(QNetworkInterface::IsRunning)) {
            ac.networkUp = true;
            break;
        }
    }

    QProcess services;
    services.start(QStringLiteral("systemctl"),
                   {QStringLiteral("--failed"), QStringLiteral("--no-legend"),
                    QStringLiteral("--plain")});
    if (services.waitForFinished(2500))
        ac.failedServices = QString::fromUtf8(services.readAllStandardOutput())
                                .split(QLatin1Char('\n'), Qt::SkipEmptyParts).size();

    if (!QStandardPaths::findExecutable(QStringLiteral("pacman")).isEmpty()) {
        QProcess updates;
        updates.start(QStringLiteral("pacman"), {QStringLiteral("-Qu")});
        if (updates.waitForFinished(5000))
            ac.updatesAvailable = QString::fromUtf8(updates.readAllStandardOutput())
                                      .split(QLatin1Char('\n'), Qt::SkipEmptyParts).size();
    }

    const QStorageInfo storage = QStorageInfo::root();
    ac.diskLow = storage.isValid() && storage.isReady() && storage.bytesTotal() > 0
        && double(storage.bytesAvailable()) / double(storage.bytesTotal()) < 0.05;
    ac.backupConfigured = QFile::exists(
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/aero7/backup.conf"));

    return ac;
}

// Sidebar
QList<SidebarLink> ActionCenterPage::sidebarLinks()
{
    return {
        Nav::plain("Change Action Center settings"),
        Nav::to("Review administrator approval", PageId::SecurityMaintenance),
        Nav::disabled("View archived messages"),
        Nav::to("View performance information", PageId::Performance),
    };
}

QList<SidebarLink> ActionCenterPage::sidebarSeeAlso()
{
    return {
        Nav::plain("Backup and Restore"),
        Nav::to("Linux Update", PageId::LinuxUpdate),
        Nav::to("Performance Information and Tools", PageId::Performance),
    };
}

using Win7::bodyLabel;

// Status row
QWidget *ActionCenterPage::buildStatusRow(const QString &item,
                                          const QString &state,
                                          const QString &description,
                                          const QString &link)
{
    auto *row = new QWidget;
    row->setStyleSheet("background: transparent;");
    auto *v = new QVBoxLayout(row);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(2);

    // Top line: bold item name on the left, state right-aligned.
    auto *topH = new QHBoxLayout;
    topH->setContentsMargins(0, 0, 0, 0);
    topH->setSpacing(8);

    auto *name = new QLabel(item);
    {
        QFont f = name->font();
        f.setPointSize(9);
        name->setFont(f);
    }
    name->setStyleSheet("color: #000000; background: transparent;");
    topH->addWidget(name, 0, Qt::AlignVCenter);
    topH->addStretch(1);

    auto *stateLabel = new QLabel(state);
    {
        QFont f = stateLabel->font();
        f.setPointSize(9);
        stateLabel->setFont(f);
    }
    stateLabel->setStyleSheet("color: #000000; background: transparent;");
    topH->addWidget(stateLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    v->addLayout(topH);

    // Description sits indented under the item name, as in the reference.
    auto *desc = bodyLabel(description);
    desc->setContentsMargins(18, 0, 0, 0);
    v->addWidget(desc);

    if (!link.isEmpty()) {
        auto *l = new LinkLabel(link);
        l->setContentsMargins(18, 0, 0, 0);
        if (link == QLatin1String("Install Aero7 Defender")) {
            connect(l, &LinkLabel::clicked, this, [this]() {
                emit optionalFeatureRequested(QStringLiteral("aero7-defender"),
                                              PageId::SecurityMaintenance);
            });
        } else {
            const PageId target = link == "Review network settings"
                    ? PageId::NetworkSettings : PageId::SecurityMaintenance;
            connect(l, &LinkLabel::clicked, this,
                    [this, target]() { emit navigateRequested(target); });
        }
        v->addWidget(l);
    }

    return row;
}

// Collapsible section
QWidget *ActionCenterPage::buildSection(const QString &title, bool expanded,
                                        QWidget *rows)
{
    auto *panel = new QWidget;
    panel->setStyleSheet("background: transparent;");
    auto *panelV = new QVBoxLayout(panel);
    panelV->setContentsMargins(0, 0, 0, 0);
    panelV->setSpacing(0);

    // Header strip: blue section title on the left, a round-ish chevron on the
    // right. The whole strip is clickable and toggles the rows below it.
    auto *header = new ClickableWidget;
    header->setStyleSheet("background: transparent;");
    header->setCursor(Qt::PointingHandCursor);
    header->setMinimumHeight(36);
    auto *headerH = new QHBoxLayout(header);
    headerH->setContentsMargins(0, 4, 8, 4);
    headerH->setSpacing(0);

    headerH->addWidget(Win7::label(title, 13, "#1A5FB4"), 0, Qt::AlignVCenter);
    headerH->addStretch(1);

    // The shared round Aero expander. The whole header strip is the click
    // target, so the button itself lets clicks fall through to it.
    auto *chevron = new Win7::ChevronButton(/*interactive=*/false);
    chevron->setChecked(expanded);
    headerH->addWidget(chevron, 0, Qt::AlignVCenter);
    panelV->addWidget(header);

    // Thin separator directly under the header, always visible.
    panelV->addWidget(Win7::hairline("#DCDCDC"));

    // Body carries a left inset so the rows line up under the title text.
    rows->setContentsMargins(2, 12, 2, 12);
    panelV->addWidget(rows);

    // Clicking the header expands/collapses the rows and flips the chevron,
    // matching the Windows Action Center sections.
    auto applyState = [rows, chevron](bool open) {
        rows->setVisible(open);
        chevron->setChecked(open);
    };
    applyState(expanded);
    header->onClick = [applyState, open = expanded]() mutable {
        open = !open;
        applyState(open);
    };

    return panel;
}

// Yellow "action needed" alert
QWidget *ActionCenterPage::buildAlertBox(const QString &title,
                                         const QString &description,
                                         const QString &buttonText,
                                         const QString &link)
{
    // Same ribbon-bar treatment as the Linux Update page's status box: a white
    // box with a 20px yellow "action needed" stripe down the left edge (rather
    // than a thin accent frame). No icon, matching the Windows backup box.
    auto *frame = new QFrame;
    frame->setObjectName("acAlert");
    frame->setMaximumWidth(620);
    frame->setStyleSheet(
        "QFrame#acAlert {"
        "  background: #FFFFFF;"
        "  border: 1px solid #CCCCCC;"
        "  border-left: 20px solid #D4A800;"
        "}"
        "QFrame#acAlert QLabel { background: transparent; border: none; }");

    auto *innerH = new QHBoxLayout(frame);
    innerH->setContentsMargins(12, 12, 12, 12);
    innerH->setSpacing(12);

    auto *textCol = new QVBoxLayout;
    textCol->setContentsMargins(0, 0, 0, 0);
    textCol->setSpacing(4);

    auto *titleLabel = new QLabel(title);
    {
        QFont f = titleLabel->font();
        f.setPointSize(9);
        f.setBold(true);
        titleLabel->setFont(f);
    }
    titleLabel->setStyleSheet("color: #000000; background: transparent;");
    textCol->addWidget(titleLabel);
    textCol->addWidget(bodyLabel(description));
    if (!link.isEmpty()) {
        auto *l = new LinkLabel(link);
        connect(l, &LinkLabel::clicked, this,
                [this]() { emit navigateRequested(PageId::BackupRestore); });
        textCol->addWidget(l);
    }
    innerH->addLayout(textCol, 1);

    auto *button = new QPushButton(buttonText);
    button->setCursor(Qt::PointingHandCursor);
    button->setIcon(themeIcon({"preferences-system-backup", "document-save",
                               "drive-harddisk"}));
    connect(button, &QPushButton::clicked, this,
            [this, buttonText]() {
                if (buttonText == QLatin1String("Install Backup and Restore"))
                    emit optionalFeatureRequested(QStringLiteral("backup-restore"),
                                                  PageId::BackupRestore);
                else
                    emit navigateRequested(PageId::BackupRestore);
            });
    innerH->addWidget(button, 0, Qt::AlignVCenter);

    return frame;
}

// Bottom troubleshooting / recovery task
QWidget *ActionCenterPage::buildBottomTask(
    std::initializer_list<const char *> iconNames,
    const QString &title, const QString &description)
{
    auto *w = new QWidget;
    w->setStyleSheet("background: transparent;");
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(10);

    auto *icon = new QLabel;
    icon->setFixedSize(40, 40);
    icon->setPixmap(themeIcon(iconNames).pixmap(38, 38));
    icon->setStyleSheet("background: transparent;");
    h->addWidget(icon, 0, Qt::AlignTop);

    auto *col = new QVBoxLayout;
    col->setContentsMargins(0, 2, 0, 0);
    col->setSpacing(2);
    auto *task = new LinkLabel(title);
    connect(task, &LinkLabel::clicked, this, [this, title]() {
        emit navigateRequested(title == "Recovery" ? PageId::BackupRestore
                                                     : PageId::SecurityMaintenance);
    });
    col->addWidget(task);
    col->addWidget(bodyLabel(description));
    h->addLayout(col, 1);

    return w;
}

// Page
ActionCenterPage::ActionCenterPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    const AcInfo info = gatherInfo();

    // Windows 7 lays the content out at a fixed width and leaves the rest of
    // the window blank on the right rather than stretching to fill it.
    auto *contentV = Win7::pageScaffold(this, sidebar, /*bottomMargin=*/20,
                                        /*fixedWidth=*/700);

    // Page heading
    contentV->addWidget(
        Win7::pageTitle("Review recent messages and resolve problems", 13));
    contentV->addSpacing(8);

    const int issueCount = (!info.firewallOn) + (!info.avPresent)
        + (!info.uacOn) + (!info.networkUp)
        + (info.failedServices > 0) + (info.updatesAvailable > 0) + info.diskLow
        + (!info.backupConfigured);
    auto *blurb = bodyLabel(issueCount == 0
        ? QStringLiteral("Action Center did not detect any current security or maintenance issues.")
        : QStringLiteral("Action Center detected %1 issue(s) for you to review.").arg(issueCount));
    contentV->addWidget(blurb);
    contentV->addSpacing(14);

    // ---- Security section -------------------------------------------------
    auto *securityRows = new QWidget;
    securityRows->setStyleSheet("background: transparent;");
    {
        auto *v = new QVBoxLayout(securityRows);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(14);
        v->addWidget(buildStatusRow(
            "Network firewall", info.firewallOn ? "On" : "Off",
            info.firewallOn
                ? QStringLiteral("Linux Firewall (%1) is actively protecting "
                                 "your computer.").arg(info.firewallName)
                : QStringLiteral("Linux Firewall (%1) is turned off.")
                      .arg(info.firewallName)));
        v->addWidget(buildStatusRow(
            "Spyware and unwanted software protection",
            info.avPresent ? "On" : "Off",
            info.avPresent
                ? QStringLiteral("%1 reports that it is turned on.")
                      .arg(info.avName)
                : QStringLiteral("Aero7 Defender is an optional feature and is not installed."),
            info.avPresent ? QStringLiteral("View installed antivirus programs")
                           : QStringLiteral("Install Aero7 Defender")));
        v->addWidget(buildStatusRow(
            "Administrator approval", info.uacOn ? "On" : "Off",
            info.uacOn
                ? "polkit will prompt for authentication when programs try to "
                  "make administrative changes."
                : "polkit authentication is not available.",
            "About administrator approval"));
        v->addWidget(buildStatusRow(
            "Network status", info.networkUp ? "Connected" : "Disconnected",
            info.networkUp ? "At least one non-loopback network interface is active."
                           : "No active wired or wireless network interface was detected.",
            "Review network settings"));
    }
    contentV->addWidget(buildSection("Security", /*expanded=*/false,
                                     securityRows));

    // ---- Maintenance section ---------------------------------------------
    auto *maintRows = new QWidget;
    maintRows->setStyleSheet("background: transparent;");
    {
        auto *v = new QVBoxLayout(maintRows);
        v->setContentsMargins(0, 0, 0, 0);
        v->setSpacing(14);
        v->addWidget(buildStatusRow(
            "Backup", info.backupConfigured ? "Configured" : "Not set up",
            info.backupConfigured
                ? "Aero7 has a saved backup configuration."
                : "No Aero7 backup configuration was found."));
        v->addWidget(buildStatusRow(
            "Check for updates",
            info.updatesAvailable < 0 ? "Unavailable"
              : info.updatesAvailable == 0 ? "No action needed"
                                           : QStringLiteral("%1 available").arg(info.updatesAvailable),
            info.updatesAvailable < 0
                ? "No supported package update query is available."
                : info.updatesAvailable == 0
                    ? "The local package databases report no pending updates."
                    : QStringLiteral("The local package databases report %1 pending update(s).")
                          .arg(info.updatesAvailable)));
        v->addWidget(buildStatusRow(
            "System services", info.failedServices == 0 ? "No action needed"
                                                         : QStringLiteral("%1 failed").arg(info.failedServices),
            info.failedServices == 0
                ? "systemd reports no failed system services."
                : QStringLiteral("systemd reports %1 failed system service(s).")
                      .arg(info.failedServices)));
        v->addWidget(buildStatusRow(
            "Storage", info.diskLow ? "Low disk space" : "No action needed",
            info.diskLow ? "The system drive has less than 5% free space."
                         : "The system drive has at least 5% free space."));
    }
    contentV->addWidget(buildSection("Maintenance", /*expanded=*/false,
                                     maintRows));
    contentV->addSpacing(14);

    // Backup availability is shown outside the collapsed maintenance section.
    if (!info.backupConfigured) {
        contentV->addWidget(buildAlertBox(
            info.backupFeaturePresent ? "Backup is not configured"
                                      : "Backup and Restore is not installed",
            info.backupFeaturePresent
                ? "No Aero7 backup configuration exists for this user."
                : "Install the optional Backup and Restore feature before creating a backup plan.",
            info.backupFeaturePresent ? "Review backup options"
                                      : "Install Backup and Restore",
            "About backup availability"));
        contentV->addSpacing(24);
    }

    // ---- Bottom "If you don't see your problem listed" --------------------
    auto *bottomIntro = bodyLabel(
        "If you don't see your problem listed, try one of these:");
    contentV->addWidget(bottomIntro);
    contentV->addSpacing(12);

    auto *bottomRow = new QHBoxLayout;
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->setSpacing(40);
    bottomRow->addWidget(buildBottomTask(
        {"tools-report-bug", "system-run", "preferences-system"},
        "Troubleshooting", "Find and fix problems"), 0, Qt::AlignTop);
    bottomRow->addWidget(buildBottomTask(
        {"document-revert", "chronometer", "system-reboot"},
        "Recovery", "Review available recovery options"), 0,
        Qt::AlignTop);
    bottomRow->addStretch(1);
    contentV->addLayout(bottomRow);

    contentV->addStretch(1);
}
