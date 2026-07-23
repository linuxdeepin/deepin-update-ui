// SPDX-FileCopyrightText: 2015 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "updatewidget.h"
#include "checksystemwidget.h"
#include "fullscreen.h"
#include "fullscreenmanager.h"
#include "fullscreenbackground.h"
#include "global_util/public_func.h"
#include "updateworker.h"
#include "securityloaderhelper.h"

#include <DApplication>
#include <DGuiApplicationHelper>
#include <DLog>
#include <QLoggingCategory>

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>

Q_LOGGING_CATEGORY(logUpdateModal, "dde.update.modalupdate")

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

// Write all of `data` to `fd` opened O_NONBLOCK, using poll(2) for POLLOUT
// on EAGAIN/EWOULDBLOCK instead of busy-looping. Returns true iff every byte
// was written within `timeoutMs`. Handles EINTR for both write() and poll().
static bool writeAllFifo(int fd, const QByteArray &data, int timeoutMs)
{
    ssize_t total = 0;
    while (total < data.size()) {
        ssize_t w = write(fd, data.constData() + total, data.size() - total);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd;
                pfd.fd = fd;
                pfd.events = POLLOUT;
                int pr = -1;
                do {
                    pr = poll(&pfd, 1, timeoutMs);
                } while (pr < 0 && errno == EINTR);
                if (pr <= 0) {
                    return false;
                }
                continue;
            }
            return false;
        }
        total += w;
    }
    return total == data.size();
}

// RAII notifier: writes to a FIFO on destruction so the launching script
// can block until dde-update exits. Covers all return paths from main().
class FifoNotifier
{
public:
    explicit FifoNotifier(const QString &fifoPath)
        : m_path(fifoPath)
    {
        reportPid();
    }
    ~FifoNotifier()
    {
        notify();
    }
    // Disable copy to avoid double-notify.
    FifoNotifier(const FifoNotifier &) = delete;
    FifoNotifier &operator=(const FifoNotifier &) = delete;

private:
    void reportPid()
    {
        if (m_path.isEmpty()) {
            return;
        }
        // Open first, then verify it's a FIFO via fstat().
        // This avoids the TOCTOU race of stat() before open().
        int fd = open(m_path.toUtf8().constData(), O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            qCWarning(logUpdateModal) << "FifoNotifier: cannot report PID, open failed:" << strerror(errno);
            return;
        }
        struct stat st;
        if (fstat(fd, &st) != 0 || !S_ISFIFO(st.st_mode)) {
            qCWarning(logUpdateModal) << "FifoNotifier: path is not a FIFO, cannot report PID";
            close(fd);
            return;
        }
        // Write our PID so the wrapper can track this exact process via kill -0.
        QByteArray msg = "PID:" + QByteArray::number(getpid()) + "\n";
        bool ok = writeAllFifo(fd, msg, 5000);
        close(fd);
        if (ok) {
            qCInfo(logUpdateModal) << "FifoNotifier: reported PID" << getpid() << "to FIFO";
        } else {
            qCWarning(logUpdateModal) << "FifoNotifier: failed to report PID (write incomplete)";
        }
    }

    void notify()
    {
        if (m_path.isEmpty()) {
            return;
        }
        qCInfo(logUpdateModal) << "FifoNotifier: notifying FIFO" << m_path;
        // Open first, then verify it's a FIFO via fstat().
        // This avoids the TOCTOU race of stat() before open().
        // O_NONBLOCK so we don't hang if the reader already closed;
        // the script opens fd 3<> before launching us so a reader exists.
        int fd = open(m_path.toUtf8().constData(), O_WRONLY | O_NONBLOCK);
        if (fd < 0) {
            qCWarning(logUpdateModal) << "FifoNotifier: cannot open" << m_path << ":" << strerror(errno);
            return;
        }
        struct stat st;
        if (fstat(fd, &st) != 0 || !S_ISFIFO(st.st_mode)) {
            qCWarning(logUpdateModal) << "FifoNotifier: path is not a FIFO, cannot notify";
            close(fd);
            return;
        }
        // Write "EXIT\n" to signal exit to the wrapper.
        QByteArray msg("EXIT\n");
        bool ok = writeAllFifo(fd, msg, 2000);
        close(fd);
        m_path.clear();
        if (ok) {
            qCInfo(logUpdateModal) << "FifoNotifier: exit notify sent (EXIT)";
        } else {
            qCWarning(logUpdateModal) << "FifoNotifier: failed to notify EXIT (write incomplete)";
        }
    }

