#include "PersonalizationPage.h"
#include "Commands.h"
#include "LinkLabel.h"
#include "IconHelper.h"
#include "Win7Ui.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QFont>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QProcess>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QSet>
#include <QMouseEvent>
#include <QEvent>
#include "Aero7FileDialog.h"
#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QFormLayout>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDirIterator>
#include <QImageReader>
#include <QFile>
#include <algorithm>
#include <functional>

namespace {

QString personalizationConfigPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/aero7-personalization.conf");
}

bool isSupportedImage(const QString &path)
{
    const QByteArray suffix = QFileInfo(path).suffix().toLower().toLatin1();
    return QImageReader::supportedImageFormats().contains(suffix);
}

QStringList findWallpapers()
{
    QStringList roots = {
        QStringLiteral("/usr/share/wallpapers"),
        QStringLiteral("/usr/share/backgrounds"),
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
            + QStringLiteral("/wallpapers"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
    };

    QSettings settings(personalizationConfigPath(), QSettings::IniFormat);
    const QString configured =
        settings.value(QStringLiteral("DesktopBackground/Image")).toString();

    QStringList images;
    if (QFileInfo::exists(configured) && isSupportedImage(configured))
        images << QFileInfo(configured).canonicalFilePath();

    QSet<QString> seen(images.cbegin(), images.cend());
    for (const QString &root : std::as_const(roots)) {
        if (!QFileInfo::exists(root))
            continue;
        QDirIterator it(root, QDir::Files | QDir::Readable,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            if (!isSupportedImage(path))
                continue;
            const QString canonical = QFileInfo(path).canonicalFilePath();
            if (canonical.isEmpty() || seen.contains(canonical))
                continue;
            seen.insert(canonical);
            images << canonical;
            if (images.size() >= 80)
                return images;
        }
    }
    return images;
}

class DesktopBackgroundDialog final : public QDialog {
public:
    explicit DesktopBackgroundDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("Desktop Background"));
        resize(830, 600);
        setMinimumSize(680, 500);

        auto *outer = new QVBoxLayout(this);
        outer->setContentsMargins(18, 14, 18, 14);
        outer->setSpacing(10);

        auto *title = Win7::pageTitle(
            QStringLiteral("Choose your desktop background"));
        outer->addWidget(title);

        auto *locationRow = new QHBoxLayout;
        locationRow->addWidget(new QLabel(QStringLiteral("Picture location:")));
        m_location = new QComboBox;
        m_location->addItem(QStringLiteral("Aero7 Desktop Backgrounds"));
        m_location->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        locationRow->addWidget(m_location, 1);
        auto *browse = new QPushButton(QStringLiteral("Browse..."));
        connect(browse, &QPushButton::clicked, this,
                [this]() { browseForImage(); });
        locationRow->addWidget(browse);
        outer->addLayout(locationRow);

        m_wallpapers = new QListWidget;
        m_wallpapers->setObjectName(QStringLiteral("wallpaperGallery"));
        m_wallpapers->setViewMode(QListView::IconMode);
        m_wallpapers->setMovement(QListView::Static);
        m_wallpapers->setResizeMode(QListView::Adjust);
        m_wallpapers->setSelectionMode(QAbstractItemView::SingleSelection);
        m_wallpapers->setIconSize(QSize(144, 86));
        m_wallpapers->setGridSize(QSize(164, 122));
        m_wallpapers->setSpacing(4);
        m_wallpapers->setWordWrap(true);
        m_wallpapers->setStyleSheet(
            "QListWidget#wallpaperGallery { background: white; border: 1px solid #8E9EAE; }"
            "QListWidget#wallpaperGallery::item { padding: 5px; border: 1px solid transparent; }"
            "QListWidget#wallpaperGallery::item:selected { background: #D9ECFF; border: 1px solid #7DA2CE; color: black; }");
        outer->addWidget(m_wallpapers, 1);

        auto *bottom = new QHBoxLayout;
        bottom->addWidget(new QLabel(QStringLiteral("Picture position:")));
        m_position = new QComboBox;
        m_position->addItem(QStringLiteral("Fill"),
                            QStringLiteral("preserveAspectCrop"));
        m_position->addItem(QStringLiteral("Fit"),
                            QStringLiteral("preserveAspectFit"));
        m_position->addItem(QStringLiteral("Stretch"),
                            QStringLiteral("stretch"));
        m_position->addItem(QStringLiteral("Tile"), QStringLiteral("tile"));
        m_position->addItem(QStringLiteral("Center"),
                            QStringLiteral("preserveAspectFit"));
        bottom->addWidget(m_position);
        bottom->addStretch(1);
        outer->addLayout(bottom);

        auto *buttons = new QDialogButtonBox;
        auto *save = buttons->addButton(QStringLiteral("Save changes"),
                                        QDialogButtonBox::AcceptRole);
        buttons->addButton(QDialogButtonBox::Cancel);
        connect(save, &QPushButton::clicked, this, [this]() { apply(); });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        outer->addWidget(buttons);

        load();
    }

