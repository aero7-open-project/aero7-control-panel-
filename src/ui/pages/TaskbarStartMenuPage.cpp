#include "TaskbarStartMenuPage.h"
#include "Win7Ui.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace {

QGroupBox *groupBox(const QString &title, QVBoxLayout **layout)
{
    auto *box = new QGroupBox(title);
    auto *content = new QVBoxLayout(box);
    content->setContentsMargins(12, 12, 12, 12);
    content->setSpacing(8);
    *layout = content;
    return box;
}

QString boolLiteral(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString readStateScript()
{
    return QStringLiteral(
        "var state={panel:false,tasks:false,start:false,locked:false,hiding:'none',"
        "height:40,grouping:1,onlyWhenFull:false,labels:false,previews:true,"
        "recents:true,jumpLists:true,rows:8};"
        "for(var p of panels()){if(p.type!=='io.gitgud.wackyideas.panel')continue;"
        "state.panel=true;state.locked=!!p.locked;state.hiding=String(p.hiding);"
        "state.height=Number(p.height);"
        "for(var w of p.widgets()){w.currentConfigGroup=['General'];"
        "if(w.type==='io.gitgud.wackyideas.seventasks'){"
        "state.tasks=true;state.grouping=Number(w.readConfig('groupingStrategy',1));"
        "state.onlyWhenFull=!!w.readConfig('onlyGroupWhenFull',false);"
        "state.labels=!!w.readConfig('showLabels',false);"
        "state.previews=!!w.readConfig('showPreviews',true);"
        "state.jumpLists=!w.readConfig('disableJumplists',false);"
        "}else if(w.type==='io.gitgud.wackyideas.SevenStart'){"
        "state.start=true;state.recents=!!w.readConfig('showRecentsView',true);"
        "state.rows=Number(w.readConfig('numberRows',8));}}break;}"
        "print(JSON.stringify(state));");
}

} // namespace

TaskbarStartMenuPage::TaskbarStartMenuPage(QScrollArea *sidebar,
                                           QWidget *parent)
    : QWidget(parent)
{
    auto *content = Win7::pageScaffold(this, sidebar, 18, 680);
    content->addWidget(Win7::pageTitle(
        QStringLiteral("Taskbar and Start Menu Properties")));
    content->addSpacing(8);

    auto *tabs = new QTabWidget;
    tabs->setDocumentMode(false);

    auto *taskbar = new QWidget;
    auto *taskbarLayout = new QVBoxLayout(taskbar);
    taskbarLayout->setContentsMargins(12, 14, 12, 12);
    taskbarLayout->setSpacing(10);

    QVBoxLayout *appearanceLayout = nullptr;
    auto *appearance = groupBox(QStringLiteral("Taskbar appearance"),
                                &appearanceLayout);
    m_lockTaskbar = new QCheckBox(QStringLiteral("Lock the taskbar"));
    m_autoHide = new QCheckBox(QStringLiteral("Auto-hide the taskbar"));
    m_smallIcons = new QCheckBox(QStringLiteral("Use small icons"));
    appearanceLayout->addWidget(m_lockTaskbar);
    appearanceLayout->addWidget(m_autoHide);
    appearanceLayout->addWidget(m_smallIcons);
    taskbarLayout->addWidget(appearance);

    QVBoxLayout *buttonsLayout = nullptr;
    auto *buttons = groupBox(QStringLiteral("Taskbar buttons"), &buttonsLayout);
    auto *buttonsForm = new QFormLayout;
    buttonsForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_grouping = new QComboBox;
    m_grouping->addItem(QStringLiteral("Always combine, hide labels"), 0);
    m_grouping->addItem(QStringLiteral("Combine when taskbar is full"), 1);
    m_grouping->addItem(QStringLiteral("Never combine"), 2);
    buttonsForm->addRow(QStringLiteral("Taskbar buttons:"), m_grouping);
    buttonsLayout->addLayout(buttonsForm);
    m_previews = new QCheckBox(
        QStringLiteral("Use Aero Peek to preview open windows"));
    buttonsLayout->addWidget(m_previews);
    taskbarLayout->addWidget(buttons);

    auto *notificationHint = new QLabel(
        QStringLiteral("Notification-area icon visibility is managed by the "
                       "Aero7 notification-area page."));
    notificationHint->setWordWrap(true);
    notificationHint->setStyleSheet(QStringLiteral("color: #4B4B4B;"));
    taskbarLayout->addWidget(notificationHint);
    taskbarLayout->addStretch(1);
    tabs->addTab(taskbar, QStringLiteral("Taskbar"));

    auto *startMenu = new QWidget;
    auto *startLayout = new QVBoxLayout(startMenu);
    startLayout->setContentsMargins(12, 14, 12, 12);
    startLayout->setSpacing(10);

    QVBoxLayout *privacyLayout = nullptr;
    auto *privacy = groupBox(QStringLiteral("Privacy"), &privacyLayout);
    m_recentPrograms = new QCheckBox(
        QStringLiteral("Store and display recently opened programs in the Start menu"));
    m_jumpLists = new QCheckBox(
        QStringLiteral("Store and display recently opened items in the Start menu and taskbar"));
    privacyLayout->addWidget(m_recentPrograms);
    privacyLayout->addWidget(m_jumpLists);
    startLayout->addWidget(privacy);

    QVBoxLayout *sizeLayout = nullptr;
    auto *size = groupBox(QStringLiteral("Start menu size"), &sizeLayout);
    auto *sizeForm = new QFormLayout;
    m_programCount = new QSpinBox;
    m_programCount->setRange(4, 16);
    sizeForm->addRow(QStringLiteral("Number of recent programs to display:"),
                     m_programCount);
    sizeLayout->addLayout(sizeForm);
    startLayout->addWidget(size);
    startLayout->addStretch(1);
    tabs->addTab(startMenu, QStringLiteral("Start Menu"));

    if (qEnvironmentVariable("AERO7_TASKBAR_PROPERTIES_TAB")
            .compare(QStringLiteral("start-menu"), Qt::CaseInsensitive) == 0) {
        tabs->setCurrentIndex(1);
    }

    content->addWidget(tabs, 1);

    auto *footer = new QHBoxLayout;
    m_status = new QLabel;
    m_status->setObjectName(QStringLiteral("taskbarStatus"));
    m_status->setStyleSheet(QStringLiteral("color: #4B4B4B;"));
    footer->addWidget(m_status, 1);
    auto *apply = new QPushButton(QStringLiteral("Apply"));
    apply->setObjectName(QStringLiteral("taskbarApply"));
    connect(apply, &QPushButton::clicked, this,
            &TaskbarStartMenuPage::applyState);
    footer->addWidget(apply);
    content->addLayout(footer);

    QTimer::singleShot(0, this, &TaskbarStartMenuPage::loadState);
}

