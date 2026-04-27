// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "tipswidget.h"
#include "privatelastoreplugin.h"
#include "common/common/dconfig_helper.h"

#include <QPainter>
#include <QAccessible>

Q_LOGGING_CATEGORY(dockUpdatePlugin, "org.deepin.dde.dock.update")
#define PADDING 4
#define SHUTDOWNUPDATESTATUS 5

TipsWidget::TipsWidget(QWidget *parent)
    : QFrame(parent)
    , m_managerInter(new UpdateDBusProxy(this))
{
    m_type = TipsWidget::MultiLine;
    if (m_managerInter) {
        connect(m_managerInter, &UpdateDBusProxy::JobListChanged, this, &TipsWidget::onRefreshJobList);
        connect(m_managerInter, &UpdateDBusProxy::DownloadLimitOnChangingChanged, this, &TipsWidget::onDownloadLimitChanged);
    }
}

void TipsWidget::setText(const QString &text)
{
    m_text = text;
    setFixedSize(fontMetrics().boundingRect(m_text).width() + 20, fontMetrics().boundingRect(m_text).height() + PADDING);
    update();

#ifndef QT_NO_ACCESSIBILITY
    if (accessibleName().isEmpty()) {
        QAccessibleEvent event(this, QAccessible::NameChanged);
        QAccessible::updateAccessibility(&event);
    }
#endif
}

void TipsWidget::setTextList(const QStringList &textList)
{
    m_type = TipsWidget::MultiLine;
    m_textList = textList;

    int width = 0;
    int height = 0;
    for (const QString& text : m_textList) {
        width = qMax(width, fontMetrics().boundingRect(text).width());
        height += fontMetrics().boundingRect(text).height();
    }

    setFixedSize(width + 20, height + PADDING);

    update();
}

bool TipsWidget::checkShutdownUpdate()
{
    QString systemUpgradeStatus = checkHasSystemUpdate(m_managerInter->updateStatus());
    if (systemUpgradeStatus != UPDATE_STATUS_Downloaded) {
        return false;
    }
    int lastoreStatus = DConfigHelper::instance()->getConfig("org.deepin.dde.lastore", "org.deepin.dde.lastore", "","lastore-daemon-status", 0).toInt();
    if (lastoreStatus == SHUTDOWNUPDATESTATUS) {
        m_textList.append(tr("Download complete"));
        m_textList.append(tr("Shutdown update"));
        return true;
    } else {
        return false;
    }
}

bool TipsWidget::checkRegularlyUpdate()
{
    QString systemUpgradeStatus = checkHasSystemUpdate(m_managerInter->updateStatus());
    if (systemUpgradeStatus != UPDATE_STATUS_Downloaded) {
        return false;
    }

    QString updateTime = DConfigHelper::instance()->getConfig("org.deepin.dde.lastore", "org.deepin.dde.lastore", "","update-time", "").toString();
    if (!updateTime.isEmpty()) {
        QDateTime dateTime = QDateTime::fromString(updateTime, Qt::ISODate);
        if (dateTime.isValid()) {
            QString formattedDateTime = dateTime.toString("HH:mm:ss");
            m_textList.append(tr("Download complete"));
            QString info = tr("Will upgrade at %1");
            m_textList.append(info.arg(formattedDateTime));
            return true;
        }
        return false;
    }
    return false;
}

bool TipsWidget::hasUnfinishedJob(const QList<QDBusObjectPath> &jobs) const
{
    for (const auto &job : jobs) {
        UpdateJobDBusProxy jobInter(job.path());
        if (!jobInter.isValid()) {
            continue;
        }

        QString curJobId = jobInter.id();
        if (curJobId != "prepare_dist_upgrade"
                && curJobId != "dist_upgrade"
                && curJobId != "update_source") {
            continue;
        }

        const QString status = jobInter.status();
        if (status != "failed" && status != "end") {
            return true;
        }
    }

    return false;
}

void TipsWidget::onSetUpdateProto(const QString& proto)
{
    m_proto = proto;
}

void TipsWidget::onSetUpdateSpeed(qlonglong speed)
{
    m_speed = speed;
}

void TipsWidget::onDownloadLimitChanged(bool value) {
    m_downloadLimitOnChanging = value;
}

void TipsWidget::onSetUpdateProgress(double progress)
{
    m_updateProgress = progress;
    refreshContent();
    update();
}

