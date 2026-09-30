#pragma once

#include <QWidget>
#include <QString>
#include <QStringList>
#include "PageId.h"
#include <QList>
#include <QHash>

class QScrollArea;
class QVBoxLayout;
class QButtonGroup;
class QAbstractButton;
class QRadioButton;
class QLabel;
class QObject;
class QEvent;

// The "Power Options" detail page, a Linux-flavoured take on the Windows 7
// "Select a power plan" screen.
//
// On machines that run power-profiles-daemon (PPD), the same service KDE's and
// GNOME's power widgets drive, the radio plans are wired live to it over the
// system bus:
//   * Reading  net.hadess.PowerProfiles.Profiles / .ActiveProfile lists the
//     available plans and the active one.
//   * Selecting a plan writes .ActiveProfile and confirms the result with a
//     readback before reporting success.
//   * A PropertiesChanged subscription keeps the selection in sync when the
//     profile is changed elsewhere (KDE tray, another app, on AC/battery).
//
// PPD's three standard profiles map onto the three Windows plans:
//   performance  -> "High performance"
//   balanced     -> "Balanced (recommended)"
//   power-saver  -> "Power saver"
//
// When PPD is absent (typical on a desktop with no profile switching) the page
// shows a disabled "Balanced (recommended)" placeholder and explains that
// profile switching is unavailable rather than offering a no-op selection.
class PowerOptionsPage : public QWidget {
    Q_OBJECT

public:
    explicit PowerOptionsPage(QScrollArea *sidebar, QWidget *parent = nullptr);

    // Left-nav entries shown by MainWindow's subpage sidebar.
    static QList<SidebarLink> sidebarLinks();
    static QList<SidebarLink> sidebarSeeAlso();

protected:
    // Lets a click on a plan's name label select that plan, like Windows.
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    // One selectable plan, as presented to the user. `profileId` is the raw PPD
    // profile name ("balanced", …) used to read/write the daemon; empty for the
    // synthetic fall-back plan shown when PPD is unavailable.
    struct Plan {
        QString profileId;
        QString name;          // e.g. "Balanced (recommended)"
        QString description;   // the grey line under the name
        bool    additional;    // true -> lives under "Show additional plans"
    };
    struct BatteryInfo {
        bool available = false;
        bool onBattery = false;
        double percentage = 0.0;
        QString state;
        QString remaining;
        QString model;
    };

    // Query PPD over the system bus. Returns the profiles it advertises (in the
    // daemon's own low-to-high order) and the active one. `ok` is false when PPD is
    // not reachable, which triggers the static fall-back layout.
    QStringList availableProfiles(bool &ok) const;
    QString     activeProfile() const;
    void        setActiveProfile(const QString &profileId);
    BatteryInfo batteryInfo() const;

    // Build the visual plan list and the additional-plans expander.
    void buildPlans(const QStringList &profiles);
    QWidget *buildPlanRow(const Plan &plan);

    // Re-checks the radio matching the daemon's current ActiveProfile. Used both
    // on load and from the PropertiesChanged signal handler.
    void syncSelection();

private slots:
    void onProfilesPropertiesChanged(const QString &interface,
                                     const QVariantMap &changed,
                                     const QStringList &invalidated);

private:
    QVBoxLayout  *m_planList      = nullptr;   // holds the preferred-plan rows
    QVBoxLayout  *m_additionalBox = nullptr;   // holds the additional-plan rows
    QWidget      *m_additionalWrap = nullptr;  // toggled by the expander
    QWidget      *m_additionalHeader = nullptr; // heading row, hidden if empty
    QButtonGroup *m_group         = nullptr;
    QLabel       *m_status        = nullptr;
    bool          m_ppdAvailable  = false;
    bool          m_applying      = false;
    bool          m_syncing       = false;     // guards programmatic re-checks

    // Maps each plan's name label to its radio, so clicking the name selects it.
    QHash<QObject *, QRadioButton *> m_nameToRadio;
};
