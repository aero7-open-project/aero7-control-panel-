#include "DisplayPage.h"
#include "Win7Ui.h"

#include <QCheckBox>
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSet>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

#include <algorithm>
#include <functional>

struct DisplayLayoutEntry {
    QString name;
    QSize size;
    QPoint position;
    bool enabled = false;
};

class DisplayLayoutWidget : public QWidget {
public:
    explicit DisplayLayoutWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumSize(420, 112);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::OpenHandCursor);
    }

    void setEntries(const QVector<DisplayLayoutEntry> &entries, int selected)
    {
        m_entries = entries;
        m_selected = qBound(0, selected, qMax(0, entries.size() - 1));
        updateGeometryCache();
        update();
    }

    void setSelected(int selected)
    {
        if (selected < 0 || selected >= m_entries.size() || selected == m_selected)
            return;
        m_selected = selected;
        update();
    }

    std::function<void(int)> selectedChanged;
    std::function<void(int, QPoint)> positionChanged;

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        updateGeometryCache();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor("#F4F4F4"));
        painter.setPen(QPen(QColor("#B6B6B6"), 1));
        painter.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 3, 3);

        for (int index = 0; index < m_entries.size(); ++index) {
            const DisplayLayoutEntry &entry = m_entries.at(index);
            const QRectF outputRect = m_rects.value(index);
            const bool selected = index == m_selected;
            painter.setPen(QPen(selected ? QColor("#174A8B") : QColor("#6F7F91"),
                                selected ? 3 : 1));
            painter.setBrush(entry.enabled ? QColor("#4B80C4") : QColor("#8D939A"));
            painter.drawRoundedRect(outputRect, 3, 3);
            painter.setPen(Qt::white);
            QFont numberFont = painter.font();
            numberFont.setPointSize(18);
            numberFont.setBold(true);
            painter.setFont(numberFont);
            painter.drawText(outputRect, Qt::AlignCenter, QString::number(index + 1));
            painter.setPen(QColor("#202020"));
            QFont nameFont = painter.font();
            nameFont.setPointSize(8);
            nameFont.setBold(false);
            painter.setFont(nameFont);
            painter.drawText(QRectF(outputRect.left(), outputRect.bottom() + 3,
                                    outputRect.width(), 18),
                             Qt::AlignHCenter | Qt::AlignTop, entry.name);
        }
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton)
            return;
        for (int index = m_rects.size() - 1; index >= 0; --index) {
            if (!m_rects.at(index).contains(event->position()))
                continue;
            m_selected = index;
            m_dragIndex = m_entries.at(index).enabled ? index : -1;
            m_dragOrigin = event->position();
            m_positionOrigin = m_entries.at(index).position;
            if (m_dragIndex >= 0)
                setCursor(Qt::ClosedHandCursor);
            if (selectedChanged)
                selectedChanged(index);
            update();
            event->accept();
            return;
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (m_dragIndex < 0 || m_scale <= 0.0)
            return;
        const QPointF delta = event->position() - m_dragOrigin;
        QPoint position = m_positionOrigin
            + QPoint(qRound(delta.x() / m_scale), qRound(delta.y() / m_scale));
        position.setX(qRound(position.x() / 10.0) * 10);
        position.setY(qRound(position.y() / 10.0) * 10);
        m_entries[m_dragIndex].position = position;
        updateGeometryCache();
        update();
        event->accept();
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() != Qt::LeftButton || m_dragIndex < 0)
            return;
        const int index = m_dragIndex;
        m_dragIndex = -1;
        setCursor(Qt::OpenHandCursor);
        if (positionChanged)
            positionChanged(index, m_entries.at(index).position);
        event->accept();
    }

