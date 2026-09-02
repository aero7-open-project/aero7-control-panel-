#pragma once

#include "FeatureCatalog.h"

#include <QWidget>

class QLabel;
class QPushButton;

class FeatureRequiredPage : public QWidget {
    Q_OBJECT

public:
    FeatureRequiredPage(const FeatureDefinition &feature,
                        const FeatureStatus &status,
                        QWidget *parent = nullptr);
    void setWaitingForInstaller(bool waiting);

signals:
    void installRequested(const QString &featureId);
    void optionalFeaturesRequested(const QString &featureId);
    void cancelRequested();

private:
    QLabel *m_status = nullptr;
    QPushButton *m_install = nullptr;
    QPushButton *m_openFeatures = nullptr;
};