bool TaskbarStartMenuPage::evaluateScript(const QString &script,
                                          QString *output) const
{
    QProcess process;
    process.start(QStringLiteral("qdbus6"),
                  {QStringLiteral("org.kde.plasmashell"),
                   QStringLiteral("/PlasmaShell"),
                   QStringLiteral("org.kde.PlasmaShell.evaluateScript"),
                   script});
    if (!process.waitForStarted(2000) || !process.waitForFinished(5000)
        || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        return false;
    }
    if (output)
        *output = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    return true;
}

void TaskbarStartMenuPage::loadState()
{
    QString output;
    if (!evaluateScript(readStateScript(), &output)) {
        m_status->setText(QStringLiteral("AeroShell is not available."));
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(output.toUtf8());
    if (!document.isObject()) {
        m_status->setText(QStringLiteral("Could not read AeroShell settings."));
        return;
    }
    const QJsonObject state = document.object();
    if (!state.value(QStringLiteral("panel")).toBool()
        || !state.value(QStringLiteral("tasks")).toBool()
        || !state.value(QStringLiteral("start")).toBool()) {
        m_status->setText(QStringLiteral(
            "The Aero7 taskbar or Start menu component is not available."));
        return;
    }
    m_lockTaskbar->setChecked(state.value(QStringLiteral("locked")).toBool());
    m_autoHide->setChecked(
        state.value(QStringLiteral("hiding")).toString() == QStringLiteral("autohide"));
    m_smallIcons->setChecked(state.value(QStringLiteral("height")).toInt(40) <= 30);
    m_previews->setChecked(state.value(QStringLiteral("previews")).toBool(true));
    m_recentPrograms->setChecked(state.value(QStringLiteral("recents")).toBool(true));
    m_jumpLists->setChecked(state.value(QStringLiteral("jumpLists")).toBool(true));
    m_programCount->setValue(state.value(QStringLiteral("rows")).toInt(8));

    const int grouping = state.value(QStringLiteral("grouping")).toInt(1);
    const bool onlyWhenFull =
        state.value(QStringLiteral("onlyWhenFull")).toBool(false);
    m_grouping->setCurrentIndex(grouping == 0 ? 2 : (onlyWhenFull ? 1 : 0));
    m_status->setText(QStringLiteral("Settings loaded from AeroShell."));
}

void TaskbarStartMenuPage::applyState()
{
    const int choice = m_grouping->currentIndex();
    const int grouping = choice == 2 ? 0 : 1;
    const bool onlyWhenFull = choice == 1;
    const bool labels = choice != 0;

    const QString script = QStringLiteral(
        "var panel=null,tasks=null,start=null;"
        "for(var p of panels()){if(p.type!=='io.gitgud.wackyideas.panel')continue;"
        "panel=p;for(var w of p.widgets()){"
        "if(w.type==='io.gitgud.wackyideas.seventasks')tasks=w;"
        "else if(w.type==='io.gitgud.wackyideas.SevenStart')start=w;}break;}"
        "if(panel&&tasks&&start){"
        "panel.locked=%1;panel.hiding='%2';panel.height=%3;"
        "tasks.currentConfigGroup=['General'];"
        "tasks.writeConfig('groupingStrategy',%4);"
        "tasks.writeConfig('onlyGroupWhenFull',%5);"
        "tasks.writeConfig('showLabels',%6);"
        "tasks.writeConfig('showPreviews',%7);"
        "tasks.writeConfig('disableJumplists',%8);"
        "start.currentConfigGroup=['General'];"
        "start.writeConfig('showRecentsView',%9);"
        "start.writeConfig('numberRows',%10);}"
        "print(JSON.stringify({panel:!!panel,tasks:!!tasks,start:!!start}));")
        .arg(boolLiteral(m_lockTaskbar->isChecked()),
             m_autoHide->isChecked() ? QStringLiteral("autohide")
                                     : QStringLiteral("none"))
        .arg(m_smallIcons->isChecked() ? 30 : 40)
        .arg(grouping)
        .arg(boolLiteral(onlyWhenFull), boolLiteral(labels),
             boolLiteral(m_previews->isChecked()),
             boolLiteral(!m_jumpLists->isChecked()),
             boolLiteral(m_recentPrograms->isChecked()))
        .arg(m_programCount->value());

    QString result;
    if (!evaluateScript(script, &result)) {
        m_status->setText(QStringLiteral("Could not apply AeroShell settings."));
        return;
    }
    const QJsonDocument applied = QJsonDocument::fromJson(result.toUtf8());
    if (!applied.isObject()
        || !applied.object().value(QStringLiteral("panel")).toBool()
        || !applied.object().value(QStringLiteral("tasks")).toBool()
        || !applied.object().value(QStringLiteral("start")).toBool()) {
        m_status->setText(QStringLiteral(
            "The Aero7 taskbar or Start menu component is missing; nothing was applied."));
        return;
    }

    QString readback;
    if (!evaluateScript(readStateScript(), &readback)) {
        m_status->setText(QStringLiteral("AeroShell did not confirm the saved settings."));
        return;
    }
    const QJsonDocument verified = QJsonDocument::fromJson(readback.toUtf8());
    const QJsonObject actual = verified.object();
    if (!verified.isObject()
        || !actual.value(QStringLiteral("panel")).toBool()
        || !actual.value(QStringLiteral("tasks")).toBool()
        || !actual.value(QStringLiteral("start")).toBool()
        || actual.value(QStringLiteral("locked")).toBool() != m_lockTaskbar->isChecked()
        || actual.value(QStringLiteral("hiding")).toString() !=
           (m_autoHide->isChecked() ? QStringLiteral("autohide") : QStringLiteral("none"))
        || actual.value(QStringLiteral("height")).toInt() != (m_smallIcons->isChecked() ? 30 : 40)
        || actual.value(QStringLiteral("grouping")).toInt() != grouping
        || actual.value(QStringLiteral("onlyWhenFull")).toBool() != onlyWhenFull
        || actual.value(QStringLiteral("labels")).toBool() != labels
        || actual.value(QStringLiteral("previews")).toBool() != m_previews->isChecked()
        || actual.value(QStringLiteral("recents")).toBool() != m_recentPrograms->isChecked()
        || actual.value(QStringLiteral("jumpLists")).toBool() != m_jumpLists->isChecked()
        || actual.value(QStringLiteral("rows")).toInt() != m_programCount->value()) {
        m_status->setText(QStringLiteral("AeroShell did not retain every requested setting."));
        return;
    }
    m_status->setText(QStringLiteral("Taskbar and Start menu settings applied."));
}