private:
    void addImage(const QString &path, bool select)
    {
        if (!QFileInfo::exists(path) || !isSupportedImage(path))
            return;
        for (int i = 0; i < m_wallpapers->count(); ++i) {
            auto *existing = m_wallpapers->item(i);
            if (existing->data(Qt::UserRole).toString() == path) {
                if (select)
                    m_wallpapers->setCurrentItem(existing);
                return;
            }
        }

        QPixmap preview(path);
        if (preview.isNull())
            return;
        preview = preview.scaled(QSize(144, 86), Qt::KeepAspectRatioByExpanding,
                                 Qt::SmoothTransformation);
        if (preview.width() > 144 || preview.height() > 86) {
            preview = preview.copy((preview.width() - 144) / 2,
                                   (preview.height() - 86) / 2, 144, 86);
        }
        auto *item = new QListWidgetItem(QIcon(preview),
                                         QFileInfo(path).completeBaseName());
        item->setData(Qt::UserRole, path);
        item->setToolTip(path);
        m_wallpapers->addItem(item);
        if (select)
            m_wallpapers->setCurrentItem(item);
    }

    void load()
    {
        QSettings settings(personalizationConfigPath(), QSettings::IniFormat);
        const QString current =
            settings.value(QStringLiteral("DesktopBackground/Image")).toString();
        const QString mode = settings.value(
            QStringLiteral("DesktopBackground/FillMode"),
            QStringLiteral("preserveAspectCrop")).toString();
        const int position = m_position->findData(mode);
        if (position >= 0)
            m_position->setCurrentIndex(position);

        for (const QString &path : findWallpapers())
            addImage(path, path == current);
        if (!m_wallpapers->currentItem() && m_wallpapers->count() > 0)
            m_wallpapers->setCurrentRow(0);
    }

    void browseForImage()
    {
        const QString path = Aero7FileDialog::openFile(
            this, QStringLiteral("control-personalization"),
            QStringLiteral("Images (*.png *.jpg *.jpeg *.webp *.bmp);;All files (*)"));
        if (!path.isEmpty())
            addImage(QFileInfo(path).canonicalFilePath(), true);
    }

    void apply()
    {
        auto *item = m_wallpapers->currentItem();
        if (!item) {
            QMessageBox::warning(this, QStringLiteral("Desktop Background"),
                                 QStringLiteral("Select a picture first."));
            return;
        }
        const QString image = item->data(Qt::UserRole).toString();
        const QString mode = m_position->currentData().toString();
        const QString tool = QStandardPaths::findExecutable(
            QStringLiteral("plasma-apply-wallpaperimage"));
        if (tool.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Desktop Background"),
                                 QStringLiteral("The wallpaper backend is not installed."));
            return;
        }

        QProcess process;
        process.start(tool, {QStringLiteral("--fill-mode"), mode, image});
        if (!process.waitForStarted(2000) || !process.waitForFinished(10000)
            || process.exitCode() != 0) {
            const QString detail =
                QString::fromUtf8(process.readAllStandardError()).trimmed();
            QMessageBox::warning(
                this, QStringLiteral("Desktop Background"),
                detail.isEmpty()
                    ? QStringLiteral("The background could not be changed.")
                    : detail);
            return;
        }

        QSettings settings(personalizationConfigPath(), QSettings::IniFormat);
        settings.setValue(QStringLiteral("DesktopBackground/Image"), image);
        settings.setValue(QStringLiteral("DesktopBackground/FillMode"), mode);
        settings.sync();
        accept();
    }

    QComboBox *m_location = nullptr;
    QListWidget *m_wallpapers = nullptr;
    QComboBox *m_position = nullptr;
};

bool isAero7Scheme(const QString &id, const QString &name)
{
    const QString value = (id + QLatin1Char(' ') + name).toLower();
    return value.contains(QStringLiteral("aero"))
        || value.contains(QStringLiteral("windows 7"))
        || value.contains(QStringLiteral("seven"));
}

} // namespace

