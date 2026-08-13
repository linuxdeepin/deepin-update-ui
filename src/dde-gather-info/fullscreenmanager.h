// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef LIGHTERSCREENMANAGER_H
#define LIGHTERSCREENMANAGER_H

#include <QObject>
#include <QScreen>
#include <QMap>
#include <QWidget>

class FullScreenManager : public QObject
{
    Q_OBJECT
public:
    explicit FullScreenManager(std::function<QWidget *(QScreen *)> function, QObject *parent = nullptr);

    typedef QPointer<QScreen> ScreenPtr;

Q_SIGNALS:
    void copyModeChanged(bool isCopyMode);

public Q_SLOTS:
    void screenCountChanged();
    void handleScreenChanged();
    bool isCopyMode() const;
    void checkCopyModeChanged();
    void handleCopyModeChanged(bool);


private:
    std::function<QWidget *(QScreen *)> m_registerFun;
    QMap<QWidget *, ScreenPtr> m_screenContents;
    bool m_copyModeFlag;
};

#endif // LIGHTERSCREENMANAGER_H
