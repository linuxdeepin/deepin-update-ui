// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATEWORK_H
#define UPDATEWORK_H

#include "common.h"
#include "updatemodel.h"
#include "common/dbus/updatedbusproxy.h"
#include "common/dbus/updatejobdbusproxy.h"

#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QObject>

class UpdateWorker : public QObject 
{
    Q_OBJECT

public:
    explicit UpdateWorker(UpdateModel* model, QObject* parent = nullptr);
    ~UpdateWorker();

    void initConnect();
    void activate();
    void getLicenseState();

    bool checkDbusIsValid();
    bool checkJobIsValid(QPointer<UpdateJobDBusProxy> dbusJob);
    void deleteJob(QPointer<UpdateJobDBusProxy> dbusJob);
    void cleanLaStoreJob(QPointer<UpdateJobDBusProxy> dbusJob);
    UpdateErrorType analyzeJobErrorMessage(const QString& jobDescription, UpdatesStatus status = Default);

    // 检查更新
    Q_INVOKABLE void updateNeedDoCheck();
    Q_INVOKABLE void checkForUpdates();
    void setCheckUpdatesJob(const QString& jobPath);
    void createCheckUpdateJob(const QString& jobPath);
    void refreshLastTimeAndCheckCircle();
    void setUpdateInfo();
    QMap<UpdateType, UpdateItemInfo*> getAllUpdateInfo(const QMap<QString, QStringList>& updatePackages);
    void setUpdateItemDownloadSize(UpdateItemInfo* updateItem);

    // 下载更新
    Q_INVOKABLE void startDownload(int updateTypes);
    Q_INVOKABLE void stopDownload();
    Q_INVOKABLE void onDownloadJobCtrl(int updateCtrlType);
    void setDownloadJob(const QString& jobPath);

    // 备份并安装更新
    Q_INVOKABLE void doUpgrade(int updateTypes, bool doBackup);
    Q_INVOKABLE void reStart();
    void setBackupJob(const QString& jobPath);
    void setDistUpgradeJob(const QString& jobPath);
    void updateSystemVersion();

    // 更新设置-更新类型
    Q_INVOKABLE void setFunctionUpdate(bool update);
    Q_INVOKABLE void setSecurityUpdate(bool update);
    Q_INVOKABLE void setThirdPartyUpdate(bool update);

    // 更新设置-下载设置
    Q_INVOKABLE void setDownloadSpeedLimitEnabled(bool enable);
    Q_INVOKABLE void setDownloadSpeedLimitSize(const QString& size);
    void setDownloadSpeedLimitConfig(const QString& config);
    Q_INVOKABLE void setAutoDownloadUpdates(const bool& autoDownload);
    Q_INVOKABLE void setIdleDownloadEnabled(bool enable);
    Q_INVOKABLE void setIdleDownloadBeginTime(int time);
    Q_INVOKABLE void setIdleDownloadEndTime(int time);
    void setIdleDownloadConfig(const IdleDownloadConfig& config);
    QString timeToString(int value);
    QString adjustTimeFunc(const QString& start, const QString& end, bool returnEndTime);

    // 更新设置-更新提醒
    Q_INVOKABLE void setUpdateNotify(const bool notify);
    // 更新设置-清除软件缓存包
    Q_INVOKABLE void setAutoCleanCache(const bool autoCleanCache);

    // 更新设置-镜像源
    Q_INVOKABLE void setSmartMirror(bool enable);
    Q_INVOKABLE void setMirrorSource(const MirrorInfo& mirror);
    void testMirrorSpeed();
    void checkNetselect();
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    void refreshMirrors();
#endif

    // 更新设置-内测通道
    Q_INVOKABLE void setTestingChannelEnable(const bool& enable);
    Q_INVOKABLE bool openTestingChannelUrl();
    Q_INVOKABLE void exitTestingChannel(bool value);
    std::optional<QString> getMachineId();
    std::optional<QUrl> getTestingChannelUrl();
    void initTestingChannel();
    void checkTestingChannelStatus();
    void setInstallPackageJob(const QString& jobPath);
    void setRemovePackageJob(const QString& jobPath);

    Q_INVOKABLE bool openUrl(const QString& url);
    Q_INVOKABLE void onRequestRetry(int type, int updateTypes);

    Q_INVOKABLE void setCheckUpdateMode(int type, bool isChecked);

public Q_SLOTS:
    void onLicenseStateChange();
    void onPowerChange();
    void onUpdateModeChanged(qulonglong value);
    void onJobListChanged(const QList<QDBusObjectPath>& jobs);
    void onUpdateStatusChanged(const QString& value);
    void onClassifiedUpdatablePackagesChanged(const QMap<QString, QStringList>& packages);

    void onCheckUpdateStatusChanged(const QString& value);
    void onDownloadStatusChanged(const QString& value);
    void onBackupStatusChanged(const QString& value);
    void onDistUpgradeStatusChanged(const QString& status);
    void onInstallPackageStatusChanged(const QString& value);
    void onRemovePackageStatusChanged(const QString& value);

Q_SIGNALS:
    void systemActivationChanged(bool systemActivation);
    void requestCloseTestingChannel();

private:
    UpdateModel* m_model;
    UpdateDBusProxy *m_updateInter;
    QTimer *m_lastoreHeartBeatTimer; // lastore-daemon 心跳信号，防止lastore-daemon自动退出

    std::optional<QString> m_machineid;
    std::optional<QUrl> m_testingChannelUrl;
    QMutex m_downloadMutex;

    QPointer<UpdateJobDBusProxy> m_checkUpdateJob;
    QPointer<UpdateJobDBusProxy> m_fixErrorJob;
    QPointer<UpdateJobDBusProxy> m_downloadJob;
    QPointer<UpdateJobDBusProxy> m_distUpgradeJob;
    QPointer<UpdateJobDBusProxy> m_backupJob;
    QPointer<UpdateJobDBusProxy> m_installPackageJob;
    QPointer<UpdateJobDBusProxy> m_removePackageJob;
};

#endif // UPDATEWORK_H
