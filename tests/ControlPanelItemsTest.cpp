#include "ControlPanelItems.h"
#include "PageRegistry.h"

#include <QCoreApplication>
#include <QSet>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const auto &items = controlPanelItems();
    if (items.size() != 45)
        return 1;

    QSet<QString> names;
    QString previous;
    for (const ControlPanelItem &item : items) {
        if (item.windowsName.isEmpty() || item.kdeName.isEmpty()
            || item.iconName.isEmpty()
            || item.target.kind == LinkTarget::None
            || names.contains(item.windowsName))
            return 2;
        if (!previous.isEmpty()
            && QString::localeAwareCompare(previous, item.windowsName) >= 0)
            return 3;
        names.insert(item.windowsName);
        previous = item.windowsName;

        switch (item.target.kind) {
        case LinkTarget::Page:
            if (PageRegistry::pathFor(item.target.page).isEmpty())
                return 5;
            break;
        case LinkTarget::Applet:
            if (item.target.applet.isEmpty())
                return 6;
            break;
        case LinkTarget::Command:
            if (item.target.command.isEmpty()
                || item.target.command.first().isEmpty())
                return 7;
            break;
        default:
            return 8;
        }
    }
    if (items.first().windowsName != QStringLiteral("Action Center")
        || items.last().windowsName != QStringLiteral("Windows Update")
        || items.last().kdeName != QStringLiteral("Software Update"))
        return 4;
    return 0;
}
