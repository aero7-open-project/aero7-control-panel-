#pragma once

#include "PageId.h"

#include <QPoint>
#include <QSize>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QScrollArea;
class DisplayLayoutWidget;

class DisplayPage : public QWidget {
    Q_OBJECT

public:
    explicit DisplayPage(QScrollArea *sidebar, QWidget *parent = nullptr);

    static QList<SidebarLink> sidebarLinks();
    static QList<SidebarLink> sidebarSeeAlso();

private:
    struct Mode {
        QString id;
        QString name;
        QSize size;
        double refreshRate = 0.0;
    };

    struct Output {
        QString name;
        bool connected = false;
        bool enabled = false;
        int priority = 0;
        QString currentModeId;
        double scale = 1.0;
        int rotation = 1;
        QPoint position;
        QVector<Mode> modes;
    };

    bool loadConfiguration(QString *error = nullptr);
    bool runDoctor(const QStringList &arguments, QString *error = nullptr) const;
    QStringList restoreArguments() const;
    void populateOutputControls();
    void applyChanges();
    void setStatus(const QString &text, bool error = false);

    QVector<Output> m_outputs;
    QVector<Output> m_originalOutputs;
    QComboBox *m_display = nullptr;
    QComboBox *m_resolution = nullptr;
    QComboBox *m_orientation = nullptr;
    QComboBox *m_scale = nullptr;
    QComboBox *m_multiple = nullptr;
    QCheckBox *m_primary = nullptr;
    DisplayLayoutWidget *m_monitorDiagram = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_apply = nullptr;
};