void TipsWidget::onSetBackUpProgress(double progress)
{
    m_backupProgress = progress;
    refreshContent();
    update();
}

void TipsWidget::refreshContent()
{
    m_textList.clear();
    if (m_downloadLimitOnChanging) {
        m_textList.append(tr("Changing download speed limit. Please wait"));
        return;
    }

    // 优先根据当前任务生成提示，没有任务时再根据总体状态生成提示。
    if (m_managerInter) {
        QList<QDBusObjectPath> jobList = m_managerInter->jobList();
        if (jobList.isEmpty() || !hasUnfinishedJob(jobList)) {
            // 没有任务时优先显示下载完成后的后续状态。
            if (checkShutdownUpdate()) return;
            if (checkRegularlyUpdate()) return;
            QString systemUpgradeStatus = checkHasSystemUpdate(m_managerInter->updateStatus());
            if (systemUpgradeStatus == UPDATE_STATUS_BackupFailed || systemUpgradeStatus == UPDATE_STATUS_UpgradeFailed) {
                m_textList.append(tr("Upgrade failed. Please check."));
                return;
            } else if (systemUpgradeStatus == UPDATE_STATUS_UpgradeSuccess) {
                m_textList.append(tr("Upgrade complete. Please reboot."));
                return;
            } else if (systemUpgradeStatus == UPDATE_STATUS_Downloaded) {
                m_textList.append(tr("Download complete. Please open Control Center to check."));
            } else if (systemUpgradeStatus == UPDATE_STATUS_UpdatesAvailable || systemUpgradeStatus == UPDATE_STATUS_Downloading||
                systemUpgradeStatus == UPDATE_STATUS_DownloadPaused || systemUpgradeStatus == UPDATE_STATUS_UpgradeReady
                || systemUpgradeStatus == UPDATE_STATUS_Upgrading) {
                m_textList.append(tr("New version available. Please check."));
                return;
            }
        } else {
            for (const auto &job : jobList) {
                const QString &jobPath = job.path();
                qInfo() << "Update job path:" << jobPath;
                UpdateJobDBusProxy jobInter(jobPath, this);
                if (!jobInter.isValid()) {
                    qWarning() << "Invalid update job";
                    continue;
                }
                QString curJobStatus = jobInter.status();
                QString curJobId = jobInter.id();
                QString curStatus;
                if (curJobStatus == "running" || curJobStatus == "ready") {
                    // 按任务类型生成当前提示内容。
                    if (curJobId == "backup" && m_backupJobInter) {
                        QString curProgress = tr("Current upgrade progress");
                        curStatus = tr("Backing up");
                        m_textList.append(curStatus);
                        m_textList.append(QString("%1: %2%")
                            .arg(curProgress)
                            .arg(QString::number(qRound(m_backupProgress / 0.01))));
                    } else if (curJobId == "prepare_dist_upgrade" && m_updateJobInter) {
                        // 下载阶段显示进度、速度和协议。
                        curStatus = tr("Downloading");
                        QString curProgress = tr("Current download progress");
                        QString curSpeed = tr("Current speed");
                        m_textList.append(QString("%1, %2: %3%")
                                          .arg(curStatus)
                                          .arg(curProgress)
                                          .arg(QString::number(qRound(m_updateProgress / 0.01))));
                        m_textList.append(QString("%1: %2(%3)")
                                .arg(curSpeed)
                                .arg(regulateSpeed())
                                .arg(m_proto));
                    } else if (curJobId == "dist_upgrade" && m_updateJobInter) {
                        // 安装阶段仅显示升级进度。
                        QString curProgress = tr("Current upgrade progress");
                        curStatus = tr("Installing");
                        m_textList.append(curStatus);
                        m_textList.append(QString("%1: %2%")
                                    .arg(curProgress)
                                    .arg(QString::number(qRound(m_updateProgress / 0.01))));
                    } else if (curJobId == "update_source" && m_updateJobInter) {
                        QString systemUpgradeStatus = checkHasSystemUpdate(m_managerInter->updateStatus());
                        if (systemUpgradeStatus == UPDATE_STATUS_UpdatesAvailable || systemUpgradeStatus == UPDATE_STATUS_Downloading||
                            systemUpgradeStatus == UPDATE_STATUS_DownloadPaused || systemUpgradeStatus == UPDATE_STATUS_UpgradeReady
                            || systemUpgradeStatus == UPDATE_STATUS_Upgrading || systemUpgradeStatus == UPDATE_STATUS_Downloaded) {
                            // 更新源检查任务存在时仍保持新版本提示。
                            m_textList.append(tr("New version available. Please check."));
                        }
                    }
                } else {
                    // 非 failed/end 且暂不展示进度的任务先跳过。
                    continue;
                }
            }
        }
    }
}

