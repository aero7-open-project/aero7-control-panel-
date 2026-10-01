#include "UserAccountsPage.h"
#include "KdeSettingsBridge.h"
#include "LinkLabel.h"
#include "IconHelper.h"
#include "Win7Ui.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QStandardPaths>

#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <vector>
#include <functional>
#include <utility>

namespace {

constexpr auto kUsersModule = "kcm_users";

bool userIsAdministrator(const passwd *pw)
{
    if (!pw)
        return false;
    int ngroups = 0;
    getgrouplist(pw->pw_name, pw->pw_gid, nullptr, &ngroups);
    if (ngroups <= 0)
        return false;
    std::vector<gid_t> gids(ngroups);
    if (getgrouplist(pw->pw_name, pw->pw_gid, gids.data(), &ngroups) == -1)
        return false;
    for (gid_t gid : gids) {
        if (const group *gr = getgrgid(gid)) {
            const QString name = QString::fromLocal8Bit(gr->gr_name);
            if (name == QLatin1String("wheel") || name == QLatin1String("sudo"))
                return true;
        }
    }
    return false;
}

QPixmap avatarPixmap(const QString &path, int size)
{
    QPixmap src;
    if (!path.isEmpty())
        src.load(path);
    if (src.isNull())
        src = themeIcon({"user-identity", "avatar-default",
                         "system-users"}).pixmap(size, size);

    QPixmap out(size, size);
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    QPainterPath clip;
    clip.addRoundedRect(QRectF(0, 0, size, size), 6, 6);
    p.setClipPath(clip);
    const QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding,
                                      Qt::SmoothTransformation);
    p.drawPixmap((size - scaled.width()) / 2, (size - scaled.height()) / 2, scaled);

    p.setClipping(false);
    p.setPen(QPen(QColor("#9DA7B5"), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(0.5, 0.5, size - 1, size - 1), 6, 6);
    return out;
}

} // namespace

UserAccountsPage::Account UserAccountsPage::gatherAccount()
{
    Account a;

    const uid_t uid = getuid();
    const passwd *pw = getpwuid(uid);
    if (pw) {
        a.userName = QString::fromLocal8Bit(pw->pw_name);
        a.fullName = QString::fromLocal8Bit(pw->pw_gecos)
                         .section(QLatin1Char(','), 0, 0);
        if (userIsAdministrator(pw))
            a.accountType = QStringLiteral("Administrator");
    }
    if (a.userName.isEmpty())
        a.userName = QString::fromLocal8Bit(qgetenv("USER"));
    if (a.fullName.isEmpty())
        a.fullName = a.userName;
    if (a.accountType.isEmpty())
        a.accountType = QStringLiteral("Standard user");

    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    const QStringList candidates = {
        home + QStringLiteral("/.face.icon"),
        home + QStringLiteral("/.face"),
        QStringLiteral("/var/lib/AccountsService/icons/") + a.userName,
    };
    for (const QString &path : candidates) {
        if (QFile::exists(path)) {
            a.picturePath = path;
            break;
        }
    }
    return a;
}

QList<SidebarLink> UserAccountsPage::sidebarLinks()
{
    const SidebarLink advanced = KdeSettingsBridge::moduleAvailable(
        QString::fromLatin1(kUsersModule))
        ? Nav::command("Advanced account settings",
                       {"kcmshell6", "--caption", "User Accounts", kUsersModule})
        : Nav::disabled("Advanced account settings");
    return {
        advanced,
        Nav::to("Review administrator approval", PageId::SecurityMaintenance),
    };
}

QList<SidebarLink> UserAccountsPage::sidebarSeeAlso()
{
    return { Nav::disabled("Parental Controls") };
}

UserAccountsPage::UserAccountsPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    const Account acct = gatherAccount();
    auto *contentV = Win7::pageScaffold(this, sidebar, /*bottomMargin=*/20,
                                        /*fixedWidth=*/700);

    contentV->addWidget(Win7::pageTitle("Make changes to your user account"));
    contentV->addSpacing(12);
    contentV->addWidget(Win7::bodyLabel(
        "Account changes currently open the KDE Users settings module."));
    auto *accountWarning = Win7::bodyLabel(
        "After adding an account, check that it appears in the account list and that "
        "its password works before signing out. If administrator approval takes "
        "too long, the editor can time out while account creation still finishes. "
        "Use Change Password on the new account if needed; do not assume a timeout "
        "means nothing changed.");
    accountWarning->setObjectName(QStringLiteral("accountCreationWarning"));
    contentV->addWidget(accountWarning);
    contentV->addSpacing(18);

    auto *body = new QHBoxLayout;
    body->setContentsMargins(6, 0, 0, 0);
    body->setSpacing(24);

    auto *tasks = new QVBoxLayout;
    tasks->setContentsMargins(0, 0, 0, 0);
    tasks->setSpacing(12);
    const bool usersModuleAvailable = KdeSettingsBridge::moduleAvailable(
        QString::fromLatin1(kUsersModule));
    auto addTask = [&](const QString &text, std::function<void()> action,
                       bool needsUsersModule = false) {
        auto *link = new LinkLabel(text);
        if (needsUsersModule && !usersModuleAvailable) {
            link->setEnabled(false);
            link->setCursor(Qt::ArrowCursor);
            link->setStyleSheet("color: #888888; background: transparent;");
            link->setToolTip("The KDE Users settings module is not installed.");
        } else {
            QObject::connect(link, &LinkLabel::clicked, this, std::move(action));
        }
        tasks->addWidget(link, 0, Qt::AlignLeft);
    };
    auto openUsers = [this]() {
        KdeSettingsBridge::open(this, QString::fromLatin1(kUsersModule),
                                QStringLiteral("User Accounts"));
    };
    addTask("Change your password", openUsers, true);
    addTask("Change your picture", openUsers, true);
    addTask("Change your account name", openUsers, true);
    addTask("Change your account type", openUsers, true);
    addTask("Manage another account", openUsers, true);
    addTask("Review administrator approval", [this]() {
        emit navigateRequested(PageId::SecurityMaintenance);
    });

    tasks->addStretch(1);
    body->addLayout(tasks, 0);

    auto *card = new QHBoxLayout;
    card->setContentsMargins(0, 0, 0, 0);
    card->setSpacing(14);
    auto *avatar = new QLabel;
    avatar->setFixedSize(96, 96);
    avatar->setPixmap(avatarPixmap(acct.picturePath, 96));
    avatar->setStyleSheet("background: transparent;");
    card->addWidget(avatar, 0, Qt::AlignTop);

    auto *summary = new QVBoxLayout;
    summary->setContentsMargins(0, 2, 0, 0);
    summary->setSpacing(2);
    summary->addWidget(Win7::label(acct.fullName, 11, "#1A3C7A"));
    summary->addWidget(Win7::label(acct.accountType));
    summary->addWidget(Win7::label("Password protected"));
    summary->addStretch(1);
    card->addLayout(summary, 0);
    body->addLayout(card, 0);
    body->addStretch(1);

    contentV->addLayout(body);
    contentV->addStretch(1);
}
