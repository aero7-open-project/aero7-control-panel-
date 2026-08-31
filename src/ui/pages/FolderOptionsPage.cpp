#include "FolderOptionsPage.h"
#include "Win7Ui.h"

#include <QCheckBox>
#include <QDir>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>

namespace {
QGroupBox *optionGroup(const QString &title, QVBoxLayout **contents)
{
    auto *group = new QGroupBox(title);
    auto *layout = new QVBoxLayout(group);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(7);
    *contents = layout;
    return group;
}

QString configPath(const QString &name)
{
    return QDir::homePath() + QStringLiteral("/.config/") + name;
}
} // namespace

FolderOptionsPage::FolderOptionsPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    auto *content = Win7::pageScaffold(this, sidebar, 18, 680);
    content->addWidget(Win7::pageTitle(QStringLiteral("Folder Options")));
    content->addSpacing(8);
    auto *tabs = new QTabWidget;

    auto *general = new QWidget;
    auto *generalLayout = new QVBoxLayout(general);
    generalLayout->setContentsMargins(12, 14, 12, 12);
    QVBoxLayout *clickLayout = nullptr;
    generalLayout->addWidget(optionGroup(QStringLiteral("Click items as follows"),
                                          &clickLayout));
    m_singleClick = new QRadioButton(
        QStringLiteral("Single-click to open an item (point to select)"));
    m_doubleClick = new QRadioButton(
        QStringLiteral("Double-click to open an item (single-click to select)"));
    clickLayout->addWidget(m_singleClick);
    clickLayout->addWidget(m_doubleClick);
    generalLayout->addStretch(1);
    tabs->addTab(general, QStringLiteral("General"));

    auto *view = new QWidget;
    auto *viewLayout = new QVBoxLayout(view);
    viewLayout->setContentsMargins(12, 14, 12, 12);
    QVBoxLayout *advancedLayout = nullptr;
    viewLayout->addWidget(optionGroup(QStringLiteral("Advanced settings"),
                                      &advancedLayout));
    m_showHidden = new QCheckBox(QStringLiteral("Show hidden files and folders"));
    m_showPreviews = new QCheckBox(
        QStringLiteral("Show thumbnails and previews in folder windows"));
    m_confirmTrash = new QCheckBox(
        QStringLiteral("Ask for confirmation before moving files to the Trash"));
    auto *extensions = new QCheckBox(QStringLiteral("Show file name extensions"));
    extensions->setChecked(true);
    extensions->setEnabled(false);
    extensions->setToolTip(
        QStringLiteral("Aero7 always shows the complete Linux file name."));
    advancedLayout->addWidget(m_showHidden);
    advancedLayout->addWidget(m_showPreviews);
    advancedLayout->addWidget(m_confirmTrash);
    advancedLayout->addWidget(extensions);
    viewLayout->addStretch(1);
    tabs->addTab(view, QStringLiteral("View"));

    auto *search = new QWidget;
    auto *searchLayout = new QVBoxLayout(search);
    searchLayout->setContentsMargins(12, 14, 12, 12);
    QVBoxLayout *indexLayout = nullptr;
    searchLayout->addWidget(optionGroup(QStringLiteral("What to search"),
                                         &indexLayout));
    m_indexContent = new QCheckBox(
        QStringLiteral("Use the file index to search names and file contents"));
    indexLayout->addWidget(m_indexContent);
    auto *hint = new QLabel(QStringLiteral(
        "When indexing is off, Start menu searches still find applications "
        "and can search file names directly."));
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color:#4B4B4B;"));
    indexLayout->addWidget(hint);
    searchLayout->addStretch(1);
    tabs->addTab(search, QStringLiteral("Search"));
    content->addWidget(tabs, 1);

    auto *footer = new QHBoxLayout;
    m_status = new QLabel;
    m_status->setStyleSheet(QStringLiteral("color:#4B4B4B;"));
    footer->addWidget(m_status, 1);
    auto *defaults = new QPushButton(QStringLiteral("Restore Defaults"));
    connect(defaults, &QPushButton::clicked, this,
            &FolderOptionsPage::restoreDefaults);
    footer->addWidget(defaults);
    auto *apply = new QPushButton(QStringLiteral("Apply"));
    connect(apply, &QPushButton::clicked, this, &FolderOptionsPage::applyState);
    footer->addWidget(apply);
    content->addLayout(footer);
    loadState();
}

void FolderOptionsPage::loadState()
{
    QSettings globals(configPath(QStringLiteral("kdeglobals")), QSettings::IniFormat);
    globals.beginGroup(QStringLiteral("KDE"));
    const bool single = globals.value(QStringLiteral("SingleClick"), false).toBool();
    m_singleClick->setChecked(single);
    m_doubleClick->setChecked(!single);

    QSettings dolphin(configPath(QStringLiteral("dolphinrc")), QSettings::IniFormat);
    dolphin.beginGroup(QStringLiteral("General"));
    m_showHidden->setChecked(dolphin.value(QStringLiteral("ShowHiddenFiles"), false).toBool());
    m_showPreviews->setChecked(dolphin.value(QStringLiteral("ShowPreview"), true).toBool());

    QSettings trash(configPath(QStringLiteral("kiorc")), QSettings::IniFormat);
    trash.beginGroup(QStringLiteral("Confirmations"));
    m_confirmTrash->setChecked(trash.value(QStringLiteral("ConfirmTrash"), true).toBool());

    QSettings baloo(configPath(QStringLiteral("baloofilerc")), QSettings::IniFormat);
    baloo.beginGroup(QStringLiteral("Basic Settings"));
    m_indexContent->setChecked(baloo.value(QStringLiteral("Indexing-Enabled"), true).toBool());
    m_status->setText(QStringLiteral("Folder settings loaded."));
}

void FolderOptionsPage::applyState()
{
    QSettings globals(configPath(QStringLiteral("kdeglobals")), QSettings::IniFormat);
    globals.beginGroup(QStringLiteral("KDE"));
    globals.setValue(QStringLiteral("SingleClick"), m_singleClick->isChecked());
    QSettings dolphin(configPath(QStringLiteral("dolphinrc")), QSettings::IniFormat);
    dolphin.beginGroup(QStringLiteral("General"));
    dolphin.setValue(QStringLiteral("ShowHiddenFiles"), m_showHidden->isChecked());
    dolphin.setValue(QStringLiteral("ShowPreview"), m_showPreviews->isChecked());
    QSettings trash(configPath(QStringLiteral("kiorc")), QSettings::IniFormat);
    trash.beginGroup(QStringLiteral("Confirmations"));
    trash.setValue(QStringLiteral("ConfirmTrash"), m_confirmTrash->isChecked());
    QSettings baloo(configPath(QStringLiteral("baloofilerc")), QSettings::IniFormat);
    baloo.beginGroup(QStringLiteral("Basic Settings"));
    baloo.setValue(QStringLiteral("Indexing-Enabled"), m_indexContent->isChecked());
    QProcess::startDetached(QStringLiteral("balooctl6"),
                            {m_indexContent->isChecked() ? QStringLiteral("enable")
                                                        : QStringLiteral("disable")});
    m_status->setText(QStringLiteral("Folder settings applied."));
}

void FolderOptionsPage::restoreDefaults()
{
    m_doubleClick->setChecked(true);
    m_showHidden->setChecked(false);
    m_showPreviews->setChecked(true);
    m_confirmTrash->setChecked(true);
    m_indexContent->setChecked(true);
    m_status->setText(QStringLiteral("Default choices selected. Choose Apply to save."));
}
