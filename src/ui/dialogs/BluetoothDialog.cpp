#include "BluetoothDialog.h"
#include "IconHelper.h"
#include "Win7Ui.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace {
QString capture(const QStringList &arguments, int timeout = 3000)
{
    QProcess process;
    process.start(QStringLiteral("bluetoothctl"), arguments);
    if (!process.waitForFinished(timeout)) {
        process.kill();
        process.waitForFinished();
        return {};
    }
    return QString::fromUtf8(process.readAllStandardOutput())
        + QString::fromUtf8(process.readAllStandardError());
}
}

BluetoothDialog::BluetoothDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Add a device"));
    setModal(true);
    resize(570, 430);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 16, 18, 14);
    root->setSpacing(10);
    root->addWidget(Win7::pageTitle(QStringLiteral("Select a Bluetooth device to add"), 13, "#1A5DAB"));
    root->addWidget(Win7::label(QStringLiteral("Aero7 uses the system BlueZ service. Devices already known to the adapter and devices found during scanning appear below."), 9, "#333333"));

    m_adapterState = Win7::label(QString(), 9, "#333333");
    root->addWidget(m_adapterState);
    m_devices = new QListWidget;
    m_devices->setIconSize(QSize(32, 32));
    m_devices->setAlternatingRowColors(true);
    root->addWidget(m_devices, 1);

    auto *actions = new QHBoxLayout;
    m_power = new QPushButton;
    m_scan = new QPushButton(QStringLiteral("Scan"));
    m_pair = new QPushButton(QStringLiteral("Pair"));
    m_connect = new QPushButton(QStringLiteral("Connect"));
    m_disconnect = new QPushButton(QStringLiteral("Disconnect"));
    m_remove = new QPushButton(QStringLiteral("Remove"));
    for (QPushButton *button : {m_power, m_scan, m_pair, m_connect, m_disconnect, m_remove})
        actions->addWidget(button);
    actions->addStretch(1);
    root->addLayout(actions);

    m_status = Win7::label(QString(), 8, "#555555");
    m_status->setWordWrap(true);
    root->addWidget(m_status);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    m_process = new QProcess(this);
    connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        const QString text = QString::fromUtf8(m_process->readAllStandardOutput());
        m_output += text;
        handlePrompt(text);
    });
    connect(m_process, &QProcess::readyReadStandardError, this, [this]() {
        const QString text = QString::fromUtf8(m_process->readAllStandardError());
        m_output += text;
        handlePrompt(text);
    });
    connect(m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
        const bool success = status == QProcess::NormalExit && exitCode == 0
            && !m_output.contains(QStringLiteral("Failed"), Qt::CaseInsensitive);
        m_status->setText(success
            ? QStringLiteral("%1 completed.").arg(m_action)
            : QStringLiteral("%1 could not be completed. %2")
                  .arg(m_action, m_output.trimmed().section(QLatin1Char('\n'), -1)));
        m_status->setStyleSheet(success ? QStringLiteral("color: #1B6E1B;")
                                        : QStringLiteral("color: #A00000;"));
        refresh();
    });

    connect(m_devices, &QListWidget::itemSelectionChanged, this, &BluetoothDialog::updateButtons);
    connect(m_power, &QPushButton::clicked, this, [this]() {
        run({QStringLiteral("power"), m_power->property("powered").toBool()
                                        ? QStringLiteral("off") : QStringLiteral("on")},
            QStringLiteral("Changing Bluetooth power"));
    });
    connect(m_scan, &QPushButton::clicked, this, [this]() {
        run({QStringLiteral("--timeout"), QStringLiteral("10"), QStringLiteral("scan"), QStringLiteral("on")},
            QStringLiteral("Device scan"));
    });
    connect(m_pair, &QPushButton::clicked, this, [this]() {
        run({QStringLiteral("--timeout"), QStringLiteral("45"), QStringLiteral("--agent"),
             QStringLiteral("KeyboardDisplay"), QStringLiteral("pair"), selectedAddress()},
            QStringLiteral("Pairing"), true);
    });
    connect(m_connect, &QPushButton::clicked, this, [this]() {
        run({QStringLiteral("connect"), selectedAddress()}, QStringLiteral("Connection"));
    });
    connect(m_disconnect, &QPushButton::clicked, this, [this]() {
        run({QStringLiteral("disconnect"), selectedAddress()}, QStringLiteral("Disconnection"));
    });
    connect(m_remove, &QPushButton::clicked, this, [this]() {
        if (QMessageBox::question(this, QStringLiteral("Remove Device"),
                QStringLiteral("Are you sure you want to remove the selected Bluetooth device?")) == QMessageBox::Yes)
            run({QStringLiteral("remove"), selectedAddress()}, QStringLiteral("Removing device"));
    });

    if (QStandardPaths::findExecutable(QStringLiteral("bluetoothctl")).isEmpty()) {
        m_adapterState->setText(QStringLiteral("Bluetooth is unavailable because the BlueZ command-line service is not installed."));
        for (QPushButton *button : {m_power, m_scan, m_pair, m_connect, m_disconnect, m_remove})
            button->setEnabled(false);
    } else {
        refresh();
    }
}

