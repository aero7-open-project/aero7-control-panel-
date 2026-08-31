#pragma once

#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;

class AddPrinterDialog final : public QDialog {
    Q_OBJECT
public:
    explicit AddPrinterDialog(QWidget *parent = nullptr);

private:
    void addPrinter();
    QLineEdit *m_name = nullptr;
    QComboBox *m_uri = nullptr;
    QLineEdit *m_description = nullptr;
    QLineEdit *m_location = nullptr;
    QLabel *m_status = nullptr;
};
