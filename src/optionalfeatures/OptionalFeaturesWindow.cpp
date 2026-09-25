#include "OptionalFeaturesWindow.h"

#include "IconHelper.h"

#include <QApplication>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSet>
#include <QStandardPaths>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <utility>

namespace {
constexpr int FeatureIdRole = Qt::UserRole + 1;
const QString KdeSettingsFeatureId = QStringLiteral("kde-system-settings-visibility");

QString desktopUser()
{
    QString user = qEnvironmentVariable("USER");
    if (user.isEmpty())
        user = qEnvironmentVariable("LOGNAME");
    return user;
}

QString actionText(const QString &change, const FeatureCatalog &catalog)
{
    const QString id = change.section(QLatin1Char('='), 0, 0);
    const bool install = change.endsWith(QLatin1String("=install"));
    const FeatureDefinition *feature = catalog.find(id);
    return QStringLiteral("%1 %2")
        .arg(install ? QStringLiteral("Install") : QStringLiteral("Remove"),
             feature ? feature->name : id);
}

QString systemSettingsVisibilityProgram()
{
    return QStringLiteral("/usr/bin/aero7-system-settings-visibility");
}

bool systemSettingsShown()
{
    QProcess process;
    process.start(systemSettingsVisibilityProgram(), {QStringLiteral("status")});
    return process.waitForFinished(3000)
        && process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0
        && process.readAllStandardOutput().trimmed() == QByteArray("enabled");
}
} // namespace

OptionalFeaturesWindow::OptionalFeaturesWindow(QWidget *parent)
    : QWidget(parent), m_catalog(FeatureCatalog::load())
{
    setWindowTitle(tr("Aero7 Features"));
    setWindowIcon(aero7ResourceIcon(
        QStringLiteral(":/aero7/icons/app/aero7-optional-features.png"),
        QStringLiteral("optional-features")));
    resize(650, 520);
    setMinimumSize(560, 430);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 12);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("Turn Aero7 features on or off"));
    QFont titleFont = title->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setStyleSheet(QStringLiteral("color: #1f4e99;"));
    layout->addWidget(title);

    auto *intro = new QLabel(tr(
        "Select a check box to turn on a feature. Clear a check box to turn it off. "
        "A filled box means that only part of the feature is installed."));
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_tree = new QTreeWidget;
    m_tree->setObjectName(QStringLiteral("optionalFeaturesTree"));
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({tr("Feature"), tr("Status"), tr("Size")});
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->setRootIsDecorated(true);
    m_tree->setAlternatingRowColors(false);
    layout->addWidget(m_tree, 1);

    auto *detailsFrame = new QWidget;
    auto *detailsLayout = new QVBoxLayout(detailsFrame);
    detailsLayout->setContentsMargins(4, 3, 4, 3);
    detailsLayout->setSpacing(2);
    m_description = new QLabel;
    m_description->setWordWrap(true);
    m_compatibility = new QLabel;
    m_compatibility->setWordWrap(true);
    m_compatibility->setStyleSheet(QStringLiteral("color: #555555;"));
    detailsLayout->addWidget(m_description);
    detailsLayout->addWidget(m_compatibility);
    layout->addWidget(detailsFrame);

    auto *buttons = new QHBoxLayout;
    m_open = new QPushButton(tr("Open"));
    m_open->setEnabled(false);
    buttons->addWidget(m_open);
    buttons->addStretch(1);
    m_ok = new QPushButton(tr("OK"));
    m_ok->setDefault(true);
    m_cancel = new QPushButton(tr("Cancel"));
    buttons->addWidget(m_ok);
    buttons->addWidget(m_cancel);
    layout->addLayout(buttons);

    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this, &OptionalFeaturesWindow::updateDescription);
    connect(m_tree, &QTreeWidget::itemChanged, this,
            [this](QTreeWidgetItem *item) {
                if (m_populating)
                    return;
                if (item == m_kdeSettingsItem) {
                    const QString status = item->checkState(0) == Qt::Checked
                        ? tr("Shown") : tr("Hidden");
                    if (item->text(1) != status)
                        item->setText(1, status);
                }
                updateButtons();
            });
    connect(m_ok, &QPushButton::clicked, this, &OptionalFeaturesWindow::applyChanges);
    connect(m_cancel, &QPushButton::clicked, this, &QWidget::close);
    connect(m_open, &QPushButton::clicked, this, [this]() {
        const auto selected = m_tree->selectedItems();
        if (selected.isEmpty())
            return;
        const FeatureDefinition *feature = m_catalog.find(
            selected.first()->data(0, FeatureIdRole).toString());
        if (!feature || feature->launch.isEmpty())
            return;
        QProcess::startDetached(feature->launch.first(), feature->launch.mid(1));
    });

    populate();
}

