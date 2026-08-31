#include "SoundDialog.h"
#include "IconHelper.h"
#include "LinkLabel.h"
#include "Branding.h"

#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QMenu>
#include <QScrollArea>
#include <QFrame>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QComboBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDialogButtonBox>
#include <QFont>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QLineEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QMouseEvent>
#include <QEvent>
#include <QRegularExpression>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QPolygonF>
#include <QSlider>
#include <QFormLayout>
#include <QSet>

// Helpers
namespace {

// Run a read-only helper under a C locale so pactl's field labels stay parseable.
QString runTool(const QString &program, const QStringList &args)
{
    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    proc.setProcessEnvironment(env);
    proc.start(program, args);
    if (!proc.waitForFinished(2500))
        return QString();
    return QString::fromUtf8(proc.readAllStandardOutput());
}

// Pick a device glyph from the name/description.
QIcon deviceIcon(const QString &label, bool sinks)
{
    const QString n = label.toLower();
    if (n.contains("head"))
        return themeIcon({"audio-headphones", "audio-headset", "audio-speakers"});
    if (!sinks || n.contains("mic"))
        return themeIcon({"audio-input-microphone", "audio-headset",
                          "audio-card"});
    return themeIcon({"audio-speakers", "audio-card",
                      "preferences-desktop-sound"});
}

// Compose the device glyph with the green "default" check badge, as Windows
// overlays on the default device.
QPixmap devicePixmap(const QIcon &icon, int size, bool isDefault)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawPixmap(0, 0, icon.pixmap(size, size));

    if (isDefault) {
        const double b = size * 0.46;
        const double x = size - b, y = size - b;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#FFFFFF"));
        p.drawEllipse(QRectF(x - 1, y - 1, b + 2, b + 2));
        p.setBrush(QColor("#3FA535"));
        p.drawEllipse(QRectF(x, y, b, b));

        QPen wp(QColor("#FFFFFF"), qMax(1.4, b * 0.14));
        wp.setCapStyle(Qt::RoundCap);
        wp.setJoinStyle(Qt::RoundJoin);
        p.setPen(wp);
        p.setBrush(Qt::NoBrush);
        QPolygonF check;
        check << QPointF(x + b * 0.24, y + b * 0.52)
              << QPointF(x + b * 0.42, y + b * 0.70)
              << QPointF(x + b * 0.78, y + b * 0.28);
        p.drawPolyline(check);
    }
    return pm;
}

} // namespace

// Data gathering
QList<SoundDialog::Device> SoundDialog::gatherDevices(bool sinks) const
{
    QList<Device> out;
    const QString text = runTool(QStringLiteral("pactl"),
        { QStringLiteral("list"), sinks ? QStringLiteral("sinks")
                                        : QStringLiteral("sources") });
    if (text.isEmpty())
        return out;

    static const QRegularExpression volRe(QStringLiteral("(\\d+)%"));
    Device cur;
    QString curName;
    bool in = false;

    auto flush = [&]() {
        // Skip monitor sources and the dummy sink; they aren't real devices.
        if (in && !curName.endsWith(QLatin1String(".monitor"))
            && curName != QLatin1String("auto_null"))
            out << cur;
    };

    const QList<QStringView> lines = QStringView(text).split(u'\n');
    for (const QStringView &raw : lines) {
        const QStringView line = raw.trimmed();
        if (line.startsWith(u"Sink #") || line.startsWith(u"Source #")) {
            flush();
            cur = Device();
            curName.clear();
            in = true;
        } else if (line.startsWith(u"Name:")) {
            curName = line.mid(5).trimmed().toString();
            cur.name = curName;
        } else if (line.startsWith(u"Description:")) {
            cur.description = line.mid(12).trimmed().toString();
        } else if (line.startsWith(u"Mute:")) {
            cur.muted = (line.mid(5).trimmed() == u"yes");
        } else if (line.startsWith(u"Volume:")) {
            const auto m = volRe.match(line.toString());
            if (m.hasMatch())
                cur.volumePercent = m.captured(1).toInt();
        } else if (line.startsWith(u"alsa.card_name = \"")) {
            QString v = line.mid(18).toString();
            if (v.endsWith(QLatin1Char('"')))
                v.chop(1);
            cur.cardName = v;
        }
    }
    flush();
    return out;
}

