// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updateiteminfo.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

UpdateItemInfo::UpdateItemInfo(UpdateType type, QObject* parent)
    : QObject(parent)
    , m_updateType(type)
    , m_downloadSize(0)
    , m_name("")
    , m_currentVersion("")
    , m_availableVersion("")
    , m_explain("")
    , m_updateTime("")
    , m_isChecked(true)
    , m_updateStatus(UpdatesStatus::Default)
{
    qCDebug(logDccUpdatePlugin) << "Initialize UpdateItemInfo with type:" << type;
}

void UpdateItemInfo::setDownloadSize(qlonglong downloadSize)
{
    if (downloadSize != m_downloadSize) {
        qCDebug(logDccUpdatePlugin) << "Set download size:" << downloadSize << "for item:" << m_name;
        m_downloadSize = downloadSize;
        Q_EMIT downloadSizeChanged(downloadSize);
    }
}

QString UpdateItemInfo::name() const
{
    return m_name;
}

void UpdateItemInfo::setName(const QString& name)
{
    qCDebug(logDccUpdatePlugin) << "Set name:" << name;
    m_name = name;
}

QString UpdateItemInfo::currentVersion() const
{
    return m_currentVersion;
}

void UpdateItemInfo::setCurrentVersion(const QString& currentVersion)
{
    qCDebug(logDccUpdatePlugin) << "Set current version:" << currentVersion << "for item:" << m_name;
    m_currentVersion = currentVersion;
}

QString UpdateItemInfo::availableVersion() const
{
    return m_availableVersion;
}

void UpdateItemInfo::setAvailableVersion(const QString& availableVersion)
{
    qCDebug(logDccUpdatePlugin) << "Set available version:" << availableVersion << "for item:" << m_name;
    m_availableVersion = availableVersion;
}

QString UpdateItemInfo::explain() const
{
    return m_explain;
}

void UpdateItemInfo::setExplain(const QString& explain)
{
    qCDebug(logDccUpdatePlugin) << "Set explain:" << explain << "for item:" << m_name;
    m_explain = explain;
}

QString UpdateItemInfo::updateTime() const
{
    return m_updateTime;
}

void UpdateItemInfo::setUpdateTime(const QString& updateTime)
{
    qCDebug(logDccUpdatePlugin) << "Set update time:" << updateTime << "for item:" << m_name;
    m_updateTime = updateTime;
}

QList<DetailInfo> UpdateItemInfo::detailInfos() const
{
    return m_detailInfos;
}

void UpdateItemInfo::setDetailInfos(QList<DetailInfo>& detailInfos)
{
    qCDebug(logDccUpdatePlugin) << "Set detail infos count:" << detailInfos.size() << "for item:" << m_name;
    m_detailInfos.clear();
    m_detailInfos = detailInfos;
}

void UpdateItemInfo::addDetailInfo(DetailInfo detailInfo)
{
    qCDebug(logDccUpdatePlugin) << "Add detail info for item:" << m_name;
    m_detailInfos.append(std::move(detailInfo));
}

void UpdateItemInfo::setPackages(const QStringList& packages)
{
    qCDebug(logDccUpdatePlugin) << "Set packages count:" << packages.size() << "for item:" << m_name;
    m_packages = packages;
}

void UpdateItemInfo::reset()
{
    qCDebug(logDccUpdatePlugin) << "Reset item info for:" << m_name;
    m_packages = QStringList();
    m_downloadSize = 0;
}

void UpdateItemInfo::setIsChecked(bool isChecked)
{
    if (m_isChecked == isChecked)
        return;

    qCDebug(logDccUpdatePlugin) << "Set checked state:" << isChecked << "for item:" << m_name;
    m_isChecked = isChecked;
    Q_EMIT checkStateChanged(isChecked);
}

void UpdateItemInfo::setUpdateStatus(UpdatesStatus status)
{
    qCDebug(logDccUpdatePlugin) << "Set update status:" << status << "for item:" << m_name;
    if (m_updateStatus == status) {
        return;
    }

    m_updateStatus = status;
    Q_EMIT updateStatusChanged(m_updateStatus);
}