void OptionalFeaturesWindow::populate()
{
    m_populating = true;
    m_kdeSettingsItem = nullptr;
    m_tree->clear();
    m_initialStates.clear();
    m_originalKdeSettingsShown = systemSettingsShown();
    QHash<QString, QTreeWidgetItem *> categories;

    if (!m_catalog.isValid()) {
        auto *error = new QTreeWidgetItem(m_tree, {tr("The feature catalog could not be loaded.")});
        error->setDisabled(true);
        m_ok->setEnabled(false);
        m_populating = false;
        return;
    }

    const auto categoryFor = [this, &categories](const QString &name) {
        QTreeWidgetItem *category = categories.value(name);
        if (!category) {
            category = new QTreeWidgetItem(m_tree, {name});
            category->setFlags(Qt::ItemIsEnabled);
            QFont font = category->font(0);
            font.setBold(true);
            category->setFont(0, font);
            categories.insert(name, category);
        }
        return category;
    };

    for (const FeatureDefinition &feature : m_catalog.features()) {
        if (!feature.visibleInOptionalFeatures)
            continue;
        QTreeWidgetItem *category = categoryFor(feature.category);

        const FeatureStatus status = m_catalog.status(feature);
        auto *item = new QTreeWidgetItem(category);
        item->setText(0, feature.name);
        item->setText(1, FeatureCatalog::stateName(status.state));
        item->setText(2, feature.installSizeMiB > 0
                           ? tr("%1 MB").arg(feature.installSizeMiB)
                           : QString());
        item->setData(0, FeatureIdRole, feature.id);
        item->setToolTip(0, status.detail);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        if (FeatureCatalog::isEnabled(status.state))
            item->setCheckState(0, Qt::Checked);
        else if (status.state == FeatureState::HardwareNotPresent
                 && status.missingPackages.isEmpty())
            item->setCheckState(0, Qt::Checked);
        else if (status.state == FeatureState::PartiallyInstalled)
            item->setCheckState(0, Qt::PartiallyChecked);
        else
            item->setCheckState(0, Qt::Unchecked);

        if (feature.availabilityMode == QLatin1String("core")
            || status.state == FeatureState::Unavailable
            || status.state == FeatureState::HardwareNotPresent) {
            item->setDisabled(true);
        }
        m_initialStates.insert(feature.id, status.state);
    }
    m_kdeSettingsItem = new QTreeWidgetItem(
        categoryFor(QStringLiteral("System and security")));
    m_kdeSettingsItem->setText(0, tr("KDE System Settings"));
    m_kdeSettingsItem->setText(1, m_originalKdeSettingsShown ? tr("Shown") : tr("Hidden"));
    m_kdeSettingsItem->setData(0, FeatureIdRole, KdeSettingsFeatureId);
    m_kdeSettingsItem->setToolTip(0, tr(
        "Show or hide the KDE System Settings application in menus and search."));
    m_kdeSettingsItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable
                                | Qt::ItemIsUserCheckable);
    m_kdeSettingsItem->setCheckState(0, m_originalKdeSettingsShown
                                           ? Qt::Checked : Qt::Unchecked);
    m_tree->expandAll();
    if (m_tree->topLevelItemCount() && m_tree->topLevelItem(0)->childCount())
        m_tree->setCurrentItem(m_tree->topLevelItem(0)->child(0));
    m_populating = false;
    updateDescription();
    updateButtons();
}