QString SoundDialog::defaultDevice(bool sinks) const
{
    return runTool(QStringLiteral("pactl"),
        { sinks ? QStringLiteral("get-default-sink")
                : QStringLiteral("get-default-source") }).trimmed();
}

// Dialog
SoundDialog::SoundDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Sound"));
    setWindowIcon(themeIcon({"preferences-desktop-sound", "audio-card",
                             "multimedia-volume-control"}));
    setModal(true);
    setStyleSheet("SoundDialog { background: #F0F0F0; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(9, 9, 9, 9);
    root->setSpacing(8);

    m_tabs = new QTabWidget;
    m_tabs->addTab(buildDeviceTab(m_playback, true,
        QStringLiteral("Select a playback device below to modify its settings:")),
        QStringLiteral("Playback"));
    m_tabs->addTab(buildDeviceTab(m_recording, false,
        QStringLiteral("Select a recording device below to modify its settings:")),
        QStringLiteral("Recording"));
    m_tabs->addTab(buildSoundsTab(), QStringLiteral("Sounds"));
    m_tabs->addTab(buildCommunicationsTab(), QStringLiteral("Communications"));
    root->addWidget(m_tabs, 1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel
                                         | QDialogButtonBox::Apply);
    root->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SoundDialog::applyRequested);

    // Meter only the device on the visible tab (and don't hold the mic open
    // except while the Recording tab is shown).
    connect(m_tabs, &QTabWidget::currentChanged,
            this, [this](int) { updateMonitors(); });
    updateMonitors();

    setFixedSize(410, 446);
}

SoundDialog::~SoundDialog()
{
    stopMonitor(m_playback);
    stopMonitor(m_recording);
}

void SoundDialog::showTab(Tab tab)
{
    if (m_tabs)
        m_tabs->setCurrentIndex(tab);
}

