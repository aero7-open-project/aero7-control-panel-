#pragma once

#include <QDialog>

class QLabel;
class QListWidget;
class QPushButton;
class QProcess;

class BluetoothDialog final : public QDialog {
    Q_OBJECT
public:
    explicit BluetoothDialog(QWidget *parent = nullptr);

private:
    void refresh();
    void run(const QStringList &arguments, const QString &description,
             bool interactive = false);
    QString selectedAddress() const;
    void updateButtons();
    void handlePrompt(const QString &text);

    QListWidget *m_devices = nullptr;
    QLabel *m_adapterState = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_power = nullptr;
    QPushButton *m_scan = nullptr;
    QPushButton *m_pair = nullptr;
    QPushButton *m_connect = nullptr;
    QPushButton *m_disconnect = nullptr;
    QPushButton *m_remove = nullptr;
    QProcess *m_process = nullptr;
    QString m_action;
    QString m_output;
    bool m_answeredPrompt = false;
};