QTreeWidgetItem *OptionalFeaturesWindow::itemForFeature(const QString &id) const
{
    QTreeWidgetItemIterator iterator(m_tree);
    while (*iterator) {
        if ((*iterator)->data(0, FeatureIdRole).toString() == id)
            return *iterator;
        ++iterator;
    }
    return nullptr;
}

bool OptionalFeaturesWindow::focusFeature(const QString &id)
{
    QTreeWidgetItem *item = itemForFeature(id);
    if (!item)
        return false;
    m_tree->setCurrentItem(item);
    m_tree->scrollToItem(item, QAbstractItemView::PositionAtCenter);
    return true;
}

bool OptionalFeaturesWindow::requestInstall(const QString &id)
{
    QTreeWidgetItem *item = itemForFeature(id);
    if (!item || item->isDisabled())
        return false;
    item->setCheckState(0, Qt::Checked);
    m_closeAfterTransaction = true;
    // Direct Control Panel requests arrive before the first window frame has
    // been exposed.  Let KWin activate the parent first; otherwise a modal
    // confirmation can be stacked behind it on Wayland.
    QTimer::singleShot(250, this, [this]() {
        raise();
        activateWindow();
        applyChanges();
    });
    return true;
}

bool OptionalFeaturesWindow::requestRemove(const QString &id)
{
    QTreeWidgetItem *item = itemForFeature(id);
    if (!item || item->isDisabled()
        || !FeatureCatalog::isEnabled(m_initialStates.value(id)))
        return false;
    item->setCheckState(0, Qt::Unchecked);
    m_closeAfterTransaction = true;
    QTimer::singleShot(250, this, [this]() {
        raise();
        activateWindow();
        applyChanges();
    });
    return true;
}

void OptionalFeaturesWindow::updateDescription()
{
    const auto selected = m_tree->selectedItems();
    if (selected.isEmpty()) {
        m_description->clear();
        m_compatibility->clear();
        m_open->setEnabled(false);
        return;
    }
    const QString id = selected.first()->data(0, FeatureIdRole).toString();
    if (id == KdeSettingsFeatureId) {
        m_description->setText(tr(
            "Show the KDE System Settings application in menus and search. "
            "Aero7 Control Panel stays available when this is turned off."));
        m_compatibility->clear();
        m_open->setEnabled(false);
        return;
    }
    const FeatureDefinition *feature = m_catalog.find(id);
    if (!feature) {
        m_description->clear();
        m_compatibility->clear();
        m_open->setEnabled(false);
        return;
    }
    const FeatureStatus status = m_catalog.status(*feature, false);
    m_description->setText(feature->description + QStringLiteral("\n") + status.detail);
    m_compatibility->setText(feature->compatibility);
    m_open->setEnabled(FeatureCatalog::isEnabled(status.state)
                       && !feature->launch.isEmpty());
}

void OptionalFeaturesWindow::updateButtons()
{
    bool changed = (m_kdeSettingsItem->checkState(0) == Qt::Checked)
                   != m_originalKdeSettingsShown;
    for (auto it = m_initialStates.cbegin(); it != m_initialStates.cend(); ++it) {
        QTreeWidgetItem *item = itemForFeature(it.key());
        if (!item || item->isDisabled())
            continue;
        const bool initiallyOn = FeatureCatalog::isEnabled(it.value());
        const bool requestedOn = item->checkState(0) == Qt::Checked;
        const bool repair = it.value() == FeatureState::PartiallyInstalled && requestedOn;
        if (initiallyOn != requestedOn || repair) {
            changed = true;
            break;
        }
    }
    m_ok->setEnabled(changed);
}

