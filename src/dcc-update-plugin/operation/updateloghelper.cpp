// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updateloghelper.h"
#include "common.h"

#include <QMap>
#include <QPair>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

UpdateLogHelper::UpdateLogHelper()
{
    qCDebug(logDccUpdatePlugin) << "Initialize UpdateLogHelper";
}

UpdateLogHelper::~UpdateLogHelper()
{
    qCDebug(logDccUpdatePlugin) << "Destroying UpdateLogHelper";
}

const QMap<QString, QPair<VulLevel, QString>>& UpdateLogHelper::vulLevelMap()
{
    qCDebug(logDccUpdatePlugin) << "Getting vulnerability level map";
    const static QMap<QString, QPair<VulLevel, QString>> VulLevelMap = {
        {"none", QPair<VulLevel, QString>(VulLevel_None, tr("NONE"))},
        {"low", QPair<VulLevel, QString>(VulLevel_Low, tr("LOW"))},
        {"medium", QPair<VulLevel, QString>(VulLevel_Medium, tr("MEDIUM"))},
        {"high", QPair<VulLevel, QString>(VulLevel_High, tr("HIGH"))},
        {"critical", QPair<VulLevel, QString>(VulLevel_Critical, tr("CRITICAL"))}
    };

    return VulLevelMap;
}

QString UpdateLogHelper::sumCveLevelUp(const QMap<VulLevel, int>& vulCount)
{
    qCDebug(logDccUpdatePlugin) << "Summarizing CVE levels, vulnerability count:" << vulCount.size();
    // 拼接描述：本次更新修复n%个高危漏洞、n%个中危漏洞、n%个低危漏洞、n％个未知漏洞
    // 没有的漏洞则不显示
    const static QMap<VulLevel, QString> textMap = {
        {VulLevel_Critical, tr("critical-risk")},
        {VulLevel_High, tr("high-risk")},
        {VulLevel_Medium, tr("medium-risk")},
        {VulLevel_Low, tr("low-risk")},
        {VulLevel_None, tr("unknown")},
    };
    QString explain = tr("This update fixes");
    for (auto it = --vulCount.cend() ; it != --vulCount.begin(); it--) {
        const int count = it.value();
        QString vulnerability = count > 1 ? tr("vulnerabilities") : tr("vulnerability");
        //~ content_explain `数字+%`会在代码中替换为字符串，例如：3 of high-risk vulnerabilities；各语言需要根据实际情况增加空格(例如：中文没有空格，英文有空格)
        const QString &content = tr("%1 of %2 %3").arg(QString::number(count)).arg(textMap.value(it.key())).arg(vulnerability);
        //~ content_explain 中文逗号不需要空格，英文逗号需要空格For more details, please visit
        explain.append(content + tr(", "));
    }

    //~ content_explain 这句话后面会带上一个超链接，各语言自行决定末尾需不需要加空格
    explain.append(tr("for more details, please visit "));
    const auto link = "https://src.uniontech.com";
    explain.append(QString("<a href=\"%1\">%2").arg(link).arg(link));
    return explain;
}

void UpdateLogHelper::handleUpdateLog(const QString &log)
{
    qCDebug(logDccUpdatePlugin) << "Handling update log, length:" << log.length();
    const QJsonDocument& doc = QJsonDocument::fromJson(log.toLocal8Bit());
    const QJsonObject& rootObj = doc.object();
    if (rootObj.isEmpty()) {
        qCWarning(logDccUpdatePlugin) << "Update log json object is empty";
        return;
    }

    qCDebug(logDccUpdatePlugin) << "Processing system and security update logs";
    handleSystem(rootObj.value(QString::number(static_cast<int>(SystemUpdate))).toArray());
    handleSecurity(rootObj.value(QString::number(static_cast<int>(SecurityUpdate))).toObject());
}

void UpdateLogHelper::handleSystem(const QJsonArray &log)
{
    qCDebug(logDccUpdatePlugin) << "Handling system log with" << log.size() << "entries";
    m_systemLog.clear();
    for (const QJsonValue& value : log) {
        QJsonObject obj = value.toObject();
        if (obj.isEmpty())
            continue;

        m_systemLog.append(std::move(SystemUpdateLog::fromJsonObj(obj)));
    }

    // 不依赖服务器返回来日志顺序，用showVersion进行排序
    // 如果showVersion版本号相同，则用发布时间排序；不考虑版本号相同且发布时间相同的情况，这种情况应该由运维人员避免
    std::sort(m_systemLog.begin(), m_systemLog.end(), [](const SystemUpdateLog& v1, const SystemUpdateLog& v2) -> bool {
        int compareRet = v1.showVersion.compare(v2.showVersion);
        if (compareRet == 0) {
            return v1.publishTime.compare(v2.publishTime) >= 0;
        }
        return compareRet > 0;
    });
}

void UpdateLogHelper::handleSecurity(const QJsonObject &log)
{
    qCDebug(logDccUpdatePlugin) << "Handling security log with" << log.keys().size() << "entries";
    for (const auto& key : log.keys()) {
        const auto& obj = log.value(key).toObject();
        if (obj.isEmpty())
            continue;

        m_securityLog.append(std::move(SecurityUpdateLog::fromJsonObj(obj)));
    }

    std::sort(m_securityLog.begin(), m_securityLog.end(), [](const SecurityUpdateLog& v1, const SecurityUpdateLog& v2) -> bool {
        auto v1Level = vulLevelMap().value(v1.vulLevel).first;
        auto v2Level = vulLevelMap().value(v2.vulLevel).first;
        if (v1Level == v2Level) {
            return v1.cveId.compare(v2.cveId) >= 0;
        }
        return v1Level > v2Level;
    });
}

