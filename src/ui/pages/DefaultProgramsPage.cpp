#include "DefaultProgramsPage.h"
#include "IconHelper.h"
#include "Win7Ui.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QFrame *separator()
{
    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet(QStringLiteral("color: #D5DFEB;"));
    return line;
}

} // namespace

DefaultProgramsPage::DefaultProgramsPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("defaultProgramsPage"));
    auto *content = Win7::pageScaffold(this, sidebar, 20, 760, 14);
    content->addWidget(Win7::pageTitle(QStringLiteral("Default Programs"),
                                       13, "#1A5DAB"));
    auto *intro = Win7::label(
        QStringLiteral("Choose the programs that Aero7 uses by default."),
        9, "#333333");
    intro->setWordWrap(true);
    content->addWidget(intro);
    content->addSpacing(14);

    auto *card = new QFrame;
    card->setObjectName(QStringLiteral("internetExplorerCard"));
    card->setStyleSheet(QStringLiteral(
        "QFrame#internetExplorerCard { background: white; border: 1px solid #B8C9DC; }"));
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(14, 12, 14, 14);
    cardLayout->setSpacing(9);

    auto *headingRow = new QHBoxLayout;
    auto *icon = new QLabel;
    icon->setObjectName(QStringLiteral("internetExplorerIcon"));
    icon->setFixedSize(48, 48);
    icon->setPixmap(resolveIcon(QStringLiteral("internet-web-browser"))
                        .pixmap(48, 48));
    headingRow->addWidget(icon, 0, Qt::AlignTop);
    auto *headingText = new QVBoxLayout;
    headingText->setSpacing(2);
    auto *heading = Win7::label(QStringLiteral("Internet Explorer"),
                                12, "#1A5DAB");
    heading->setObjectName(QStringLiteral("internetExplorerHeading"));
    headingText->addWidget(heading);
    auto *description = Win7::label(
        QStringLiteral("Aero7 keeps this familiar identity while a modern, "
                       "installed browser safely opens websites."),
        9, "#333333");
    description->setWordWrap(true);
    headingText->addWidget(description);
    headingRow->addLayout(headingText, 1);
    cardLayout->addLayout(headingRow);
    cardLayout->addWidget(separator());

    auto *browserRow = new QHBoxLayout;
    browserRow->addWidget(new QLabel(QStringLiteral("Browser used by Internet Explorer:")));
    m_browser = new QComboBox;
    m_browser->setObjectName(QStringLiteral("internetExplorerBackend"));
    m_browser->setEnabled(false);
    m_browser->setMinimumWidth(240);
    browserRow->addWidget(m_browser, 1);
    cardLayout->addLayout(browserRow);

    m_backendDetails = new QLabel;
    m_backendDetails->setObjectName(QStringLiteral("backendDetails"));
    m_backendDetails->setWordWrap(true);
    m_backendDetails->setStyleSheet(QStringLiteral("color: #555555;"));
    cardLayout->addWidget(m_backendDetails);

    m_defaultBrowser = new QCheckBox(
        QStringLiteral("Make Internet Explorer my default web browser"));
    m_defaultBrowser->setObjectName(QStringLiteral("internetExplorerDefault"));
    m_defaultBrowser->setEnabled(false);
    m_defaultBrowser->setToolTip(QStringLiteral(
        "Clearing this choice restores the browser associations recorded before "
        "Internet Explorer became default. If no earlier browser is recorded, "
        "choose another browser using the Default Programs settings module."));
    cardLayout->addWidget(m_defaultBrowser);

    auto *actions = new QHBoxLayout;
    auto *shortcut = new QPushButton(QStringLiteral("Create desktop shortcut"));
    shortcut->setObjectName(QStringLiteral("createInternetExplorerShortcut"));
    connect(shortcut, &QPushButton::clicked,
            this, &DefaultProgramsPage::createShortcut);
    actions->addWidget(shortcut);
    auto *settings = new QPushButton(QStringLiteral("Advanced settings..."));
    connect(settings, &QPushButton::clicked, this, [this] {
        if (!QProcess::startDetached(DefaultProgramsPage::launcherExecutable(),
                                     {QStringLiteral("--settings")}))
            setStatus(QStringLiteral("Could not open Internet Explorer's advanced settings."),
                      true);
    });
    actions->addWidget(settings);
    actions->addStretch(1);
    m_apply = new QPushButton(QStringLiteral("Save changes"));
    m_apply->setObjectName(QStringLiteral("saveInternetExplorerDefaults"));
    m_apply->setEnabled(false);
    connect(m_apply, &QPushButton::clicked,
            this, &DefaultProgramsPage::applySelection);
    actions->addWidget(m_apply);
    cardLayout->addLayout(actions);

    m_status = new QLabel;
    m_status->setObjectName(QStringLiteral("internetExplorerStatus"));
    m_status->setWordWrap(true);
    cardLayout->addWidget(m_status);
    content->addWidget(card);

    auto *note = Win7::label(
        QStringLiteral("HTTP, HTTPS and local HTML files can use Internet Explorer. "
                       "The selected browser remains responsible for rendering pages, "
                       "security updates, profiles and extensions."),
        9, "#555555");
    note->setWordWrap(true);
    content->addWidget(note);
    content->addStretch(1);

    connect(m_browser, &QComboBox::currentIndexChanged, this, [this] {
        updateBackendDetails();
    });
    QTimer::singleShot(0, this, &DefaultProgramsPage::loadStatus);
}

