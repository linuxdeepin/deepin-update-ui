// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef TIPSWIDGET_H
#define TIPSWIDGET_H

#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include "common/dbus/updatedbusproxy.h"
#include "common/dbus/updatejobdbusproxy.h"

#define UPDATE_STATUS_Default "noUpdate"
#define UPDATE_STATUS_UpdatesAvailable "notDownload"
#define UPDATE_STATUS_Downloading "isDownloading"
#define UPDATE_STATUS_DownloadPaused "downloadPause"
#define UPDATE_STATUS_DownloadFailed "downloadFailed"
#define UPDATE_STATUS_Downloaded "downloaded"
#define UPDATE_STATUS_BackingUp "backingUp"
#define UPDATE_STATUS_BackupFailed "backupFailed"
#define UPDATE_STATUS_BackupSuccess "hasBackedUp"
#define UPDATE_STATUS_UpgradeReady "upgradeReady"
#define UPDATE_STATUS_Upgrading "upgrading"
#define UPDATE_STATUS_UpgradeFailed "upgradeFailed"
#define UPDATE_STATUS_UpgradeSuccess "needReboot"

// 从更新状态 JSON 中提取 system_upgrade 字段。
inline QString checkHasSystemUpdate(const QString& updateStatus)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(updateStatus.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return "";
    }
    if (!doc.isObject()) {
        qWarning() << "JSON is not an object";
        return "";
    }
    QJsonObject rootObj = doc.object();
    QJsonObject updateStatusObj = rootObj.value("UpdateStatus").toObject();
    QString systemUpgradeStatus = updateStatusObj.value("system_upgrade").toString();
    return systemUpgradeStatus;
}

// 托盘提示气泡，按任务状态生成显示文案。
class TipsWidget : public QFrame
{
    Q_OBJECT
    enum ShowType
    {
        SingleLine,
        MultiLine
    };
public:
    explicit TipsWidget(QWidget *parent = nullptr);

    void setText(const QString &text);
    void setTextList(const QStringList &textList);
    void refreshContent();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    // 判断是否处于关机更新状态。
    bool checkShutdownUpdate();
    // 判断是否已设置定时升级。
    bool checkRegularlyUpdate();
    // 判断任务列表中是否仍有未进入 failed/end 的任务。
    bool hasUnfinishedJob(const QList<QDBusObjectPath> &jobs) const;
    // 将下载速度格式化为展示文本。
    QString regulateSpeed();

private slots:
    void onRefreshJobList(const QList<QDBusObjectPath> &jobs);
    void onSetUpdateProgress(double progress);
    void onSetBackUpProgress(double progress);
    void onSetUpdateProto(const QString &proto);
    void onSetUpdateSpeed(qlonglong speed);
    void onDownloadLimitChanged(bool value);

private:
    UpdateDBusProxy *m_managerInter = nullptr;
    UpdateJobDBusProxy *m_updateJobInter = nullptr;
    UpdateJobDBusProxy *m_backupJobInter = nullptr;
    QString m_text;
    QStringList m_textList;
    ShowType m_type;
    qlonglong m_speed = 0;
    QString m_proto = "";
    bool m_downloadLimitOnChanging = false;
    double m_updateProgress = 0.0;
    double m_backupProgress = 0.0;
};

#endif // TIPSWIDGET_H
