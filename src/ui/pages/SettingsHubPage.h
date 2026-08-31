#pragma once

#include "SettingsCatalog.h"

#include <QWidget>

class QScrollArea;

// Aero7's replacement landing page for groups that previously opened the KDE
// System Settings application.  It owns all user-facing wording while keeping
// compatibility metadata on the controls for automated audits and progressive
// replacement of each bridge with a native implementation.
class SettingsHubPage : public QWidget {
    Q_OBJECT

public:
    explicit SettingsHubPage(SettingsSection section, QScrollArea *sidebar,
                             const QString &titleOverride = {},
                             const QString &descriptionOverride = {},
                             QWidget *parent = nullptr);

signals:
    void navigateRequested(PageId page);
    void appletRequested(const QString &id);

private:
    void activate(const SettingDefinition &setting);
};
