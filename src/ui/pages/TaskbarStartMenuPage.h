#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QScrollArea;
class QSpinBox;

class TaskbarStartMenuPage : public QWidget {
    Q_OBJECT

public:
    explicit TaskbarStartMenuPage(QScrollArea *sidebar,
                                  QWidget *parent = nullptr);

private:
    void loadState();
    void applyState();
    bool evaluateScript(const QString &script, QString *output = nullptr) const;

    QCheckBox *m_lockTaskbar = nullptr;
    QCheckBox *m_autoHide = nullptr;
    QCheckBox *m_smallIcons = nullptr;
    QComboBox *m_grouping = nullptr;
    QCheckBox *m_previews = nullptr;
    QCheckBox *m_recentPrograms = nullptr;
    QCheckBox *m_jumpLists = nullptr;
    QSpinBox *m_programCount = nullptr;
    QLabel *m_status = nullptr;
};