    QString m_path;
};

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
    DLogManager::setLogFormat("%{time}{yyyy-MM-dd, HH:mm:ss.zzz} [%{type:-7}] [ %{function:-35} %{line}] %{message}");
    DLogManager::registerConsoleAppender();
    DLogManager::registerJournalAppender();

    qCDebug(logUpdateModal) << "Setting up command line options";
    QCommandLineOption doUpgrade(QStringList() << "u" << "do-upgrade", "Do upgrade, backup system and install packages.");
    QCommandLineOption checkSystem(QStringList() << "c" << "check-upgrade", "Check if the update was successful.");
    QCommandLineOption checkSystemBeforeLogin(QStringList() << "b" << "before-login", "Check system before login.");
    QCommandLineOption checkSystemAfterLogin(QStringList() << "a" << "after-login", "Check system after login.");
    QCommandLineOption notifyFifoOption(QStringList() << "notify-fifo", "FIFO path to notify on exit", "path");
    // --fd1/--fd2 由 security-loader 注入, 注册给 parser 避免未知选项退出,
    // 实际解析在 SecurityLoaderHelper::parseLoaderArgs 中完成
    QCommandLineOption fd1Option(QStringList() << "fd1", "File descriptor for security loader communication", "fd");
    QCommandLineOption fd2Option(QStringList() << "fd2", "File descriptor for security loader notification", "fd");
    QCommandLineParser parser;
    parser.setApplicationDescription("DDE UPGRADE");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(doUpgrade);
    parser.addOption(checkSystem);
    parser.addOption(checkSystemBeforeLogin);
    parser.addOption(checkSystemAfterLogin);
    parser.addOption(notifyFifoOption);
    parser.addOption(fd1Option);
    parser.addOption(fd2Option);
    parser.process(*app);

    // Security loader handshake: parse --fd1/--fd2 injected by security-loader,
    // perform D-Bus authorization handshake if loaded by loader. The handshake
    // is best-effort: even if it fails we keep running, since the check is not
    // strictly required for dde-update to function.
    SecurityLoaderHelper securityLoader;
    if (!securityLoader.doSecurityLoader(argc, argv)) {
        qCWarning(logUpdateModal) << "Security loader handshake failed, continuing anyway";
    }

    // RAII: notifies the launching script via FIFO when main() returns (any exit path).
    QString fifoPath = parser.value(notifyFifoOption);
    if (!fifoPath.isEmpty()) {
        qCInfo(logUpdateModal) << "FifoNotifier: enabled, FIFO path:" << fifoPath;
    }
    FifoNotifier fifoNotifier(fifoPath);

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

    QString locale = getCurrentLocale();
    qCDebug(logUpdateModal) << "Loading translation for locale:" << locale;
    QTranslator translatorLanguage;
    if (!translatorLanguage.load("/usr/share/deepin-update-ui/translations/dde-update_" + locale)) {
        qCWarning(logUpdateModal) << "Failed to load translation file for locale" << locale;
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

            qCInfo(logUpdateModal) << "Exiting: invalid update options (return 1)";
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
        UpdateWorker::instance()->checkSystem(UpdateModel::instance()->updateMode(), UpdateModel::instance()->checkSystemStage());
    }

    int ret = app->exec();
    qCInfo(logUpdateModal) << "Exiting: app->exec() returned" << ret;
    return ret;
}
