// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "privatelastoreitem.h"
#include "constants.h"
#include "tipswidget.h"

#include <DDBusSender>
#include <DGuiApplicationHelper>
#include <DIconTheme>

#include <QDBusConnection>
#include <QDBusReply>
#include <QIcon>
#include <QJsonDocument>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QMouseEvent>

DGUI_USE_NAMESPACE

PrivateLastoreItem::PrivateLastoreItem(QWidget* parent)
    : QWidget(parent)
    , m_tipsLabel(new TipsWidget(this))
    , m_icon(new CommonIconButton(this))
    , m_managerInter(new UpdateDBusProxy(this))
    , m_controlCenterInterface(new QDBusInterface("com.deepin.dde.ControlCenter", "/com/deepin/dde/ControlCenter", "com.deepin.dde.ControlCenter", QDBusConnection::sessionBus()))
{
    m_tipsLabel->setVisible(false);
    auto vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(0);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(m_icon, 0, Qt::AlignCenter);
    m_icon->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    m_icon->setIcon(QIcon(":resources/private-lastore-sleep_16px.svg"));
    m_icon->setContentsMargins(0, 0, 0, 0);
    connect(m_managerInter, &UpdateDBusProxy::JobListChanged, this, &PrivateLastoreItem::onRefreshIcon);
}

QWidget* PrivateLastoreItem::tipsWidget()
{
    m_tipsLabel->refreshContent();
    return m_tipsLabel;
}

void PrivateLastoreItem::onRefreshIcon(const QList<QDBusObjectPath> &jobs)
{
    qInfo() << "Job list changed";

    for (const auto &job : jobs) {
        const QString &jobPath = job.path();
        qInfo() << "Path : " << jobPath;
        UpdateJobDBusProxy jobInter(jobPath, this);
        if (!jobInter.isValid()) {
            qWarning() << "Job is invalid";
            continue;
        }

        const QString &id = jobInter.id();
        qInfo() << "Job id : " << id;
        if (id == "dist_upgrade" || id == "prepare_dist_upgrade") {
            m_icon->setIcon(QIcon(":resources/private-lastore-active_16px.svg"));
            return;
        }
    }
    m_icon->setIcon(QIcon(":resources/private-lastore-sleep_16px.svg"));
}

void PrivateLastoreItem::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
    if (position == Dock::Bottom || position == Dock::Top) {
        setMaximumWidth(height());
        setMaximumHeight(QWIDGETSIZE_MAX);
    } else {
        setMaximumHeight(width());
        setMaximumWidth(QWIDGETSIZE_MAX);
    }
}

void PrivateLastoreItem::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && m_controlCenterInterface) {
        DDBusSender()
            .service("org.deepin.dde.ControlCenter1")
            .interface("org.deepin.dde.ControlCenter1")
            .path("/org/deepin/dde/ControlCenter1")
            .method("ShowModule")
            .arg(QString("update"))
            .call();
    }
    QWidget::mouseReleaseEvent(event);
}

