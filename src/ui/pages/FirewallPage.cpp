#include "FirewallPage.h"
#include "ConfFile.h"
#include "IconHelper.h"
#include "Win7Ui.h"
#include "Branding.h"
#include "FirewallBackend.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFont>
#include <QFile>
#include <QHostInfo>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QPushButton>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QSysInfo>

using Win7::ClickableWidget;

namespace {

struct FirewallBackendStatus {
    bool ready = false;
    bool restartRequired = false;
    QString message;
};

// UFW uses the system iptables frontend.  After a kernel package update the
// running kernel can temporarily have no matching modules on disk; nft then
// reports "Protocol not supported" and UFW turns that into the rather opaque
// "Couldn't determine iptables version" error.  Detect that state before an
// authenticated action so the Control Panel can give the user the real fix.
FirewallBackendStatus probeFirewallBackend()
{
    FirewallBackendStatus status;
    const QString iptables = QStandardPaths::findExecutable(
        QStringLiteral("iptables"));
    if (iptables.isEmpty()) {
        status.message = QStringLiteral(
            "The firewall backend is not installed. Run System Update to "
            "install the required firewall components, then try again.");
        return status;
    }

    QProcess probe;
    probe.start(iptables, {QStringLiteral("-V")});
    if (!probe.waitForStarted(1500) || !probe.waitForFinished(3000)
        || probe.exitStatus() != QProcess::NormalExit
        || probe.exitCode() != 0) {
        const QString runningKernel = QSysInfo::kernelVersion();
        const QDir modulesRoot(QStringLiteral("/usr/lib/modules"));
        const QStringList installedKernels = modulesRoot.entryList(
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        const bool runningModulesPresent =
            modulesRoot.exists(runningKernel);

        if (!runningKernel.isEmpty() && !runningModulesPresent
            && !installedKernels.isEmpty()) {
            status.restartRequired = true;
            status.message = QStringLiteral(
                "A system update installed a new Linux kernel, but Aero7 is "
                "still running the previous kernel (%1). Restart Aero7 to "
                "finish the update, then open Linux Firewall again.")
                                 .arg(runningKernel);
            return status;
        }

        QString detail = QString::fromUtf8(probe.readAllStandardError()).trimmed();
        if (detail.isEmpty())
            detail = QString::fromUtf8(probe.readAllStandardOutput()).trimmed();
        status.message = QStringLiteral(
            "The firewall backend could not start. Run System Update, restart "
            "Aero7, and try again.");
        if (!detail.isEmpty())
            status.message += QStringLiteral("\n\nTechnical detail: %1").arg(detail);
        return status;
    }

    status.ready = true;
    return status;
}

QWidget *backendWarning(const FirewallBackendStatus &status)
{
    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("firewallBackendWarning"));
    panel->setStyleSheet(
        "#firewallBackendWarning { background: #FFF4CE; border: 1px solid "
        "#D6B656; }"
        "#firewallBackendWarning QLabel { background: transparent; color: "
        "#4A3B00; }");
    auto *layout = new QHBoxLayout(panel);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(8);

    auto *icon = new QLabel;
    icon->setPixmap(themeIcon({"system-reboot", "dialog-warning"}).pixmap(24, 24));
    icon->setFixedSize(24, 24);
    layout->addWidget(icon, 0, Qt::AlignTop);

    auto *text = Win7::bodyLabel(status.message);
    text->setWordWrap(true);
    layout->addWidget(text, 1);
    return panel;
}

} // namespace

