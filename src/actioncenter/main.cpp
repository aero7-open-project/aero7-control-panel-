#include "IconHelper.h"
#include "FirewallBackend.h"

#include <Aero7Qt/stylesheet.h>
#include <LayerShellQt/Window>

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDialog>
#include <QFrame>
#include <QGuiApplication>
#include <QLabel>
#include <QNetworkInterface>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QStorageInfo>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QVBoxLayout>

namespace {
struct Issue { QString title; QString detail; QString page; };

QString commandOutput(const QString &program, const QStringList &arguments, int timeout = 2500)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(timeout)) {
        process.kill();
        process.waitForFinished();
        return {};
    }
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

QVector<Issue> gatherIssues()
{
    QVector<Issue> issues;
    if (!FirewallBackend::detect().active)
        issues.push_back({QStringLiteral("Network firewall is off"),
                          QStringLiteral("Turn on the firewall or review its current configuration."),
                          QStringLiteral("firewall")});
    if (auto *bus = QDBusConnection::systemBus().interface();
        !bus || !bus->isServiceRegistered(QStringLiteral("org.freedesktop.PolicyKit1"))) {
        issues.push_back({QStringLiteral("Administrator approval is unavailable"),
                          QStringLiteral("The polkit authority is not running, so protected changes cannot be authenticated."),
                          QStringLiteral("security-maintenance")});
    }
    const QStringList failedLines = commandOutput(QStringLiteral("systemctl"),
        {QStringLiteral("--failed"), QStringLiteral("--no-legend"), QStringLiteral("--plain")})
            .split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (!failedLines.isEmpty())
        issues.push_back({QStringLiteral("System services need attention"),
                          QStringLiteral("%1 failed system service(s) were detected.").arg(failedLines.size()),
                          QStringLiteral("security-maintenance")});
    const QStorageInfo rootStorage = QStorageInfo::root();
    if (rootStorage.isValid() && rootStorage.isReady() && rootStorage.bytesTotal() > 0
        && double(rootStorage.bytesAvailable()) / double(rootStorage.bytesTotal()) < 0.05) {
        issues.push_back({QStringLiteral("Low disk space"),
                          QStringLiteral("The system drive has less than 5% free space."),
                          QStringLiteral("storage-administration")});
    }
    bool networkUp = false;
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        const auto flags = interface.flags();
        if (!flags.testFlag(QNetworkInterface::IsLoopBack)
            && flags.testFlag(QNetworkInterface::IsUp)
            && flags.testFlag(QNetworkInterface::IsRunning)) {
            networkUp = true;
            break;
        }
    }
    if (!networkUp)
        issues.push_back({QStringLiteral("No active network connection"),
                          QStringLiteral("Connect to a wired or wireless network, or troubleshoot the adapter."),
                          QStringLiteral("network-status")});
    return issues;
}

void openControlPanel(const QString &page)
{
    if (page == QLatin1String("action-center")
        || page == QLatin1String("security-maintenance")
        || page == QLatin1String("storage-administration"))
        QProcess::startDetached(QStringLiteral("control"), {QStringLiteral("--page"), page});
    else
        QProcess::startDetached(QStringLiteral("control"), {QStringLiteral("--setting"), page});
}

class Flyout final : public QDialog {
public:
    explicit Flyout(QWidget *parent = nullptr)
        : QDialog(parent, Qt::FramelessWindowHint)
    {
        setObjectName(QStringLiteral("actionCenterFlyout"));
        setStyleSheet(QStringLiteral(
            "#actionCenterFlyout { background: #F7F9FC; border: 1px solid #7896B5; }"
            "QLabel { background: transparent; color: #1A1A1A; }"
            "QPushButton { text-align: left; color: #0645AD; border: none; background: transparent; padding: 3px; }"
            "QPushButton:hover { text-decoration: underline; background: #E8F2FB; }"));
        resize(360, 210);
        if (QGuiApplication::platformName().contains(QStringLiteral("wayland"), Qt::CaseInsensitive)) {
            setAttribute(Qt::WA_NativeWindow);
            m_layerWindow = LayerShellQt::Window::get(windowHandle());
            LayerShellQt::Window::Anchors anchors(LayerShellQt::Window::AnchorRight);
            anchors.setFlag(LayerShellQt::Window::AnchorBottom);
            m_layerWindow->setAnchors(anchors);
            m_layerWindow->setMargins(QMargins(0, 0, 8, 8));
            m_layerWindow->setExclusiveZone(0);
            m_layerWindow->setLayer(LayerShellQt::Window::LayerTop);
            m_layerWindow->setKeyboardInteractivity(
                LayerShellQt::Window::KeyboardInteractivityOnDemand);
            m_layerWindow->setScope(QStringLiteral("aero7-action-center"));
        } else {
            setWindowFlag(Qt::Tool, true);
            setWindowFlag(Qt::WindowStaysOnTopHint, true);
        }
    }

