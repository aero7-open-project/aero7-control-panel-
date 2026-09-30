#include "NativeSettingsDialog.h"
#include "Win7Ui.h"
#include "Branding.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QFile>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace {
using Kind = NativeSettingsDialog::Kind;
using Option = NativeSettingsDialog::Option;

Option boolean(const char *label, const char *file, const char *group,
               const char *key, bool value)
{
    return {QString::fromUtf8(label), QString::fromLatin1(file),
            QString::fromLatin1(group), QString::fromLatin1(key),
            Kind::Boolean, value, {}, nullptr};
}

Option integer(const char *label, const char *file, const char *group,
               const char *key, int value)
{
    return {QString::fromUtf8(label), QString::fromLatin1(file),
            QString::fromLatin1(group), QString::fromLatin1(key),
            Kind::Integer, value, {}, nullptr};
}

Option text(const char *label, const char *file, const char *group,
            const char *key, const char *value = "")
{
    return {QString::fromUtf8(label), QString::fromLatin1(file),
            QString::fromLatin1(group), QString::fromLatin1(key),
            Kind::Text, QString::fromUtf8(value), {}, nullptr};
}

Option choice(const char *label, const char *file, const char *group,
              const char *key, const char *value, QStringList choices)
{
    return {QString::fromUtf8(label), QString::fromLatin1(file),
            QString::fromLatin1(group), QString::fromLatin1(key),
            Kind::Choice, QString::fromLatin1(value), std::move(choices), nullptr};
}

QString readUserDirectory(const QString &key, const QString &fallback)
{
    QFile file(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
               + QStringLiteral("/user-dirs.dirs"));
    if (!file.open(QIODevice::ReadOnly))
        return fallback;
    const QString prefix = key + QLatin1Char('=');
    for (const QString &line : QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'))) {
        if (!line.startsWith(prefix))
            continue;
        QString value = line.mid(prefix.size()).trimmed();
        if (value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
            value = value.mid(1, value.size() - 2);
        value.replace(QStringLiteral("$HOME"), QDir::homePath());
        return value;
    }
    return fallback;
}
}

NativeSettingsDialog::NativeSettingsDialog(const SettingDefinition &setting,
                                           QWidget *parent)
    : QDialog(parent), m_setting(setting), m_options(optionsFor(setting.key))
{
    const QString displayName = Branding::useWindowsNames()
        ? setting.aeroName : setting.kdeName;
    setWindowTitle(displayName);
    setModal(true);
    resize(520, 300);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 14);
    layout->setSpacing(10);
    layout->addWidget(Win7::pageTitle(displayName, 13, "#1A5DAB"));

    auto *description = Win7::label(setting.description, 9, "#333333");
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setHorizontalSpacing(14);
    form->setVerticalSpacing(10);

    for (Option &option : m_options) {
        QVariant current = option.defaultValue;
        if (option.file == QLatin1String("@network-manager")) {
            QProcess process;
            process.start(QStringLiteral("nmcli"), {QStringLiteral("networking")});
            if (process.waitForFinished(1500) && process.exitCode() == 0)
                current = QString::fromUtf8(process.readAllStandardOutput()).trimmed()
                              .compare(QStringLiteral("enabled"), Qt::CaseInsensitive) == 0;
        } else if (option.file == QLatin1String("@user-dirs")) {
            current = readUserDirectory(option.key, option.defaultValue.toString());
        } else {
            QSettings settings(configPath(option.file), QSettings::IniFormat);
            settings.beginGroup(option.group);
            current = settings.value(option.key, option.defaultValue);
            settings.endGroup();
        }

        switch (option.kind) {
        case Kind::Boolean: {
            auto *box = new QCheckBox(option.label);
            box->setChecked(current.toBool());
            option.editor = box;
            form->addRow(QString(), box);
            break;
        }
        case Kind::Integer: {
            auto *spin = new QSpinBox;
            spin->setRange(0, 100000);
            spin->setValue(current.toInt());
            option.editor = spin;
            form->addRow(option.label, spin);
            break;
        }
        case Kind::Text: {
            auto *edit = new QLineEdit(current.toString());
            option.editor = edit;
            form->addRow(option.label, edit);
            break;
        }
        case Kind::Choice: {
            auto *combo = new QComboBox;
            for (const QString &encoded : option.choices) {
                const qsizetype split = encoded.indexOf(QLatin1Char('|'));
                combo->addItem(split < 0 ? encoded : encoded.left(split),
                               split < 0 ? encoded : encoded.mid(split + 1));
            }
            const int index = combo->findData(current.toString());
            combo->setCurrentIndex(index >= 0 ? index : 0);
            option.editor = combo;
            form->addRow(option.label, combo);
            break;
        }
        }
    }
    layout->addLayout(form);

    m_status = Win7::label(
        QStringLiteral("Changes are applied through the desktop's existing backend; no external settings window is opened."),
        8, "#555555");
    if (m_options.isEmpty()) {
        m_status->setText(QStringLiteral("This setting has no supported Aero7 editor yet."));
        m_status->setStyleSheet(QStringLiteral("color: #A00000;"));
    }
    m_status->setWordWrap(true);
    layout->addWidget(m_status);
    layout->addStretch(1);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                         | QDialogButtonBox::Cancel
                                         | QDialogButtonBox::Apply);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (apply())
            accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, [this]() { apply(); });
    buttons->button(QDialogButtonBox::Ok)->setEnabled(!m_options.isEmpty());
    buttons->button(QDialogButtonBox::Apply)->setEnabled(!m_options.isEmpty());
    layout->addWidget(buttons);
}

