// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "recoverydialog.h"
#include "global_util/public_func.h"
#include "securityloaderhelper.h"
#include "fifonotifier.h"

#include <DApplication>
#include <DLog>
#include <QLoggingCategory>

#include <QTranslator>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <DDBusSender>
#include <QDBusConnection>
#include <QDebug>

Q_LOGGING_CATEGORY(logUpdateRecovery, "dde.update.recovery")

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    qCInfo(logUpdateRecovery) << "Starting dde-abrecovery application";
    DApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    qCDebug(logUpdateRecovery) << "Loading translation for locale:" << getCurrentLocale();
    QTranslator translator;
    if (!translator.load("/usr/share/deepin-update-ui/translations/dde-rollback_" + getCurrentLocale())) {
        qCWarning(logUpdateRecovery) << "Failed to load translation file for locale" << getCurrentLocale();
    } else {
        qCDebug(logUpdateRecovery) << "Translation loaded successfully";
    }

    a.installTranslator(&translator);

    qCDebug(logUpdateRecovery) << "Registering DLog appenders";
    DLogManager::registerConsoleAppender();
    DLogManager::registerJournalAppender();

    // --fd1/--fd2 由 security-loader 注入；--notify-fifo 由 /usr/bin/dde-rollback --wait 传入。
    // 都注册给 parser，避免未知选项直接退出；fd 的取值仍由 SecurityLoaderHelper 解析。
    QCommandLineOption fd1Option(QStringList() << "fd1", "File descriptor for security loader communication", "fd");
    QCommandLineOption fd2Option(QStringList() << "fd2", "File descriptor for security loader notification", "fd");
    QCommandLineOption notifyFifoOption(QStringList() << "notify-fifo", "FIFO path to notify on exit", "path");
    QCommandLineParser parser;
    parser.setApplicationDescription("DDE ROLLBACK");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(fd1Option);
    parser.addOption(fd2Option);
    parser.addOption(notifyFifoOption);
    parser.process(a);

    // security-loader 的 fd1/fd2 握手：treeland 下本进程以 dde 身份运行，必须籍此
    // 把自己的系统总线 unique name 注册进 lastore 的 allow-caller 白名单，否则
    // CanRollback / ConfirmRollback 会被 polkit 拒绝。未经 loader 启动时自动跳过
    // （X11 由 lightdm 身份直接放行）。
    SecurityLoaderHelper securityLoader(QStringLiteral(":/misc/org.deepin.dde-rollback.json"));
    if (!securityLoader.doSecurityLoader(argc, argv)) {
        qCWarning(logUpdateRecovery) << "Security loader handshake failed, continuing anyway";
    }

    // 启动时向 FIFO 上报 PID、退出时上报 EXIT，让 /usr/bin/dde-rollback --wait 能阻塞等待。
    QString fifoPath = parser.value(notifyFifoOption);
    if (!fifoPath.isEmpty()) {
        qCInfo(logUpdateRecovery) << "FifoNotifier: enabled, FIFO path:" << fifoPath;
    }
    FifoNotifier fifoNotifier(fifoPath);

    qCDebug(logUpdateRecovery) << "Creating recovery manager";
    Manage recovery;

    qCDebug(logUpdateRecovery) << "Starting application event loop";
    return a.exec();
}
