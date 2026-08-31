#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QRadioButton;
class QScrollArea;

class FolderOptionsPage final : public QWidget
{
    Q_OBJECT
public:
    explicit FolderOptionsPage(QScrollArea *sidebar, QWidget *parent = nullptr);

private slots:
    void loadState();
    void applyState();
    void restoreDefaults();

private:
    QRadioButton *m_singleClick = nullptr;
    QRadioButton *m_doubleClick = nullptr;
    QCheckBox *m_showHidden = nullptr;
    QCheckBox *m_showPreviews = nullptr;
    QCheckBox *m_confirmTrash = nullptr;
    QCheckBox *m_indexContent = nullptr;
    QLabel *m_status = nullptr;
};
