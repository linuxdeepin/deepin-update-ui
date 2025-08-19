// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "recoverydialog.h"
#include "global_util/public_func.h"

#include <DApplication>
#include <DLog>
#include <QLoggingCategory>

#include <QTranslator>
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

    qCDebug(logUpdateRecovery) << "Creating recovery manager";
    Manage recovery;

    qCDebug(logUpdateRecovery) << "Starting application event loop";
    return a.exec();
}