// True when the machine has a default route (i.e. is on a network). Mirrors the
// Network and Sharing Center's reading of /proc/net/route: the default route is
// the row whose Destination column ("00000000") is all-zero.
static bool hasDefaultRoute()
{
    QFile f(QStringLiteral("/proc/net/route"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const QString text = QString::fromUtf8(f.readAll());
    const QList<QStringView> lines = QStringView(text).split(u'\n');
    for (int i = 1; i < lines.size(); ++i) {           // skip the header row
        const QList<QStringView> cols = lines[i].split(u'\t', Qt::SkipEmptyParts);
        if (cols.size() >= 2 && cols[1].trimmed() == u"00000000")
            return true;
    }
    return false;
}

// Count the `### tuple` rule entries in a ufw user.rules file, one per rule
// the user has added, matching the rule count KDE's firewall module lists.
static int countRules(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;
    const QString text = QString::fromUtf8(f.readAll());
    int n = 0;
    for (const QStringView &line : QStringView(text).split(u'\n')) {
        if (line.trimmed().startsWith(QLatin1String("### tuple ###")))
            ++n;
    }
    return n;
}

FirewallPage::FwInfo FirewallPage::gatherInfo()
{
    FwInfo fw;

    const auto backend = FirewallBackend::detect();
    fw.backend = backend.kind;
    fw.backendName = backend.name;
    fw.enabled = backend.active;

    if (fw.backend == FirewallBackend::Kind::Ufw) {
        fw.logLevel = readConfField(QStringLiteral("/etc/ufw/ufw.conf"),
                                QStringLiteral("LOGLEVEL"));

        fw.inputPolicy = readConfField(QStringLiteral("/etc/default/ufw"),
                                   QStringLiteral("DEFAULT_INPUT_POLICY")).toUpper();
        fw.outputPolicy = readConfField(QStringLiteral("/etc/default/ufw"),
                                    QStringLiteral("DEFAULT_OUTPUT_POLICY")).toUpper();
        if (fw.inputPolicy.isEmpty())
            fw.inputPolicy = QStringLiteral("DROP");

        fw.ruleCount = countRules(QStringLiteral("/etc/ufw/user.rules"))
                     + countRules(QStringLiteral("/etc/ufw/user6.rules"));
    }

    fw.netConnected = hasDefaultRoute();
    fw.networkName  = QStringLiteral("Network");

    return fw;
}

// Sidebar
QList<SidebarLink> FirewallPage::sidebarLinks()
{
    const bool ufw = FirewallBackend::detect().kind == FirewallBackend::Kind::Ufw;
    return {
        ufw ? Nav::to("Allow a port or service through Linux Firewall", PageId::Firewall)
            : Nav::disabled("Allow a port or service through Linux Firewall"),
        ufw ? Nav::to("Change notification settings", PageId::Firewall)
            : Nav::disabled("Change notification settings"),
        Nav::to("Turn Linux Firewall on or off", PageId::Firewall),
        ufw ? Nav::to("Restore defaults", PageId::Firewall)
            : Nav::disabled("Restore defaults"),
        Nav::disabled("Advanced settings"),
        Nav::to("Troubleshoot my network", PageId::NetworkSharing),
    };
}

QList<SidebarLink> FirewallPage::sidebarSeeAlso()
{
    return {
        Nav::to("Action Center", PageId::ActionCenter),
        Nav::to("Network and Sharing Center", PageId::NetworkSharing),
    };
}

// Network-location panel
// Human-readable phrasing for the default-incoming-policy line, mirroring the
// way KDE's firewall module summarises DEFAULT_INPUT_POLICY.
static QString incomingText(const QString &policy)
{
    if (policy == QLatin1String("ACCEPT"))
        return QStringLiteral("Allow all incoming connections");
    if (policy == QLatin1String("REJECT"))
        return QStringLiteral("Reject all connections to programs that are not "
                              "on the list of allowed programs");
    // DROP (the ufw default): silently dropped, matching Windows' "Block all".
    return QStringLiteral("Block all connections to programs that are not on "
                          "the list of allowed programs");
}

// Notification/logging phrasing derived from ufw's LOGLEVEL. UFW records
// firewall events; it does not provide interactive per-program approval popups.
static QString notifyText(const QString &logLevel)
{
    if (logLevel.isEmpty())
        return QStringLiteral("Unknown");
    if (logLevel.compare(QLatin1String("off"), Qt::CaseInsensitive) == 0)
        return QStringLiteral("Off");
    QString level = logLevel.toLower();
    level[0] = level[0].toUpper();
    return QStringLiteral("%1 event logging").arg(level);
}

QWidget *FirewallPage::buildLocationPanel(const QString &title,
                                          const QString &connState,
                                          bool expanded,
                                          const FwInfo &info)
{
    auto *panel = new QFrame;
    panel->setObjectName("fwPanel");
    panel->setStyleSheet(
        "#fwPanel { background: #FFFFFF; border: 1px solid #DCDCDC; }");
    auto *panelV = new QVBoxLayout(panel);
    panelV->setContentsMargins(0, 0, 0, 0);
    panelV->setSpacing(0);

    // Header strip: green accent bar, shield icon, title, conn-state, chevron.
    // The header is clickable and toggles the body below, like the Windows
    // firewall network sections.
    auto *header = new ClickableWidget;
    header->setStyleSheet("background: transparent;");
    header->setCursor(Qt::PointingHandCursor);
    auto *headerH = new QHBoxLayout(header);
    headerH->setContentsMargins(0, 0, 8, 0);
    headerH->setSpacing(0);

    auto *accent = new QFrame;
    accent->setFixedWidth(14);
    accent->setMinimumHeight(34);
    accent->setStyleSheet("background: #4E9A2A; border: none;");
    headerH->addWidget(accent);

    auto *shield = new QLabel;
    shield->setFixedSize(20, 20);
    shield->setPixmap(themeIcon({"security-high", "preferences-security-firewall",
                                 "security-medium", "firewall-config"})
                          .pixmap(16, 16));
    shield->setStyleSheet("background: transparent;");
    headerH->addSpacing(8);
    headerH->addWidget(shield, 0, Qt::AlignVCenter);

    headerH->addSpacing(6);
    headerH->addWidget(Win7::label(title, 11, "#1A5FB4"), 0, Qt::AlignVCenter);
    headerH->addStretch(1);

    headerH->addWidget(Win7::label(connState, 11, "#2D2D2D"), 0, Qt::AlignVCenter);

    // The shared round Aero expander. The whole header strip is the click
    // target, so the button itself lets clicks fall through to it.
    auto *chevron = new Win7::ChevronButton(/*interactive=*/false);
    chevron->setChecked(expanded);
    headerH->addSpacing(8);
    headerH->addWidget(chevron, 0, Qt::AlignVCenter);

    header->setMinimumHeight(34);
    panelV->addWidget(header);

    // Body: caption line + the status grid. Built regardless of the initial
    // state so the header can toggle it on click; visibility is set at the end.
    auto *sep = Win7::hairline("#E2E2E2");
    panelV->addWidget(sep);

    // Body has no horizontal margin so the dividers below span the full panel
    // width; the caption and grid carry their own left inset (kLeftInset) so
    // their text lines up under the green banner rather than under the icon.
    const int kLeftInset = 8;
    const int kRightInset = 16;

    auto *body = new QWidget;
    body->setStyleSheet("background: transparent;");
    auto *bodyV = new QVBoxLayout(body);
    bodyV->setContentsMargins(0, 4, 0, 14);
    bodyV->setSpacing(0);

    auto *caption = Win7::bodyLabel(info.backend == FirewallBackend::Kind::Firewalld
        ? QStringLiteral("Linux Firewall uses firewalld zones. The current "
                         "Aero7 page shows service status; use firewall-cmd "
                         "to inspect or change advanced zone rules.")
        : QStringLiteral("Linux Firewall (UFW) applies one set of rules to every "
                         "network. It has no separate Home, Work, or Public rule sets."));
    caption->setContentsMargins(kLeftInset, 0, kRightInset, 0);
    bodyV->addWidget(caption);
    bodyV->addSpacing(12);

    // Divider between the caption and the status grid, as in the Windows panel.
    bodyV->addWidget(Win7::hairline("#E2E2E2"));
    bodyV->addSpacing(14);

    auto *grid = new QGridLayout;
    grid->setContentsMargins(kLeftInset, 0, kRightInset, 0);
    grid->setHorizontalSpacing(55);
    grid->setVerticalSpacing(12);
    grid->setColumnMinimumWidth(0, 200);
    grid->setColumnStretch(1, 1);

    int r = 0;
    grid->addWidget(Win7::bodyLabel(QStringLiteral("Linux Firewall (%1) state:")
                                       .arg(info.backendName)),
                    r, 0);
    grid->addWidget(Win7::bodyLabel(info.enabled ? "On" : "Off"), r, 1);
    ++r;

    grid->addWidget(Win7::bodyLabel("Incoming connections:"), r, 0);
    grid->addWidget(Win7::bodyLabel(
        info.backend == FirewallBackend::Kind::Firewalld
            ? QStringLiteral("Managed by firewalld zones")
            : incomingText(info.inputPolicy)), r, 1);
    ++r;

    grid->addWidget(Win7::bodyLabel("Active networks:"), r, 0);
    // Icon + network-name cell.
    {
        auto *cell = new QWidget;
        cell->setStyleSheet("background: transparent;");
        auto *h = new QHBoxLayout(cell);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(6);
        auto *icon = new QLabel;
        icon->setFixedSize(16, 16);
        icon->setPixmap(themeIcon({"network-workgroup", "network-type-public",
                                   "preferences-system-network", "network-wired"})
                            .pixmap(16, 16));
        icon->setStyleSheet("background: transparent;");
        h->addWidget(icon, 0, Qt::AlignVCenter);
        h->addWidget(Win7::bodyLabel(info.netConnected ? info.networkName
                                                       : QStringLiteral("None")),
                     0, Qt::AlignVCenter);
        h->addStretch(1);
        grid->addWidget(cell, r, 1, Qt::AlignLeft | Qt::AlignTop);
    }
    ++r;

    grid->addWidget(Win7::bodyLabel("Notification state:"), r, 0);
    grid->addWidget(Win7::bodyLabel(
        info.backend == FirewallBackend::Kind::Firewalld
            ? QStringLiteral("Managed by firewalld")
            : notifyText(info.logLevel)), r, 1);
    ++r;

    bodyV->addLayout(grid);
    panelV->addWidget(body);

    // Dropdown behaviour: clicking the header expands/collapses the body and
    // flips the chevron, matching the Windows firewall network sections.
    auto applyState = [body, sep, chevron](bool open) {
        sep->setVisible(open);
        body->setVisible(open);
        chevron->setChecked(open);
    };
    applyState(expanded);

    // The mutable lambda is stored in header->onClick, so its captured `open`
    // copy persists between clicks and tracks the open/closed state.
    header->onClick = [applyState, open = expanded]() mutable {
        open = !open;
        applyState(open);
    };

    return panel;
}

// Page
FirewallPage::FirewallPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    const FwInfo info = gatherInfo();
    const FirewallBackendStatus backend = info.backend == FirewallBackend::Kind::Ufw
        ? probeFirewallBackend() : FirewallBackendStatus{
            info.backend == FirewallBackend::Kind::Firewalld, false,
            QStringLiteral("No supported firewall backend is installed.")};

    // Windows 7 lays the content out at a fixed width and leaves the rest of
    // the window blank on the right rather than stretching to fill it.
    auto *contentV = Win7::pageScaffold(this, sidebar, /*bottomMargin=*/20,
                                        /*fixedWidth=*/700);

    contentV->addWidget(
        Win7::pageTitle(Branding::brand(
            "Help protect your computer with Linux Firewall")));
    contentV->addSpacing(10);

    contentV->addWidget(Win7::bodyLabel(Branding::brand(
        "Linux Firewall can help prevent hackers or malicious software from "
        "gaining access to your computer through the Internet or a network.")));
    contentV->addSpacing(10);

    auto addHelpLink = [&](const QString &text) {
        auto *label = Win7::label(text + "  (help article coming later)", 9, "#777777");
        label->setToolTip("This help article is not available yet.");
        contentV->addWidget(label, 0, Qt::AlignLeft);
        contentV->addSpacing(4);
    };
    addHelpLink("How does a firewall help protect my computer?");
    addHelpLink("What are network locations?");

    contentV->addSpacing(12);

    if (!backend.ready) {
        contentV->addWidget(backendWarning(backend));
        contentV->addSpacing(12);
    }

    auto *controls = new QHBoxLayout;
    controls->setSpacing(8);
    auto *toggle = new QPushButton(info.enabled ? "Turn firewall off"
                                                : "Turn firewall on");
    toggle->setObjectName("firewall-toggle");
    toggle->setIcon(themeIcon({"preferences-security-firewall", "security-high"}));
    toggle->setEnabled(backend.ready);
    if (!backend.ready)
        toggle->setToolTip(backend.message);
    connect(toggle, &QPushButton::clicked, this, [this, info]() {
        if (info.backend == FirewallBackend::Kind::Firewalld) {
            setFirewalldEnabled(!info.enabled);
            return;
        }
        const QStringList args = info.enabled
            ? QStringList{"disable"} : QStringList{"--force", "enable"};
        runUfw(args, info.enabled ? "The firewall was turned off."
                                  : "The firewall was turned on.", !info.enabled);
    });
    controls->addWidget(toggle);

    auto *allow = new QPushButton("Allow a port or service…");
    allow->setObjectName("firewall-allow-service");
    allow->setEnabled(backend.ready && info.backend == FirewallBackend::Kind::Ufw);
    if (!allow->isEnabled())
        allow->setToolTip(info.backend == FirewallBackend::Kind::Firewalld
            ? QStringLiteral("Firewalld rule editing is not available in this "
                             "Aero7 version. Use firewall-cmd for advanced rules.")
            : backend.message);
    connect(allow, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        const QString rule = QInputDialog::getText(
            this, "Allow a port or service",
            "Enter a port, port/protocol, or installed UFW application profile\n"
            "(examples: 22/tcp, 5353/udp, OpenSSH):",
            QLineEdit::Normal, {}, &ok).trimmed();
        if (ok && !rule.isEmpty())
            runUfw({"allow", rule}, QStringLiteral("Allowed %1.").arg(rule));
    });
    controls->addWidget(allow);

    auto *notifications = new QPushButton("Notification settings…");
    notifications->setObjectName("firewall-notification-settings");
    notifications->setEnabled(backend.ready && info.backend == FirewallBackend::Kind::Ufw);
    if (!notifications->isEnabled())
        notifications->setToolTip(info.backend == FirewallBackend::Kind::Firewalld
            ? QStringLiteral("UFW logging settings do not apply to firewalld.")
            : backend.message);
    connect(notifications, &QPushButton::clicked, this,
            [this, info]() { showNotificationSettings(info.logLevel); });
    controls->addWidget(notifications);

    auto *reset = new QPushButton("Restore defaults…");
    reset->setObjectName("firewall-restore-defaults");
    reset->setEnabled(backend.ready && info.backend == FirewallBackend::Kind::Ufw);
    if (!reset->isEnabled())
        reset->setToolTip(info.backend == FirewallBackend::Kind::Firewalld
            ? QStringLiteral("Restoring UFW rules does not apply to firewalld.")
            : backend.message);
    connect(reset, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::warning(
                this, "Restore firewall defaults",
                "This removes all custom firewall rules. Continue?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            == QMessageBox::Yes) {
            runUfw({"--force", "reset"}, "Firewall defaults were restored.");
        }
    });
    controls->addWidget(reset);
    controls->addStretch(1);
    contentV->addLayout(controls);
    contentV->addSpacing(12);

    // The network panel. ufw has no per-network profiles (no Home/Work/Public
    // split), so rather than mimic Windows' two location panels we show a
    // single "All networks" section expanded to the live ufw state. It is
    // "Connected" when a default route exists.
    contentV->addWidget(buildLocationPanel(
        "All networks",
        info.netConnected ? "Connected" : "Not Connected",
        /*expanded=*/true, info));

    contentV->addStretch(1);
}