// Data gathering
// Parse a "r,g,b" (optionally with a trailing alpha) triple into a QColor.
static QColor parseRgb(const QString &value, const QColor &fallback)
{
    const QStringList parts = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (parts.size() < 3)
        return fallback;
    return QColor(parts[0].trimmed().toInt(),
                  parts[1].trimmed().toInt(),
                  parts[2].trimmed().toInt());
}

QList<PersonalizationPage::Scheme> PersonalizationPage::gatherSchemes()
{
    QList<Scheme> schemes;
    QSet<QString> seenIds;

    // Search every standard data dir's color-schemes folder; a user scheme in
    // ~/.local/share shadows a system one of the same id.
    QStringList dirs;
    for (const QString &base :
         QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation))
        dirs << base + QStringLiteral("/color-schemes");

    for (const QString &dirPath : dirs) {
        QDir dir(dirPath);
        if (!dir.exists())
            continue;
        const QStringList files = dir.entryList({QStringLiteral("*.colors")},
                                                QDir::Files, QDir::Name);
        for (const QString &file : files) {
            const QString id = QFileInfo(file).completeBaseName();
            if (seenIds.contains(id))
                continue;
            seenIds.insert(id);

            QSettings ini(dir.filePath(file), QSettings::IniFormat);
            Scheme s;
            s.id = id;
            s.name = ini.value(QStringLiteral("General/Name"), id).toString();

            if (!isAero7Scheme(s.id, s.name))
                continue;

            s.window = parseRgb(
                ini.value(QStringLiteral("Colors:Window/BackgroundNormal"))
                    .toString(), QColor("#F0F0F0"));
            s.view = parseRgb(
                ini.value(QStringLiteral("Colors:View/BackgroundNormal"))
                    .toString(), QColor("#FFFFFF"));
            s.accent = parseRgb(
                ini.value(QStringLiteral("Colors:Selection/BackgroundNormal"))
                    .toString(), QColor("#3399FF"));
            s.titlebar = parseRgb(
                ini.value(QStringLiteral("WM/activeBackground")).toString(),
                s.window);
            s.text = parseRgb(
                ini.value(QStringLiteral("Colors:Window/ForegroundNormal"))
                    .toString(), QColor("#202020"));

            schemes << s;
        }
    }

    std::sort(schemes.begin(), schemes.end(),
              [](const Scheme &a, const Scheme &b) {
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
              });
    return schemes;
}

QString PersonalizationPage::currentSchemeId()
{
    // The active scheme id lives in kdeglobals [General] ColorScheme.
    QProcess proc;
    proc.start(QStringLiteral("kreadconfig6"),
               { QStringLiteral("--group"), QStringLiteral("General"),
                 QStringLiteral("--key"), QStringLiteral("ColorScheme") });
    if (proc.waitForFinished(2000)) {
        const QString id = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!id.isEmpty())
            return id;
    }
    return QString();
}

// Paint a miniature window preview from the scheme's own colours.
QPixmap PersonalizationPage::swatchPixmap(const Scheme &s) const
{
    const int w = 116, h = 78;
    QPixmap pm(w, h);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    // Window body.
    QRectF body(0.5, 0.5, w - 1, h - 1);
    p.setPen(QPen(QColor("#7F7F7F"), 1));
    p.setBrush(s.window);
    p.drawRoundedRect(body, 3, 3);

    // Title bar band.
    QRectF title(1, 1, w - 2, 16);
    p.setPen(Qt::NoPen);
    p.setBrush(s.titlebar);
    QPainterPath titlePath;
    titlePath.addRoundedRect(title, 3, 3);
    // Square off the bottom of the titlebar so only the top corners are round.
    titlePath.addRect(QRectF(1, 9, w - 2, 8));
    p.drawPath(titlePath.simplified());

    // Three faux window buttons.
    p.setBrush(QColor(255, 255, 255, 150));
    for (int i = 0; i < 3; ++i)
        p.drawEllipse(QPointF(w - 10 - i * 8, 9), 2.2, 2.2);

    // Content view with a highlighted "selected" row in the accent colour.
    QRectF view(9, 24, w - 18, h - 33);
    p.setPen(QPen(QColor(0, 0, 0, 40), 1));
    p.setBrush(s.view);
    p.drawRect(view);

    p.setPen(Qt::NoPen);
    p.setBrush(s.accent);
    p.drawRect(QRectF(view.left() + 4, view.top() + 6, view.width() - 8, 9));

    // A couple of faux text lines in the foreground colour.
    QColor line = s.text;
    line.setAlpha(120);
    p.setBrush(line);
    p.drawRect(QRectF(view.left() + 4, view.top() + 21, view.width() - 24, 4));
    p.drawRect(QRectF(view.left() + 4, view.top() + 29, view.width() - 36, 4));

    return pm;
}

