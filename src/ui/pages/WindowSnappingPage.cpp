#include "WindowSnappingPage.h"
#include "Win7Ui.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QVBoxLayout>
#include <memory>

namespace {
QString controlProgram()
{
    const QString override = qEnvironmentVariable("AERO7_SNAP_CONTROL_PROGRAM");
    return override.isEmpty()
        ? QStringLiteral("/usr/lib/aero7-desktop/aero7-snap-control") : override;
}

bool readSensitivity(int *value)
{
    QProcess reader;
    reader.start(controlProgram(), {QStringLiteral("sensitivity")});
    if (!reader.waitForStarted(3000) || !reader.waitForFinished(3000)
        || reader.exitStatus() != QProcess::NormalExit || reader.exitCode() != 0)
        return false;
    bool valid = false;
    const int current = QString::fromUtf8(reader.readAllStandardOutput())
                            .trimmed().toInt(&valid);
    if (!valid || current < 0 || current > 100)
        return false;
    *value = current;
    return true;
}
}

WindowSnappingPage::WindowSnappingPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    auto *content = Win7::pageScaffold(this, sidebar, 20, 720, 16);
    content->addWidget(Win7::pageTitle(tr("Windows Snapping"), 13, "#1A5DAB"));
    auto *intro = Win7::label(tr(
        "Drag a window to a screen edge to place it on the left or right, "
        "or drag it to the top to fill the screen."), 9, "#333333");
    intro->setWordWrap(true);
    content->addWidget(intro);
    content->addSpacing(14);

    auto *heading = Win7::label(tr("Snap sensitivity"), 10, "#1A5DAB");
    content->addWidget(heading);
    auto *description = Win7::label(tr(
        "Lower sensitivity requires you to hold the window at the edge longer. "
        "Higher sensitivity snaps sooner."), 9, "#333333");
    description->setWordWrap(true);
    content->addWidget(description);

    auto *row = new QHBoxLayout;
    row->addWidget(Win7::label(tr("Low"), 9, "#333333"));
    auto *slider = new QSlider(Qt::Horizontal);
    slider->setObjectName(QStringLiteral("snapSensitivity"));
    slider->setRange(0, 100);
    slider->setTickInterval(25);
    slider->setTickPosition(QSlider::TicksBelow);
    row->addWidget(slider, 1);
    row->addWidget(Win7::label(tr("High"), 9, "#333333"));
    content->addLayout(row);

    int current = 35;
    const bool available = readSensitivity(&current);
    slider->setValue(current);
    slider->setEnabled(available);
    auto savedValue = std::make_shared<int>(current);
    auto *status = Win7::label(QString(), 9, "#333333");
    status->setObjectName(QStringLiteral("snapSensitivityStatus"));
    if (!available)
        status->setText(tr("Snap control is unavailable; sensitivity was not loaded."));
    auto *apply = new QPushButton(tr("Apply"));
    apply->setObjectName(QStringLiteral("applySnapSensitivity"));
    apply->setEnabled(false);
    connect(slider, &QSlider::valueChanged, this,
            [apply, savedValue](int value) { apply->setEnabled(value != *savedValue); });
    connect(apply, &QPushButton::clicked, this, [slider, apply, status, savedValue]() {
        QProcess writer;
        writer.start(controlProgram(), {QStringLiteral("set-sensitivity"),
                               QString::number(slider->value())});
        if (!writer.waitForStarted(3000) || !writer.waitForFinished(10000)
            || writer.exitStatus() != QProcess::NormalExit || writer.exitCode() != 0) {
            status->setText(QObject::tr("Could not save sensitivity: %1")
                .arg(QString::fromUtf8(writer.readAllStandardError()).trimmed()));
            return;
        }
        int verified = -1;
        if (!readSensitivity(&verified) || verified != slider->value()) {
            status->setText(QObject::tr(
                "Snap control did not retain the requested sensitivity."));
            return;
        }
        status->setText(QObject::tr("Snap sensitivity saved."));
        *savedValue = slider->value();
        apply->setEnabled(false);
    });
    content->addWidget(apply, 0, Qt::AlignRight);
    content->addWidget(status);
    content->addStretch(1);
}