void FirewallPage::showNotificationSettings(const QString &currentLogLevel)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Firewall notification settings");
    dialog.setModal(true);
    dialog.setMinimumWidth(500);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 16, 18, 14);
    layout->setSpacing(12);

    auto *description = Win7::bodyLabel(
        "UFW does not show per-program approval popups. These settings control "
        "which firewall events are recorded in the system journal for review "
        "and troubleshooting.");
    description->setWordWrap(true);
    layout->addWidget(description);

    layout->addWidget(Win7::bodyLabel("Firewall event logging:"));

    auto *level = new QComboBox;
    level->setObjectName("firewall-log-level");
    level->addItem("Off — do not record firewall events", "off");
    level->addItem("Low (recommended) — record important blocked traffic", "low");
    level->addItem("Medium — record additional firewall activity", "medium");
    level->addItem("High — record most traffic with rate limiting", "high");
    level->addItem("Full — record all firewall traffic", "full");
    const QString normalized = currentLogLevel.trimmed().toLower();
    const int current = level->findData(normalized);
    level->setCurrentIndex(current >= 0 ? current : 1);
    layout->addWidget(level);

    auto *warning = Win7::bodyLabel(
        "High and Full can create a large amount of journal data. Low is the "
        "recommended setting for normal use.");
    warning->setWordWrap(true);
    layout->addWidget(warning);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString selected = level->currentData().toString();
    if (selected == normalized)
        return;

    runUfw({"logging", selected},
           QStringLiteral("Firewall event logging was changed to %1.")
               .arg(level->currentText().section(QStringLiteral(" — "), 0, 0)));
}

