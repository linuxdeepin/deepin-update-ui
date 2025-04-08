// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATEITEMINFO_H
#define UPDATEITEMINFO_H

#include "common.h"
#include "utils.h"

using namespace dcc::update::common;

struct DetailInfo {
    QString name;
    QString updateTime;
    QString info;
    QString link;
    QString vulLevel;

    DetailInfo() { }
};

class UpdateItemInfo : public QObject {
    Q_OBJECT
public:
    explicit UpdateItemInfo(UpdateType type, QObject* parent = nullptr);
    virtual ~UpdateItemInfo() { }

    inline qlonglong downloadSize() const { return m_downloadSize; }
    void setDownloadSize(qlonglong downloadSize);

    QString name() const;
    void setName(const QString& name);

    QString currentVersion() const;
    void setCurrentVersion(const QString& currentVersion);

    QString availableVersion() const;
    void setAvailableVersion(const QString& availableVersion);

    QString explain() const;
    void setExplain(const QString& explain);

    QString updateTime() const;
    void setUpdateTime(const QString& updateTime);

    QStringList packages() const { return m_packages; }
    void setPackages(const QStringList& packages);

    QList<DetailInfo> detailInfos() const;
    void setDetailInfos(QList<DetailInfo>& detailInfos);
    void addDetailInfo(DetailInfo detailInfo);

    void setUpdateModeEnabled(bool enable) { m_enabled = enable; }
    bool isUpdateModeEnabled() const { return m_enabled; }

    bool isUpdateAvailable() const { return (m_enabled && m_packages.size() > 0) || UpdatesStatus::UpgradeSuccess == m_updateStatus; }

    void reset();

    void setIsChecked(bool isChecked);
    bool isChecked() const { return m_isChecked; }

    UpdateType updateType() const { return m_updateType; }

    void setUpdateStatus(UpdatesStatus status);
    UpdatesStatus updateStatus() const { return m_updateStatus; }

    void setTypeString(const QString& type) { m_typeString = type; }
    QString typeString() const { return m_typeString; }

    inline void setBaseline(const QString& baseline) { m_baseline = baseline; }
    inline QString baseline() const { return m_baseline; }

Q_SIGNALS:
    void downloadSizeChanged(const qlonglong size);
    void checkStateChanged(bool);
    void updateStatusChanged(UpdatesStatus status);

private:
    UpdateType m_updateType;
    qlonglong m_downloadSize;
    bool m_enabled;
    QString m_name;
    QString m_currentVersion;
    QString m_availableVersion;
    QString m_baseline;
    QString m_explain;
    QString m_updateTime;
    QStringList m_packages;
    QList<DetailInfo> m_detailInfos;
    bool m_isChecked;
    UpdatesStatus m_updateStatus;
    QString m_typeString;
};

#endif // UPDATEITEMINFO_H