// Playback / Recording tabs
QWidget *SoundDialog::buildDeviceTab(DeviceList &dl, bool sinks,
                                     const QString &prompt)
{
    dl.sinks = sinks;
    dl.devices = gatherDevices(sinks);
    dl.defaultName = defaultDevice(sinks);

    auto *page = new QWidget;
    page->setObjectName("devPage");
    page->setStyleSheet("#devPage { background: #FFFFFF; }");
    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(14, 12, 14, 12);
    v->setSpacing(8);

    auto *promptLabel = new QLabel(prompt);
    {
        QFont f = promptLabel->font();
        f.setPointSize(9);
        promptLabel->setFont(f);
    }
    v->addWidget(promptLabel);

    // The bordered device list.
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(
        "QScrollArea { border: 1px solid #A0A0A0; background: #FFFFFF; }");
    auto *container = new QWidget;
    container->setStyleSheet("background: #FFFFFF;");
    dl.box = new QVBoxLayout(container);
    dl.box->setContentsMargins(0, 0, 0, 0);
    dl.box->setSpacing(0);
    scroll->setWidget(container);
    v->addWidget(scroll, 1);

    // Buttons: Configure | (stretch) | Set Default ▾ | Properties.
    auto *btnRow = new QHBoxLayout;
    btnRow->setContentsMargins(0, 0, 0, 0);
    btnRow->setSpacing(8);

    dl.configureBtn = new QPushButton(QStringLiteral("Configure"));
    dl.configureBtn->setCursor(Qt::PointingHandCursor);
    connect(dl.configureBtn, &QPushButton::clicked, this,
            [this, &dl]() { showProperties(dl); });
    btnRow->addWidget(dl.configureBtn);
    btnRow->addStretch(1);

    dl.setDefaultBtn = new QToolButton;
    dl.setDefaultBtn->setText(QStringLiteral("Set Default"));
    dl.setDefaultBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    dl.setDefaultBtn->setPopupMode(QToolButton::MenuButtonPopup);
    dl.setDefaultBtn->setCursor(Qt::PointingHandCursor);
    auto *menu = new QMenu(dl.setDefaultBtn);
    menu->addAction(QStringLiteral("Default Device"),
                    this, [this, &dl]() { applyDefault(dl); });
    menu->addAction(QStringLiteral("Default Communication Device"),
                    this, [this, &dl]() { applyDefault(dl); });
    dl.setDefaultBtn->setMenu(menu);
    connect(dl.setDefaultBtn, &QToolButton::clicked,
            this, [this, &dl]() { applyDefault(dl); });
    btnRow->addWidget(dl.setDefaultBtn);

    dl.propertiesBtn = new QPushButton(QStringLiteral("Properties"));
    dl.propertiesBtn->setCursor(Qt::PointingHandCursor);
    connect(dl.propertiesBtn, &QPushButton::clicked, this,
            [this, &dl]() { showProperties(dl); });
    btnRow->addWidget(dl.propertiesBtn);

    v->addLayout(btnRow);

    rebuildRows(dl);

    // Pre-select the default device (or the first one).
    int initial = 0;
    for (int i = 0; i < dl.devices.size(); ++i)
        if (dl.devices[i].name == dl.defaultName)
            initial = i;
    if (!dl.devices.isEmpty())
        selectRow(dl, initial);
    else {
        dl.setDefaultBtn->setEnabled(false);
        dl.propertiesBtn->setEnabled(false);
        dl.configureBtn->setEnabled(false);
    }

    return page;
}

void SoundDialog::rebuildRows(DeviceList &dl)
{
    // The old meter widget is about to be deleted, so stop the capture feeding
    // it first; callers restart it via updateMonitors once rows are rebuilt.
    stopMonitor(dl);
    dl.activeMeter = nullptr;

    // Fully rebuild: clear the list (rows + trailing stretch), then re-add.
    while (QLayoutItem *item = dl.box->takeAt(0)) {
        if (QWidget *w = item->widget())
            w->deleteLater();
        delete item;
    }
    dl.rows.clear();
    dl.rowToIndex.clear();

    if (dl.devices.isEmpty()) {
        auto *empty = new QLabel(dl.sinks
            ? QStringLiteral("No playback devices were found.")
            : QStringLiteral("No recording devices were found."));
        empty->setStyleSheet("color: #555555; padding: 14px;");
        dl.box->addWidget(empty);
        dl.box->addStretch(1);
        return;
    }

    for (int i = 0; i < dl.devices.size(); ++i) {
        const Device &d = dl.devices[i];
        const bool isDefault = (d.name == dl.defaultName);

        auto *row = new QWidget;
        row->setObjectName("devRow");
        row->setAutoFillBackground(true);
        row->setCursor(Qt::PointingHandCursor);
        auto *h = new QHBoxLayout(row);
        h->setContentsMargins(8, 7, 8, 7);
        h->setSpacing(10);

        auto *icon = new QLabel;
        icon->setFixedSize(44, 44);
        icon->setPixmap(devicePixmap(
            deviceIcon(d.description + ' ' + d.name, dl.sinks), 44, isDefault));
        icon->setStyleSheet("background: transparent;");
        h->addWidget(icon, 0, Qt::AlignVCenter);

        auto *textCol = new QVBoxLayout;
        textCol->setContentsMargins(0, 0, 0, 0);
        textCol->setSpacing(0);

        auto *name = new QLabel(d.description.isEmpty() ? d.name : d.description);
        {
            QFont f = name->font();
            f.setPointSize(9);
            f.setBold(true);
            name->setFont(f);
        }
        name->setStyleSheet("color: #000000; background: transparent;");
        textCol->addWidget(name);

        QString cardLine = d.cardName;
        if (cardLine.isEmpty() || cardLine == d.description)
            cardLine = QStringLiteral("Audio Device");
        auto *card = new QLabel(cardLine);
        { QFont f = card->font(); f.setPointSize(9); card->setFont(f); }
        card->setStyleSheet("color: #333333; background: transparent;");
        textCol->addWidget(card);

        auto *status = new QLabel(isDefault ? QStringLiteral("Default Device")
                                            : QStringLiteral("Ready"));
        { QFont f = status->font(); f.setPointSize(9); status->setFont(f); }
        status->setStyleSheet("color: #333333; background: transparent;");
        textCol->addWidget(status);

        h->addLayout(textCol, 1);

        auto *meter = new LevelMeter;
        h->addWidget(meter, 0, Qt::AlignVCenter);
        if (isDefault)
            dl.activeMeter = meter;   // the device whose level we capture live

        row->installEventFilter(this);
        dl.rowToIndex.insert(row, i);
        dl.rows << row;
        dl.box->addWidget(row);
    }
    dl.box->addStretch(1);
}

