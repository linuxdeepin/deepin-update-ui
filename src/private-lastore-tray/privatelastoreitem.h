// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PRIVATELASTOREITEM_H
#define PRIVATELASTOREITEM_H

#include "commoniconbutton.h"
#include "common/dbus/updatedbusproxy.h"
#include "common/dbus/updatejobdbusproxy.h"
#include "tipswidget.h"

#include <QWidget>
#include <QDBusAbstractInterface>

namespace Dock {
class TipsWidget;
}

class AirplaneModeApplet;
class PrivateLastoreItem : public QWidget
{
    Q_OBJECT

public:
    explicit PrivateLastoreItem(QWidget *parent = nullptr);

    QWidget *tipsWidget();

protected:
    void resizeEvent(QResizeEvent *e);
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onRefreshIcon(const QList<QDBusObjectPath> &jobs);

private:
    TipsWidget *m_tipsLabel;
    CommonIconButton *m_icon;
    UpdateDBusProxy *m_managerInter = nullptr;
    QDBusInterface *m_controlCenterInterface = nullptr;
};

#endif // PRIVATELASTOREITEM_H