void UpdateLogHelper::updateItemInfo(UpdateItemInfo* itemInfo)
{
    qCDebug(logDccUpdatePlugin) << "Updating item info";
    if (!itemInfo) {
        qCWarning(logDccUpdatePlugin) << "Item info is null";
        return;
    }

    qCDebug(logDccUpdatePlugin) << "Item type:" << itemInfo->updateType();
    if (itemInfo->updateType() == SystemUpdate) {
        qCDebug(logDccUpdatePlugin) << "Handling system item info";
        handleSystemItemInfo(itemInfo);
    } else if (itemInfo->updateType() == SecurityUpdate) {
        qCDebug(logDccUpdatePlugin) << "Handling security item info";
        handleSecurityItemInfo(itemInfo);
    } else {
        qCDebug(logDccUpdatePlugin) << "Unknown update type, skipping";
    }
}

void UpdateLogHelper::handleSystemItemInfo(UpdateItemInfo *itemInfo) const
{
    qCDebug(logDccUpdatePlugin) << "Handling system item info with" << m_systemLog.size() << "system logs";
    if (m_systemLog.isEmpty()) {
        qCDebug(logDccUpdatePlugin) << "No system logs available";
        return;
    }

    for (const auto &log : m_systemLog) {
        const QString& explain = getLanguageType() == "CN" ? log.cnLog : log.enLog;
        itemInfo->setBaseline(log.baseline);
        // 写入最近的更新
        if (itemInfo->currentVersion().isEmpty()) {
            qCDebug(logDccUpdatePlugin) << "Setting current version info for:" << log.showVersion;
            itemInfo->setCurrentVersion(log.showVersion);
            itemInfo->setAvailableVersion(log.showVersion);
            itemInfo->setExplain(explain);
            itemInfo->setUpdateTime(log.publishTime);
        } else {
            DetailInfo detailInfo;
            const QString& systemVersion = log.showVersion;
            // 专业版不不在详细信息中显示维护线版本
            if (!systemVersion.isEmpty() && systemVersion.back() == '0') {
                qCDebug(logDccUpdatePlugin) << "Adding detail info for version:" << log.showVersion;
                detailInfo.name = log.showVersion;
                detailInfo.updateTime = log.publishTime;
                detailInfo.info = explain;
                itemInfo->addDetailInfo(detailInfo);
            }
        }
    }
}

void UpdateLogHelper::handleSecurityItemInfo(UpdateItemInfo *itemInfo) const
{
    qCDebug(logDccUpdatePlugin) << "Handling security item info with" << m_securityLog.size() << "security logs";
    if (m_securityLog.isEmpty()) {
        qCDebug(logDccUpdatePlugin) << "No security logs available";
        return;
    }

    QMap<VulLevel, int> vulCount;
    for (const auto &log : m_securityLog) {
        // 写入最近的更新
        DetailInfo detailInfo;
        const auto &pair = vulLevelMap().value(log.vulLevel);
        detailInfo.vulLevel = pair.second;
        auto count = vulCount.value(pair.first, 0);
        vulCount[pair.first] = ++count;
        detailInfo.name = log.cveId;
        detailInfo.info = log.cveDescription;
        itemInfo->addDetailInfo(detailInfo);
    }
    itemInfo->setExplain(sumCveLevelUp(vulCount));
}

QList<HistoryItemInfo> UpdateLogHelper::handleHistoryUpdateLog(const QString &log)
{
    qCDebug(logDccUpdatePlugin) << "Handling history update log, length:" << log.length();
    QList<HistoryItemInfo> infos;
    QJsonParseError error;
    const QJsonDocument& doc = QJsonDocument::fromJson(log.toLocal8Bit(), &error);
    if (QJsonParseError::NoError != error.error) {
        qCWarning(logDccUpdatePlugin) << "Parse update history log failed:" << error.errorString();
        return infos;
    }

    const auto& rootArray = doc.array();
    if (rootArray.isEmpty()) {
        qCWarning(logDccUpdatePlugin) << "History log json array is empty";
        return infos;
    }

    qCDebug(logDccUpdatePlugin) << "Processing" << rootArray.size() << "history items";
    for (const auto &obj : rootArray) {
        auto item = HistoryItemInfo::fromJsonObj(obj.toObject());
        if (item.type == UpdateType::SecurityUpdate) {
            qCDebug(logDccUpdatePlugin) << "Processing security update history item";
            QMap<VulLevel, int> vulCount;
            for (auto &detail : item.details) {
                const auto &pair = vulLevelMap().value(detail.vulLevel);
                auto count = vulCount.value(pair.first, 0);
                vulCount[pair.first] = ++count;
                item.summary = sumCveLevelUp(vulCount);
                detail.displayVulLevel = pair.second;
            }
            std::sort(item.details.begin(), item.details.end(), [](const HistoryItemDetail& v1, const HistoryItemDetail& v2) -> bool {
                auto v1Level = vulLevelMap().value(v1.vulLevel).first;
                auto v2Level = vulLevelMap().value(v2.vulLevel).first;
                if (v1Level == v2Level) {
                    return v1.name.compare(v2.name) >= 0;
                }
                return v1Level > v2Level;
            });
        }
        infos.append(std::move(item));

    }
    return infos;
}
