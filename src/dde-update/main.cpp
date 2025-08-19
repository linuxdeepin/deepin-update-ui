// SPDX-FileCopyrightText: 2015 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "updatewidget.h"
#include "checksystemwidget.h"
#include "fullscreen.h"
#include "fullscreenmanager.h"
#include "fullscreenbackground.h"
#include "global_util/public_func.h"
#include "updateworker.h"

#include <DApplication>
#include <DGuiApplicationHelper>
#include <DLog>
#include <QLoggingCategory>

#include <QCommandLineParser>
#include <QCommandLineOption>

Q_LOGGING_CATEGORY(logUpdateModal, "dde.update.modalupdate")

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    qCInfo(logUpdateModal) << "Starting dde-update application";
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
        qCDebug(logUpdateModal) << "Set XDG_CURRENT_DESKTOP to Deepin";
    }

    DGuiApplicationHelper::setAttribute(DGuiApplicationHelper::UseInactiveColorGroup, false);

    // qt默认当最后一个窗口析构后，会自动退出程序，这里设置成false，防止插拔时，没有屏幕，导致进程退出
    QApplication::setQuitOnLastWindowClosed(false);

    DApplication *app = nullptr;
#if (DTK_VERSION < DTK_VERSION_CHECK(5, 4, 0, 0))
    app = new DApplication(argc, argv);
#else
    app = DApplication::globalApplication(argc, argv);
#endif

    // qt默认当最后一个窗口析构后，会自动退出程序，这里设置成false，防止插拔时，没有屏幕，导致进程退出
    QApplication::setQuitOnLastWindowClosed(false);
    app->setOrganizationName("deepin");
    app->setApplicationName("dde-update");
    app->setApplicationVersion("2015.1.0");

    qCDebug(logUpdateModal) << "Setting up DLog format and appenders";
    DLogManager::setLogFormat("%{time}{yyyy-MM-dd, HH:mm:ss.zzz} [%{type:-7}] [ %{function:-35} %{line}] %{message}\n");
    DLogManager::registerConsoleAppender();
    DLogManager::registerJournalAppender();

    qCDebug(logUpdateModal) << "Setting up command line options";
    QCommandLineOption doUpgrade(QStringList() << "u" << "do-upgrade", "Do upgrade, backup system and install packages.");
    QCommandLineOption checkSystem(QStringList() << "c" << "check-upgrade", "Check if the update was successful.");
    QCommandLineOption checkSystemBeforeLogin(QStringList() << "b" << "before-login", "Check system before login.");
    QCommandLineOption checkSystemAfterLogin(QStringList() << "a" << "after-login", "Check system after login.");
    QCommandLineParser parser;
    parser.setApplicationDescription("DDE UPGRADE");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(doUpgrade);
    parser.addOption(checkSystem);
    parser.addOption(checkSystemBeforeLogin);
    parser.addOption(checkSystemAfterLogin);
    parser.process(*app);

    DPalette pa = DGuiApplicationHelper::instance()->standardPalette(DGuiApplicationHelper::LightType);
    pa.setColor(QPalette::Normal, DPalette::WindowText, QColor("#FFFFFF"));
    pa.setColor(QPalette::Normal, DPalette::Text, QColor("#FFFFFF"));
    pa.setColor(QPalette::Normal, DPalette::AlternateBase, QColor(0, 0, 0, 76));
    pa.setColor(QPalette::Normal, DPalette::Button, QColor(255, 255, 255, 76));
    pa.setColor(QPalette::Normal, DPalette::Light, QColor(255, 255, 255, 76));
    pa.setColor(QPalette::Normal, DPalette::Dark, QColor(255, 255, 255, 76));
    pa.setColor(QPalette::Normal, DPalette::ButtonText, QColor("#FFFFFF"));

    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::WindowText, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::Text, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::AlternateBase, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::Button, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::Light, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::Dark, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::generatePaletteColor(pa, DPalette::ButtonText, DGuiApplicationHelper::LightType);
    DGuiApplicationHelper::instance()->setApplicationPalette(pa);

    qCDebug(logUpdateModal) << "Loading translation for locale:" << getCurrentLocale();
    QTranslator translatorLanguage;
    if (!translatorLanguage.load("/usr/share/deepin-update-ui/translations/dde-update_" + getCurrentLocale())) {
        qCWarning(logUpdateModal) << "Failed to load translation file for locale" << getCurrentLocale();
    }

    app->installTranslator(&translatorLanguage);

    qCDebug(logUpdateModal) << "Initializing UpdateWorker instance";
    UpdateWorker::instance()->init();

    const bool whetherDoUpgrade = UpdateModel::instance()->whetherDoUpgrade() || parser.isSet(doUpgrade);
    qCDebug(logUpdateModal) << "Whether do upgrade:" << whetherDoUpgrade;
    if (whetherDoUpgrade) {
        qCInfo(logUpdateModal) << "Showing update widget";
        UpdateWidget::instance()->showUpdate();
    } else {
        if (parser.isSet(checkSystem)) {
            UpdateModel::instance()->setCheckSystemStage(parser.isSet(checkSystemBeforeLogin) ? UpdateModel::CSS_BeforeLogin : UpdateModel::CSS_AfterLogin);
        }

        if (UpdateModel::instance()->checkSystemStage() == UpdateModel::CSS_None || UpdateModel::instance()->updateMode() <= 0) {
            qCWarning(logUpdateModal) << "Update options are invalid, check system stage:" << UpdateModel::instance()->checkSystemStage()
                << ", check update mode:" << UpdateModel::instance()->updateMode();

            return 1;
        }
    }

    auto createFrame = [whetherDoUpgrade](QPointer<QScreen> screen) -> QWidget * {
        // 所有的界面共用一块内容
        FullScreenBackground *bg = nullptr;
        if (whetherDoUpgrade) {
            bg = new FullScreenBackground();
            FullScreenBackground::setContent(UpdateWidget::instance());
        } else {
            bg = new FullScreenBackground();
            FullScreenBackground::setContent(CheckSystemWidget::instance());
        }

        bg->setScreen(screen);
        return bg;
    };

    new FullScreenManager(createFrame);

    if (!whetherDoUpgrade) {
        UpdateWorker::instance()->doCheckSystem(UpdateModel::instance()->updateMode(), UpdateModel::instance()->checkSystemStage());
    }

    return app->exec();
}
