// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATELOGHELPER_H
#define UPDATELOGHELPER_H

#include "updateiteminfo.h"
#include "utils.h"

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <DSingleton>

const int LogTypeSystem = 1; // 系统更新
const int LogTypeSecurity = 2; // 安全更新

static QString getLanguageType()
{
    static QString languageType;

    if (!languageType.isEmpty())
        return languageType;

    QStringList language = QLocale::system().name().split('_');
    QString tmpLanguageType = "CN";
    if (language.count() > 1) {
        tmpLanguageType = language.value(1);
        if (tmpLanguageType == "CN"
            || tmpLanguageType == "TW"
            || tmpLanguageType == "HK") {
            tmpLanguageType = "CN";
        } else {
            tmpLanguageType = "US";
        }
    }

    languageType = tmpLanguageType;

    return languageType;
}

/**
 * @brief 更新日志中一个版本的信息
 *
 * 示例数据：
 * {
      //"id":"自增id",   // 自增ID
      //"platformType": 1, // 平台类型,1桌面专业版 2桌面家庭版 3桌面社区版 4桌面教育版 5专用设备 6服务器a版 7服务器d  8服务器e
      "baseline": "professional-v20-010", // 基线号
      "showVersion": "1060" // 展示给终端的更新版本号
      "cnLog": "<p>中文日志</p>", // 中文日志
      "enLog": "<p>enlish log</p>", // 英文日志
      //"logType": 1, // 日志类型：1系统更新 2安全更新
      "isUnstable": 1, // 日志范围，1：对外发布；2：内测，不传则默认值为1
      //"serverType":0, // 已弃用
      //"mainVersion": "V20", // 大版本 例如：V20
      //"systemVersion": "123123123", // 系统版本，例如：1040
      //"createdAt": "2022-05-05T10:56:19+08:00" // 创建时间
      "publishTime": "2022-05-05T00:00:00+08:00" // 发布日期
    }
 */
struct SystemUpdateLog {
    QString baseline;
    QString showVersion;
    QString cnLog = "";
    QString enLog = "";
    QString systemVersion;
    QString publishTime;
    int isUnstable=0;
    int logType = 1;

    bool isValid() const { return !baseline.isEmpty(); }

    static SystemUpdateLog fromJsonObj(const QJsonObject& obj) {
        SystemUpdateLog item;
        item.baseline = obj.value("baseline").toString();
        item.showVersion = obj.value("showVersion").toString();
        item.cnLog = obj.value("cnLog").toString();
        item.enLog = obj.value("enLog").toString();
        item.systemVersion = obj.value("systemVersion").toString();
        item.publishTime = DCC_NAMESPACE::utcDateTime2LocalDate(obj.value("publishTime").toString());
        item.isUnstable = obj.value("isUnstable").toInt();
        if (obj.contains("logType")) {
            item.logType = obj.value("logType").toInt();
        }
        if (item.logType != LogTypeSystem && item.logType != LogTypeSecurity) {
            item.logType = LogTypeSystem; // 没logType字段按系统更新处理
        }

        return item;
    }
};


/**
 * @brief 安全更新日志，主要针对 cve 漏洞
 *
 * 数据示例：
 {
    "synctime": "0001-01-01T00:00:00Z",
    "cveid": "CVE-2021-22204",
    "source": "libimage-exiftool-perl",
    "fixedVersion": "11.16.1.1-1+dde",
    "archs": "X86,MIPS,ARM",
    "score": "7.8",
    "status": "fixed",
    "vulCategory": "输出中的特殊元素转义处理不恰当（注入）",
    "vulName": "",
    "vulLevel": "high",
    "pubTme": "2021-04-23",
    "binary": "['libimage-exiftool-perl']",
    "description": "",
    "cveDescription": "Improper neutralization of user data in the DjVu file format in ExifTool versions 7.44 and up allows arbitrary code execution when parsing the malicious image"
  }
 */

