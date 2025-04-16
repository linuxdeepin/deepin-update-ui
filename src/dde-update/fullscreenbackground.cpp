// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fullscreenbackground.h"

#include <DGuiApplicationHelper>

#include <QDebug>
#include <QImageReader>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QWindow>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QFile>

DGUI_USE_NAMESPACE

const int PIXMAP_TYPE_BLUR_BACKGROUND = 1;

static const QString SOLID_BACKGROUND_COLOR = "#000F27";        // 纯色背景色号

QString FullScreenBackground::blurBackgroundPath;

QMap<QString, QPixmap> FullScreenBackground::blurBackgroundCacheMap;
QList<FullScreenBackground *> FullScreenBackground::frameList;
QPointer<FullScreenBackground> FullScreenBackground::currentFrame = nullptr;
QPointer<QWidget> FullScreenBackground::currentContent = nullptr;

const bool IS_WAYLAND_DISPLAY = !qgetenv("WAYLAND_DISPLAY").isEmpty();

static const QString DEFAULT_BACKGROUND = QStringLiteral("/usr/share/backgrounds/default_background.jpg");

void loadPixmap(const QString &fileName, QPixmap &pixmap)
{
    if (!pixmap.load(fileName)) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            pixmap.loadFromData(file.readAll());
        }
    }
}

bool checkPictureCanRead(const QString &fileName)
{
    QImageReader reader;
    reader.setDecideFormatFromContent(true);
    reader.setFileName(fileName);
    return reader.canRead();
}

FullScreenBackground::FullScreenBackground(QWidget *parent)
    : QWidget(parent)
    , m_screen(nullptr)
    , m_useSolidBackground(false)
    , m_getBlurImageSuccess(false)
{
#ifndef QT_DEBUG
    if (!qgetenv("XDG_SESSION_TYPE").startsWith("wayland")) {
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
    } else {
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Window);

        setAttribute(Qt::WA_NativeWindow); // 创建窗口 handle
        create();
        // onScreenDisplay 低于override，高于tooltip，希望显示在锁屏上方的界面，均需要调整层级为onScreenDisplay或者override
        windowHandle()->setProperty("_d_dwayland_window-type", "onScreenDisplay");
    }
#endif
    frameList.append(this);
}

FullScreenBackground::~FullScreenBackground()
{
    frameList.removeAll(this);
}

void FullScreenBackground::updateBackground(const QString &path)
{
    if (m_useSolidBackground || !isPicture(path))
        return;

    m_getBlurImageSuccess = false;
    updateBlurBackground(path);
}

void FullScreenBackground::updateBlurBackground(const QString &path)
{
    auto updateBlurBackgroundFunc = [this](const QString &blurPath) {
        if (!blurPath.isEmpty() && blurBackgroundPath != blurPath) {
            blurBackgroundPath = blurPath;
        }

        QString scaledPath;
        if (getScaledBlurImage(blurPath, scaledPath)) {
            QPixmap pixmap;
            loadPixmap(scaledPath, pixmap);
            addPixmap(pixmap, PIXMAP_TYPE_BLUR_BACKGROUND);
        } else {
            handleBackground(blurBackgroundPath, PIXMAP_TYPE_BLUR_BACKGROUND);
        }

        update();
    };

    QDBusMessage message = QDBusMessage::createMethodCall("org.deepin.dde.ImageEffect1", "/org/deepin/dde/ImageEffect1",
                                                          "org.deepin.dde.ImageEffect1", "Get");
    message << "" << path;
    // 异步调用
    QDBusPendingCall async = QDBusConnection::systemBus().asyncCall(message);
    auto *watcher = new QDBusPendingCallWatcher(async);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher, updateBlurBackgroundFunc, this](QDBusPendingCallWatcher *callWatcher) {
        // 获取模糊壁纸路径
        QDBusPendingReply<QString> reply = *callWatcher;
        QString blurPath;
        if (!reply.isError()) {
            blurPath = reply.value();
            bool isPicture = QFile::exists(blurPath) && QFile(blurPath).size() && checkPictureCanRead(blurPath);
            if (!isPicture) {
                blurPath = "/usr/share/backgrounds/default_background.jpg";
            }
        } else {
            blurPath = "/usr/share/backgrounds/default_background.jpg";
            qWarning() << "Get blur background path error:" << reply.error().message();
        }

        m_getBlurImageSuccess = true;
        Q_EMIT blurImageReturned();

        // 处理模糊背景图和执行动画或update；
        updateBlurBackgroundFunc(blurPath);

        callWatcher->deleteLater();
        watcher->deleteLater();
    });
}

