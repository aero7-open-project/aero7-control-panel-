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
const QString program = QStringLiteral("/usr/lib/aero7-desktop/aero7-snap-control");
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

    QProcess reader;
    reader.start(program, {QStringLiteral("sensitivity")});
    int current = 35;
    if (reader.waitForFinished(3000) && reader.exitCode() == 0) {
        bool valid = false;
        const int value = QString::fromUtf8(reader.readAllStandardOutput()).trimmed().toInt(&valid);
        if (valid)
            current = qBound(0, value, 100);
    }
    slider->setValue(current);
    auto savedValue = std::make_shared<int>(current);
    auto *status = Win7::label(QString(), 9, "#333333");
    status->setObjectName(QStringLiteral("snapSensitivityStatus"));
    auto *apply = new QPushButton(tr("Apply"));
    apply->setObjectName(QStringLiteral("applySnapSensitivity"));
    apply->setEnabled(false);
    connect(slider, &QSlider::valueChanged, this,
            [apply, savedValue](int value) { apply->setEnabled(value != *savedValue); });
    connect(apply, &QPushButton::clicked, this, [slider, apply, status, savedValue]() {
        QProcess writer;
        writer.start(program, {QStringLiteral("set-sensitivity"),
                               QString::number(slider->value())});
        if (!writer.waitForFinished(10000) || writer.exitCode() != 0) {
            status->setText(QObject::tr("Could not save sensitivity: %1")
                .arg(QString::fromUtf8(writer.readAllStandardError()).trimmed()));
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