void SoundDialog::selectRow(DeviceList &dl, int index)
{
    if (index < 0 || index >= dl.devices.size())
        return;
    dl.selected = index;

    for (auto it = dl.rowToIndex.constBegin(); it != dl.rowToIndex.constEnd();
         ++it) {
        auto *w = qobject_cast<QWidget *>(const_cast<QObject *>(it.key()));
        if (!w)
            continue;
        const bool sel = (it.value() == index);
        w->setStyleSheet(sel
            ? "#devRow { background: #CDE6FF; border: 1px solid #7DA2CE; }"
            : "#devRow { background: transparent; border: 1px solid transparent; }");
    }

    dl.propertiesBtn->setEnabled(true);
    dl.configureBtn->setEnabled(true);
    dl.setDefaultBtn->setEnabled(dl.devices[index].name != dl.defaultName);
}

void SoundDialog::applyDefault(DeviceList &dl)
{
    if (dl.selected < 0 || dl.selected >= dl.devices.size())
        return;
    runTool(QStringLiteral("pactl"),
            { dl.sinks ? QStringLiteral("set-default-sink")
                       : QStringLiteral("set-default-source"),
              dl.devices[dl.selected].name });

    dl.defaultName = defaultDevice(dl.sinks);
    rebuildRows(dl);
    selectRow(dl, dl.selected);
    updateMonitors();   // the meter now belongs to the new default device
}

void SoundDialog::showProperties(DeviceList &dl)
{
    if (dl.selected < 0 || dl.selected >= dl.devices.size())
        return;
    const Device device = dl.devices[dl.selected];

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("%1 Properties")
                              .arg(device.description.isEmpty()
                                       ? device.name : device.description));
    auto *layout = new QVBoxLayout(&dialog);
    auto *description = new QLabel(
        QStringLiteral("%1\n%2")
            .arg(device.description.isEmpty() ? device.name : device.description,
                 device.cardName.isEmpty() ? QStringLiteral("Audio Device")
                                           : device.cardName));
    layout->addWidget(description);

    auto *form = new QFormLayout;
    auto *volume = new QSlider(Qt::Horizontal);
    volume->setRange(0, 150);
    volume->setValue(qBound(0, device.volumePercent, 150));
    auto *volumeRow = new QWidget;
    auto *volumeLayout = new QHBoxLayout(volumeRow);
    volumeLayout->setContentsMargins(0, 0, 0, 0);
    auto *percent = new QLabel(QStringLiteral("%1%").arg(volume->value()));
    connect(volume, &QSlider::valueChanged, percent,
            [percent](int value) {
                percent->setText(QStringLiteral("%1%").arg(value));
            });
    volumeLayout->addWidget(volume, 1);
    volumeLayout->addWidget(percent);
    form->addRow(QStringLiteral("Volume:"), volumeRow);

    auto *mute = new QCheckBox(QStringLiteral("Mute this device"));
    mute->setChecked(device.muted);
    form->addRow(QString(), mute);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString kind = dl.sinks ? QStringLiteral("sink")
                                   : QStringLiteral("source");
    runTool(QStringLiteral("pactl"),
            {QStringLiteral("set-%1-volume").arg(kind), device.name,
             QStringLiteral("%1%").arg(volume->value())});
    runTool(QStringLiteral("pactl"),
            {QStringLiteral("set-%1-mute").arg(kind), device.name,
             mute->isChecked() ? QStringLiteral("1") : QStringLiteral("0")});
    dl.devices = gatherDevices(dl.sinks);
    dl.defaultName = defaultDevice(dl.sinks);
    rebuildRows(dl);
    int selected = 0;
    for (int i = 0; i < dl.devices.size(); ++i)
        if (dl.devices[i].name == device.name)
            selected = i;
    if (!dl.devices.isEmpty())
        selectRow(dl, selected);
    updateMonitors();
}

