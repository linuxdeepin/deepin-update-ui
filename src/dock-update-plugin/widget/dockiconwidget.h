// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DOCKICONWIDGET_H
#define DOCKICONWIDGET_H

#include <QWidget>
#include <DDciIcon>

DCORE_USE_NAMESPACE
DGUI_USE_NAMESPACE

class DockIconWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DockIconWidget(QWidget *parent = nullptr);
    ~DockIconWidget();
    void setIconPath(const QString &iconPath);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    
private slots:
    void onThemeChanged();
    
private:
    QString m_iconPath;
};

#endif // DOCKICONWIDGET_H

