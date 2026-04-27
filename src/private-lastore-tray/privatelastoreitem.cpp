// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "privatelastoreitem.h"
#include "constants.h"
#include "tipswidget.h"

#include <DDBusSender>

#include <QDBusConnection>
#include <QIcon>
#include <QVBoxLayout>
#include <QMouseEvent>

PrivateLastoreItem::PrivateLastoreItem(QWidget* parent)
    : QWidget(parent)
    , m_tipsLabel(new TipsWidget(this))
    , m_icon(new CommonIconButton(this))
    , m_managerInter(new UpdateDBusProxy(this))
    , m_controlCenterInterface(new QDBusInterface("com.deepin.dde.ControlCenter", "/com/deepin/dde/ControlCenter", "com.deepin.dde.ControlCenter", QDBusConnection::sessionBus(), this))
{
    m_tipsLabel->setVisible(false);
    auto vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(0);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->addWidget(m_icon, 0, Qt::AlignCenter);
    m_icon->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    m_icon->setIcon(QIcon(":resources/private-lastore-sleep_16px.svg"));
    m_icon->setContentsMargins(0, 0, 0, 0);
    onStartAnimation("");
    connect(m_managerInter, &UpdateDBusProxy::JobListChanged, this, &PrivateLastoreItem::onRefreshIcon);
    connect(m_managerInter, &UpdateDBusProxy::UpdateStatusChanged, this, &PrivateLastoreItem::onStartAnimation);
}

QWidget* PrivateLastoreItem::tipsWidget()
{
    m_tipsLabel->refreshContent();
    return m_tipsLabel;
}

void PrivateLastoreItem::refreshTrayIcon()
{
    m_icon->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    m_icon->setIcon(QIcon(":resources/private-lastore-sleep_16px.svg"));
    m_icon->setContentsMargins(0, 0, 0, 0);
}

void PrivateLastoreItem::onRefreshIcon(const QList<QDBusObjectPath> &jobs)
{
    qInfo() << "Update job list changed";

    for (const auto &job : jobs) {
        const QString &jobPath = job.path();
        qInfo() << "Update job path:" << jobPath;
        UpdateJobDBusProxy jobInter(jobPath, this);
        if (!jobInter.isValid()) {
            qWarning() << "Invalid update job";
            continue;
        }

        const QString &id = jobInter.id();
        qInfo() << "Update job id:" << id;
        // 下载或安装任务存在时显示更新动画。
        if (id == "dist_upgrade" || id == "prepare_dist_upgrade") {
            m_icon->startAnimation();
            return;
        }
    }
    m_icon->setIcon(QIcon(":resources/private-lastore-sleep_16px.svg"));
}

void PrivateLastoreItem::onStartAnimation(const QString &updateStatus)
{
    Q_UNUSED(updateStatus)

    // 从更新状态中提取 system_upgrade 状态控制动画。
    QString systemUpgradeStatus = checkHasSystemUpdate(m_managerInter->updateStatus());
    if (systemUpgradeStatus == UPDATE_STATUS_UpdatesAvailable || systemUpgradeStatus == UPDATE_STATUS_Downloading ||
        systemUpgradeStatus == UPDATE_STATUS_DownloadPaused || systemUpgradeStatus == UPDATE_STATUS_UpgradeReady
        || systemUpgradeStatus == UPDATE_STATUS_Upgrading || systemUpgradeStatus == UPDATE_STATUS_Downloaded)
        m_icon->startAnimation();
    else
        m_icon->stopAnimation();
}

void PrivateLastoreItem::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    // 按 dock 停靠方向将条目约束为正方形区域。
    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
    if (position == Dock::Bottom || position == Dock::Top) {
        setMaximumWidth(height());
        setMaximumHeight(QWIDGETSIZE_MAX);
    } else {
        setMaximumHeight(width());
        setMaximumWidth(QWIDGETSIZE_MAX);
    }
}

void PrivateLastoreItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_controlCenterInterface) {
        // 左键点击后打开控制中心更新模块。
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
