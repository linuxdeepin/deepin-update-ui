// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "common.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QObject>

#ifndef UPDATE_DATA_STRUCTS_H
#define UPDATE_DATA_STRUCTS_H

using namespace dcc::update::common;

/**
 * @brief 空闲下载配置
 */
struct IdleDownloadConfig {
    bool idleDownloadEnabled = true;
    QString beginTime = "17:00";
    QString endTime = "20:00";

    static IdleDownloadConfig toConfig(const QByteArray& configStr)
    {
        IdleDownloadConfig config;
        QJsonParseError jsonParseError;
        const QJsonDocument doc = QJsonDocument::fromJson(configStr, &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError || doc.isEmpty()) {
            qWarning() << "Parse idle download config failed: " << jsonParseError.errorString();
            return config;
        }

        QJsonObject obj = doc.object();
        config.idleDownloadEnabled = obj.contains("IdleDownloadEnabled") ? obj.value("IdleDownloadEnabled").toBool() : true;
        config.beginTime = obj.contains("BeginTime") ? obj.value("BeginTime").toString() : "17:00";
        config.endTime = obj.contains("EndTime") ? obj.value("EndTime").toString() : "20:00";
        return config;
    }

    QByteArray toJson() const
    {
        QJsonObject obj;
        obj.insert("IdleDownloadEnabled", idleDownloadEnabled);
        obj.insert("BeginTime", beginTime);
        obj.insert("EndTime", endTime);

        QJsonDocument doc;
        doc.setObject(obj);
        return doc.toJson();
    }

    bool operator==(const IdleDownloadConfig& config) const
    {
        return config.idleDownloadEnabled == idleDownloadEnabled && config.beginTime == beginTime && config.endTime == endTime;
    }
};

/**
 * @brief 下载限速配置
 */
struct DownloadSpeedLimitConfig {
    bool downloadSpeedLimitEnabled = false;
    QString limitSpeed = "10240";

    QString toJson() const
    {
        QJsonObject obj;
        obj.insert("DownloadSpeedLimitEnabled", downloadSpeedLimitEnabled);
        obj.insert("LimitSpeed", limitSpeed);
        QJsonDocument doc;
        doc.setObject(obj);
        return doc.toJson();
    }

    static DownloadSpeedLimitConfig fromJson(const QByteArray& configStr)
    {
        DownloadSpeedLimitConfig config;
        QJsonParseError jsonParseError;
        const QJsonDocument doc = QJsonDocument::fromJson(configStr, &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError || doc.isEmpty()) {
            qWarning() << "Parse download speed limit config failed: " << jsonParseError.errorString();
            return config;
        }

        QJsonObject obj = doc.object();
        config.downloadSpeedLimitEnabled = obj.contains("DownloadSpeedLimitEnabled") ? obj.value("DownloadSpeedLimitEnabled").toBool() : false;
        config.limitSpeed = obj.contains("LimitSpeed") ? obj.value("LimitSpeed").toString() : "10240";

        return config;
    }
};

struct LastoreDaemonUpdateStatus {
    UpdatesStatus backupStatus = UpdatesStatus::Default;
    UpdateErrorType backupError = UpdateErrorType::NoError;
    QMap<UpdateType, UpdatesStatus> m_statusMap;
    int triggerBackingUpType = 0;
    int backupFailedType = 0;

    static UpdatesStatus string2Status(const QString& strStatus)
    {
        const static QMap<QString, UpdatesStatus> statusMap = {
            { "noUpdate", Default },
            { "notDownload", UpdatesAvailable },
            { "isDownloading", Downloading },
            { "downloadPause", DownloadPaused },
            { "downloadFailed", DownloadFailed },
            { "downloaded", Downloaded },
            { "backingUp", BackingUp },
            { "backupFailed", BackupFailed },
            { "hasBackedUp", BackupSuccess },
            { "upgradeReady", UpgradeReady },
            { "upgrading", Upgrading },
            { "upgradeFailed", UpgradeFailed },
            { "needReboot", UpgradeSuccess },
        };

        return statusMap.value(strStatus, UpdatesStatus::Default);
    }