bool FullScreenBackground::contentVisible() const
{
    return currentContent && currentContent->isVisible() && currentContent->parent() == this;
}

void FullScreenBackground::setScreen(QPointer<QScreen> screen, bool isVisible)
{
    if (screen.isNull()) {
        qWarning() << "Screen is nullptr";
        return;
    }

    qInfo() << "Set screen:" << screen
            << ", screen geometry:" << screen->geometry()
            << ", full screen background object:" << this;
    if (isVisible) {
        updateCurrentFrame(this);
    } else {
        // 如果有多个屏幕则根据鼠标所在的屏幕来显示
        QTimer::singleShot(1000, this, [this] {
            setMouseTracking(true);
        });
    }

    updateScreen(screen);

    // 更新屏幕后设置壁纸，这样避免处理多个尺寸的壁纸
    updateBackground(DEFAULT_BACKGROUND);
}

void FullScreenBackground::setContent(QWidget *const w)
{
    if (!w) {
        qWarning() << "Content is null";
        return;
    }
    qInfo() << "Incoming  content:" << w << ", current content:" << currentContent;
    // 不重复设置content
    if (currentContent && currentContent->isVisible() && currentContent == w && currentFrame && w->parent() == currentFrame) {
        qDebug() << "Parent is current frame";
        return;
    }

    if (!currentContent.isNull()) {
        currentContent->setParent(nullptr);
        currentContent->hide();
    }
    currentContent = w;
    if (!currentFrame) {
        qWarning() << "Current frame is null";
        return;
    }

    qInfo() << "Set current frame:" << currentFrame;
    currentContent->setParent(currentFrame);
    currentContent->move(0, 0);
    currentContent->resize(currentFrame->size());
    currentFrame->setFocusProxy(currentContent);
    currentFrame->setFocus();
    currentFrame->activateWindow();
    currentContent->raise();
    currentContent->show();
}

bool FullScreenBackground::isPicture(const QString &file)
{
    return QFile::exists(file) && QFile(file).size() && checkPictureCanRead(file);
}

void FullScreenBackground::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRect trueRect(QPoint(0, 0), QSize(size() * devicePixelRatioF()));
    if (m_useSolidBackground) {
        painter.fillRect(trueRect, QColor(SOLID_BACKGROUND_COLOR));
    } else {
        const QPixmap &blurBackground = getPixmap(PIXMAP_TYPE_BLUR_BACKGROUND);
        if (blurBackground.isNull()) {
            painter.fillRect(trueRect, QColor(SOLID_BACKGROUND_COLOR));
        } else {
            painter.drawPixmap(trueRect,
                               blurBackground,
                               QRect(trueRect.topLeft(), trueRect.size() * devicePixelRatioF()));
        }
    }
}

void FullScreenBackground::enterEvent(QEnterEvent *event)
{
    updateCurrentFrame(this);
    // 多屏情况下，此Frame晚于其它Frame显示出来时，可能处于未激活状态（特别是在wayland环境下比较明显）
    activateWindow();

    return QWidget::enterEvent(event);
}

void FullScreenBackground::leaveEvent(QEvent *event)
{
    return QWidget::leaveEvent(event);
}

