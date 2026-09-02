#include "ParentalControlsPage.h"

#include "IconHelper.h"
#include "Win7Ui.h"

#include <QComboBox>
#include <QFile>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>

namespace {
struct LocalUser { QString name; QString display; };

QList<LocalUser> localDesktopUsers()
{
    QList<LocalUser> users;
    QFile passwd(QStringLiteral("/etc/passwd"));
    if (!passwd.open(QIODevice::ReadOnly | QIODevice::Text))
        return users;
    while (!passwd.atEnd()) {
        const QList<QByteArray> fields = passwd.readLine().trimmed().split(':');
        if (fields.size() < 7)
            continue;
        bool ok = false;
        const uint uid = fields[2].toUInt(&ok);
        if (!ok || uid < 1000 || uid == 65534
            || fields[6].contains("nologin") || fields[6].contains("false"))
            continue;
        const QString name = QString::fromUtf8(fields[0]);
        QString display = QString::fromUtf8(fields[4]).section(',', 0, 0).trimmed();
        if (display.isEmpty())
            display = name;
        users << LocalUser{name, display};
    }
    return users;
}
} // namespace

ParentalControlsPage::ParentalControlsPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    auto *content = Win7::pageScaffold(this, sidebar, 24, 720);
    content->addWidget(Win7::pageTitle(tr("Set up how users can use the computer"), 13));
    content->addWidget(Win7::label(
        tr("Choose an account, then limit when or for how long that user may sign in."),
        9, "#333333"));
    content->addSpacing(16);

    auto *accountRow = new QHBoxLayout;
    auto *accountIcon = new QLabel;
    accountIcon->setPixmap(themeIcon({"preferences-system-users", "user-identity"})
                               .pixmap(48, 48));
    accountIcon->setFixedSize(48, 48);
    accountRow->addWidget(accountIcon, 0, Qt::AlignTop);
    auto *accountForm = new QFormLayout;
    m_user = new QComboBox;
    for (const LocalUser &user : localDesktopUsers())
        m_user->addItem(user.display + QStringLiteral(" (") + user.name + QLatin1Char(')'),
                        user.name);
    accountForm->addRow(tr("User account:"), m_user);
    accountRow->addLayout(accountForm, 1);
    content->addLayout(accountRow);
    content->addSpacing(12);
    content->addWidget(Win7::hairline());
    content->addSpacing(12);

    auto *form = new QFormLayout;
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(10);
    m_mode = new QComboBox;
    m_mode->addItem(tr("Off — no time restrictions"), QStringLiteral("none"));
    m_mode->addItem(tr("Daily time allowance"), QStringLiteral("daily-limit"));
    m_mode->addItem(tr("Allowed hours each day"), QStringLiteral("daily-schedule"));
    form->addRow(tr("Parental Controls:"), m_mode);
    m_hours = new QSpinBox;
    m_hours->setRange(1, 24);
    m_hours->setSuffix(tr(" hours"));
    form->addRow(tr("Daily allowance:"), m_hours);
    auto *schedule = new QHBoxLayout;
    m_start = new QTimeEdit(QTime(8, 0));
    m_end = new QTimeEdit(QTime(20, 0));
    m_start->setDisplayFormat(QStringLiteral("HH:mm"));
    m_end->setDisplayFormat(QStringLiteral("HH:mm"));
    schedule->addWidget(m_start);
    schedule->addWidget(new QLabel(tr("to")));
    schedule->addWidget(m_end);
    schedule->addStretch(1);
    form->addRow(tr("Allowed time:"), schedule);
    content->addLayout(form);

    m_status = new QLabel;
    m_status->setWordWrap(true);
    m_status->setStyleSheet(QStringLiteral("color: #555555;"));
    content->addWidget(m_status);
    content->addStretch(1);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    m_apply = new QPushButton(tr("Apply"));
    buttons->addWidget(m_apply);
    content->addLayout(buttons);

    const bool backendAvailable = !QStandardPaths::findExecutable(
        QStringLiteral("malcontent-client")).isEmpty();
    if (!backendAvailable || m_user->count() == 0) {
        m_status->setText(!backendAvailable
            ? tr("The Parental Controls backend is not installed.")
            : tr("No desktop user accounts were found."));
        m_mode->setEnabled(false);
        m_apply->setEnabled(false);
    }

    connect(m_user, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this]() { loadSelectedUser(); });
    connect(m_mode, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this]() { updateModeControls(); });
    connect(m_apply, &QPushButton::clicked,
            this, &ParentalControlsPage::applyLimits);
    updateModeControls();
    if (backendAvailable && m_user->count())
        loadSelectedUser();
}

