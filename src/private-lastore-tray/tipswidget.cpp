// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "tipswidget.h"
#include "privatelastoreplugin.h"
#include "common/common/dconfig_helper.h"

#include <QPainter>
#include <QAccessible>
#include <QFile>
#include <QTextDocument>

Q_LOGGING_CATEGORY(dockUpdatePlugin, "org.deepin.dde.dock.update")
#define PADDING 4
#define SHUTDOWNUPDATESTATUS 5
static const int BACKUP_START_PROGRESS = 20;
static const int BACKUP_SUCCESS_PROGRESS = 50;

TipsWidget::TipsWidget(QWidget *parent)
    : QFrame(parent)
    , m_managerInter(new UpdateDBusProxy(this))
{
    m_type = TipsWidget::MultiLine;
    if (m_managerInter) {
        connect(m_managerInter, &UpdateDBusProxy::JobListChanged, this, &TipsWidget::onRefreshJobList);
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
    int lastoreStatus = DConfigHelper::instance()->getConfig("org.deepin.dde.lastore", "org.deepin.dde.lastore", "","lastore-daemon-status", 0).toInt();
    if (lastoreStatus == SHUTDOWNUPDATESTATUS) {
        m_textList.append(tr("Download complete"));
        m_textList.append(tr("shutdown update"));
        return true;
    } else {
        return false;
    }
}

bool TipsWidget::checkRegularlyUpdate()
{
    QString updateTime = DConfigHelper::instance()->getConfig("org.deepin.dde.lastore", "org.deepin.dde.lastore", "","update-time", "").toString();

    if (!updateTime.isEmpty()) {
        QDateTime dateTime = QDateTime::fromString(updateTime, Qt::ISODate);
        if (dateTime.isValid()) {
            QString formattedDateTime = dateTime.toString("HH:mm:ss");
            m_textList.append(tr("Download complete"));
            QString info = tr("will upgrade at %1");
            m_textList.append(info.arg(formattedDateTime));
            return true;
        }
        return false;
    }
    return false;
}

void TipsWidget::onUpdatePropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties, const QStringList& invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties)
    qCInfo(dockUpdatePlugin) << "xiongbo111 onUpdatePropertiesChanged " << interfaceName << " changedProperties " << changedProperties;

    if (interfaceName == "org.deepin.dde.Lastore1.Job") {
        if (changedProperties.contains("Speed")) {
            m_speed = changedProperties.value("Speed").toULongLong();
        }

        if (changedProperties.contains("Proto")) {
            m_proto = changedProperties.value("Proto").toString();
        }
    }
    refreshContent();
    update();
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
        m_textList.append(tr("Is changing download speed limit. Please wait"));
        return;
    }
    if (m_managerInter) {
        QList<QDBusObjectPath> jobList = m_managerInter->jobList();
        if (jobList.isEmpty()) {
            //当前无任务，需检测是否为任务执行完成状态
            //检查当前是否为关机更新状态
            if (checkShutdownUpdate()) return;
            //检查当前是否为定时更新状态
            if (checkRegularlyUpdate()) return;
            //检查是否有可更新内容
            QString systemUpgradeStatus = checkHasSystemUpdate(m_managerInter->updateStatus());
            if (systemUpgradeStatus == "downloaded") {
                m_textList.append(tr("Download complete.Please go to control-center to check."));
            } else if (systemUpgradeStatus == "notDownload" || systemUpgradeStatus == "isDownloading"||
                systemUpgradeStatus == "downloadPause" || systemUpgradeStatus == "upgradeReady"
                || systemUpgradeStatus == "upgrading") {
                m_textList.append(tr("Has new version.Please go to check."));
                return;
            }
        } else {
            for (const auto &job : jobList) {
                const QString &jobPath = job.path();
                qInfo() << "Path : " << jobPath;
                UpdateJobDBusProxy jobInter(jobPath, this);
                if (!jobInter.isValid()) {
                    qWarning() << "Job is invalid";
                    continue;
                }
                QString curJobStatus = jobInter.status();
                QString curJobId = jobInter.id();
                QString curStatus;
                if (curJobStatus == "running" || curJobStatus == "ready") {
                    //当前有任务在进行
                    if (curJobId == "backup" && m_backupJobInter) {
                        QString curProgress = tr("current upgrade process");
                        curStatus = tr("backing up");
                        m_textList.append(curStatus);
                        m_textList.append(QString("%1: %2%")
                            .arg(curProgress)
                            .arg(QString::number(qRound(m_backupProgress / 0.01))));
                    } else if (curJobId == "prepare_dist_upgrade" && m_updateJobInter) {
                        //任务类型为下载任务，展示下载信息
                        curStatus = tr("download");
                        QString curProgress = tr("current download process");
                        QString curSpeed = tr("current speed");
                        m_textList.append(QString("%1, %2: %3%")
                                          .arg(curStatus)
                                          .arg(curProgress)
                                          .arg(QString::number(qRound(m_updateProgress / 0.01))));
                        m_textList.append(QString("%1: %2(%3)")
                                .arg(curSpeed)
                                .arg(regulateSpeed())
                                .arg(m_proto));
                    } else if (curJobId == "dist_upgrade" && m_updateJobInter) {
                        //任务类型为更新任务，展示更新信息
                        QString curProgress = tr("current upgrade process");
                        curStatus = tr("install");
                        m_textList.append(curStatus);
                        m_textList.append(QString("%1: %2%")
                                    .arg(curProgress)
                                    .arg(QString::number(qRound(m_updateProgress / 0.01))));
                    } else if (curJobId == "update_source" && m_updateJobInter) {
                            QString systemUpgradeStatus = checkHasSystemUpdate(m_managerInter->updateStatus());
                            if (systemUpgradeStatus == "notDownload" || systemUpgradeStatus == "isDownloading"||
                                systemUpgradeStatus == "downloadPause" || systemUpgradeStatus == "upgradeReady"
                                || systemUpgradeStatus == "upgrading" || systemUpgradeStatus == "downloaded") {
                        // 由于每次打开控制中心都会触发检查更新，此时有可能已经是downloaded状态且也有job
                            m_textList.append(tr("Has new version.Please go to check."));
                        }
                    }
                } else {
                    //无running任务状态下没有监控任务状态的必要，展示标题文案，防止出现空白气泡
                    m_speed = 0.0;
                    if (m_textList.length() != 0)
                        m_textList.clear();
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
    bool needDecimal = false; //是否需要保留一位小数

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

void TipsWidget::onDownloadLimitChanged(bool value) {
    m_downloadLimitOnChanging = value;
}

void TipsWidget::onRefreshJobList(const QList<QDBusObjectPath> &jobs)
{
    qInfo() << "Job list changed";

    for (const auto &job : jobs) {
        const QString &jobPath = job.path();
        UpdateJobDBusProxy jobInter(jobPath, this);

        //检测到有更新任务
        const QString &id = jobInter.id();
        if (id == "dist_upgrade" || id == "prepare_dist_upgrade") {
            m_updateJobInter = new UpdateJobDBusProxy(jobPath, this);
            connect(m_updateJobInter, &UpdateJobDBusProxy::ProgressChanged, this, &TipsWidget::onSetUpdateProgress);
        } else if (id == "backup") {
            m_backupJobInter = new UpdateJobDBusProxy(jobPath, this);
            connect(m_backupJobInter, &UpdateJobDBusProxy::ProgressChanged, this, &TipsWidget::onSetBackUpProgress);
        }
    }
}


/**
 * @brief TipsWidget::paintEvent 任务栏插件提示信息绘制
 * @param event
 */
void TipsWidget::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

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
            m_textList.append(tr("Enterprise-level Upgrade Management System"));
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
