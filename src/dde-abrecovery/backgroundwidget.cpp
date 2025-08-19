// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "backgroundwidget.h"

#include <DSpinner>
#include <DLabel>

#include <QPainter>
#include <QScreen>
#include <QApplication>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logUpdateRecovery)

DWIDGET_USE_NAMESPACE

BackgroundWidget::BackgroundWidget(bool bRestoring, QWidget *parent)
    : QWidget(parent)
    , m_bIsRestoring(bRestoring)
{
    qCDebug(logUpdateRecovery) << "Initialize BackgroundWidget, restoring:" << bRestoring;
    setAttribute(Qt::WA_TranslucentBackground, true);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    QScreen *pScreen = qApp->primaryScreen();
    this->setFixedSize(pScreen->size());

    if (m_bIsRestoring) {
        m_loadingIndicator = new DSpinner(this);
        m_loadingIndicator->setFixedSize(30, 30);
        m_loadingIndicator->start();

        // TODO：双屏下，文字显示位置异常，后续和产品商量是否可以直接放在主屏中间
        // 计算文字宽高，用于将文字放到中间
        QFont font;
        QFontMetrics fm(font);
        QRect rec = fm.boundingRect(tr("Rolling back the system, please wait..."));

        m_loadingIndicator->move((pScreen->size().width() - rec.width() - m_loadingIndicator->width()) / 2, (pScreen->size().height() - rec.height()) / 2);
    }
}

void BackgroundWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(this->rect(), QColor(0, 0, 0, 150));

    // 绘制文字无背景
    if (m_bIsRestoring) {
        painter.setPen(Qt::white);
        painter.drawText(QPointF(m_loadingIndicator->x() + m_loadingIndicator->width() + 10, m_loadingIndicator->y() + m_loadingIndicator->height() * 0.8), tr("Rolling back the system, please wait..."));
    }
    painter.end();
}
