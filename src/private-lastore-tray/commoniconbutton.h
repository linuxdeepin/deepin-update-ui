// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#ifndef ICONBUTTON_H
#define ICONBUTTON_H
#include "tipswidget.h"

#include <QIcon>
#include <QWidget>
#include <QMap>
#include <QLabel>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

// 托盘图标按钮，负责显示静态图标和更新动画。
class CommonIconButton : public QWidget
{
public:
    enum State {
        Default,
        On,
        Off
    };

    Q_OBJECT
public:
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)

    explicit CommonIconButton(QWidget *parent = nullptr);

    void setStateIconMapping(QMap<State, QPair<QString, QString>> mapping);
    void setState(State state);
    void setActiveState(bool state);
    bool activeState() const { return m_activeState; }
    void setHoverEnable(bool enable);
    void setIconSize(const QSize &size);
    void setAllEnabled(bool enable);

    void startRotate();
    void stopRotate();

    inline void setRotation(qreal rotation) { m_rotation = rotation; update(); }
    inline qreal rotation() const { return m_rotation;}
    void startAnimation();
    void stopAnimation();

public Q_SLOTS:
    void setIcon(const QString &icon, const QString &fallback = "", const QString &suffix = ".svg");
    void setIcon(const QIcon &icon, QColor lightThemeColor = QColor(QColor::Invalid), QColor darkThemeColor = QColor(QColor::Invalid));
    void setHoverIcon(const QIcon &icon);

    void setClickable(bool clickable);

signals:
    void clicked();

protected:
    bool event(QEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    // 按当前状态重新加载图标资源。
    void refreshIcon();
    // 按主题和激活状态更新图标颜色。
    void updatePalette();
    // 在两张动画帧之间切换透明度。
    void switchIcon();

private:
    QTimer *m_refreshTimer;
    QIcon m_icon;
    QIcon m_hoverIcon;
    QPoint m_pressPos;
    bool m_clickable;
    bool m_hover;
    QMap<State, QPair<QString, QString>> m_fileMapping;
    State m_state;
    QColor m_lightThemeColor;
    QColor m_darkThemeColor;
    bool m_activeState;
    bool m_hoverEnable;
    QSize m_iconSize;
    qreal m_rotation;
    QPalette m_defaultPalette;

    QLabel *m_animLabel1;
    QLabel *m_animLabel2;
    QGraphicsOpacityEffect* m_effect1;
    QGraphicsOpacityEffect* m_effect2;
    QPropertyAnimation* m_fadeOutAnim;
    QPropertyAnimation* m_fadeInAnim;
    QTimer* m_animTimer;
    bool m_showingFirst;
};

#endif // DOCKICONBUTTON_H