QString BluetoothDialog::selectedAddress() const
{
    return m_devices->currentItem() ? m_devices->currentItem()->data(Qt::UserRole).toString() : QString();
}

void BluetoothDialog::refresh()
{
    if (m_process->state() != QProcess::NotRunning)
        return;
    const QString adapter = capture({QStringLiteral("show")});
    const bool powered = adapter.contains(QRegularExpression(QStringLiteral("Powered:\\s+yes"), QRegularExpression::CaseInsensitiveOption));
    m_adapterState->setText(adapter.contains(QStringLiteral("Controller "))
        ? QStringLiteral("Bluetooth adapter: %1").arg(powered ? QStringLiteral("On") : QStringLiteral("Off"))
        : QStringLiteral("No Bluetooth adapter was detected."));
    m_power->setText(powered ? QStringLiteral("Turn off") : QStringLiteral("Turn on"));
    m_power->setProperty("powered", powered);
    m_power->setEnabled(adapter.contains(QStringLiteral("Controller ")));
    m_scan->setEnabled(powered);

    const QString previous = selectedAddress();
    m_devices->clear();
    const QString listing = capture({QStringLiteral("devices")});
    const QRegularExpression deviceLine(QStringLiteral("^Device\\s+([0-9A-F:]{17})\\s+(.+)$"),
                                        QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);
    auto match = deviceLine.globalMatch(listing);
    while (match.hasNext()) {
        const auto itemMatch = match.next();
        const QString address = itemMatch.captured(1).toUpper();
        const QString info = capture({QStringLiteral("info"), address}, 1500);
        const bool paired = info.contains(QRegularExpression(QStringLiteral("Paired:\\s+yes"), QRegularExpression::CaseInsensitiveOption));
        const bool connected = info.contains(QRegularExpression(QStringLiteral("Connected:\\s+yes"), QRegularExpression::CaseInsensitiveOption));
        QString suffix;
        if (connected) suffix = QStringLiteral(" — Connected");
        else if (paired) suffix = QStringLiteral(" — Paired");
        auto *item = new QListWidgetItem(resolveIcon(connected ? QStringLiteral("network-bluetooth-activated")
                                                               : QStringLiteral("network-bluetooth")),
                                         itemMatch.captured(2) + suffix, m_devices);
        item->setData(Qt::UserRole, address);
        item->setData(Qt::UserRole + 1, paired);
        item->setData(Qt::UserRole + 2, connected);
        if (address == previous)
            m_devices->setCurrentItem(item);
    }
    if (m_devices->count() == 0)
        m_status->setText(powered ? QStringLiteral("No devices found. Select Scan to search for nearby devices.")
                                  : QStringLiteral("Turn Bluetooth on to search for devices."));
    updateButtons();
}

void BluetoothDialog::updateButtons()
{
    const auto *item = m_devices->currentItem();
    const bool selected = item != nullptr;
    const bool paired = selected && item->data(Qt::UserRole + 1).toBool();
    const bool connected = selected && item->data(Qt::UserRole + 2).toBool();
    const bool idle = m_process->state() == QProcess::NotRunning;
    m_pair->setEnabled(idle && selected && !paired);
    m_connect->setEnabled(idle && paired && !connected);
    m_disconnect->setEnabled(idle && connected);
    m_remove->setEnabled(idle && paired);
}

void BluetoothDialog::run(const QStringList &arguments, const QString &description, bool interactive)
{
    if (m_process->state() != QProcess::NotRunning || arguments.contains(QString()))
        return;
    m_action = description;
    m_output.clear();
    m_answeredPrompt = !interactive;
    m_status->setStyleSheet(QStringLiteral("color: #555555;"));
    m_status->setText(description + QStringLiteral("…"));
    m_process->start(QStringLiteral("bluetoothctl"), arguments);
    updateButtons();
}

void BluetoothDialog::handlePrompt(const QString &text)
{
    if (m_answeredPrompt || m_process->state() == QProcess::NotRunning)
        return;
    if (text.contains(QStringLiteral("Confirm passkey"), Qt::CaseInsensitive)
        || text.contains(QStringLiteral("Authorize service"), Qt::CaseInsensitive)) {
        m_answeredPrompt = true;
        const auto answer = QMessageBox::question(this, QStringLiteral("Bluetooth Pairing"), text.trimmed());
        m_process->write(answer == QMessageBox::Yes ? "yes\n" : "no\n");
    } else if (text.contains(QStringLiteral("PIN code"), Qt::CaseInsensitive)
               || text.contains(QStringLiteral("passkey"), Qt::CaseInsensitive)) {
        m_answeredPrompt = true;
        bool ok = false;
        const QString pin = QInputDialog::getText(this, QStringLiteral("Bluetooth Pairing"),
                                                  QStringLiteral("Enter the PIN or passkey:"),
                                                  QLineEdit::Normal, QString(), &ok);
        m_process->write((ok ? pin : QStringLiteral("no")).toUtf8() + '\n');
    }
}