// Live level metering
void SoundDialog::startMonitor(DeviceList &dl)
{
    if (dl.monitorProc || !dl.activeMeter || dl.defaultName.isEmpty())
        return;

    // Capture the sink's monitor (playback) or the source itself (recording) as
    // mono s16le and drive the meter from each chunk's peak amplitude.
    const QString dev = dl.sinks ? dl.defaultName + QStringLiteral(".monitor")
                                 : dl.defaultName;
    auto *proc = new QProcess(this);
    dl.monitorProc = proc;
    LevelMeter *meter = dl.activeMeter;
    connect(proc, &QProcess::readyReadStandardOutput, meter, [proc, meter]() {
        const QByteArray data = proc->readAllStandardOutput();
        const int count = data.size() / 2;
        const auto *samples =
            reinterpret_cast<const qint16 *>(data.constData());
        int peak = 0;
        for (int i = 0; i < count; ++i) {
            const int a = qAbs(int(samples[i]));
            if (a > peak)
                peak = a;
        }
        meter->setLevel(peak * 100 / 32768);
    });
    proc->start(QStringLiteral("parec"),
                { QStringLiteral("--format=s16le"), QStringLiteral("--channels=1"),
                  QStringLiteral("--rate=22050"), QStringLiteral("--latency-msec=60"),
                  QStringLiteral("-d"), dev });
}

void SoundDialog::stopMonitor(DeviceList &dl)
{
    if (!dl.monitorProc)
        return;
    dl.monitorProc->kill();
    dl.monitorProc->deleteLater();
    dl.monitorProc = nullptr;
    if (dl.activeMeter)
        dl.activeMeter->setLevel(0);
}

void SoundDialog::updateMonitors()
{
    const int cur = m_tabs->currentIndex();
    if (cur == TabPlayback)  startMonitor(m_playback);  else stopMonitor(m_playback);
    if (cur == TabRecording) startMonitor(m_recording); else stopMonitor(m_recording);
}

