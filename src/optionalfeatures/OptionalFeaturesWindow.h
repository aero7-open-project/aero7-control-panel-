#pragma once

#include "FeatureCatalog.h"

#include <QHash>
#include <QProcess>
#include <QWidget>

class QLabel;
class QProgressBar;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QDialog;
class QPlainTextEdit;

class OptionalFeaturesWindow : public QWidget {
    Q_OBJECT

public:
    explicit OptionalFeaturesWindow(QWidget *parent = nullptr);
    bool focusFeature(const QString &id);
    bool requestInstall(const QString &id);
    bool requestRemove(const QString &id);

private:
    void populate();
    void updateDescription();
    void updateButtons();
    void applyChanges();
    void startTransaction(const QStringList &changes);
    void readTransactionOutput();
    void finishTransaction(int exitCode, QProcess::ExitStatus status);
    void appendLog(const QString &result, const QString &detail);
    QTreeWidgetItem *itemForFeature(const QString &id) const;

    FeatureCatalog m_catalog;
    QTreeWidget *m_tree = nullptr;
    QTreeWidgetItem *m_kdeSettingsItem = nullptr;
    bool m_originalKdeSettingsShown = false;
    QLabel *m_description = nullptr;
    QLabel *m_compatibility = nullptr;
    QPushButton *m_ok = nullptr;
    QPushButton *m_cancel = nullptr;
    QPushButton *m_open = nullptr;
    QHash<QString, FeatureState> m_initialStates;
    bool m_populating = false;

    QProcess *m_process = nullptr;
    QDialog *m_progressDialog = nullptr;
    QLabel *m_progressLabel = nullptr;
    QProgressBar *m_progress = nullptr;
    QPlainTextEdit *m_details = nullptr;
    QByteArray m_pendingOutput;
    QString m_transactionTechnicalDetails;
    QStringList m_activeChanges;
    bool m_closeAfterTransaction = false;
};