void FullScreenBackground::resizeEvent(QResizeEvent *event)
{
    if (!m_getBlurImageSuccess) {
        // 避免页面显示出来，模糊壁纸还没返回导致先出现纯色背景的问题。
        QEventLoop loop;
        QTimer::singleShot(1*1000, &loop, &QEventLoop::quit);
        connect(this, &FullScreenBackground::blurImageReturned, &loop, &QEventLoop::quit);
        loop.exec();
    }

    if (!blurBackgroundCacheMap.isEmpty() && isPicture(blurBackgroundPath) && !contains(PIXMAP_TYPE_BLUR_BACKGROUND)) {
        QString scaledPath;
        if (getScaledBlurImage(blurBackgroundPath, scaledPath)) {
            QPixmap pixmap;
            loadPixmap(scaledPath, pixmap);
            addPixmap(pixmap, PIXMAP_TYPE_BLUR_BACKGROUND);
        } else {
            handleBackground(blurBackgroundPath, PIXMAP_TYPE_BLUR_BACKGROUND);
        }
    }

    updatePixmap();

    if (currentFrame == this && currentContent) {
        currentContent->resize(size());
    }

    QWidget::resizeEvent(event);
}

/**
 * @brief 鼠标移动触发这个事件，可能需要配合setMouseTracking(true)使用
 *
 * @param event
 */
void FullScreenBackground::mouseMoveEvent(QMouseEvent *event)
{
    updateCurrentFrame(this);

    QWidget::mouseMoveEvent(event);
}

void FullScreenBackground::showEvent(QShowEvent *event)
{
    qDebug() << "Frame is already displayed:" << this;
    if (IS_WAYLAND_DISPLAY) {
        if (geometry().contains(QCursor::pos())) {
            currentContent->show();
            // 多屏情况下，此Frame晚于其它Frame显示出来时，可能处于未激活状态（特别是在wayland环境下比较明显）
            activateWindow();
        }
    }

    // setScreen中有设置updateGeometry，单屏不需要再次设置
    if (qApp->screens().size() > 1) {
        updateGeometry();
    }

    raise();

    return QWidget::showEvent(event);
}

void FullScreenBackground::hideEvent(QHideEvent *event)
{
    qInfo() << "Frame is hidden:" << this;

    QWidget::hideEvent(event);
}

void FullScreenBackground::updateScreen(QPointer<QScreen> screen)
{
    if (screen == m_screen)
        return;

    if (!m_screen.isNull())
        disconnect(m_screen, &QScreen::geometryChanged, this, &FullScreenBackground::updateGeometry);

    if (!screen.isNull()) {
        connect(screen, &QScreen::geometryChanged, this, &FullScreenBackground::updateGeometry);
    }

    m_screen = screen;

    updateGeometry();
}

void FullScreenBackground::updateGeometry()
{
    if (m_screen.isNull()) {
        return;
    }

    qInfo() << "Set background screen:" << m_screen << ", geometry:" << m_screen->geometry();
    if (!m_screen.isNull()) {
        setGeometry(m_screen->geometry());

        qInfo() << "Update geometry, screen:" << m_screen
                << ", screen geometry:" << m_screen->geometry()
                << ", lockFrame:" << this
                << ", frame geometry:" << this->geometry();
    } else {
        qWarning() << "Screen is nullptr";
    }
}

const QPixmap& FullScreenBackground::getPixmap(int type)
{
    static QPixmap nullPixmap;

    QString strSize = sizeToString(trueSize());
    if (PIXMAP_TYPE_BLUR_BACKGROUND == type && blurBackgroundCacheMap.contains(strSize)) {
        return blurBackgroundCacheMap[strSize];
    }

    return nullPixmap;
}

QSize FullScreenBackground::trueSize() const
{
    return size() * devicePixelRatioF();
}

/**
 * @brief FullScreenBackground::addPixmap
 * 新增pixmap，存在相同size的则替换，没有则新增
 * @param pixmap pixmap对象
 * @param type 清晰壁纸还是模糊壁纸
 */
void FullScreenBackground::addPixmap(const QPixmap &pixmap, const int type)
{
    QString strSize = sizeToString(trueSize());
    if (type == PIXMAP_TYPE_BLUR_BACKGROUND) {
        blurBackgroundCacheMap[strSize] = pixmap;
    }
}

/**
 * @brief FullScreenBackground::updatePixmap
 * 更新壁纸列表，移除当前所有屏幕都不会使用的壁纸数据
 */
