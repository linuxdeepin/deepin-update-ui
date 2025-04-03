// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updateiteminfo.h"

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
}

void UpdateItemInfo::setDownloadSize(qlonglong downloadSize)
{
    if (downloadSize != m_downloadSize) {
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
    m_name = name;
}

QString UpdateItemInfo::currentVersion() const
{
    return m_currentVersion;
}

void UpdateItemInfo::setCurrentVersion(const QString& currentVersion)
{
    m_currentVersion = currentVersion;
}

QString UpdateItemInfo::availableVersion() const
{
    return m_availableVersion;
}

void UpdateItemInfo::setAvailableVersion(const QString& availableVersion)
{
    m_availableVersion = availableVersion;
}

QString UpdateItemInfo::explain() const
{
    return m_explain;
}

void UpdateItemInfo::setExplain(const QString& explain)
{
    m_explain = explain;
}

QString UpdateItemInfo::updateTime() const
{
    return m_updateTime;
}

void UpdateItemInfo::setUpdateTime(const QString& updateTime)
{
    m_updateTime = updateTime;
}

QList<DetailInfo> UpdateItemInfo::detailInfos() const
{
    return m_detailInfos;
}

void UpdateItemInfo::setDetailInfos(QList<DetailInfo>& detailInfos)
{
    m_detailInfos.clear();
    m_detailInfos = detailInfos;
}

void UpdateItemInfo::addDetailInfo(DetailInfo detailInfo)
{
    m_detailInfos.append(std::move(detailInfo));
}

void UpdateItemInfo::setPackages(const QStringList& packages)
{
    m_packages = packages;
}

void UpdateItemInfo::reset()
{
    m_packages = QStringList();
    m_downloadSize = 0;
}

void UpdateItemInfo::setIsChecked(bool isChecked)
{
    if (m_isChecked == isChecked)
        return;

    m_isChecked = isChecked;
    Q_EMIT checkStateChanged(isChecked);
}

void UpdateItemInfo::setUpdateStatus(UpdatesStatus status)
{
    if (m_updateStatus == status) {
        return;
    }

    m_updateStatus = status;
    Q_EMIT updateStatusChanged(m_updateStatus);
}
