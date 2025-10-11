// SPDX-FileCopyrightText: 2011 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FULLSCREENBACKGROUND_H
#define FULLSCREENBACKGROUND_H

#include <QWidget>
#include <QSharedPointer>
#include <QLoggingCategory>
#include <QSize>
#include <QPointer>

#include "abstractfullbackgroundinterface.h"

Q_DECLARE_LOGGING_CATEGORY(DDE_SS)

class FullScreenBackground : public QWidget, public AbstractFullBackgroundInterface
{
    Q_OBJECT
    Q_PROPERTY(bool contentVisible READ contentVisible)

public:
    explicit FullScreenBackground(QWidget *parent = nullptr);
    ~FullScreenBackground() override;

    bool contentVisible() const;
    void unBindContent();
    static void setContent(QWidget *const w);

public slots:
    void updateBackground(const QString &path);
    void updateBlurBackground(const QString &path);
    void setScreen(QPointer<QScreen> screen, bool isVisible = true) override;

signals:
    void blurImageReturned();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void moveEvent(QMoveEvent *event) Q_DECL_OVERRIDE;

private:
    void paintEvent(QPaintEvent *e) Q_DECL_OVERRIDE;
    void enterEvent(QEnterEvent *event) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void updateScreen(QPointer<QScreen> screen);
    void updateGeometry();
    bool isPicture(const QString &file);
    const QPixmap& getPixmap(int type);
    QSize trueSize() const;
    void addPixmap(const QPixmap &pixmap, const int type);
    static void updatePixmap();
    bool contains(int type);
    static void updateCurrentFrame(FullScreenBackground *frame);
    bool getScaledBlurImage(const QString &originPath, QString &scaledPath);

protected:
    static QPointer<QWidget> currentContent;
    static QList<FullScreenBackground *> frameList;
    static QPointer<FullScreenBackground> currentFrame;

    void handleBackground(const QString &path, int type);
    static QString sizeToString(const QSize &size);

private:
    static QString blurBackgroundPath;                         // 模糊背景图片路径
    static QMap<QString, QPixmap> blurBackgroundCacheMap;

    QPointer<QScreen> m_screen;
    bool m_useSolidBackground;
    bool m_getBlurImageSuccess;
};

#endif // FULLSCREENBACKGROUND_H
