// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "recoverydialog.h"
#include "global_util/public_func.h"

#include <DApplication>
#include <DLog>

#include <QTranslator>
#include <DDBusSender>
#include <QDBusConnection>
#include <QDebug>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    DApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    QTranslator translator;
    translator.load("/usr/share/deepin-update-ui/translations/dde-abrecovery_" + getCurrentLocale());
    a.installTranslator(&translator);

    DLogManager::registerConsoleAppender();
    DLogManager::registerJournalAppender();

    Manage recovery;

    return a.exec();
}