enum VulLevel {
    VulLevel_None = 0,
    VulLevel_Low,
    VulLevel_Medium,
    VulLevel_High,
    VulLevel_Critical
};

struct SecurityUpdateLog {
    QString cveId; // ID
    QString vulLevel; // 等级 high medium low
    QString cveDescription; // 描述
    QString upgradeTime; // 更新时间

    static SecurityUpdateLog fromJsonObj(const QJsonObject &obj)
    {
        SecurityUpdateLog item;
        item.cveId = obj.value("cveId").toString();
        const auto &level = obj.value("vulLevel").toString().toLower();
        item.vulLevel = level.isEmpty() ? "none" : level;
        item.cveDescription = obj.value("cveDescription").toString();
        return item;
    }
};

struct HistoryItemDetail
{
    Q_GADGET
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString vulLevel MEMBER vulLevel)
    Q_PROPERTY(QString displayVulLevel MEMBER displayVulLevel)
public:
    QString description;
    QString name;
    QString vulLevel;
    QString displayVulLevel;

    static HistoryItemDetail fromCveJsonObj(const QJsonObject &obj)
    {
        HistoryItemDetail item;
        item.name = obj.value("cveId").toString();
        const auto &level = obj.value("vulLevel").toString().toLower();
        item.vulLevel = level.isEmpty() ? "none" : level;
        item.description = obj.value("cveDescription").toString();
        return item;
    }

    static HistoryItemDetail fromSystemJsonObj(const QJsonObject &obj)
    {
        HistoryItemDetail item;
        item.name = obj.value("showVersion").toString();
        item.description = obj.value(getLanguageType() == "CN" ? "cnLog" : "enLog").toString();
        return item;
    }
};
Q_DECLARE_METATYPE(HistoryItemDetail)

struct HistoryItemInfo {
    QString upgradeTime; // 更新时间;
    QString summary;
    UpdateType type = UpdateType::Invalid;

    QList<HistoryItemDetail> details;

    bool isInvalid() const { return type != UpdateType::Invalid; }

    static HistoryItemInfo fromJsonObj(const QJsonObject &obj) {
        HistoryItemInfo item;
        item.type = static_cast<UpdateType>(obj.value("UpgradeMode").toInt());
        if (item.type == Invalid)
            return item;

        item.upgradeTime = obj.value("UpgradeTime").toString();

        if (SystemUpdate == item.type) {
            const auto &originChangLog = obj.value("OriginChangelog").toArray();
            for (const auto &value : originChangLog) {
                item.details.append(HistoryItemDetail::fromSystemJsonObj(value.toObject()));
            }
        } else {
            const auto &originChangLog = obj.value("OriginChangelog").toObject();
            for (const auto &key : originChangLog.keys()) {
                item.details.append(HistoryItemDetail::fromCveJsonObj(originChangLog.value(key).toObject()));
            }
        }

        return item;
    }
};

class UpdateLogHelper : public QObject, public Dtk::Core::DSingleton<UpdateLogHelper>
{
    Q_OBJECT

    friend class Dtk::Core::DSingleton<UpdateLogHelper>;

public:
    void handleUpdateLog(const QString &log);
    void handleLocalUpdateLog();
    QList<HistoryItemInfo> handleHistoryUpdateLog(const QString &log);
    void updateItemInfo(UpdateItemInfo *itemInfo);

private:
    UpdateLogHelper();
    ~UpdateLogHelper();

    void handleSystem(const QJsonArray &log);
    void handleSecurity(const QJsonObject &log);
    void handleSystemItemInfo(UpdateItemInfo *itemInfo) const;
    void handleSecurityItemInfo(UpdateItemInfo *itemInfo) const;

    static const QMap<QString, QPair<VulLevel, QString>>& vulLevelMap();
    static QString sumCveLevelUp(const QMap<VulLevel, int>& vulCount);

private:
    QList<SystemUpdateLog> m_systemLog;
    QList<SecurityUpdateLog> m_securityLog;
};


#endif