QString TipsWidget::regulateSpeed()
{
    QString unitName;
    double regulatedProcess;
    bool needDecimal = false; // 控制是否保留一位小数。

    if (m_speed >= 1024 * 1024) {
        regulatedProcess = static_cast<double>(m_speed) / (1024 * 1024);
        regulatedProcess = qRound(regulatedProcess * 10) / 10.0;
        needDecimal = true;
        unitName = "MB/s";
    } else if (m_speed >= 1024) {
        regulatedProcess = static_cast<double>(m_speed) / 1024;
        regulatedProcess = qRound(regulatedProcess);
        unitName = "KB/s";
    } else {
        regulatedProcess = static_cast<double>(m_speed);
        unitName = "B/s";
    }
    return QString("%1%2").arg(regulatedProcess, 0, 'f', needDecimal ? 1 : 0).arg(unitName);
}

void TipsWidget::onRefreshJobList(const QList<QDBusObjectPath> &jobs)
{
    qInfo() << "Update job list changed";

    for (const auto &job : jobs) {
        const QString &jobPath = job.path();
        UpdateJobDBusProxy jobInter(jobPath, this);

        // 记录任务代理并监听进度变化。
        const QString &id = jobInter.id();
        if (id == "dist_upgrade" || id == "prepare_dist_upgrade") {
            if (m_updateJobInter) {
                disconnect(m_updateJobInter, nullptr, this, nullptr);
                m_updateJobInter->deleteLater();
            }
            m_updateJobInter = new UpdateJobDBusProxy(jobPath, this);
            connect(m_updateJobInter, &UpdateJobDBusProxy::ProgressChanged, this, &TipsWidget::onSetUpdateProgress);
            connect(m_updateJobInter, &UpdateJobDBusProxy::ProtoChanged, this, &TipsWidget::onSetUpdateProto);
            connect(m_updateJobInter, &UpdateJobDBusProxy::SpeedChanged, this, &TipsWidget::onSetUpdateSpeed);
        } else if (id == "backup") {
            m_backupJobInter = new UpdateJobDBusProxy(jobPath, this);
            connect(m_backupJobInter, &UpdateJobDBusProxy::ProgressChanged, this, &TipsWidget::onSetBackUpProgress);
        }
    }
}
void TipsWidget::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    // 按当前文本内容绘制提示气泡。
    QPainter painter(this);
    painter.setPen(QPen(palette().brightText(), 1));

    QTextOption option;
    option.setAlignment(Qt::AlignCenter);

    switch (m_type) {
    case SingleLine: {
        painter.drawText(rect(), m_text, option);
    }
        break;
    case MultiLine: {
        if (m_textList.size() == 0) {
            m_textList.append(tr("Enterprise Upgrade Management System"));
        }

        int x = rect().x();
        int y = rect().y();
        if (m_textList.size() != 1) {
            x += 10;
            option.setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        }

        int width = 0;
        int height = 0;
        for (const QString& text : m_textList) {
            int lineHeight = fontMetrics().boundingRect(text).height();
            painter.drawText(QRect(x, y, rect().width(), lineHeight), text, option);
            y += lineHeight;

            width = qMax(width, fontMetrics().boundingRect(text).width());
            height += fontMetrics().boundingRect(text).height();
        }
        setFixedSize(width + 20, height + PADDING);
    } break;
    }
}

bool TipsWidget::event(QEvent *event)
{
    if (event->type() == QEvent::FontChange) {
        switch (m_type) {
        case SingleLine:
        {
            setText(m_text);
            break;
        }
        case MultiLine:
        {
            setTextList(m_textList);
            break;
        }
        }
    } else if (event->type() == QEvent::MouseButtonRelease
               && static_cast<QMouseEvent *>(event)->button() == Qt::RightButton) {
        return true;
    }
    return QFrame::event(event);
}