// Sounds tab
QWidget *SoundDialog::buildSoundsTab()
{
    auto *page = new QWidget;
    page->setObjectName("sndPage");
    page->setStyleSheet("#sndPage { background: #FFFFFF; }");
    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(14, 14, 14, 12);
    v->setSpacing(6);

    auto para = [&](const QString &t) {
        auto *l = new QLabel(t);
        QFont f = l->font();
        f.setPointSize(9);
        l->setFont(f);
        l->setWordWrap(true);
        v->addWidget(l);
        return l;
    };

    para(QStringLiteral("A sound theme supplies the notification sounds used by "
                        "Aero7 and Plasma applications. Changes are written to "
                        "the real Plasma sound-theme setting."));

    auto *schemeLabel = new QLabel(QStringLiteral("Sound Scheme:"));
    { QFont f = schemeLabel->font(); f.setPointSize(9); schemeLabel->setFont(f); }
    v->addWidget(schemeLabel);

    auto *schemeRow = new QHBoxLayout;
    schemeRow->setSpacing(8);
    auto *scheme = new QComboBox;
    scheme->setObjectName(QStringLiteral("soundScheme"));
    scheme->addItem(QStringLiteral("No Sounds"), QString());

    QSet<QString> seenThemes;
    for (const QString &base :
         QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
        QDir sounds(base + QStringLiteral("/sounds"));
        for (const QString &id :
             sounds.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            if (seenThemes.contains(id)
                || !QFile::exists(sounds.filePath(id + QStringLiteral("/index.theme"))))
                continue;
            seenThemes.insert(id);
            QSettings metadata(
                sounds.filePath(id + QStringLiteral("/index.theme")),
                QSettings::IniFormat);
            const QString name =
                metadata.value(QStringLiteral("Sound Theme/Name"), id).toString();
            scheme->addItem(name, id);
        }
    }

    QSettings kdeGlobals(
        QDir::homePath() + QStringLiteral("/.config/kdeglobals"),
        QSettings::IniFormat);
    kdeGlobals.beginGroup(QStringLiteral("Sounds"));
    const bool soundsEnabled =
        kdeGlobals.value(QStringLiteral("Enable"), true).toBool();
    const QString selectedTheme =
        kdeGlobals.value(QStringLiteral("Theme"), QStringLiteral("freedesktop"))
            .toString();
    kdeGlobals.endGroup();
    const int selectedIndex = soundsEnabled
        ? scheme->findData(selectedTheme) : 0;
    scheme->setCurrentIndex(selectedIndex >= 0 ? selectedIndex : 0);
    schemeRow->addWidget(scheme, 1);
    v->addLayout(schemeRow);
    v->addSpacing(2);

    para(QStringLiteral("Select a sound below and click Test to preview it."));

    auto *eventsLabel = new QLabel(QStringLiteral("Program Events:"));
    { QFont f = eventsLabel->font(); f.setPointSize(9); eventsLabel->setFont(f); }
    v->addWidget(eventsLabel);

    auto *tree = new QTreeWidget;
    tree->setHeaderHidden(true);
    tree->setRootIsDecorated(false);
    tree->setStyleSheet("QTreeWidget { border: 1px solid #A0A0A0; }");
    v->addWidget(tree, 1);

    auto populateSounds = [scheme, tree]() {
        tree->clear();
        const QString id = scheme->currentData().toString();
        if (id.isEmpty())
            return;
        QSet<QString> paths;
        for (const QString &base :
             QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation)) {
            const QDir theme(base + QStringLiteral("/sounds/") + id);
            const QStringList files = theme.entryList(
                {QStringLiteral("*.oga"), QStringLiteral("*.ogg"),
                 QStringLiteral("*.wav"), QStringLiteral("*.flac")},
                QDir::Files, QDir::Name);
            for (const QString &file : files)
                paths.insert(theme.filePath(file));
            const QDir stereo(theme.filePath(QStringLiteral("stereo")));
            for (const QString &file : stereo.entryList(
                     {QStringLiteral("*.oga"), QStringLiteral("*.ogg"),
                      QStringLiteral("*.wav"), QStringLiteral("*.flac")},
                     QDir::Files, QDir::Name))
                paths.insert(stereo.filePath(file));
        }
        QStringList ordered = paths.values();
        ordered.sort(Qt::CaseInsensitive);
        const QIcon icon = themeIcon({"audio-volume-high", "audio-speakers"});
        for (const QString &path : ordered) {
            auto *item = new QTreeWidgetItem(
                tree, {QFileInfo(path).completeBaseName()});
            item->setIcon(0, icon);
            item->setData(0, Qt::UserRole, path);
        }
        if (tree->topLevelItemCount())
            tree->setCurrentItem(tree->topLevelItem(0));
    };
    connect(scheme, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [populateSounds](int) { populateSounds(); });
    populateSounds();

    auto *soundsRow = new QHBoxLayout;
    soundsRow->addStretch(1);
    auto *test = new QPushButton(QStringLiteral("Test"));
    test->setIcon(themeIcon({"media-playback-start"}));
    connect(test, &QPushButton::clicked, this, [this, tree]() {
        const QString path = tree->currentItem()
            ? tree->currentItem()->data(0, Qt::UserRole).toString() : QString();
        if (!path.isEmpty())
            launchDetached(this, {QStringLiteral("paplay"), path});
    });
    soundsRow->addWidget(test);
    v->addLayout(soundsRow);

    auto save = [scheme]() {
        QSettings settings(
            QDir::homePath() + QStringLiteral("/.config/kdeglobals"),
            QSettings::IniFormat);
        settings.beginGroup(QStringLiteral("Sounds"));
        const QString id = scheme->currentData().toString();
        settings.setValue(QStringLiteral("Enable"), !id.isEmpty());
        if (!id.isEmpty())
            settings.setValue(QStringLiteral("Theme"), id);
        settings.endGroup();
        settings.sync();
    };
    connect(this, &QDialog::accepted, this, save);
    connect(this, &SoundDialog::applyRequested, this, save);

    return page;
}