void OptionalFeaturesWindow::applyChanges()
{
    QStringList changes;
    const bool showKdeSettings = m_kdeSettingsItem->checkState(0) == Qt::Checked;
    const bool visibilityChanged = showKdeSettings != m_originalKdeSettingsShown;
    int totalSize = 0;
    bool logout = false;
    bool reboot = false;
    for (auto it = m_initialStates.cbegin(); it != m_initialStates.cend(); ++it) {
        QTreeWidgetItem *item = itemForFeature(it.key());
        const FeatureDefinition *feature = m_catalog.find(it.key());
        if (!item || !feature || item->isDisabled())
            continue;
        const bool initiallyOn = FeatureCatalog::isEnabled(it.value());
        const bool requestedOn = item->checkState(0) == Qt::Checked;
        const bool repair = it.value() == FeatureState::PartiallyInstalled && requestedOn;
        if (initiallyOn == requestedOn && !repair)
            continue;
        changes << feature->id + (requestedOn ? QStringLiteral("=install")
                                               : QStringLiteral("=remove"));
        if (requestedOn)
            totalSize += feature->installSizeMiB;
        logout |= feature->logoutRequired;
        reboot |= feature->rebootRequired;
    }
    if (changes.isEmpty() && !visibilityChanged) {
        close();
        return;
    }

    // Expand catalog-declared dependencies before asking for confirmation, so
    // the user sees and approves every feature that the helper will touch.
    const QStringList directlyRequested = changes;
    QSet<QString> changeSet(changes.cbegin(), changes.cend());
    for (const QString &change : directlyRequested) {
        if (!change.endsWith(QLatin1String("=install")))
            continue;
        const FeatureDefinition *feature = m_catalog.find(change.section('=', 0, 0));
        if (!feature)
            continue;
        for (const QString &dependencyId : feature->dependencies) {
            const FeatureDefinition *dependency = m_catalog.find(dependencyId);
            if (!dependency || FeatureCatalog::isEnabled(
                    m_catalog.status(*dependency, false).state))
                continue;
            const QString dependencyChange = dependencyId + QStringLiteral("=install");
            if (changeSet.contains(dependencyChange))
                continue;
            changeSet.insert(dependencyChange);
            changes << dependencyChange;
            totalSize += dependency->installSizeMiB;
            logout |= dependency->logoutRequired;
            reboot |= dependency->rebootRequired;
        }
    }

    QStringList actions;
    QStringList requiredPackages;
    for (const QString &change : changes)
        actions << QStringLiteral("• ") + actionText(change, m_catalog);
    if (visibilityChanged)
        actions << (showKdeSettings
            ? tr("• Show KDE System Settings in applications and search")
            : tr("• Hide KDE System Settings from applications and search"));
    for (const QString &change : changes) {
        if (!change.endsWith(QLatin1String("=install")))
            continue;
        const FeatureDefinition *feature = m_catalog.find(change.section('=', 0, 0));
        if (!feature || (feature->packages.isEmpty() && feature->supportPackages.isEmpty()))
            continue;
        const FeatureStatus state = m_catalog.status(*feature, false);
        const QStringList packages = state.missingPackages.isEmpty()
            ? feature->packages + feature->supportPackages : state.missingPackages;
        requiredPackages << QStringLiteral("• %1: %2")
                                .arg(feature->name, packages.join(QStringLiteral(", ")));
    }
    QString message = tr("Aero7 will make these changes:\n\n%1")
                          .arg(actions.join(QLatin1Char('\n')));
    if (!requiredPackages.isEmpty())
        message += tr("\n\nRequired packages:\n%1")
                       .arg(requiredPackages.join(QLatin1Char('\n')));
    if (totalSize)
        message += tr("\n\nApproximate installed size: %1 MB").arg(totalSize);
    if (logout)
        message += tr("\nYou may need to sign out before every component is available.");
    if (reboot)
        message += tr("\nA restart will be required, but Aero7 will not restart automatically.");
    if (!changes.isEmpty())
        message += tr("\n\nRemoving a feature keeps user configuration and data.");
    if (QMessageBox::question(this, tr("Confirm feature changes"), message,
                              QMessageBox::Ok | QMessageBox::Cancel,
                              QMessageBox::Ok) != QMessageBox::Ok)
        return;
    if (visibilityChanged) {
        QProcess visibility;
        visibility.start(systemSettingsVisibilityProgram(),
                         {showKdeSettings ? QStringLiteral("enable")
                                          : QStringLiteral("disable")});
        if (!visibility.waitForFinished(35000)
            || visibility.exitStatus() != QProcess::NormalExit || visibility.exitCode() != 0) {
            QMessageBox::critical(this, tr("Aero7 Features"),
                tr("KDE System Settings visibility could not be changed.\n%1")
                    .arg(QString::fromUtf8(visibility.readAllStandardError()).trimmed()));
            populate();
            return;
        }
        m_originalKdeSettingsShown = showKdeSettings;
    }
    if (changes.isEmpty()) {
        close();
        return;
    }
    startTransaction(changes);
}