void ParentalControlsPage::loadSelectedUser()
{
    const QString user = m_user->currentData().toString();
    QProcess query;
    query.start(QStringLiteral("malcontent-client"),
                {QStringLiteral("get-session-limits"), user});
    if (!query.waitForFinished(4000)) {
        m_status->setText(tr("The current limits could not be read."));
        return;
    }
    const QString output = QString::fromUtf8(query.readAllStandardOutput())
        + QString::fromUtf8(query.readAllStandardError());
    if (query.exitCode() != 0 || output.contains(QStringLiteral("not enabled"),
                                                  Qt::CaseInsensitive)) {
        m_mode->setCurrentIndex(0);
        m_status->setText(tr("Parental Controls are off for this account."));
        return;
    }
    static const QRegularExpression secondsRe(QStringLiteral("(\\d+) seconds"));
    const auto seconds = secondsRe.match(output);
    if (seconds.hasMatch()) {
        m_mode->setCurrentIndex(1);
        m_hours->setValue(qBound(1, seconds.captured(1).toInt() / 3600, 24));
    } else {
        static const QRegularExpression timesRe(
            QStringLiteral("(\\d{2}:\\d{2}).*(\\d{2}:\\d{2})"),
            QRegularExpression::DotMatchesEverythingOption);
        const auto times = timesRe.match(output);
        if (times.hasMatch()) {
            m_mode->setCurrentIndex(2);
            m_start->setTime(QTime::fromString(times.captured(1), QStringLiteral("HH:mm")));
            m_end->setTime(QTime::fromString(times.captured(2), QStringLiteral("HH:mm")));
        }
    }
    m_status->setText(output.trimmed());
}

void ParentalControlsPage::updateModeControls()
{
    const QString mode = m_mode->currentData().toString();
    m_hours->setEnabled(mode == QLatin1String("daily-limit"));
    m_start->setEnabled(mode == QLatin1String("daily-schedule"));
    m_end->setEnabled(mode == QLatin1String("daily-schedule"));
}

void ParentalControlsPage::applyLimits()
{
    if (m_process)
        return;
    const QString user = m_user->currentData().toString();
    const QString mode = m_mode->currentData().toString();
    QStringList arguments{QStringLiteral("set-session-limits"),
                          QStringLiteral("--interactive"), user, mode};
    if (mode == QLatin1String("daily-limit"))
        arguments << QStringLiteral("--daily-limit")
                  << QString::number(m_hours->value() * 3600);
    else if (mode == QLatin1String("daily-schedule"))
        arguments << QStringLiteral("--start-time")
                  << m_start->time().toString(QStringLiteral("HH:mm"))
                  << QStringLiteral("--end-time")
                  << m_end->time().toString(QStringLiteral("HH:mm"));

    m_apply->setEnabled(false);
    m_status->setText(tr("Waiting for administrator authorization…"));
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus status) {
        const QString output = QString::fromUtf8(m_process->readAll()).trimmed();
        m_process->deleteLater();
        m_process = nullptr;
        m_apply->setEnabled(true);
        if (status == QProcess::NormalExit && code == 0) {
            m_status->setText(tr("The parental-control settings were saved."));
            QMessageBox::information(this, tr("Parental Controls"),
                                     tr("The settings were saved successfully."));
        } else {
            m_status->setText(output.isEmpty()
                ? tr("The settings were not changed.") : output);
            QMessageBox::warning(this, tr("Parental Controls"),
                                 tr("Aero7 could not save these settings."));
        }
    });
    m_process->start(QStringLiteral("malcontent-client"), arguments);
}
