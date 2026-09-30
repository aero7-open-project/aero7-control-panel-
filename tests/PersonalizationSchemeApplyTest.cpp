#include "PersonalizationPage.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFrame>
#include <QMessageBox>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QTemporaryDir sandbox;
    if (!sandbox.isValid())
        return 1;
    QFile schemeReader(sandbox.filePath(QStringLiteral("kreadconfig6")));
    if (!schemeReader.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || schemeReader.write("#!/bin/sh\nexit 0\n") < 0)
        return 2;
    schemeReader.close();
    if (!schemeReader.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                     | QFileDevice::ExeOwner))
        return 3;
    QFile tool(sandbox.filePath(QStringLiteral("plasma-apply-colorscheme")));
    if (!tool.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || tool.write("#!/bin/sh\nexit 7\n") < 0)
        return 4;
    tool.close();
    if (!tool.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                             | QFileDevice::ExeOwner))
        return 5;

    const QByteArray originalPath = qgetenv("PATH");
    qputenv("PATH", sandbox.path().toUtf8());
    auto *sidebar = new QScrollArea;
    PersonalizationPage page(sidebar);
    page.show();

    auto *swatch = page.findChild<QFrame *>(QStringLiteral("themeCell"));
    if (!swatch)
        return 6;
    const QString initialStyle = swatch->styleSheet();
    bool warningSeen = false;
    QTimer dismiss;
    QObject::connect(&dismiss, &QTimer::timeout, &app, [&] {
        for (QWidget *window : QApplication::topLevelWidgets()) {
            auto *message = qobject_cast<QMessageBox *>(window);
            if (!message || !message->isVisible())
                continue;
            warningSeen = message->text().contains(
                QStringLiteral("theme could not be applied"));
            message->accept();
        }
    });
    dismiss.start(10);
    QTest::mouseClick(swatch, Qt::LeftButton);
    QElapsedTimer elapsed;
    elapsed.start();
    while (!warningSeen && elapsed.elapsed() < 3000)
        app.processEvents(QEventLoop::AllEvents, 50);
    qputenv("PATH", originalPath);
    return warningSeen && swatch->styleSheet() == initialStyle ? 0 : 7;
}
