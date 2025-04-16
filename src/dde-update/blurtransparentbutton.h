// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INHIBITBUTTON_H
#define INHIBITBUTTON_H

#include <QWidget>
#include <QIcon>

class QLabel;
class BlurTransparentButton : public QWidget
{
    Q_OBJECT
public:
    enum State {
        Enter = 0X0,
        Leave = 0x1,
        Press = 0x2,
        Release = 0x4,
        Checked = 0x08
    };
    BlurTransparentButton(const QString &text, QWidget *parent = nullptr);
    ~BlurTransparentButton();

    void setText(const QString &text);
    QString text();
    void setNormalPixmap(const QPixmap &normalPixmap);
    QPixmap normalPixmap();
    void setHoverPixmap(const QPixmap &hoverPixmap);
    QPixmap hoverPixmap();
    void setRadius(int radius);
    void enableHighLightFocus(bool enable) { m_enableHighLightFocus = enable; }


Q_SIGNALS:
    void clicked();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    State m_state;
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    int m_radius;
    QPixmap m_normalPixmap;
    QPixmap m_hoverPixmap;
    bool m_enableHighLightFocus;
};

#endif // INHIBITBUTTON_H