void OptionalFeaturesWindow::startTransaction(const QStringList &changes)
{
    const QString helper = QStringLiteral("/usr/lib/aero7-control-panel/aero7-feature-helper");
    if (!QFile::exists(helper) || QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty()) {
        QMessageBox::critical(this, tr("Aero7 Features"),
            tr("The privileged feature helper is not installed correctly."));
        return;
    }

    m_activeChanges = changes;
    for (const QString &change : changes) {
        QTreeWidgetItem *item = itemForFeature(change.section(QLatin1Char('='), 0, 0));
        if (!item)
            continue;
        const FeatureState state = change.endsWith(QLatin1String("=install"))
            ? FeatureState::Installing : FeatureState::Removing;
        item->setText(1, FeatureCatalog::stateName(state));
    }
    m_pendingOutput.clear();
    m_transactionTechnicalDetails.clear();
    m_progressDialog = new QDialog(this);
    m_progressDialog->setWindowTitle(tr("Changing Aero7 features"));
    m_progressDialog->setModal(true);
    m_progressDialog->setMinimumWidth(500);
    auto *layout = new QVBoxLayout(m_progressDialog);
    m_progressLabel = new QLabel(tr("Waiting for administrator authorization…"));
    m_progressLabel->setWordWrap(true);
    layout->addWidget(m_progressLabel);
    m_progress = new QProgressBar;
    m_progress->setRange(0, 0);
    layout->addWidget(m_progress);
    m_details = new QPlainTextEdit;
    m_details->setReadOnly(true);
    m_details->setMaximumHeight(150);
    layout->addWidget(m_details);
    auto *closeButton = new QPushButton(tr("Cancel"));
    closeButton->setEnabled(false); // package transactions must not be interrupted midway
    auto *buttonRow = new QHBoxLayout;
    buttonRow->addStretch(1);
    buttonRow->addWidget(closeButton);
    layout->addLayout(buttonRow);

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput,
            this, &OptionalFeaturesWindow::readTransactionOutput);
    connect(m_process, &QProcess::readyReadStandardError,
            this, &OptionalFeaturesWindow::readTransactionOutput);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &OptionalFeaturesWindow::finishTransaction);

    QStringList args{helper};
    const QString user = desktopUser();
    if (!user.isEmpty())
        args << QStringLiteral("--user") << user;
    args << changes;
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->start(QStringLiteral("/usr/bin/pkexec"), args);
    m_progressDialog->show();
}