void FirewallPage::runUfw(const QStringList &arguments,
                          const QString &successMessage,
                          std::optional<bool> expectedEnabled)
{
    if (QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty()
        || QStandardPaths::findExecutable(QStringLiteral("ufw")).isEmpty()) {
        QMessageBox::warning(
            this, "Firewall",
            "The firewall action requires both pkexec and ufw to be installed.");
        return;
    }
    const FirewallBackendStatus backend = probeFirewallBackend();
    if (!backend.ready) {
        QMessageBox::warning(this,
                             backend.restartRequired ? "Restart required"
                                                     : "Firewall unavailable",
                             backend.message);
        return;
    }
    auto *process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            QMessageBox::warning(this, "Firewall",
                                 "The firewall command could not be started.");
            process->deleteLater();
        }
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, successMessage, expectedEnabled]
            (int code, QProcess::ExitStatus exitStatus) {
        const QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
        const bool observedEnabled = readConfField(
            QStringLiteral("/etc/ufw/ufw.conf"), QStringLiteral("ENABLED"))
            .compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0;
        if (exitStatus == QProcess::NormalExit && code == 0
            && (!expectedEnabled || observedEnabled == *expectedEnabled)) {
            QMessageBox::information(this, "Firewall", successMessage);
            emit refreshRequested();
        } else if (code == 126 || code == 127) {
            QMessageBox::warning(this, "Firewall",
                                 "Authentication was cancelled or denied."
                                 " The firewall was not changed.");
        } else {
            QMessageBox::warning(this, "Firewall",
                error.isEmpty() ? "The firewall command failed or its state "
                                  "did not match the requested setting." : error);
        }
        process->deleteLater();
    });
    process->start("pkexec", QStringList{"ufw"} + arguments);
}