void FullScreenBackground::updatePixmap()
{
    auto removeNotUsedPixmap = [](QMap<QString, QPixmap> &cacheMap, const QStringList &frameSizeList) {
        QStringList cacheMapKeys = cacheMap.keys();
        for (auto &key : cacheMapKeys) {
            if (!frameSizeList.contains(key)) {
                qInfo() << "Remove not used pixmap:" << key;
                cacheMap.remove(key);
            }
        }
    };

    QStringList frameSizeList;
    for (auto &frame : frameList) {
        QString strSize = sizeToString(frame->trueSize());
        frameSizeList.append(strSize);
    }

    removeNotUsedPixmap(blurBackgroundCacheMap, frameSizeList);
}

bool FullScreenBackground::contains(int type)
{
    auto containPixmap = [](const QMap<QString, QPixmap> &cacheMap, const QString &strSize) {
        if (cacheMap.contains(strSize)) {
            const QPixmap &pixmap = cacheMap[strSize];
            return !pixmap.isNull(); // pixmap 为空也返回false
        } else {
            return false;
        }
    };

    QString strSize = sizeToString(trueSize());
    if (PIXMAP_TYPE_BLUR_BACKGROUND == type) {
        return containPixmap(blurBackgroundCacheMap, strSize);
    }
    return false;
}

void FullScreenBackground::moveEvent(QMoveEvent *event)
{
    // 规避 bug189309，登录时后端屏幕变化通知太晚
    if (currentContent && !currentContent->isVisible()) {
        if (IS_WAYLAND_DISPLAY) {
            if (geometry().contains(QCursor::pos())) {
                currentContent->show();
            }
        }
    }
    QWidget::moveEvent(event);
}

void FullScreenBackground::updateCurrentFrame(FullScreenBackground *frame) {
    if (!frame) {
        qWarning() << "Update current frame failed, frame is null";
        return;
    }

    if (frame->m_screen)
        qInfo() << "Update current frame:" << frame << ", screen:" << frame->m_screen->name();

    currentFrame = frame;
    setContent(currentContent);
}

void FullScreenBackground::handleBackground(const QString &path, int type)
{
    QSize trueSize = this->trueSize();

    QPixmap pixmap;
    loadPixmap(path, pixmap);

    pixmap = pixmap.scaled(trueSize, Qt::KeepAspectRatioByExpanding, Qt::FastTransformation);
    pixmap = pixmap.copy(QRect((pixmap.width() - trueSize.width()) / 2,
                               (pixmap.height() - trueSize.height()) / 2,
                               trueSize.width(),
                               trueSize.height()));

    // draw pix to widget, so pix need set pixel ratio from qwidget devicepixelratioF
    pixmap.setDevicePixelRatio(devicePixelRatioF());
    addPixmap(pixmap, type);
}

QString FullScreenBackground::sizeToString(const QSize &size)
{
    return QString("%1x%2").arg(size.width()).arg(size.height());
}

bool FullScreenBackground::getScaledBlurImage(const QString &originPath, QString &scaledPath)
{
    // 为了兼容没有安装壁纸服务环境;Qt5.15高版本可以使用activatableServiceNames()遍历然后可判断系统有没有安装服务
    const QString wallpaperServicePath = "/lib/systemd/system/dde-wallpaper-cache.service";
    if (!QFile::exists(wallpaperServicePath)) {
        qWarning() << "dde-wallpaper-cache service not existed";
        return false;
    }

    // 壁纸服务dde-wallpaper-cache
    QDBusInterface wallpaperCacheInterface("org.deepin.dde.WallpaperCache", "/org/deepin/dde/WallpaperCache",
                                           "org.deepin.dde.WallpaperCache", QDBusConnection::systemBus());
    QVariantList sizeArray;
    sizeArray << QVariant::fromValue(this->trueSize());
    QDBusReply<QStringList> pathList= wallpaperCacheInterface.call("GetProcessedImagePaths", originPath, sizeArray);
    if (pathList.value().isEmpty()) {
        return false;
    }

    QString path = pathList.value().at(0);
    if (!path.isEmpty() &&  path != originPath) {
        scaledPath = path;
        qDebug() << "get scaled path:" << path;
        return true;
    }

    return false;
}
