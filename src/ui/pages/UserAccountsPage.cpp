#include "UserAccountsPage.h"
#include "Commands.h"
#include "LinkLabel.h"
#include "IconHelper.h"
#include "Win7Ui.h"

#include <QScrollArea>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFont>
#include <QFile>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include "Aero7FileDialog.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QStandardPaths>
#include <QDir>
#include <QImage>
#include <QLineEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QRegularExpression>
#include <functional>
#include <algorithm>
#include <utility>

#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <vector>

namespace {

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

}

// Data gathering
UserAccountsPage::Account UserAccountsPage::gatherAccount()
{
    Account a;

    const uid_t uid = getuid();
    const passwd *pw = getpwuid(uid);
    if (pw) {
        a.userName = QString::fromLocal8Bit(pw->pw_name);
        // GECOS is a comma-separated list; the first field is the full name.
        a.fullName = QString::fromLocal8Bit(pw->pw_gecos).section(QLatin1Char(','), 0, 0);

        // The account is an "Administrator" if it belongs to the wheel or sudo
        // group, the standard admin groups a polkit/sudo policy grants rights to.
        if (userIsAdministrator(pw))
            a.accountType = QStringLiteral("Administrator");
    }
    if (a.userName.isEmpty())
        a.userName = QString::fromLocal8Bit(qgetenv("USER"));
    if (a.fullName.isEmpty())
        a.fullName = a.userName;
    if (a.accountType.isEmpty())
        a.accountType = QStringLiteral("Standard user");

    // The account picture: user dotfiles first, then the AccountsService icon
    // that KDE/GDM login screens use.
    const QString home = QString::fromLocal8Bit(qgetenv("HOME"));
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

QList<UserAccountsPage::Account> UserAccountsPage::gatherAccounts()
{
    QList<Account> accounts;
    setpwent();
    while (const passwd *pw = getpwent()) {
        // Arch's regular user range begins at 1000. Exclude nobody and service
        // accounts even if a local administrator changed their UID.
        if (pw->pw_uid < 1000 || pw->pw_uid >= 60000)
            continue;
        Account account;
        account.userName = QString::fromLocal8Bit(pw->pw_name);
        account.fullName =
            QString::fromLocal8Bit(pw->pw_gecos).section(QLatin1Char(','), 0, 0);
        if (account.fullName.isEmpty())
            account.fullName = account.userName;
        account.accountType = userIsAdministrator(pw)
            ? QStringLiteral("Administrator") : QStringLiteral("Standard user");
        const QString home = QString::fromLocal8Bit(pw->pw_dir);
        for (const QString &path : {
                 home + QStringLiteral("/.face.icon"),
                 home + QStringLiteral("/.face"),
                 QStringLiteral("/var/lib/AccountsService/icons/")
                     + account.userName}) {
            if (QFile::exists(path)) {
                account.picturePath = path;
                break;
            }
        }
        accounts.append(account);
    }
    endpwent();
    std::sort(accounts.begin(), accounts.end(),
              [](const Account &left, const Account &right) {
        return left.fullName.compare(right.fullName, Qt::CaseInsensitive) < 0;
    });
    return accounts;
}

// Sidebar
QList<SidebarLink> UserAccountsPage::sidebarLinks()
{
    return {
        Nav::to("Advanced account settings", PageId::UserAccounts),
        Nav::to("Review administrator approval", PageId::SecurityMaintenance),
    };
}

QList<SidebarLink> UserAccountsPage::sidebarSeeAlso()
{
    return {
        Nav::disabled("Parental Controls"),
    };
}

// Render the avatar as a rounded, framed thumbnail; fall back to a theme icon
// when the user has no picture of their own.
static QPixmap avatarPixmap(const QString &path, int size)
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

// Page
UserAccountsPage::UserAccountsPage(QScrollArea *sidebar, QWidget *parent)
    : QWidget(parent)
{
    const Account acct = gatherAccount();

    // Windows 7 lays the content out at a fixed width and leaves the rest of
    // the window blank on the right rather than stretching to fill it.
    auto *contentV = Win7::pageScaffold(this, sidebar, /*bottomMargin=*/20,
                                        /*fixedWidth=*/700);

    // Page title.
    contentV->addWidget(Win7::pageTitle("Make changes to your user account"));
    contentV->addSpacing(18);

    // Body: task links on the left, the account summary card on the right.
    auto *body = new QHBoxLayout;
    body->setContentsMargins(6, 0, 0, 0);
    body->setSpacing(24);

    auto *tasks = new QVBoxLayout;
    tasks->setContentsMargins(0, 0, 0, 0);
    tasks->setSpacing(12);

    auto addTask = [&](const QString &text, std::function<void()> action) {
        auto *link = new LinkLabel(text);
        QObject::connect(link, &LinkLabel::clicked, this, std::move(action));
        tasks->addWidget(link, 0, Qt::AlignLeft);
    };

    addTask("Change your password", [this]() {
        const QString terminal = QStandardPaths::findExecutable("qterminal");
        if (!terminal.isEmpty())
            QProcess::startDetached(terminal, {"-e", "passwd"});
        else
            QMessageBox::warning(this, QStringLiteral("User Accounts"),
                                 QStringLiteral("A password terminal is not installed."));
    });
    addTask("Change your picture", [this]() { changePicture(); });
    addTask("Change your account name", [this, acct]() { changeDisplayName(acct); });
    addTask("Change your account type",
            [this, acct]() { changeAccountType(acct); });
    addTask("Manage another account", [this]() { manageAccounts(); });
    addTask("Review administrator approval", [this]() {
        emit navigateRequested(PageId::SecurityMaintenance);
    });

    tasks->addStretch(1);
    body->addLayout(tasks, 0);

    // Account summary card: avatar, then name / type / password state.
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

void UserAccountsPage::changePicture()
{
    const QString source = Aero7FileDialog::openFile(
        this, QStringLiteral("control-user-picture"),
        QStringLiteral("Images (*.png *.jpg *.jpeg *.webp *.bmp);;All files (*)"));
    if (source.isEmpty())
        return;
    QImage image(source);
    if (image.isNull()) {
        QMessageBox::warning(this, "User Accounts", "The selected image could not be opened.");
        return;
    }
    const QString destination = QDir::homePath() + "/.face.icon";
    if (!image.scaled(512, 512, Qt::KeepAspectRatio, Qt::SmoothTransformation)
             .save(destination, "PNG")) {
        QMessageBox::warning(this, "User Accounts", "The account picture could not be saved.");
        return;
    }
    QFile::remove(QDir::homePath() + "/.face");
    QFile::link(destination, QDir::homePath() + "/.face");
    emit refreshRequested();
}

void UserAccountsPage::changeDisplayName(const Account &account)
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, "Change account name", "New display name:", QLineEdit::Normal,
        account.fullName, &ok).trimmed();
    if (!ok || name.isEmpty() || name == account.fullName)
        return;
    if (QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty()
        || QStandardPaths::findExecutable(QStringLiteral("usermod")).isEmpty()) {
        QMessageBox::warning(
            this, "User Accounts",
            "Changing the display name requires pkexec and usermod.");
        return;
    }
    auto *process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            QMessageBox::warning(this, "User Accounts",
                                 "The account command could not be started.");
            process->deleteLater();
        }
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int code, QProcess::ExitStatus) {
        if (code == 0)
            emit refreshRequested();
        else if (code != 126 && code != 127) {
            const QString error =
                QString::fromUtf8(process->readAllStandardError()).trimmed();
            QMessageBox::warning(
                this, "User Accounts",
                error.isEmpty() ? "The account name was not changed." : error);
        }
        process->deleteLater();
    });
    process->start("pkexec", {"usermod", "-c", name, account.userName});
}

