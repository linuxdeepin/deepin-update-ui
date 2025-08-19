// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include <QApplication>
#include <QDebug>
#include <QScreen>
#include <QPaintEvent>
#include <QPainter>
#include <QPointer>
#include <QWindow>

#include "fullscreen.h"
#include "common/dconfig_helper.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logUpdateModal)

static const QString SOLID_BACKGROUND_COLOR = "#000F27";        // 纯色背景色号

FullScreen::FullScreen(QWidget *content, QWidget *parent)
    : QWidget(parent)
    , m_content(content)
    , m_showBlack(false)
{
    qCDebug(logUpdateModal) << "Initialize FullScreen with content widget";
    Q_ASSERT(m_content);

    this->installEventFilter(this);

#ifndef QT_DEBUG
    if (!qgetenv("XDG_SESSION_TYPE").startsWith("wayland")) {
        qCDebug(logUpdateModal) << "Setting X11 window flags for fullscreen";
        setWindowFlags(Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);
    } else {
        qCDebug(logUpdateModal) << "Setting Wayland window flags for fullscreen";
        setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Window);

        setAttribute(Qt::WA_NativeWindow); // 创建窗口 handle
        create();
        // onScreenDisplay 低于override，高于tooltip，希望显示在锁屏上方的界面，均需要调整层级为onScreenDisplay或者override
        windowHandle()->setProperty("_d_dwayland_window-type", "onScreenDisplay");
    }
#endif

}

void FullScreen::setScreen(QPointer<QScreen> screen, bool isVisible)
{
    Q_UNUSED(isVisible);
    qCDebug(logUpdateModal) << "Setting screen geometry:" << screen->geometry();

    setGeometry(screen->geometry());
    show();

    if (screen == qApp->primaryScreen() && !m_showBlack) {
        qCDebug(logUpdateModal) << "Activating window on primary screen";
        activateWindow();

        m_content->hide();
        m_content->setParent(this);
        m_content->resize(this->size());
        m_content->show();
    }
}

void FullScreen::onAuthFinished()
{
    qCDebug(logUpdateModal) << "Authentication finished, enabling black screen mode";
    m_showBlack = true;
    repaint();
}

void FullScreen::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const QRect trueRect(QPoint(0, 0), QSize(size() * devicePixelRatioF()));

    // 绘制纯黑色画面，提升体验
    if (m_showBlack) {
        painter.fillRect(trueRect, Qt::black);
        return;
    }

    painter.fillRect(trueRect, QColor(SOLID_BACKGROUND_COLOR));
}

bool FullScreen::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this && !m_showBlack
            && (event->type() == QEvent::Enter)) {
        activateWindow();

        m_content->hide();
        m_content->setParent(this);
        m_content->resize(this->size());
        m_content->show();
    }

    if (watched == this && !m_showBlack
            && event->type() == QEvent::Resize) {
        if (m_content && m_content->parent() == this) {
            m_content->resize(this->size());
        }
    }

    return QWidget::eventFilter(watched, event);
}
