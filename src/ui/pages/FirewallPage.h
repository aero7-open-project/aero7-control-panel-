#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include <optional>
#include "PageId.h"
#include "FirewallBackend.h"

class QScrollArea;
class QVBoxLayout;

// The "Linux Firewall" detail page.
//
// Supports firewalld on fresh installations and preserves UFW on existing
// installations. UFW state comes from its world-readable configuration:
//   * /etc/ufw/ufw.conf      -> ENABLED (firewall on/off), LOGLEVEL (notifications)
//   * /etc/default/ufw       -> DEFAULT_INPUT_POLICY / DEFAULT_OUTPUT_POLICY
//   * /etc/ufw/user.rules     -> the count of configured allow/deny rules
// Explicit changes run through polkit and the page refreshes after readback.
class FirewallPage : public QWidget {
    Q_OBJECT

public:
    explicit FirewallPage(QScrollArea *sidebar, QWidget *parent = nullptr);

    // Left-nav entries shown by MainWindow's subpage sidebar.
    static QList<SidebarLink> sidebarLinks();
    static QList<SidebarLink> sidebarSeeAlso();

signals:
    void refreshRequested();

private:
    // Live firewall facts, read once in the constructor from the ufw config
    // files that KDE's firewall module also consults.
    struct FwInfo {
        bool    enabled = false;       // ufw.conf ENABLED=yes
        FirewallBackend::Kind backend = FirewallBackend::Kind::None;
        QString backendName;
        QString inputPolicy;           // DEFAULT_INPUT_POLICY ("DROP"/"ACCEPT"/"REJECT")
        QString outputPolicy;          // DEFAULT_OUTPUT_POLICY
        QString logLevel;              // ufw.conf LOGLEVEL ("off".."high")
        int     ruleCount = 0;         // number of user.rules entries
        bool    netConnected = false;  // a default route exists
        QString networkName;           // active network profile name
    };

    static FwInfo gatherInfo();

    // Builds one collapsible network-location panel (the green-striped boxes in
    // the reference). `expanded` panels show the firewall status grid beneath
    // the header; collapsed ones show only the header strip.
    QWidget *buildLocationPanel(const QString &title,
                                const QString &connState,
                                bool expanded,
                                const FwInfo &info);
    void showNotificationSettings(const QString &currentLogLevel);
    void runUfw(const QStringList &arguments, const QString &successMessage,
                std::optional<bool> expectedEnabled = std::nullopt);
    void setFirewalldEnabled(bool enabled);
};
