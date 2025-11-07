// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockiconwidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <DGuiApplicationHelper>
#include <DIconTheme>
#include <DLabel>
DockIconWidget::DockIconWidget(QWidget *parent)
    : QWidget(parent)
{
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
            this, &DockIconWidget::onThemeChanged);
}

DockIconWidget::~DockIconWidget()
{
}

void DockIconWidget::setIconPath(const QString &iconPath)
{
    m_iconPath = iconPath;
    update(); 
}

void DockIconWidget::onThemeChanged()
{
    update(); 
}

void DockIconWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    int iconSize;
    if (width() > height()) {
        iconSize = height();
    } else {
        iconSize = width();
    }
    
    QRect rect = this->rect();
    QRect iconRect((rect.width() - iconSize) / 2, 
                   (rect.height() - iconSize) / 2, 
                   iconSize, iconSize);
    
    auto icon = DIconTheme::findQIcon(m_iconPath);
    icon.paint(&painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::On);
}

