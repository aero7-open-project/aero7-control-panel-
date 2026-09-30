#pragma once

#include "SettingsCatalog.h"

#include <QDialog>
#include <QVariant>
#include <QVector>

class QLabel;
class QWidget;

// A small, reusable Aero7 property sheet for settings that are still backed by
// Plasma/KWin/KIO configuration files.  The backend files are intentionally
// retained, but their KCM user interfaces are never launched.
class NativeSettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit NativeSettingsDialog(const SettingDefinition &setting,
                                  QWidget *parent = nullptr);
    enum class Kind { Boolean, Integer, Text, Choice };
    struct Option {
        QString label;
        QString file;
        QString group;
        QString key;
        Kind kind = Kind::Boolean;
        QVariant defaultValue;
        QStringList choices;
        QWidget *editor = nullptr;
    };

private:

    bool apply();
    static QVector<Option> optionsFor(const QString &key);
    static QString configPath(const QString &file);
    QVariant editorValue(const Option &option) const;
    void reloadBackends() const;

    SettingDefinition m_setting;
    QVector<Option> m_options;
    QLabel *m_status = nullptr;
};