void OptionalFeaturesWindow::readTransactionOutput()
{
    if (!m_process)
        return;
    m_pendingOutput += m_process->readAll();
    while (true) {
        const int newline = m_pendingOutput.indexOf('\n');
        if (newline < 0)
            break;
        const QByteArray line = m_pendingOutput.left(newline).trimmed();
        m_pendingOutput.remove(0, newline + 1);
        if (line.isEmpty())
            continue;
        const QJsonDocument document = QJsonDocument::fromJson(line);
        if (!document.isObject()) {
            m_details->appendPlainText(QString::fromUtf8(line));
            continue;
        }
        const QJsonObject object = document.object();
        const QString message = object.value(QStringLiteral("message")).toString();
        const QString technical = object.value(QStringLiteral("detail")).toString();
        if (!technical.isEmpty()) {
            if (!m_transactionTechnicalDetails.isEmpty())
                m_transactionTechnicalDetails += QLatin1Char('\n');
            m_transactionTechnicalDetails += technical;
        }
        m_progressLabel->setText(message);
        m_details->appendPlainText(message);
        if (object.contains(QStringLiteral("percent"))) {
            m_progress->setRange(0, 100);
            m_progress->setValue(object.value(QStringLiteral("percent")).toInt());
        }
    }
}

void OptionalFeaturesWindow::finishTransaction(int exitCode, QProcess::ExitStatus status)
{
    readTransactionOutput();
    const bool success = status == QProcess::NormalExit && exitCode == 0;
    const bool authCancelled = status == QProcess::NormalExit
                               && (exitCode == 126 || exitCode == 127);
    const QString visibleDetail = m_details ? m_details->toPlainText().trimmed() : QString();
    QString auditDetail = visibleDetail;
    if (!m_transactionTechnicalDetails.isEmpty()) {
        if (!auditDetail.isEmpty())
            auditDetail += QStringLiteral("\n\nTechnical detail:\n");
        auditDetail += m_transactionTechnicalDetails;
    }
    appendLog(success ? QStringLiteral("success")
                      : (authCancelled ? QStringLiteral("authorization-cancelled")
                                       : QStringLiteral("failed")),
              auditDetail);
    if (m_progressDialog)
        m_progressDialog->accept();
    if (m_process)
        m_process->deleteLater();
    if (m_progressDialog)
        m_progressDialog->deleteLater();
    m_process = nullptr;
    m_progressDialog = nullptr;
    m_details = nullptr;
    m_progress = nullptr;
    m_progressLabel = nullptr;

    if (!success) {
        for (const QString &change : std::as_const(m_activeChanges)) {
            QTreeWidgetItem *item = itemForFeature(change.section(QLatin1Char('='), 0, 0));
            if (item)
                item->setText(1, FeatureCatalog::stateName(FeatureState::Failed));
        }
        QString message = authCancelled
            ? tr("Administrator authorization was cancelled. No requested changes were made.")
            : tr("Aero7 could not complete every requested change. The feature list will be refreshed.");
        if (!authCancelled && !visibleDetail.isEmpty())
            message += QStringLiteral("\n\n") + visibleDetail;
        message += tr("\n\nSee ~/.local/state/aero7/optional-features.jsonl for details.");
        if (authCancelled) {
            QMessageBox::information(this, tr("Aero7 Features"), message);
            if (m_closeAfterTransaction)
                close();
            else
                populate();
            return;
        }

        const QStringList retryChanges = m_activeChanges;
        const QString focusId = retryChanges.isEmpty()
            ? QString() : retryChanges.first().section(QLatin1Char('='), 0, 0);
        QMessageBox box(QMessageBox::Critical, tr("Aero7 Features"), message,
                        QMessageBox::NoButton, this);
        auto *tryAgain = box.addButton(tr("Try Again"), QMessageBox::AcceptRole);
        auto *openFeatures = box.addButton(tr("Open Optional Features"),
                                           QMessageBox::ActionRole);
        box.addButton(tr("Cancel"), QMessageBox::RejectRole);
        box.exec();
        if (box.clickedButton() == tryAgain) {
            populate();
            QTimer::singleShot(0, this, [this, retryChanges]() {
                startTransaction(retryChanges);
            });
            return;
        }
        if (box.clickedButton() == openFeatures) {
            m_closeAfterTransaction = false;
            populate();
            if (!focusId.isEmpty())
                focusFeature(focusId);
            raise();
            activateWindow();
            return;
        }
        if (m_closeAfterTransaction)
            close();
        else
            populate();
        return;
    }

    const QString completedFocusId = m_activeChanges.isEmpty()
        ? QString() : m_activeChanges.first().section(QLatin1Char('='), 0, 0);
    populate();
    if (!completedFocusId.isEmpty())
        focusFeature(completedFocusId);

    bool reboot = false;
    bool logout = false;
    QStringList installed;
    for (const QString &change : std::as_const(m_activeChanges)) {
        const FeatureDefinition *feature = m_catalog.find(change.section('=', 0, 0));
        if (!feature)
            continue;
        reboot |= feature->rebootRequired;
        logout |= feature->logoutRequired;
        if (change.endsWith(QLatin1String("=install")))
            installed << feature->name;
    }
    QString completion = tr("The requested Aero7 feature changes were completed.");
    const FeatureDefinition *singleInstalledFeature = nullptr;
    if (installed.size() == 1) {
        for (const QString &change : std::as_const(m_activeChanges)) {
            if (change.endsWith(QLatin1String("=install"))) {
                singleInstalledFeature = m_catalog.find(change.section('=', 0, 0));
                break;
            }
        }
    }
    if (reboot) {
        completion += tr("\n\nRestart the computer to finish applying the changes.");
        QMessageBox box(QMessageBox::Information, tr("Aero7 Features"), completion,
                        QMessageBox::NoButton, this);
        auto *restartNow = box.addButton(tr("Restart now"), QMessageBox::AcceptRole);
        box.addButton(tr("Restart later"), QMessageBox::RejectRole);
        box.exec();
        if (box.clickedButton() == restartNow) {
            QProcess::startDetached(QStringLiteral("/usr/bin/systemctl"),
                                    {QStringLiteral("reboot")});
            close();
            return;
        }
    } else {
        if (logout)
            completion += tr("\n\nSign out and back in to make every component available.");
        if (!m_closeAfterTransaction && singleInstalledFeature
            && !singleInstalledFeature->launch.isEmpty()) {
            QMessageBox box(QMessageBox::Information, tr("Aero7 Features"), completion,
                            QMessageBox::NoButton, this);
            auto *open = box.addButton(tr("Open %1").arg(singleInstalledFeature->name),
                                       QMessageBox::AcceptRole);
            box.addButton(tr("Close"), QMessageBox::RejectRole);
            box.exec();
            if (box.clickedButton() == open) {
                QProcess::startDetached(singleInstalledFeature->launch.first(),
                                        singleInstalledFeature->launch.mid(1));
                close();
                return;
            }
        } else {
            QMessageBox::information(this, tr("Aero7 Features"), completion);
        }
    }
    if (m_closeAfterTransaction) {
        close();
        return;
    }
    populate();
}