bool UserAccountsPage::runAccountCommand(const QStringList &arguments,
                                         const QString &failureMessage)
{
    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    if (pkexec.isEmpty() || arguments.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("User Accounts"),
                             QStringLiteral("The administrator approval service "
                                            "is not installed."));
        return false;
    }
    QProcess process;
    process.start(pkexec, arguments);
    if (!process.waitForStarted(3000) || !process.waitForFinished(-1)
        || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        // 126/127 mean the user cancelled or authentication was unavailable.
        if (process.exitCode() != 126 && process.exitCode() != 127) {
            const QString detail =
                QString::fromUtf8(process.readAllStandardError()).trimmed();
            QMessageBox::warning(
                this, QStringLiteral("User Accounts"),
                detail.isEmpty() ? failureMessage : detail);
        }
        return false;
    }
    return true;
}

void UserAccountsPage::changeAccountType(const Account &account, bool refreshPage)
{
    bool accepted = false;
    const QString type = QInputDialog::getItem(
        this, QStringLiteral("Change account type"),
        QStringLiteral("New account type:"),
        {QStringLiteral("Standard user"), QStringLiteral("Administrator")},
        account.accountType == QLatin1String("Administrator") ? 1 : 0,
        false, &accepted);
    if (!accepted || type == account.accountType)
        return;

    QStringList command;
    if (type == QLatin1String("Administrator")) {
        command = {QStringLiteral("/usr/bin/usermod"), QStringLiteral("-aG"),
                   QStringLiteral("wheel"), account.userName};
    } else {
        if (account.userName == QString::fromLocal8Bit(qgetenv("USER"))
            && QMessageBox::warning(
                   this, QStringLiteral("Change account type"),
                   QStringLiteral("Removing administrator access from the "
                                  "signed-in account may prevent future system "
                                  "changes. Continue?"),
                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                   != QMessageBox::Yes)
            return;
        command = {QStringLiteral("/usr/bin/gpasswd"), QStringLiteral("-d"),
                   account.userName, QStringLiteral("wheel")};
    }
    if (runAccountCommand(command,
                          QStringLiteral("The account type was not changed."))
        && refreshPage)
        emit refreshRequested();
}

void UserAccountsPage::manageAccounts()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Manage Accounts"));
    dialog.resize(520, 360);
    auto *layout = new QVBoxLayout(&dialog);
    auto *intro = new QLabel(
        QStringLiteral("Create, remove, or change local Aero7 user accounts. "
                       "Administrator approval is required."));
    intro->setWordWrap(true);
    layout->addWidget(intro);
    auto *list = new QListWidget;
    layout->addWidget(list, 1);

    QList<Account> accounts;
    auto reload = [&]() {
        accounts = gatherAccounts();
        list->clear();
        for (const Account &account : std::as_const(accounts)) {
            auto *item = new QListWidgetItem(
                themeIcon({"user-identity", "system-users"}),
                QStringLiteral("%1 (%2) — %3")
                    .arg(account.fullName, account.userName, account.accountType));
            item->setData(Qt::UserRole, account.userName);
            list->addItem(item);
        }
        if (list->count())
            list->setCurrentRow(0);
    };
    reload();

    auto selectedAccount = [&]() -> Account {
        const QString user = list->currentItem()
            ? list->currentItem()->data(Qt::UserRole).toString() : QString();
        for (const Account &account : std::as_const(accounts))
            if (account.userName == user)
                return account;
        return {};
    };

    auto *actions = new QHBoxLayout;
    auto *add = new QPushButton(QStringLiteral("Create account…"));
    auto *type = new QPushButton(QStringLiteral("Change type…"));
    auto *remove = new QPushButton(QStringLiteral("Remove account…"));
    actions->addWidget(add);
    actions->addWidget(type);
    actions->addWidget(remove);
    actions->addStretch(1);
    layout->addLayout(actions);

    connect(add, &QPushButton::clicked, &dialog, [&, this]() {
        bool accepted = false;
        const QString username = QInputDialog::getText(
            &dialog, QStringLiteral("Create account"),
            QStringLiteral("User name (lower-case letters and numbers):"),
            QLineEdit::Normal, {}, &accepted).trimmed();
        static const QRegularExpression valid(
            QStringLiteral("^[a-z_][a-z0-9_-]{0,31}$"));
        if (!accepted)
            return;
        if (!valid.match(username).hasMatch()) {
            QMessageBox::warning(&dialog, QStringLiteral("Create account"),
                                 QStringLiteral("Enter a valid Linux user name."));
            return;
        }
        const QString fullName = QInputDialog::getText(
            &dialog, QStringLiteral("Create account"),
            QStringLiteral("Display name:"), QLineEdit::Normal,
            username, &accepted).trimmed();
        if (!accepted)
            return;
        const QString accountType = QInputDialog::getItem(
            &dialog, QStringLiteral("Create account"),
            QStringLiteral("Account type:"),
            {QStringLiteral("Standard user"), QStringLiteral("Administrator")},
            0, false, &accepted);
        if (!accepted)
            return;
        QStringList command = {QStringLiteral("/usr/bin/useradd"),
                               QStringLiteral("-m"), QStringLiteral("-c"),
                               fullName, QStringLiteral("-s"),
                               QStringLiteral("/bin/bash")};
        if (accountType == QLatin1String("Administrator"))
            command << QStringLiteral("-G") << QStringLiteral("wheel");
        command << username;
        if (!runAccountCommand(
                command, QStringLiteral("The account could not be created.")))
            return;

        const QString terminal =
            QStandardPaths::findExecutable(QStringLiteral("qterminal"));
        if (!terminal.isEmpty())
            QProcess::startDetached(
                terminal, {QStringLiteral("-e"), QStringLiteral("pkexec"),
                           QStringLiteral("/usr/bin/passwd"), username});
        QMessageBox::information(
            &dialog, QStringLiteral("Create account"),
            QStringLiteral("The account was created. Set its password in the "
                           "Command Prompt window."));
        reload();
    });

    connect(type, &QPushButton::clicked, &dialog, [&, this]() {
        const Account account = selectedAccount();
        if (account.userName.isEmpty())
            return;
        changeAccountType(account, false);
        reload();
    });

    connect(remove, &QPushButton::clicked, &dialog, [&, this]() {
        const Account account = selectedAccount();
        if (account.userName.isEmpty())
            return;
        if (account.userName == QString::fromLocal8Bit(qgetenv("USER"))) {
            QMessageBox::warning(&dialog, QStringLiteral("Remove account"),
                                 QStringLiteral("You cannot remove the account "
                                                "that is currently signed in."));
            return;
        }
        if (QMessageBox::warning(
                &dialog, QStringLiteral("Remove account"),
                QStringLiteral("Remove %1? The home folder and personal files "
                               "will be preserved.")
                    .arg(account.fullName),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
            != QMessageBox::Yes)
            return;
        if (runAccountCommand(
                {QStringLiteral("/usr/bin/userdel"), account.userName},
                QStringLiteral("The account could not be removed."))) {
            reload();
        }
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    dialog.exec();
    emit refreshRequested();
}
