#include "AddPrinterDialog.h"
#include "Win7Ui.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QVBoxLayout>

AddPrinterDialog::AddPrinterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Add Printer"));
    setModal(true);
    resize(560, 350);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 16, 18, 14);
    root->setSpacing(10);
    root->addWidget(Win7::pageTitle(QStringLiteral("Choose a printer port"), 13, "#1A5DAB"));
    auto *intro = Win7::label(QStringLiteral("Enter a CUPS device URI or select a printer discovered by the Linux printing service. Aero7 creates an IPP Everywhere queue without opening an external printer tool."), 9, "#333333");
    intro->setWordWrap(true);
    root->addWidget(intro);

    auto *form = new QFormLayout;
    m_name = new QLineEdit(QStringLiteral("Printer"));
    m_uri = new QComboBox;
    m_uri->setEditable(true);
    m_description = new QLineEdit;
    m_location = new QLineEdit;
    form->addRow(QStringLiteral("Printer name:"), m_name);
    form->addRow(QStringLiteral("Device URI:"), m_uri);
    form->addRow(QStringLiteral("Description:"), m_description);
    form->addRow(QStringLiteral("Location:"), m_location);
    root->addLayout(form);

    QProcess discovery;
    discovery.start(QStringLiteral("lpinfo"), {QStringLiteral("-v")});
    if (discovery.waitForFinished(4000)) {
        const QString output = QString::fromUtf8(discovery.readAllStandardOutput());
        const QRegularExpression line(QStringLiteral("^(?:direct|network|serial)\\s+(.+)$"), QRegularExpression::MultilineOption);
        auto matches = line.globalMatch(output);
        while (matches.hasNext())
            m_uri->addItem(matches.next().captured(1).trimmed());
    }
    if (m_uri->count() == 0)
        m_uri->addItem(QStringLiteral("ipp://printer.local/ipp/print"));

    m_status = Win7::label(QString(), 8, "#555555");
    m_status->setWordWrap(true);
    root->addWidget(m_status);
    root->addStretch(1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Add Printer"));
    connect(buttons->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &AddPrinterDialog::addPrinter);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void AddPrinterDialog::addPrinter()
{
    QString name = m_name->text().trimmed();
    name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("-"));
    const QString uri = m_uri->currentText().trimmed();
    if (name.isEmpty() || uri.isEmpty()) {
        m_status->setText(QStringLiteral("Enter both a printer name and a device URI."));
        m_status->setStyleSheet(QStringLiteral("color: #A00000;"));
        return;
    }
    if (QStandardPaths::findExecutable(QStringLiteral("lpadmin")).isEmpty()) {
        m_status->setText(QStringLiteral("The CUPS administration tool is not installed."));
        m_status->setStyleSheet(QStringLiteral("color: #A00000;"));
        return;
    }
    QStringList arguments{QStringLiteral("-p"), name, QStringLiteral("-E"),
                          QStringLiteral("-v"), uri, QStringLiteral("-m"), QStringLiteral("everywhere")};
    if (!m_description->text().trimmed().isEmpty())
        arguments << QStringLiteral("-D") << m_description->text().trimmed();
    if (!m_location->text().trimmed().isEmpty())
        arguments << QStringLiteral("-L") << m_location->text().trimmed();
    QProcess process;
    process.start(QStringLiteral("lpadmin"), arguments);
    if (!process.waitForFinished(30000) || process.exitCode() != 0) {
        const QString error = QString::fromUtf8(process.readAllStandardError()).trimmed();
        m_status->setText(error.isEmpty()
            ? QStringLiteral("The printer could not be added. Check the URI, CUPS service, and your printer-administration permission.")
            : error);
        m_status->setStyleSheet(QStringLiteral("color: #A00000;"));
        return;
    }
    accept();
}