QString NativeSettingsDialog::configPath(const QString &file)
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir().mkpath(root);
    return QDir(root).filePath(file);
}

QVariant NativeSettingsDialog::editorValue(const Option &option) const
{
    switch (option.kind) {
    case Kind::Boolean:
        return qobject_cast<QCheckBox *>(option.editor)->isChecked();
    case Kind::Integer:
        return qobject_cast<QSpinBox *>(option.editor)->value();
    case Kind::Text:
        return qobject_cast<QLineEdit *>(option.editor)->text().trimmed();
    case Kind::Choice:
        return qobject_cast<QComboBox *>(option.editor)->currentData();
    }
    return {};
}

bool NativeSettingsDialog::apply()
{
    if (m_options.isEmpty())
        return false;
    bool applied = true;
    QHash<QString, QString> userDirectories;
    for (const Option &option : std::as_const(m_options)) {
        if (option.file == QLatin1String("@network-manager")) {
            const bool enabled = editorValue(option).toBool();
            applied &= QProcess::execute(QStringLiteral("nmcli"),
                                         {QStringLiteral("networking"),
                                          enabled ? QStringLiteral("on") : QStringLiteral("off")}) == 0;
            continue;
        }
        if (option.file == QLatin1String("@user-dirs")) {
            userDirectories.insert(option.key, editorValue(option).toString());
            continue;
        }
        QSettings settings(configPath(option.file), QSettings::IniFormat);
        settings.beginGroup(option.group);
        settings.setValue(option.key, editorValue(option));
        settings.endGroup();
        settings.sync();
        applied &= settings.status() == QSettings::NoError;
    }
    if (!userDirectories.isEmpty()) {
        const QString path = configPath(QStringLiteral("user-dirs.dirs"));
        QFile existing(path);
        QStringList lines;
        if (existing.open(QIODevice::ReadOnly))
            lines = QString::fromUtf8(existing.readAll()).split(QLatin1Char('\n'));
        for (auto iterator = userDirectories.cbegin(); iterator != userDirectories.cend(); ++iterator) {
            QString value = QDir::cleanPath(iterator.value());
            if (value.startsWith(QDir::homePath()))
                value.replace(0, QDir::homePath().size(), QStringLiteral("$HOME"));
            const QString replacement = QStringLiteral("%1=\"%2\"").arg(iterator.key(), value);
            bool replaced = false;
            for (QString &line : lines) {
                if (line.startsWith(iterator.key() + QLatin1Char('='))) {
                    line = replacement;
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                lines.append(replacement);
        }
        QSaveFile output(path);
        if (!output.open(QIODevice::WriteOnly)
            || output.write(lines.join(QLatin1Char('\n')).toUtf8()) < 0
            || !output.commit()) {
            applied = false;
        } else {
            QProcess::startDetached(QStringLiteral("xdg-user-dirs-update"), {});
        }
    }
    reloadBackends();
    m_status->setText(applied ? QStringLiteral("Settings applied.")
                              : QStringLiteral("Some settings could not be applied. Check that the required service is running and that you have permission."));
    m_status->setStyleSheet(applied ? QStringLiteral("color: #1B6E1B;")
                                    : QStringLiteral("color: #A00000;"));
    return applied;
}

void NativeSettingsDialog::reloadBackends() const
{
    const bool kwin = std::any_of(m_options.cbegin(), m_options.cend(),
                                  [](const Option &o) { return o.file == QLatin1String("kwinrc"); });
    if (kwin) {
        QProcess::startDetached(QStringLiteral("qdbus6"),
                                {QStringLiteral("org.kde.KWin"),
                                 QStringLiteral("/KWin"),
                                 QStringLiteral("reconfigure")});
    }
}

QVector<NativeSettingsDialog::Option>
NativeSettingsDialog::optionsFor(const QString &key)
{
    if (key == QLatin1String("night-light"))
        return {boolean("Turn on Night Light", "kwinrc", "NightColor", "Active", false),
                integer("Night color temperature:", "kwinrc", "NightColor", "NightTemperature", 4500)};
    if (key == QLatin1String("day-night"))
        return {choice("Schedule:", "kwinrc", "NightColor", "Mode", "Automatic",
                       {"Automatic|Automatic", "Sunset to sunrise|Location", "Custom times|Times"}),
                text("Evening begins:", "kwinrc", "NightColor", "EveningBeginFixed", "21:00"),
                text("Morning begins:", "kwinrc", "NightColor", "MorningBeginFixed", "07:00")};
    if (key == QLatin1String("window-decoration"))
        return {choice("Border size:", "kwinrc", "org.kde.kdecoration2", "BorderSize", "Normal",
                       {"No border|None", "Small|Tiny", "Normal|Normal", "Large|Large"})};
    if (key == QLatin1String("window-behavior"))
        return {choice("Click behavior:", "kwinrc", "Windows", "FocusPolicy", "ClickToFocus",
                       {"Click to focus|ClickToFocus", "Focus follows mouse|FocusFollowsMouse", "Focus under mouse|FocusUnderMouse"}),
                boolean("Raise focused windows automatically", "kwinrc", "Windows", "AutoRaise", false)};
    if (key == QLatin1String("window-rules"))
        return {text("Program identifier:", "kwinrulesrc", "1", "wmclass"),
                choice("Identifier match:", "kwinrulesrc", "1", "wmclassmatch", "1",
                       {"Exact|1", "Substring|2", "Regular expression|3"})};
    if (key == QLatin1String("task-switcher"))
        return {boolean("Highlight the selected window", "kwinrc", "TabBox", "HighlightWindows", true),
                boolean("Show desktop entry", "kwinrc", "TabBox", "ShowDesktopMode", true)};
    if (key == QLatin1String("effects"))
        return {boolean("Enable Aero blur", "kwinrc", "Plugins", "blurEnabled", true),
                boolean("Enable background contrast", "kwinrc", "Plugins", "contrastEnabled", true)};
    if (key == QLatin1String("animations"))
        return {choice("Animation speed:", "kdeglobals", "KDE", "AnimationDurationFactor", "1",
                       {"Very fast|0.25", "Fast|0.5", "Normal|1", "Slow|1.5", "Very slow|2"})};
    if (key == QLatin1String("screen-edges"))
        return {integer("Edge activation delay (milliseconds):", "kwinrc", "Windows", "ElectricBorderDelay", 150),
                boolean("Allow pointer actions at screen edges", "kwinrc", "Windows", "ElectricBorders", true)};
    if (key == QLatin1String("virtual-desktops"))
        return {integer("Number of desktops:", "kwinrc", "Desktops", "Number", 1),
                integer("Desktop rows:", "kwinrc", "Desktops", "Rows", 1)};
    if (key == QLatin1String("activities"))
        return {integer("Stop unused activities after (minutes):", "kactivitymanagerdrc", "activities", "stopAfterInactivity", 0)};
    if (key == QLatin1String("kwin-scripts"))
        return {boolean("Enable Aero7 window-management scripts", "kwinrc", "Plugins", "aero7Enabled", true)};
    if (key == QLatin1String("x11-support"))
        return {choice("Keys available to legacy applications:", "kwinrc", "Xwayland", "Eavesdrops", "None",
                       {"Only while focused|None", "With modifier keys|Modifiers", "All keys|All"})};
    if (key == QLatin1String("mouse"))
        return {integer("Pointer acceleration:", "kcminputrc", "Mouse", "XLbInptPointerAcceleration", 0),
                boolean("Reverse scrolling direction", "kcminputrc", "Mouse", "ReverseScrollPolarity", false),
                integer("Double-click speed (milliseconds):", "kdeglobals", "KDE", "DoubleClickInterval", 400)};
    if (key == QLatin1String("keyboard"))
        return {boolean("Configure keyboard layouts", "kxkbrc", "Layout", "Use", false),
                text("Layout codes:", "kxkbrc", "Layout", "LayoutList", "us")};
    if (key == QLatin1String("touchpad"))
        return {boolean("Enable touchpad", "kcminputrc", "Touchpad", "Enabled", true),
                boolean("Tap to click", "kcminputrc", "Touchpad", "TapToClick", true),
                boolean("Natural scrolling", "kcminputrc", "Touchpad", "NaturalScroll", false)};
    if (key == QLatin1String("touchscreen"))
        return {boolean("Enable touchscreen input", "kcminputrc", "Touchscreen", "Enabled", true),
                text("Mapped display connector:", "kcminputrc", "Touchscreen", "OutputName")};
    if (key == QLatin1String("touchscreen-gestures"))
        return {boolean("Enable edge gestures", "kwinrc", "Touchscreen", "EdgeGestures", true)};
    if (key == QLatin1String("tablet"))
        return {boolean("Enable pen input", "kcminputrc", "Tablet", "Enabled", true),
                text("Mapped display connector:", "kcminputrc", "Tablet", "OutputName")};
    if (key == QLatin1String("game-controller"))
        return {boolean("Enable game-controller input", "gamecontrollerrc", "General", "Enabled", true),
                integer("Dead zone (percent):", "gamecontrollerrc", "General", "DeadZone", 10)};
    if (key == QLatin1String("virtual-keyboard"))
        return {boolean("Enable on-screen keyboard", "kwinrc", "Wayland", "VirtualKeyboardEnabled", false),
                text("Input method service:", "kwinrc", "Wayland", "InputMethod")};
    if (key == QLatin1String("sound-theme"))
        return {text("Sound theme name:", "kdeglobals", "Sounds", "Theme", "ocean")};
    if (key == QLatin1String("network-connections"))
        return {boolean("Enable networking", "@network-manager", "", "networking-enabled", true)};
    if (key == QLatin1String("proxy"))
        return {choice("Proxy mode:", "kioslaverc", "Proxy Settings", "ProxyType", "0",
                       {"No proxy|0", "Manual proxy|1", "Use environment settings|4"}),
                text("HTTP proxy:", "kioslaverc", "Proxy Settings", "httpProxy")};
    if (key == QLatin1String("network-preferences"))
        return {integer("Connection timeout (seconds):", "kioslaverc", "HTTP", "ConnectTimeout", 20),
                integer("Response timeout (seconds):", "kioslaverc", "HTTP", "ResponseTimeout", 600)};
    if (key == QLatin1String("power-details") || key == QLatin1String("mobile-power"))
        return {integer("Turn off display after (seconds; 0 means never):", "powermanagementprofilesrc", "AC/DPMSControl", "idleTime", 600)};
    if (key == QLatin1String("online-accounts"))
        return {boolean("Allow applications to use connected accounts", "accounts.conf", "General", "Enabled", true)};
    if (key == QLatin1String("region-language"))
        return {text("Language and locale:", "plasma-localerc", "Formats", "LANG", "en_US.UTF-8"),
                text("Region:", "plasma-localerc", "Formats", "LC_MEASUREMENT")};
    if (key == QLatin1String("spell-check"))
        return {boolean("Check spelling as you type", "kdeglobals", "Sonnet", "checkerEnabledByDefault", true),
                text("Default language:", "kdeglobals", "Sonnet", "defaultLanguage", "en_US")};
    if (key == QLatin1String("default-apps"))
        return {text("Web browser desktop ID:", "mimeapps.list", "Default Applications", "x-scheme-handler/http"),
                text("Mail desktop ID:", "mimeapps.list", "Default Applications", "x-scheme-handler/mailto")};
    if (key == QLatin1String("file-associations"))
        return {text("Plain-text application desktop ID:", "mimeapps.list", "Default Applications", "text/plain"),
                text("Folder application desktop ID:", "mimeapps.list", "Default Applications", "inode/directory", "org.kde.dolphin.desktop")};
    if (key == QLatin1String("locations"))
        return {text("Documents folder:", "@user-dirs", "", "XDG_DOCUMENTS_DIR", QDir::home().filePath(QStringLiteral("Documents")).toUtf8().constData()),
                text("Downloads folder:", "@user-dirs", "", "XDG_DOWNLOAD_DIR", QDir::home().filePath(QStringLiteral("Downloads")).toUtf8().constData())};
    if (key == QLatin1String("device-actions"))
        return {boolean("Show choices when removable media is inserted", "kded5rc", "Module-device_automounter", "autoload", true)};
    if (key == QLatin1String("recent-files"))
        return {boolean("Keep a recent-items history", "kactivitymanagerdrc", "Plugin-org.kde.ActivityManager.Resources.Scoring", "useRecentFiles", true),
                integer("Maximum recent items:", "kactivitymanagerdrc", "Plugin-org.kde.ActivityManager.Resources.Scoring", "recentItemsCount", 20)};
    if (key == QLatin1String("web-shortcuts"))
        return {boolean("Enable search keywords", "kuriikwsfilterrc", "General", "EnableWebShortcuts", true),
                text("Preferred search providers:", "kuriikwsfilterrc", "General", "PreferredWebShortcuts")};
    if (key == QLatin1String("accessibility-details"))
        return {boolean("Use system bell", "kaccessrc", "Bell", "SystemBell", true),
                boolean("Use visible bell", "kaccessrc", "Bell", "VisibleBell", false),
                boolean("Enable sticky keys", "kaccessrc", "Keyboard", "StickyKeys", false)};
    if (key == QLatin1String("autostart"))
        return {boolean("Run personal startup entries", "autostartrc", "General", "Enabled", true)};
    if (key == QLatin1String("session"))
        return {choice("When signing in:", "ksmserverrc", "General", "loginMode", "emptySession",
                       {"Start with an empty desktop|emptySession", "Restore the previous session|restorePreviousLogout", "Restore manually saved session|restoreSavedSession"})};
    if (key == QLatin1String("screen-lock"))
        return {boolean("Lock the screen automatically", "kscreenlockerrc", "Daemon", "Autolock", true),
                integer("Lock after (minutes):", "kscreenlockerrc", "Daemon", "Timeout", 5)};
    if (key == QLatin1String("background-services"))
        return {boolean("Run desktop background services", "kded5rc", "General", "EnableAutoload", true)};
    if (key == QLatin1String("feedback"))
        return {choice("Diagnostic data:", "kdeglobals", "KDE", "UserFeedbackTelemetryMode", "0",
                       {"Do not send|0", "Basic system information|1", "Detailed information|2"})};
    if (key == QLatin1String("automount"))
        return {boolean("Mount removable media automatically", "kded_device_automounterrc", "General", "AutomountEnabled", true),
                boolean("Mount removable media at sign-in", "kded_device_automounterrc", "General", "AutomountOnLogin", false)};
    if (key == QLatin1String("renderer"))
        return {choice("Desktop renderer:", "kdeglobals", "QtQuickRendererSettings", "Renderer", "auto",
                       {"Automatic|auto", "OpenGL|opengl", "Software|software"})};

    return {};
}
