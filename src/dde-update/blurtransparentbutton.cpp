// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "blurtransparentbutton.h"

#include <QPainter>
#include <QDebug>
#include <QLabel>
#include <QPixmap>
#include <QLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QStyleOption>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logUpdateModal)

const int penWidth = 2;

BlurTransparentButton::BlurTransparentButton(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_state(Leave)
    , m_iconLabel(new QLabel(this))
    , m_textLabel(new QLabel(this))
    , m_radius(10)
    , m_enableHighLightFocus(true)
{
    qCDebug(logUpdateModal) << "Initialize BlurTransparentButton with text:" << text;
    m_iconLabel->setFixedSize(64, 64);
    m_textLabel->setText(text);

    m_iconLabel->setAlignment(Qt::AlignmentFlag::AlignRight | Qt::AlignmentFlag::AlignVCenter);
    m_textLabel->setAlignment(Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter);
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addStretch();
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_iconLabel);
    layout->setSpacing(20);
    layout->addWidget(m_textLabel);
    layout->addStretch();

    m_iconLabel->setVisible(false);

    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    this->setPalette(palette);

    this->setLayout(layout);
    qCDebug(logUpdateModal) << "BlurTransparentButton setup completed";
}

BlurTransparentButton::~BlurTransparentButton()
{
    qCDebug(logUpdateModal) << "Destroying BlurTransparentButton";
}

void BlurTransparentButton::setText(const QString &text)
{
    qCDebug(logUpdateModal) << "Setting button text:" << text;
    m_textLabel->setText(text);
}

QString BlurTransparentButton::text()
{
    return m_textLabel->text();
}

void BlurTransparentButton::setNormalPixmap(const QPixmap &normalPixmap)
{
    qCDebug(logUpdateModal) << "Setting normal pixmap, size:" << normalPixmap.size();
    m_normalPixmap = normalPixmap;
    m_iconLabel->setVisible(true);
}

void BlurTransparentButton::setHoverPixmap(const QPixmap &hoverPixmap)
{
    qCDebug(logUpdateModal) << "Setting hover pixmap, size:" << hoverPixmap.size();
    m_hoverPixmap = hoverPixmap;
    m_iconLabel->setVisible(true);
}

QPixmap BlurTransparentButton::hoverPixmap()
{
    qCDebug(logUpdateModal) << "Getting hover pixmap, size:" << m_hoverPixmap.size();
    return m_hoverPixmap;
}

QPixmap BlurTransparentButton::normalPixmap()
{
    qCDebug(logUpdateModal) << "Getting normal pixmap, size:" << m_normalPixmap.size();
    return m_normalPixmap;
}

void BlurTransparentButton::setRadius(int radius)
{
    qCDebug(logUpdateModal) << "Setting border radius:" << radius;
    m_radius = radius;
}

void BlurTransparentButton::enterEvent(QEnterEvent *event)
{
    m_state = Enter;
    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, Qt::black);
    this->setPalette(palette);

    m_iconLabel->setPixmap(m_hoverPixmap);

    QWidget::enterEvent(event);
    update();
}

void BlurTransparentButton::leaveEvent(QEvent *event)
{
    m_state = Leave;
    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    this->setPalette(palette);

    m_iconLabel->setPixmap(m_normalPixmap);

    QWidget::leaveEvent(event);
    update();
}

void BlurTransparentButton::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPalette palette = this->palette();
    QColor color = palette.color(QPalette::Text);
    switch (m_state) {
    case Enter:
    case Release:
        color.setAlphaF(0.4);
        break;
    case Leave:
    case Press:
        color.setAlphaF(0.1);
        break;
    default:
        break;
    }
    painter.setBrush(color);

    QStyleOption opt;
    opt.initFrom(this);
    if (opt.state & QStyle::State_HasFocus && m_enableHighLightFocus) {
        QPen pen;
        pen.setWidth(penWidth);
        color.setAlphaF(0.5);
        pen.setColor(color);
        painter.setPen(pen);
    } else
        painter.setPen(Qt::NoPen);

    painter.drawRoundedRect(rect().marginsRemoved(QMargins(penWidth, penWidth, penWidth, penWidth)), m_radius, m_radius);

    QWidget::paintEvent(event);
}

void BlurTransparentButton::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit clicked();
    return;
}

void BlurTransparentButton::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        qCDebug(logUpdateModal) << "Enter key pressed, emitting clicked signal";
        emit clicked();
        break;
    }
    QWidget::keyPressEvent(event);
}

void BlurTransparentButton::showEvent(QShowEvent *event)
{
    m_iconLabel->setPixmap(m_normalPixmap);
    QWidget::showEvent(event);
}