private:
    QSize logicalSize(const DisplayLayoutEntry &entry) const
    {
        const QSize size = entry.size.isValid() ? entry.size : QSize(1024, 768);
        return QSize(qMax(320, size.width()), qMax(240, size.height()));
    }

    void updateGeometryCache()
    {
        m_rects.clear();
        if (m_entries.isEmpty())
            return;

        QRect bounds;
        bool first = true;
        for (const DisplayLayoutEntry &entry : std::as_const(m_entries)) {
            const QRect output(entry.position, logicalSize(entry));
            bounds = first ? output : bounds.united(output);
            first = false;
        }
        if (!bounds.isValid())
            return;

        const QMargins margins(28, 16, 28, 30);
        const QSizeF available(qMax(1, width() - margins.left() - margins.right()),
                               qMax(1, height() - margins.top() - margins.bottom()));
        m_scale = qMin(available.width() / bounds.width(),
                       available.height() / bounds.height());
        m_scale = qBound(0.02, m_scale, 0.14);
        const QSizeF rendered(bounds.width() * m_scale, bounds.height() * m_scale);
        const QPointF origin((width() - rendered.width()) / 2.0 - bounds.left() * m_scale,
                             margins.top() - bounds.top() * m_scale);
        for (const DisplayLayoutEntry &entry : std::as_const(m_entries)) {
            const QSize outputSize = logicalSize(entry);
            m_rects.push_back(QRectF(origin.x() + entry.position.x() * m_scale,
                                     origin.y() + entry.position.y() * m_scale,
                                     outputSize.width() * m_scale,
                                     outputSize.height() * m_scale));
        }
    }

    QVector<DisplayLayoutEntry> m_entries;
    QVector<QRectF> m_rects;
    int m_selected = 0;
    int m_dragIndex = -1;
    QPointF m_dragOrigin;
    QPoint m_positionOrigin;
    double m_scale = 0.1;
};

namespace {

QString rotationName(int value)
{
    switch (value) {
    case 2: return QStringLiteral("left");
    case 4: return QStringLiteral("inverted");
    case 8: return QStringLiteral("right");
    default: return QStringLiteral("none");
    }
}

int rotationValue(const QString &name)
{
    if (name == QLatin1String("left")) return 2;
    if (name == QLatin1String("inverted")) return 4;
    if (name == QLatin1String("right")) return 8;
    return 1;
}

QString outputArgument(const QString &name, const QString &setting)
{
    return QStringLiteral("output.%1.%2").arg(name, setting);
}

QString doctorExecutable()
{
    return qEnvironmentVariable("AERO7_KSCREEN_DOCTOR", QStringLiteral("kscreen-doctor"));
}

} // namespace

QList<SidebarLink> DisplayPage::sidebarLinks()
{
    return {
        Nav::to("Personalization", PageId::Personalization),
        Nav::to("Ease of Access Center", PageId::EaseOfAccess),
    };
}

QList<SidebarLink> DisplayPage::sidebarSeeAlso()
{
    return {
        Nav::to("Window Behavior", PageId::WindowBehavior),
    };
}

