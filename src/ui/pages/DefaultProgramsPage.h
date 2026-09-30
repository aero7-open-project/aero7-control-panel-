#pragma once

#include <QWidget>
#include <QByteArray>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QScrollArea;

class DefaultProgramsPage : public QWidget {
    Q_OBJECT

public:
    explicit DefaultProgramsPage(QScrollArea *sidebar,
                                 QWidget *parent = nullptr);

private:
    struct CommandResult {
        bool started = false;
        int exitCode = -1;
        QByteArray output;
        QByteArray error;
    };

    static QString launcherExecutable();
    static CommandResult runLauncher(const QStringList &arguments);
    void loadStatus();
    void applySelection();
    void createShortcut();
    void updateBackendDetails();
    void setStatus(const QString &text, bool error = false);

    QComboBox *m_browser = nullptr;
    QCheckBox *m_defaultBrowser = nullptr;
    QLabel *m_backendDetails = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_apply = nullptr;
    bool m_wasDefault = false;
    bool m_statusValid = false;
};