    static UpdateErrorType string2Error(const QString& strError)
    {
        const static QMap<QString, UpdateErrorType> errorMap = {
            { "noError", UpdateErrorType::NoError },
            { "canNotBackup", UpdateErrorType::CanNotBackup },
            { "otherError", UpdateErrorType::BackupFailedUnknownReason }
        };

        return errorMap.value(strError, UpdateErrorType::NoError);
    }

    /**
     * @brief 从json字符串中解析出更新状态
     *
     * @param status 格式如下:
        {
            "ABStatus":"backupFailed",
            "ABError":"canNotBackup",
            "TriggerBackingUpType": 1,
            "BackupFailedType":1,
            "UpdateStatus":{
                "security_upgrade":"notDownload",
                "system_upgrade":"notDownload",
                "unknown_upgrade":"downloaded"
            }
        }
     */
    static LastoreDaemonUpdateStatus fromJson(const QByteArray& status)
    {
        LastoreDaemonUpdateStatus lastoreDaemonUpdateStatus;
        QJsonParseError jsonParseError;
        const QJsonDocument doc = QJsonDocument::fromJson(status, &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError || doc.isEmpty()) {
            // qCWarning(DCC_UPDATE) << "Parse update status string failed: " << jsonParseError.errorString();
        }

        QJsonObject obj = doc.object();
        if (!obj.contains("UpdateStatus")) {
            // qCWarning(DCC_UPDATE) << "Can not find UpdateStatus key in update status string";
        }

        QJsonObject updateStatusObj = obj.value("UpdateStatus").toObject();
        if (updateStatusObj.contains(SYSTEM_UPGRADE_TYPE_STRING)) {
            lastoreDaemonUpdateStatus.m_statusMap.insert(SystemUpdate, string2Status(updateStatusObj.value(SYSTEM_UPGRADE_TYPE_STRING).toString()));
        }

        if (updateStatusObj.contains(SECURITY_UPGRADE_TYPE_STRING)) {
            lastoreDaemonUpdateStatus.m_statusMap.insert(SecurityUpdate, string2Status(updateStatusObj.value(SECURITY_UPGRADE_TYPE_STRING).toString()));
        }

        if (updateStatusObj.contains(UNKNOWN_UPGRADE_STRING)) {
            lastoreDaemonUpdateStatus.m_statusMap.insert(UnknownUpdate, string2Status(updateStatusObj.value(UNKNOWN_UPGRADE_STRING).toString()));
        }

        if (obj.contains("ABStatus")) {
            lastoreDaemonUpdateStatus.backupStatus = string2Status(obj.value("ABStatus").toString());
        }

        if (obj.contains("ABError")) {
            lastoreDaemonUpdateStatus.backupError = string2Error(obj.value("ABError").toString());
        }

        if (obj.contains("TriggerBackingUpType")) {
            lastoreDaemonUpdateStatus.triggerBackingUpType = obj.value("TriggerBackingUpType").toInt(0);
        }

        if (obj.contains("BackupFailedType")) {
            lastoreDaemonUpdateStatus.backupFailedType = obj.value("BackupFailedType").toInt(0);
        }

        return lastoreDaemonUpdateStatus;
    }
};

const int INSTALLATION_IS_READY = 1 << 0; // 是否可以安装
const int UPDATE_IS_DISABLED = 1 << 1; // 更新功能是否被禁用
struct LastoreDaemonDConfigStatusHelper {
    static bool isDownloadComplete(int status) { return status & INSTALLATION_IS_READY; }
    static bool isUpdateDisabled(int status) { return status & UPDATE_IS_DISABLED; }
};

#endif // UPDATE_DATA_STRUCTS_H