DisplayPage::DisplayPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    auto *content = Win7::pageScaffold(this, sidebar, 20, 650, 16);
    content->addWidget(Win7::pageTitle("Change the appearance of your display"));
    content->addSpacing(10);

    auto *monitorRow = new QHBoxLayout;
    monitorRow->setContentsMargins(0, 0, 0, 0);
    monitorRow->setSpacing(12);
    m_monitorDiagram = new DisplayLayoutWidget;
    m_monitorDiagram->setObjectName(QStringLiteral("displayLayout"));
    monitorRow->addWidget(m_monitorDiagram, 1);

    auto *monitorButtons = new QVBoxLayout;
    monitorButtons->setSpacing(8);
    auto *detect = new QPushButton("Detect");
    detect->setObjectName(QStringLiteral("detectDisplays"));
    auto *identify = new QPushButton("Identify");
    identify->setObjectName(QStringLiteral("identifyDisplays"));
    detect->setFixedWidth(86);
    identify->setFixedWidth(86);
    monitorButtons->addWidget(detect);
    monitorButtons->addWidget(identify);
    monitorButtons->addStretch(1);
    monitorRow->addLayout(monitorButtons);
    content->addLayout(monitorRow);
    content->addSpacing(18);

    auto *form = new QFormLayout;
    form->setContentsMargins(100, 0, 90, 0);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_display = new QComboBox;
    m_display->setObjectName(QStringLiteral("displaySelector"));
    m_resolution = new QComboBox;
    m_resolution->setObjectName(QStringLiteral("resolutionSelector"));
    m_orientation = new QComboBox;
    m_scale = new QComboBox;
    m_multiple = new QComboBox;
    m_primary = new QCheckBox("Make this my main display");

    m_orientation->addItem("Landscape", QStringLiteral("none"));
    m_orientation->addItem("Portrait", QStringLiteral("left"));
    m_orientation->addItem("Landscape (flipped)", QStringLiteral("inverted"));
    m_orientation->addItem("Portrait (flipped)", QStringLiteral("right"));
    for (const auto &entry : {
             qMakePair(QStringLiteral("100% (recommended)"), 1.0),
             qMakePair(QStringLiteral("125%"), 1.25),
             qMakePair(QStringLiteral("150%"), 1.5),
             qMakePair(QStringLiteral("175%"), 1.75),
             qMakePair(QStringLiteral("200%"), 2.0)}) {
        m_scale->addItem(entry.first, entry.second);
    }
    m_multiple->addItem("Extend these displays", QStringLiteral("extend"));
    m_multiple->addItem("Show desktop only on this display", QStringLiteral("only"));
    m_multiple->addItem("Disconnect this display", QStringLiteral("disconnect"));

    form->addRow("Display:", m_display);
    form->addRow("Resolution:", m_resolution);
    content->addLayout(form);
    content->addSpacing(8);

    auto makeLink = [](const QString &text) {
        auto *button = new QPushButton(text);
        button->setFlat(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(
            "QPushButton { color: #0645c0; border: 0; padding: 2px 0; text-align: left; background: transparent; }"
            "QPushButton:hover { text-decoration: underline; color: #003399; }");
        return button;
    };
    auto *textSizeLink = makeLink("Make text and other items larger or smaller");
    auto *helpLink = makeLink("What display settings should I choose?");
    auto *advancedLink = makeLink("Advanced settings");

    auto *linkRows = new QHBoxLayout;
    auto *leftLinks = new QVBoxLayout;
    leftLinks->setSpacing(0);
    leftLinks->addWidget(textSizeLink);
    leftLinks->addWidget(helpLink);
    linkRows->addSpacing(100);
    linkRows->addLayout(leftLinks);
    linkRows->addStretch(1);
    linkRows->addWidget(advancedLink, 0, Qt::AlignBottom);
    linkRows->addSpacing(90);
    content->addLayout(linkRows);

    // Linux exposes useful orientation, scale and multi-monitor controls that
    // Windows places behind secondary dialogs. Keep the working backend, but
    // put these controls behind the reference-style Advanced settings link.
    auto *advanced = new QWidget;
    advanced->setObjectName(QStringLiteral("advancedDisplaySettings"));
    auto *advancedForm = new QFormLayout(advanced);
    advancedForm->setContentsMargins(100, 8, 90, 4);
    advancedForm->setHorizontalSpacing(14);
    advancedForm->setVerticalSpacing(8);
    advancedForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    advancedForm->addRow("Orientation:", m_orientation);
    advancedForm->addRow("Make text and other items:", m_scale);
    advancedForm->addRow("Multiple displays:", m_multiple);
    advancedForm->addRow(QString(), m_primary);
    advanced->hide();
    content->addWidget(advanced);
    connect(advancedLink, &QPushButton::clicked, advanced,
            [advanced]() { advanced->setVisible(!advanced->isVisible()); });
    connect(textSizeLink, &QPushButton::clicked, advanced,
            [advanced]() { advanced->show(); });
    connect(helpLink, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, "Display Help",
            "Choose the recommended resolution for the sharpest picture. "
            "Use Advanced settings for rotation, scaling and multiple displays.");
    });
    content->addSpacing(6);

    m_status = Win7::bodyLabel(QString());
    m_status->setObjectName(QStringLiteral("displayStatus"));
    content->addWidget(m_status);

    auto *buttons = new QHBoxLayout;
    auto *ok = new QPushButton("OK");
    auto *cancel = new QPushButton("Cancel");
    m_apply = new QPushButton("Apply");
    ok->setObjectName(QStringLiteral("displayOk"));
    cancel->setObjectName(QStringLiteral("displayCancel"));
    m_apply->setObjectName(QStringLiteral("displayApply"));
    ok->setFixedWidth(82);
    cancel->setFixedWidth(82);
    m_apply->setFixedWidth(82);
    buttons->addStretch(1);
    buttons->addWidget(ok);
    buttons->addWidget(cancel);
    buttons->addWidget(m_apply);
    content->addLayout(buttons);
    content->addStretch(1);

    connect(detect, &QPushButton::clicked, this, [this]() {
        QString error;
        if (!loadConfiguration(&error))
            setStatus(error, true);
    });
    connect(identify, &QPushButton::clicked, this, [this]() {
        const QList<QScreen *> screens = QApplication::screens();
        for (int index = 0; index < screens.size(); ++index) {
            auto *number = new QLabel(QString::number(index + 1));
            number->setAlignment(Qt::AlignCenter);
            number->setStyleSheet(
                "QLabel { color: white; background: #1f4d91; border: 4px solid white; "
                "font-size: 52pt; font-weight: bold; }");
            number->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint
                                   | Qt::WindowStaysOnTopHint);
            number->resize(132, 104);
            number->move(screens.at(index)->geometry().center()
                         - QPoint(number->width() / 2, number->height() / 2));
            number->show();
            QTimer::singleShot(1800, number, &QWidget::deleteLater);
        }
    });
    connect(m_display, &QComboBox::currentIndexChanged,
            this, [this]() { populateOutputControls(); });
    m_monitorDiagram->selectedChanged = [this](int index) {
        if (index >= 0 && index < m_display->count())
            m_display->setCurrentIndex(index);
    };
    m_monitorDiagram->positionChanged = [this](int index, const QPoint &position) {
        if (index < 0 || index >= m_outputs.size())
            return;
        m_outputs[index].position = position;
        setStatus(QStringLiteral("Display %1 moved to %2, %3. Select Apply to keep this arrangement.")
                      .arg(index + 1).arg(position.x()).arg(position.y()));
    };
    connect(m_apply, &QPushButton::clicked, this, &DisplayPage::applyChanges);
    connect(ok, &QPushButton::clicked, this, &DisplayPage::applyChanges);
    connect(cancel, &QPushButton::clicked, this, [this]() {
        QString error;
        if (!loadConfiguration(&error))
            setStatus(error, true);
    });
    auto markDirty = [this]() { m_apply->setEnabled(true); };
    connect(m_resolution, &QComboBox::currentIndexChanged, this, markDirty);
    connect(m_orientation, &QComboBox::currentIndexChanged, this, markDirty);
    connect(m_scale, &QComboBox::currentIndexChanged, this, markDirty);
    connect(m_multiple, &QComboBox::currentIndexChanged, this, markDirty);
    connect(m_primary, &QCheckBox::toggled, this, markDirty);

    QString error;
    if (!loadConfiguration(&error)) {
        setStatus(error, true);
        m_apply->setEnabled(false);
    }
}