void OptionalFeaturesWindow::appendLog(const QString &result, const QString &detail)
{
    const QString directory = QStandardPaths::writableLocation(
        QStandardPaths::GenericStateLocation) + QStringLiteral("/aero7");
    QDir().mkpath(directory);
    QFile file(directory + QStringLiteral("/optional-features.jsonl"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    QStringList packages;
    bool reboot = false;
    bool logout = false;
    for (const QString &change : std::as_const(m_activeChanges)) {
        const FeatureDefinition *feature = m_catalog.find(change.section('=', 0, 0));
        if (!feature)
            continue;
        packages << feature->packages << feature->supportPackages;
        reboot |= feature->rebootRequired;
        logout |= feature->logoutRequired;
    }
    packages.removeDuplicates();
    const QJsonObject entry{
        {QStringLiteral("time"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {QStringLiteral("result"), result},
        {QStringLiteral("changes"), QJsonArray::fromStringList(m_activeChanges)},
        {QStringLiteral("packages"), QJsonArray::fromStringList(packages)},
        {QStringLiteral("rebootRequired"), reboot},
        {QStringLiteral("logoutRequired"), logout},
        {QStringLiteral("detail"), detail.left(4000)},
    };
    file.write(QJsonDocument(entry).toJson(QJsonDocument::Compact));
    file.write("\n");
}