// Sidebar
QList<SidebarLink> PersonalizationPage::sidebarLinks()
{
    return {
        Nav::to("Change your account picture", PageId::UserAccounts),
    };
}

QList<SidebarLink> PersonalizationPage::sidebarSeeAlso()
{
    return {
        Nav::to("Display", PageId::DisplaySettings),
        Nav::to("Taskbar and Start Menu", PageId::TaskbarStartMenu),
        Nav::to("Ease of Access Center", PageId::EaseOfAccess),
    };
}

// Page
PersonalizationPage::PersonalizationPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    m_schemes   = gatherSchemes();
    m_currentId = currentSchemeId();

    // Windows 7 lays the content out at a fixed width and leaves the rest of
    // the window blank on the right rather than stretching to fill it.
    auto *contentV = Win7::pageScaffold(this, sidebar, /*bottomMargin=*/20,
                                        /*fixedWidth=*/700);

    // Page title.
    contentV->addWidget(
        Win7::pageTitle("Change the visuals and sounds on your computer"));
    contentV->addSpacing(6);

    auto *intro = Win7::label(
        "Click a color scheme to change application and window colors. Use the "
        "shortcuts below for the desktop background, sounds and lock screen.",
        9, "#1A1A1A");
    intro->setWordWrap(true);
    contentV->addWidget(intro);
    contentV->addSpacing(14);

    // Section heading with a hairline rule.
    auto addHeading = [&](const QString &text) {
        contentV->addLayout(
            Win7::sectionHeading(text, nullptr, nullptr, "#000000"));
        contentV->addSpacing(10);
    };

    addHeading(QStringLiteral("Aero Themes (%1)").arg(m_schemes.size()));

    // Swatch grid: four themes per row.
    auto *grid = new QGridLayout;
    grid->setContentsMargins(6, 0, 0, 0);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(14);

    const int columns = 4;
    for (int i = 0; i < m_schemes.size(); ++i) {
        const Scheme &s = m_schemes[i];

        auto *cell = new QFrame;
        cell->setObjectName("themeCell");
        cell->setCursor(Qt::PointingHandCursor);
        auto *cellV = new QVBoxLayout(cell);
        cellV->setContentsMargins(4, 4, 4, 4);
        cellV->setSpacing(3);

        auto *swatch = new QLabel;
        swatch->setPixmap(swatchPixmap(s));
        swatch->setFixedSize(116, 78);
        swatch->setStyleSheet("background: transparent;");
        cellV->addWidget(swatch, 0, Qt::AlignHCenter);

        auto *name = new QLabel(s.name);
        {
            QFont f = name->font();
            f.setPointSize(8);
            name->setFont(f);
        }
        name->setAlignment(Qt::AlignHCenter);
        name->setWordWrap(true);
        name->setFixedWidth(120);
        name->setStyleSheet("color: #000000; background: transparent;");
        cellV->addWidget(name, 0, Qt::AlignHCenter);

        cell->installEventFilter(this);
        m_frameToIndex.insert(cell, i);
        m_frames << cell;

        grid->addWidget(cell, i / columns, i % columns, Qt::AlignTop);
    }
    contentV->addLayout(grid);
    contentV->addSpacing(20);

    // Bottom action row: the four Windows Personalization shortcuts.
    auto *actionsRow = new QHBoxLayout;
    actionsRow->setContentsMargins(6, 0, 0, 0);
    actionsRow->setSpacing(24);

    auto addAction = [&](const QString &iconName, const QString &text,
                         std::function<void()> action) {
        auto *col = new QVBoxLayout;
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(4);

        auto *icon = new QLabel;
        icon->setFixedSize(32, 32);
        icon->setPixmap(themeIcon({iconName.toUtf8().constData(),
                                    "preferences-desktop"}).pixmap(32, 32));
        icon->setStyleSheet("background: transparent;");
        col->addWidget(icon, 0, Qt::AlignHCenter);

        auto *link = new LinkLabel(text);
        link->setAlignment(Qt::AlignHCenter);
        QObject::connect(link, &LinkLabel::clicked, this, std::move(action));
        col->addWidget(link, 0, Qt::AlignHCenter);

        actionsRow->addLayout(col);
    };

    addAction("preferences-desktop-wallpaper", "Desktop\nBackground",
              [this]() { chooseWallpaper(); });
    addAction("preferences-desktop-color", "Window\nColor",
              [this]() { openWindowColor(); });
    addAction("preferences-desktop-sound", "Sounds",
              [this]() { emit soundRequested(); });
    addAction("preferences-desktop-screensaver", "Screen\nSaver",
              [this]() { configureLockScreen(); });
    actionsRow->addStretch(1);

    contentV->addLayout(actionsRow);
    contentV->addStretch(1);

    refreshHighlight();
}