bool DisplayPage::runDoctor(const QStringList &arguments, QString *error) const
{
    QProcess process;
    process.start(doctorExecutable(), arguments);
    if (!process.waitForStarted(3000) || !process.waitForFinished(10000)
        || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) {
            const QString details = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
            *error = details.isEmpty()
                ? QStringLiteral("Display settings could not be applied.") : details;
        }
        return false;
    }
    return true;
}

bool DisplayPage::loadConfiguration(QString *error)
{
    QProcess process;
    process.start(doctorExecutable(), {QStringLiteral("--json")});
    if (!process.waitForStarted(3000) || !process.waitForFinished(10000)
        || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error)
            *error = QStringLiteral("No display service is available in this session.");
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(
        process.readAllStandardOutput(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = QStringLiteral("The display service returned invalid information.");
        return false;
    }

    QVector<Output> outputs;
    const QJsonArray rows = document.object().value(QStringLiteral("outputs")).toArray();
    for (const QJsonValue &value : rows) {
        const QJsonObject row = value.toObject();
        if (!row.value(QStringLiteral("connected")).toBool())
            continue;
        Output output;
        output.name = row.value(QStringLiteral("name")).toString();
        output.connected = true;
        output.enabled = row.value(QStringLiteral("enabled")).toBool();
        output.priority = row.value(QStringLiteral("priority")).toInt();
        output.currentModeId = row.value(QStringLiteral("currentModeId")).toString();
        output.scale = row.value(QStringLiteral("scale")).toDouble(1.0);
        output.rotation = row.value(QStringLiteral("rotation")).toInt(1);
        const QJsonObject position = row.value(QStringLiteral("pos")).toObject();
        output.position = QPoint(position.value(QStringLiteral("x")).toInt(),
                                 position.value(QStringLiteral("y")).toInt());
        for (const QJsonValue &modeValue : row.value(QStringLiteral("modes")).toArray()) {
            const QJsonObject modeObject = modeValue.toObject();
            const QJsonObject size = modeObject.value(QStringLiteral("size")).toObject();
            output.modes.push_back({
                modeObject.value(QStringLiteral("id")).toString(),
                modeObject.value(QStringLiteral("name")).toString(),
                QSize(size.value(QStringLiteral("width")).toInt(),
                      size.value(QStringLiteral("height")).toInt()),
                modeObject.value(QStringLiteral("refreshRate")).toDouble(),
            });
        }
        if (!output.name.isEmpty())
            outputs.push_back(output);
    }
    if (outputs.isEmpty()) {
        if (error)
            *error = QStringLiteral("No connected displays were detected.");
        return false;
    }

    const QString selectedName = m_display->currentData().toString();
    m_outputs = outputs;
    m_display->blockSignals(true);
    m_display->clear();
    for (int index = 0; index < m_outputs.size(); ++index) {
        const Output &output = m_outputs.at(index);
        m_display->addItem(QStringLiteral("%1. %2%3")
                               .arg(index + 1).arg(output.name,
                                   output.enabled ? QString() : QStringLiteral(" (disconnected)")),
                           output.name);
    }
    int selected = m_display->findData(selectedName);
    m_display->setCurrentIndex(selected >= 0 ? selected : 0);
    m_display->blockSignals(false);
    m_multiple->setEnabled(m_outputs.size() > 1);
    QVector<DisplayLayoutEntry> diagramEntries;
    diagramEntries.reserve(m_outputs.size());
    for (const Output &output : std::as_const(m_outputs)) {
        QSize size(1024, 768);
        for (const Mode &mode : output.modes) {
            if (mode.id == output.currentModeId) {
                size = mode.size;
                break;
            }
        }
        if (output.scale > 0.0)
            size = QSize(qRound(size.width() / output.scale),
                         qRound(size.height() / output.scale));
        if (output.rotation == 2 || output.rotation == 8)
            size.transpose();
        diagramEntries.push_back({output.name, size, output.position, output.enabled});
    }
    m_monitorDiagram->setEntries(diagramEntries, m_display->currentIndex());
    populateOutputControls();
    setStatus(QStringLiteral("%1 connected display%2 detected.")
                  .arg(m_outputs.size()).arg(m_outputs.size() == 1 ? QString() : QStringLiteral("s")));
    m_apply->setEnabled(false);
    return true;
}

void DisplayPage::populateOutputControls()
{
    const int outputIndex = m_display->currentIndex();
    if (outputIndex < 0 || outputIndex >= m_outputs.size())
        return;
    const Output &output = m_outputs.at(outputIndex);

    m_monitorDiagram->setSelected(outputIndex);
    m_resolution->blockSignals(true);
    m_resolution->clear();
    QVector<Mode> modes = output.modes;
    std::sort(modes.begin(), modes.end(), [](const Mode &left, const Mode &right) {
        const qint64 leftArea = qint64(left.size.width()) * left.size.height();
        const qint64 rightArea = qint64(right.size.width()) * right.size.height();
        if (leftArea != rightArea) return leftArea > rightArea;
        if (left.size.width() != right.size.width()) return left.size.width() > right.size.width();
        return left.refreshRate > right.refreshRate;
    });
    QSet<QSize> sizes;
    for (const Mode &mode : std::as_const(modes)) {
        if (sizes.contains(mode.size))
            continue;
        sizes.insert(mode.size);
        QString modeId = mode.id;
        for (const Mode &candidate : std::as_const(output.modes)) {
            if (candidate.size == mode.size && candidate.id == output.currentModeId) {
                modeId = candidate.id;
                break;
            }
        }
        m_resolution->addItem(QStringLiteral("%1 x %2")
                                  .arg(mode.size.width()).arg(mode.size.height()), modeId);
    }
    int currentMode = -1;
    for (int index = 0; index < m_resolution->count(); ++index) {
        if (m_resolution->itemData(index).toString() == output.currentModeId) {
            currentMode = index;
            break;
        }
    }
    if (currentMode >= 0)
        m_resolution->setCurrentIndex(currentMode);
    m_resolution->blockSignals(false);
    const int orientation = m_orientation->findData(rotationName(output.rotation));
    m_orientation->setCurrentIndex(qMax(0, orientation));
    int scaleIndex = -1;
    for (int i = 0; i < m_scale->count(); ++i) {
        if (qAbs(m_scale->itemData(i).toDouble() - output.scale) < 0.01) {
            scaleIndex = i;
            break;
        }
    }
    if (scaleIndex < 0) {
        m_scale->addItem(QStringLiteral("%1%").arg(qRound(output.scale * 100)), output.scale);
        scaleIndex = m_scale->count() - 1;
    }
    m_scale->setCurrentIndex(scaleIndex);
    m_primary->setChecked(output.priority == 1);
    m_multiple->setCurrentIndex(0);
}

QStringList DisplayPage::restoreArguments() const
{
    QStringList arguments;
    for (const Output &output : m_outputs) {
        arguments << outputArgument(output.name, output.enabled ? QStringLiteral("enable")
                                                                : QStringLiteral("disable"));
        if (!output.currentModeId.isEmpty())
            arguments << outputArgument(output.name,
                                        QStringLiteral("mode.%1").arg(output.currentModeId));
        arguments << outputArgument(output.name,
                                    QStringLiteral("scale.%1").arg(output.scale, 0, 'g', 4));
        arguments << outputArgument(output.name,
                                    QStringLiteral("rotation.%1").arg(rotationName(output.rotation)));
        arguments << outputArgument(output.name,
                                    QStringLiteral("position.%1,%2")
                                        .arg(output.position.x()).arg(output.position.y()));
        if (output.priority > 0)
            arguments << outputArgument(output.name,
                                        QStringLiteral("priority.%1").arg(output.priority));
    }
    return arguments;
}

void DisplayPage::applyChanges()
{
    const int outputIndex = m_display->currentIndex();
    if (outputIndex < 0 || outputIndex >= m_outputs.size())
        return;
    const Output &selected = m_outputs.at(outputIndex);
    const QStringList restore = restoreArguments();
    QStringList arguments;

    const QString multiple = m_multiple->currentData().toString();
    if (multiple == QLatin1String("only")) {
        for (const Output &output : m_outputs)
            arguments << outputArgument(output.name,
                                        output.name == selected.name ? QStringLiteral("enable")
                                                                     : QStringLiteral("disable"));
    } else if (multiple == QLatin1String("disconnect")) {
        arguments << outputArgument(selected.name, QStringLiteral("disable"));
    } else {
        arguments << outputArgument(selected.name, QStringLiteral("enable"));
        for (const Output &output : std::as_const(m_outputs)) {
            if (output.enabled || output.name == selected.name) {
                arguments << outputArgument(output.name,
                                            QStringLiteral("position.%1,%2")
                                                .arg(output.position.x()).arg(output.position.y()));
            }
        }
    }

    if (!m_resolution->currentData().toString().isEmpty())
        arguments << outputArgument(selected.name,
                                    QStringLiteral("mode.%1").arg(m_resolution->currentData().toString()));
    arguments << outputArgument(selected.name,
                                QStringLiteral("scale.%1")
                                    .arg(m_scale->currentData().toDouble(), 0, 'g', 4));
    arguments << outputArgument(selected.name,
                                QStringLiteral("rotation.%1")
                                    .arg(m_orientation->currentData().toString()));
    if (m_primary->isChecked())
        arguments << outputArgument(selected.name, QStringLiteral("priority.1"));

    QString error;
    if (!runDoctor(arguments, &error)) {
        setStatus(error, true);
        return;
    }

    QMessageBox confirmation(QMessageBox::Question, "Display Settings",
        "Keep these display settings?\n\nReverting automatically in 15 seconds.",
        QMessageBox::Yes | QMessageBox::No, this);
    confirmation.setDefaultButton(QMessageBox::Yes);
    int seconds = 15;
    QTimer countdown;
    connect(&countdown, &QTimer::timeout, &confirmation, [&]() {
        --seconds;
        confirmation.setText(QStringLiteral(
            "Keep these display settings?\n\nReverting automatically in %1 seconds.").arg(seconds));
        if (seconds <= 0)
            confirmation.reject();
    });
    countdown.start(1000);
    if (confirmation.exec() != QMessageBox::Yes) {
        runDoctor(restore, nullptr);
        setStatus("The previous display settings were restored.");
    } else {
        setStatus("The new display settings were kept.");
    }
    loadConfiguration(nullptr);
}

void DisplayPage::setStatus(const QString &text, bool error)
{
    m_status->setText(text);
    m_status->setStyleSheet(QStringLiteral("color: %1; background: transparent;")
                                .arg(error ? QStringLiteral("#A00000")
                                           : QStringLiteral("#333333")));
}
