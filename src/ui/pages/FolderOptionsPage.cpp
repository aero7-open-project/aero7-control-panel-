#include "FolderOptionsPage.h"
#include "Win7Ui.h"

#include <QCheckBox>
#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTabWidget>
#include <QVBoxLayout>
#include <KConfig>
#include <KConfigGroup>
#include <memory>
#ifdef Q_OS_LINUX
#include <sys/xattr.h>
#include <cerrno>
#endif

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
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QLatin1Char('/') + name;
}

QString viewPropertiesDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/aero7-file-explorer/view_properties/global");
}

// Dolphin reads its view defaults from .directory or the folder's xattr,
// not General/ShowHiddenFiles and General/ShowPreview in dolphinrc.
// Prefer a legacy .directory with actual view groups, just as Dolphin does.
std::unique_ptr<KConfig> viewProperties(QBuffer &metadata, QString &error)
{
    const QString directory = viewPropertiesDirectory();
    auto config = std::make_unique<KConfig>(directory + "/.directory", KConfig::SimpleConfig);
    if (config->hasGroup("Dolphin") || config->hasGroup("Settings"))
        return config;
#ifdef Q_OS_LINUX
    const QByteArray path = QFile::encodeName(directory);
    constexpr auto key = "user.kde.fm.viewproperties#1";
    const auto size = getxattr(path.constData(), key, nullptr, 0);
    if (size < 0) {
        if (errno != ENODATA && errno != ENOTSUP && errno != ENOENT)
            error = "Could not read File Explorer's view metadata. No folder choices were saved.";
        return config;
    }
    if (size > 1024 * 1024) {
        error = "File Explorer's view metadata is too large to read safely.";
        return config;
    }
    QByteArray contents(size, '\0');
    const auto read = getxattr(path.constData(), key, contents.data(), contents.size());
    if (read < 0) {
        error = "File Explorer's view metadata changed or could not be read. Try again.";
        return config;
    }
    contents.resize(read);
    metadata.setData(contents);
    metadata.open(QIODevice::ReadWrite);
    KConfig stored(std::shared_ptr<QIODevice>(&metadata, [](QIODevice *) {}), KConfig::SimpleConfig);
    for (const auto &name : {"Dolphin", "Settings"}) {
        KConfigGroup source(&stored, name);
        KConfigGroup target(config.get(), name);
        source.copyTo(&target);
    }
#else
    Q_UNUSED(metadata);
#endif
    return config;
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
    m_singleClick->setObjectName("folderSingleClick");
    m_doubleClick = new QRadioButton(
        QStringLiteral("Double-click to open an item (single-click to select)"));
    m_doubleClick->setObjectName("folderDoubleClick");
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
    m_showHidden->setObjectName("folderShowHidden");
    m_showPreviews = new QCheckBox(
        QStringLiteral("Show thumbnails and previews in folder windows"));
    m_showPreviews->setObjectName("folderShowPreviews");
    m_confirmTrash = new QCheckBox(
        QStringLiteral("Ask for confirmation before moving files to the Trash"));
    m_confirmTrash->setObjectName("folderConfirmTrash");
    auto *extensions = new QCheckBox(QStringLiteral("Show file name extensions"));
    extensions->setChecked(true);
    extensions->setEnabled(false);
    extensions->setToolTip(
        QStringLiteral("Aero7 always shows the complete Linux file name."));
    advancedLayout->addWidget(m_showHidden);
    advancedLayout->addWidget(m_showPreviews);
    advancedLayout->addWidget(m_confirmTrash);
    advancedLayout->addWidget(extensions);
    auto *viewHint = new QLabel(
        "These are File Explorer's default folder-view choices. Reopen File Explorer "
        "to use them. Custom views saved for individual folders are preserved.");
    viewHint->setWordWrap(true);
    advancedLayout->addWidget(viewHint);
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
    m_indexContent->setObjectName("folderIndexContent");
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
    m_status->setObjectName(QStringLiteral("folderOptionsStatus"));
    m_status->setWordWrap(true);
    m_status->setStyleSheet(QStringLiteral("color:#4B4B4B;"));
    footer->addWidget(m_status, 1);
    auto *defaults = new QPushButton(QStringLiteral("Restore Defaults"));
    connect(defaults, &QPushButton::clicked, this,
            &FolderOptionsPage::restoreDefaults);
    footer->addWidget(defaults);
    auto *apply = new QPushButton(QStringLiteral("Apply"));
    apply->setObjectName(QStringLiteral("folderOptionsApply"));
    connect(apply, &QPushButton::clicked, this, &FolderOptionsPage::applyState);
    footer->addWidget(apply);
    content->addLayout(footer);
    loadState();
}

