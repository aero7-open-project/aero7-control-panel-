#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTimeEdit;
class QProcess;
class QScrollArea;

class ParentalControlsPage : public QWidget {
    Q_OBJECT

public:
    explicit ParentalControlsPage(QScrollArea *sidebar, QWidget *parent = nullptr);

private:
    void loadSelectedUser();
    void updateModeControls();
    void applyLimits();

    QComboBox *m_user = nullptr;
    QComboBox *m_mode = nullptr;
    QSpinBox *m_hours = nullptr;
    QTimeEdit *m_start = nullptr;
    QTimeEdit *m_end = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_apply = nullptr;
    QProcess *m_process = nullptr;
};
