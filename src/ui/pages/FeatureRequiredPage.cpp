#include "FeatureRequiredPage.h"

#include "IconHelper.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

FeatureRequiredPage::FeatureRequiredPage(const FeatureDefinition &feature,
                                         const FeatureStatus &status,
                                         QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("featureRequiredPage"));
    setStyleSheet(QStringLiteral("#featureRequiredPage { background: white; }"));

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(42, 34, 42, 30);
    outer->setSpacing(16);

    auto *heading = new QLabel(
        status.state == FeatureState::Unavailable
            ? tr("This feature is not available")
            : status.state == FeatureState::HardwareNotPresent
                ? tr("Required hardware was not found")
                : tr("This Aero7 feature is not installed"));
    QFont headingFont = heading->font();
    headingFont.setPointSize(15);
    heading->setFont(headingFont);
    heading->setStyleSheet(QStringLiteral("color: #1f4e99;"));
    outer->addWidget(heading);

    auto *row = new QHBoxLayout;
    row->setSpacing(18);
    auto *icon = new QLabel;
    icon->setPixmap(themeIcon({"system-software-install", "preferences-system"})
                        .pixmap(48, 48));
    icon->setFixedSize(48, 48);
    row->addWidget(icon, 0, Qt::AlignTop);
    auto *text = new QVBoxLayout;
    auto *name = new QLabel(feature.name);
    QFont nameFont = name->font();
    nameFont.setPointSize(12);
    nameFont.setBold(true);
    name->setFont(nameFont);
    text->addWidget(name);
    auto *description = new QLabel(feature.description);
    description->setWordWrap(true);
    text->addWidget(description);
    m_status = new QLabel(status.detail);
    m_status->setWordWrap(true);
    m_status->setStyleSheet(QStringLiteral("color: #555555;"));
    text->addWidget(m_status);
    if (!feature.compatibility.isEmpty()) {
        auto *compatibility = new QLabel(feature.compatibility);
        compatibility->setWordWrap(true);
        compatibility->setStyleSheet(QStringLiteral("color: #555555;"));
        text->addWidget(compatibility);
    }
    row->addLayout(text, 1);
    outer->addLayout(row);

    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    outer->addWidget(line);
    outer->addStretch(1);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    m_install = new QPushButton(
        status.state == FeatureState::PartiallyInstalled ? tr("Repair")
        : status.state == FeatureState::Failed ? tr("Try Again")
                                              : tr("Install"));
    m_install->setDefault(true);
    m_openFeatures = new QPushButton(tr("Open Optional Features"));
    auto *cancel = new QPushButton(tr("Cancel"));
    const bool installable = status.state == FeatureState::NotInstalled
                             || status.state == FeatureState::PartiallyInstalled
                             || status.state == FeatureState::Failed;
    m_install->setVisible(installable);
    m_openFeatures->setVisible(feature.visibleInOptionalFeatures);
    buttons->addWidget(m_install);
    buttons->addWidget(m_openFeatures);
    buttons->addWidget(cancel);
    outer->addLayout(buttons);

    connect(m_install, &QPushButton::clicked, this,
            [this, id = feature.id]() { emit installRequested(id); });
    connect(m_openFeatures, &QPushButton::clicked, this,
            [this, id = feature.id]() { emit optionalFeaturesRequested(id); });
    connect(cancel, &QPushButton::clicked, this, &FeatureRequiredPage::cancelRequested);

    QTimer::singleShot(0, this, [this, installable]() {
        if (installable)
            m_install->setFocus(Qt::OtherFocusReason);
        else
            m_openFeatures->setFocus(Qt::OtherFocusReason);
    });
}

void FeatureRequiredPage::setWaitingForInstaller(bool waiting)
{
    m_install->setEnabled(!waiting);
    m_openFeatures->setEnabled(!waiting);
    if (waiting)
        m_status->setText(tr("Waiting for the feature installation to finish…"));
}