void FirewallPage::setFirewalldEnabled(bool enabled)
{
    if (QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty()
        || QStandardPaths::findExecutable(QStringLiteral("systemctl")).isEmpty()) {
        QMessageBox::warning(this, "Firewall",
                             "Changing firewalld requires pkexec and systemctl.");
        return;
    }

    auto *process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            QMessageBox::warning(this, "Firewall",
                                 "The firewall command could not be started.");
            process->deleteLater();
        }
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process, enabled](int code, QProcess::ExitStatus exitStatus) {
        const QString error = QString::fromUtf8(process->readAllStandardError()).trimmed();
        const bool observed = FirewallBackend::serviceActive(
            QStringLiteral("firewalld.service"));
        if (exitStatus == QProcess::NormalExit && code == 0
            && observed == enabled) {
            QMessageBox::information(this, "Firewall", enabled
                ? "The firewall was turned on."
                : "The firewall was turned off.");
            emit refreshRequested();
        } else if (code == 126 || code == 127) {
            QMessageBox::warning(this, "Firewall",
                                 "Authentication was cancelled or denied."
                                 " The firewall was not changed.");
        } else {
            QMessageBox::warning(this, "Firewall", error.isEmpty()
                ? "The firewalld state did not match the requested setting."
                : error);
        }
        process->deleteLater();
    });
    process->start(QStringLiteral("pkexec"),
                   {QStringLiteral("systemctl"),
                    enabled ? QStringLiteral("enable") : QStringLiteral("disable"),
                    QStringLiteral("--now"), QStringLiteral("firewalld.service")});
}