QString DefaultProgramsPage::launcherExecutable()
{
    const QString override = qEnvironmentVariable("AERO7_IE_EXECUTABLE");
    if (!override.isEmpty()) return override;
    const QString found = QStandardPaths::findExecutable(
        QStringLiteral("aero7-internet-explorer"));
    return found.isEmpty() ? QStringLiteral("aero7-internet-explorer") : found;
}

DefaultProgramsPage::CommandResult
DefaultProgramsPage::runLauncher(const QStringList &arguments)
{
    QProcess process;
    process.start(launcherExecutable(), arguments);
    CommandResult result;
    result.started = process.waitForStarted(3000);
    if (!result.started) return result;
    if (!process.waitForFinished(10000)) {
        process.kill();
        process.waitForFinished(1000);
        result.error = QByteArrayLiteral("The operation timed out.");
        return result;
    }
    result.exitCode = process.exitStatus() == QProcess::NormalExit
        ? process.exitCode() : -1;
    result.output = process.readAllStandardOutput();
    result.error = process.readAllStandardError();
    return result;
}

void DefaultProgramsPage::loadStatus()
{
    m_statusValid = false;
    m_browser->setEnabled(false);
    m_defaultBrowser->setEnabled(false);
    m_apply->setEnabled(false);
    const CommandResult result = runLauncher({QStringLiteral("--status-json")});
    if (!result.started || result.exitCode != 0) {
        m_browser->clear();
        setStatus(QStringLiteral(
            "Internet Explorer integration is not installed. Install or update "
            "the aero7-internet-explorer package."), true);
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(result.output);
    if (!document.isObject()) {
        m_browser->clear();
        setStatus(QStringLiteral("Internet Explorer returned invalid status information."), true);
        return;
    }
    const QJsonObject status = document.object();
    const QString selected = status.value(QStringLiteral("selectedDesktopId")).toString();
    m_browser->clear();
    for (const QJsonValue &value : status.value(QStringLiteral("backends")).toArray()) {
        const QJsonObject backend = value.toObject();
        m_browser->addItem(QIcon::fromTheme(backend.value(QStringLiteral("icon")).toString()),
            backend.value(QStringLiteral("displayName")).toString(),
            backend.value(QStringLiteral("desktopId")).toString());
        const int row = m_browser->count() - 1;
        m_browser->setItemData(row,
            backend.value(QStringLiteral("supportsPrivateMode")).toBool(),
            Qt::UserRole + 1);
        m_browser->setItemData(row,
            backend.value(QStringLiteral("supportsNewWindow")).toBool(),
            Qt::UserRole + 2);
    }
    const int index = m_browser->findData(selected);
    if (index >= 0) m_browser->setCurrentIndex(index);
    updateBackendDetails();
    m_wasDefault = status.value(QStringLiteral("isDefault")).toBool();
    m_defaultBrowser->setChecked(m_wasDefault);
    const bool locked = status.value(QStringLiteral("policyLocked")).toBool();
    m_browser->setEnabled(!locked && m_browser->count() > 0);
    m_defaultBrowser->setEnabled(!locked && m_browser->count() > 0);
    m_apply->setEnabled(!locked && m_browser->count() > 0);
    m_statusValid = true;
    if (m_browser->count() == 0) {
        setStatus(QStringLiteral("No compatible web browser is installed."), true);
    } else if (locked) {
        setStatus(QStringLiteral("An administrator policy controls the browser selection."));
    } else {
        setStatus(QStringLiteral("Ready."));
    }
}

void DefaultProgramsPage::applySelection()
{
    if (!m_statusValid || m_browser->currentIndex() < 0)
        return;
    const QString requestedBackend = m_browser->currentData().toString();
    const bool requestedDefault = m_defaultBrowser->isChecked();
    const CommandResult backend = runLauncher({QStringLiteral("--set-backend"),
                                                requestedBackend});
    if (!backend.started || backend.exitCode != 0) {
        setStatus(QString::fromUtf8(backend.error).trimmed().isEmpty()
            ? QStringLiteral("The browser selection could not be saved.")
            : QString::fromUtf8(backend.error).trimmed(), true);
        return;
    }
    if (requestedDefault != m_wasDefault) {
        const QString option = requestedDefault
            ? QStringLiteral("--set-default") : QStringLiteral("--restore-defaults");
        const CommandResult defaults = runLauncher({option});
        if (!defaults.started || defaults.exitCode != 0) {
            const QString detail = QString::fromUtf8(defaults.error).trimmed();
            setStatus(!requestedDefault && detail.contains(QStringLiteral("No previous browser default"))
                ? QStringLiteral("No previous browser default was recorded. Internet Explorer "
                                 "is still the default. Choose another browser in the "
                                 "Default Programs settings module.")
                : detail.isEmpty()
                ? QStringLiteral("The default web associations could not be changed.")
                : detail, true);
            return;
        }
    }
    const CommandResult verified = runLauncher({QStringLiteral("--status-json")});
    const QJsonDocument document = QJsonDocument::fromJson(verified.output);
    if (!verified.started || verified.exitCode != 0 || !document.isObject()
        || document.object().value(QStringLiteral("selectedDesktopId")).toString()
               != requestedBackend
        || document.object().value(QStringLiteral("isDefault")).toBool()
               != requestedDefault) {
        setStatus(QStringLiteral(
            "Internet Explorer did not retain every requested default."), true);
        return;
    }
    loadStatus();
    if (!m_statusValid
        || m_browser->currentData().toString() != requestedBackend
        || m_defaultBrowser->isChecked() != requestedDefault) {
        setStatus(QStringLiteral("Internet Explorer could not confirm the saved defaults."), true);
        return;
    }
    setStatus(QStringLiteral("Internet Explorer defaults were updated."));
}

void DefaultProgramsPage::createShortcut()
{
    const CommandResult result = runLauncher(
        {QStringLiteral("--create-desktop-shortcut")});
    if (!result.started || result.exitCode != 0) {
        setStatus(QString::fromUtf8(result.error).trimmed().isEmpty()
            ? QStringLiteral("The desktop shortcut could not be created.")
            : QString::fromUtf8(result.error).trimmed(), true);
        return;
    }
    setStatus(QStringLiteral("The Internet Explorer shortcut was added to the desktop."));
}

void DefaultProgramsPage::updateBackendDetails()
{
    const bool privateMode = m_browser->currentData(Qt::UserRole + 1).toBool();
    const bool newWindow = m_browser->currentData(Qt::UserRole + 2).toBool();
    m_backendDetails->setText(
        QStringLiteral("New window: %1    InPrivate browsing: %2")
            .arg(newWindow ? QStringLiteral("available")
                           : QStringLiteral("browser default"),
                 privateMode ? QStringLiteral("available")
                             : QStringLiteral("browser default")));
}

void DefaultProgramsPage::setStatus(const QString &text, bool error)
{
    m_status->setStyleSheet(error ? QStringLiteral("color: #A00000;")
                                  : QStringLiteral("color: #2D5D2D;"));
    m_status->setText(text);
}