void PersonalizationPage::applyScheme(int index)
{
    if (index < 0 || index >= m_schemes.size())
        return;
    const Scheme &s = m_schemes[index];
    // plasma-apply-colorscheme writes the user's own config; no polkit needed.
    QProcess::startDetached(QStringLiteral("plasma-apply-colorscheme"),
                            { s.id });
    m_currentId = s.id;
    refreshHighlight();
}

void PersonalizationPage::refreshHighlight()
{
    for (auto it = m_frameToIndex.constBegin();
         it != m_frameToIndex.constEnd(); ++it) {
        auto *frame = qobject_cast<QFrame *>(const_cast<QObject *>(it.key()));
        if (!frame)
            continue;
        const bool cur = (m_schemes[it.value()].id == m_currentId);
        frame->setStyleSheet(cur
            ? "#themeCell { background: #DCEBFB; border: 1px solid #7DA2CE;"
              " border-radius: 3px; }"
            : "#themeCell { background: transparent; border: 1px solid transparent; }");
    }
}

void PersonalizationPage::chooseWallpaper()
{
    DesktopBackgroundDialog dialog(this);
    dialog.exec();
}

void PersonalizationPage::openWindowColor()
{
    const QString loader =
        QStandardPaths::findExecutable(QStringLiteral("aeroshell-kcmloader"));
    const QString module = QStringLiteral(
        "/usr/lib/qt6/plugins/kwin/effects/configs/"
        "kwin_aeroglassblur_config.so");
    if (loader.isEmpty() || !QFileInfo::exists(module)) {
        QMessageBox::warning(
            this, QStringLiteral("Window Color and Appearance"),
            QStringLiteral("The Aero7 window-color component is not installed."));
        return;
    }
    // The AeroShell loader expects a module path and an icon name.
    QProcess::startDetached(loader,
                            {module, QStringLiteral("preferences-desktop-color")});
}

void PersonalizationPage::configureLockScreen()
{
    const QString config =
        QDir::homePath() + QStringLiteral("/.config/kscreenlockerrc");
    QSettings settings(config, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Daemon"));

    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Screen Saver and Lock"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *enabled = new QCheckBox(
        QStringLiteral("Lock the computer automatically when it is idle"));
    enabled->setChecked(
        settings.value(QStringLiteral("Autolock"), true).toBool());
    layout->addWidget(enabled);

    auto *form = new QFormLayout;
    auto *minutes = new QSpinBox;
    minutes->setRange(1, 120);
    minutes->setSuffix(QStringLiteral(" minutes"));
    minutes->setValue(
        settings.value(QStringLiteral("Timeout"), 5).toInt());
    form->addRow(QStringLiteral("Wait:"), minutes);
    layout->addLayout(form);

    auto *resume = new QCheckBox(
        QStringLiteral("Require a password after sleep or suspend"));
    resume->setChecked(
        settings.value(QStringLiteral("LockOnResume"), true).toBool());
    layout->addWidget(resume);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    if (dialog.exec() != QDialog::Accepted) {
        settings.endGroup();
        return;
    }

    settings.setValue(QStringLiteral("Autolock"), enabled->isChecked());
    settings.setValue(QStringLiteral("Timeout"), minutes->value());
    settings.setValue(QStringLiteral("LockOnResume"), resume->isChecked());
    settings.endGroup();
    settings.sync();
}

bool PersonalizationPage::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress
        || event->type() == QEvent::MouseButtonDblClick) {
        auto it = m_frameToIndex.constFind(watched);
        if (it != m_frameToIndex.constEnd()) {
            auto *me = static_cast<QMouseEvent *>(event);
            if (me->button() == Qt::LeftButton) {
                applyScheme(it.value());
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