// Communications tab
QWidget *SoundDialog::buildCommunicationsTab()
{
    auto *page = new QWidget;
    page->setObjectName("commPage");
    page->setStyleSheet("#commPage { background: #FFFFFF; }");
    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(16, 16, 16, 12);
    v->setSpacing(0);

    auto *top = new QHBoxLayout;
    top->setSpacing(12);
    auto *icon = new QLabel;
    icon->setFixedSize(32, 32);
    icon->setPixmap(themeIcon({"call-start", "phone", "audio-input-microphone"})
                        .pixmap(32, 32));
    icon->setStyleSheet("background: transparent;");
    top->addWidget(icon, 0, Qt::AlignTop);
    auto *intro = new QLabel(
        QStringLiteral("Communications applications may adjust their own audio "
                       "while a call is active."));
    { QFont f = intro->font(); f.setPointSize(9); intro->setFont(f); }
    intro->setWordWrap(true);
    top->addWidget(intro, 1);
    v->addLayout(top);
    v->addSpacing(16);

    auto *prompt = new QLabel(
        QStringLiteral("When Aero7 detects communications activity:"));
    { QFont f = prompt->font(); f.setPointSize(9); prompt->setFont(f); }
    v->addWidget(prompt);
    v->addSpacing(8);

    auto *choice = new QRadioButton(QStringLiteral("Do nothing"));
    choice->setChecked(true);
    choice->setEnabled(false);
    v->addWidget(choice);
    v->addSpacing(12);

    auto *note = new QLabel(
        QStringLiteral("PipeWire does not expose a reliable system-wide call "
                       "detection signal. Aero7 therefore leaves application "
                       "volumes unchanged instead of saving a setting that "
                       "would not work. Individual calling applications can "
                       "still manage their own audio."));
    note->setWordWrap(true);
    note->setStyleSheet(
        QStringLiteral("QLabel { color: #5C4800; background: #FFF8D8; "
                       "border: 1px solid #D7C56B; padding: 8px; }"));
    v->addWidget(note);

    v->addStretch(1);
    return page;
}

// Row selection
bool SoundDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress
        || event->type() == QEvent::MouseButtonDblClick) {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() != Qt::LeftButton)
            return QDialog::eventFilter(watched, event);

        for (DeviceList *dl : { &m_playback, &m_recording }) {
            auto it = dl->rowToIndex.constFind(watched);
            if (it != dl->rowToIndex.constEnd()) {
                selectRow(*dl, it.value());
                if (event->type() == QEvent::MouseButtonDblClick)
                    applyDefault(*dl);
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}