void FolderOptionsPage::loadState()
{
    KConfig globals(configPath("kdeglobals"), KConfig::SimpleConfig);
    const bool single = KConfigGroup(&globals, "KDE").readEntry("SingleClick", false);
    m_singleClick->setChecked(single);
    m_doubleClick->setChecked(!single);

    QBuffer metadata;
    QString error;
    const auto view = viewProperties(metadata, error);
    m_showHidden->setChecked(KConfigGroup(view.get(), "Settings").readEntry("HiddenFilesShown", false));
    m_showPreviews->setChecked(KConfigGroup(view.get(), "Dolphin").readEntry("PreviewsShown", true));
    // The metadata copy is for reading here. KConfig otherwise syncs it on
    // destruction, which would change the user's defaults just by opening us.
    view->markAsClean();

    KConfig trash(configPath("kiorc"), KConfig::SimpleConfig);
    KConfig recycleBin(configPath("trashrc"), KConfig::SimpleConfig);
    const KConfigGroup recycleChoices(&recycleBin, "Aero7");
    // Aero7 File Explorer's Windows-style confirmation uses its Recycle Bin
    // preferences, while other KIO clients use Confirmations/ConfirmTrash.
    m_confirmTrash->setChecked(recycleChoices.readEntry("ConfirmDelete",
        KConfigGroup(&trash, "Confirmations").readEntry("ConfirmTrash", true)));
    const bool deleteImmediately = recycleChoices.readEntry("DeleteImmediately", false);
    m_confirmTrash->setEnabled(!deleteImmediately);
    m_confirmTrash->setToolTip(deleteImmediately
        ? "The Recycle Bin is configured to delete files immediately. Change "
          "Recycle Bin properties to enable moving files to the Trash. "
          "Permanent deletion keeps its own confirmation."
        : "Controls File Explorer's Recycle Bin prompt and KIO's Trash confirmation.");

    KConfig baloo(configPath("baloofilerc"), KConfig::SimpleConfig);
    m_indexContent->setChecked(KConfigGroup(&baloo, "Basic Settings").readEntry("Indexing-Enabled", true));
    m_status->setText(error.isEmpty() ? "Folder settings loaded." : error);
    findChild<QPushButton *>("folderOptionsApply")->setEnabled(error.isEmpty());
}

void FolderOptionsPage::applyState()
{
    QBuffer metadata;
    QString error;
    const auto view = viewProperties(metadata, error);
    if (!error.isEmpty()) {
        m_status->setText(error);
        return;
    }
    if (!QDir().mkpath(viewPropertiesDirectory())) {
        m_status->setText("Could not create File Explorer's view-defaults folder. No choices were saved.");
        return;
    }
    KConfig globals(configPath("kdeglobals"), KConfig::SimpleConfig);
    KConfigGroup(&globals, "KDE").writeEntry("SingleClick", m_singleClick->isChecked(), KConfigGroup::Notify);
    KConfigGroup(view.get(), "Settings").writeEntry("HiddenFilesShown", m_showHidden->isChecked());
    KConfigGroup appearance(view.get(), "Dolphin");
    appearance.writeEntry("PreviewsShown", m_showPreviews->isChecked());
    appearance.writeEntry("Version", qMax(4, appearance.readEntry("Version", 4)));
    appearance.writeEntry("Timestamp", QDateTime::currentDateTime());
    KConfig trash(configPath("kiorc"), KConfig::SimpleConfig);
    KConfig recycleBin(configPath("trashrc"), KConfig::SimpleConfig);
    if (m_confirmTrash->isEnabled()) {
        KConfigGroup(&trash, "Confirmations").writeEntry("ConfirmTrash", m_confirmTrash->isChecked(), KConfigGroup::Notify);
        KConfigGroup(&recycleBin, "Aero7").writeEntry("ConfirmDelete", m_confirmTrash->isChecked());
    }
    KConfig baloo(configPath("baloofilerc"), KConfig::SimpleConfig);
    KConfigGroup(&baloo, "Basic Settings").writeEntry("Indexing-Enabled", m_indexContent->isChecked());
    // Remove only the ineffective keys written by previous Control Panel versions.
    KConfig dolphin(configPath("dolphinrc"), KConfig::SimpleConfig);
    for (const auto &name : {"%General", "General"}) {
        KConfigGroup legacy(&dolphin, name);
        legacy.deleteEntry("ShowHiddenFiles");
        legacy.deleteEntry("ShowPreview");
    }
    KConfigGroup(&baloo, "Basic%20Settings").deleteEntry("Indexing-Enabled");
    for (KConfig *settings : {&globals, view.get(), &trash, &recycleBin, &baloo, &dolphin}) {
        if (!settings->sync()) {
            m_status->setText(QStringLiteral("Could not save all folder settings. Check file permissions."));
            return;
        }
    }

    const QString tool = QStandardPaths::findExecutable(QStringLiteral("balooctl6"));
    if (tool.isEmpty()) {
        m_status->setText(QStringLiteral("Folder choices saved, but the file-index service is not installed."));
        return;
    }
    QProcess indexer;
    indexer.start(tool, {m_indexContent->isChecked() ? QStringLiteral("enable")
                                                   : QStringLiteral("disable")});
    if (!indexer.waitForStarted(2000) || !indexer.waitForFinished(5000)
        || indexer.exitStatus() != QProcess::NormalExit || indexer.exitCode() != 0) {
        const QString detail = QString::fromUtf8(indexer.readAllStandardError()).trimmed();
        m_status->setText(detail.isEmpty()
            ? QStringLiteral("Folder choices saved, but the file index could not be changed.")
            : QStringLiteral("Folder choices saved, but the file index failed: %1").arg(detail));
        return;
    }
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