    void setIssues(const QVector<Issue> &issues)
    {
        if (layout()) {
            QLayoutItem *item = nullptr;
            while ((item = layout()->takeAt(0))) {
                delete item->widget();
                delete item;
            }
            delete layout();
        }
        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(12, 10, 12, 10);
        root->setSpacing(7);
        auto *heading = new QLabel(issues.isEmpty()
            ? QStringLiteral("No current issues detected")
            : QStringLiteral("%1 important message(s)").arg(issues.size()));
        QFont font = heading->font();
        font.setBold(true);
        heading->setFont(font);
        root->addWidget(heading);
        if (issues.isEmpty()) {
            auto *detail = new QLabel(QStringLiteral("Aero7 did not detect a current security or maintenance problem."));
            detail->setWordWrap(true);
            root->addWidget(detail);
        } else {
            for (const Issue &issue : issues) {
                auto *button = new QPushButton(issue.title);
                button->setToolTip(issue.detail);
                connect(button, &QPushButton::clicked, this, [this, page = issue.page]() {
                    openControlPanel(page);
                    close();
                });
                root->addWidget(button);
            }
        }
        root->addStretch(1);
        auto *open = new QPushButton(QStringLiteral("Open Action Center"));
        connect(open, &QPushButton::clicked, this, [this]() {
            openControlPanel(QStringLiteral("action-center"));
            close();
        });
        root->addWidget(open, 0, Qt::AlignHCenter);
        adjustSize();
        setMinimumWidth(360);
    }

    bool usesLayerShell() const { return m_layerWindow != nullptr; }

    void setFlyoutSize(const QSize &size)
    {
        resize(size);
        if (m_layerWindow)
            m_layerWindow->setDesiredSize(size);
    }

private:
    LayerShellQt::Window *m_layerWindow = nullptr;

};
}

int main(int argc, char **argv)
{
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY")
        && !qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("wayland"));
    }
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("aero7-action-center"));
    app.setWindowIcon(aero7ResourceIcon(
        QStringLiteral(":/aero7/icons/app/aero7-action-center.png"),
        QStringLiteral("action-center")));
    app.setQuitOnLastWindowClosed(false);
    Aero7::applyApplicationStyle(&app);
    QSystemTrayIcon tray;
    Flyout flyout;
    QVector<Issue> issues;
    const auto refresh = [&]() {
        issues = gatherIssues();
        tray.setIcon(resolveIcon(issues.isEmpty() ? QStringLiteral("flag")
                                                  : QStringLiteral("dialog-warning")));
        tray.setToolTip(issues.isEmpty() ? QStringLiteral("Action Center: No current issues detected")
                                         : QStringLiteral("Action Center: %1 issue(s) need attention").arg(issues.size()));
        flyout.setIssues(issues);
    };
    refresh();
    QObject::connect(&tray, &QSystemTrayIcon::activated, &app,
                     [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason != QSystemTrayIcon::Trigger && reason != QSystemTrayIcon::DoubleClick)
            return;
        if (flyout.isVisible()) {
            flyout.hide();
            return;
        }
        flyout.setIssues(issues);
        const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
        const QSize size = flyout.sizeHint().expandedTo(QSize(360, 160));
        flyout.setFlyoutSize(size);
        if (!flyout.usesLayerShell()) {
            flyout.move(available.right() - size.width() - 8,
                        available.bottom() - size.height() - 8);
        }
        flyout.show();
        flyout.raise();
        flyout.activateWindow();
    });
    QTimer timer;
    timer.setInterval(5 * 60 * 1000);
    QObject::connect(&timer, &QTimer::timeout, &app, refresh);
    timer.start();
    tray.show();
    return app.exec();
}
