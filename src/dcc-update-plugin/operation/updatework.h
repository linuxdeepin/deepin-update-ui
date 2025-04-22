// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATEWORK_H
#define UPDATEWORK_H

#include "common.h"
#include "updatemodel.h"

#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QObject>

#include "common/dbus/updatedbusproxy.h"
#include "common/dbus/updatejobdbusproxy.h"

//#include <com_deepin_abrecovery.h>
//#include <com_deepin_daemon_appearance.h>
//#include <com_deepin_daemon_network.h>
//#include <com_deepin_daemon_power.h>
//#include <com_deepin_lastore_job.h>
//#include <com_deepin_lastore_jobmanager.h>
//#include <com_deepin_lastore_smartmirror.h>
//#include <com_deepin_lastore_updater.h>
//#include <com_deepin_lastoresessionhelper.h>
//#include <com_deepin_system_systempower.h>

//using UpdateInter = com::deepin::lastore::Updater;
//using JobInter = com::deepin::lastore::Job;
//using ManagerInter = com::deepin::lastore::Manager;
//using Network = com::deepin::daemon::Network;
//using LastoressionHelper = com::deepin::LastoreSessionHelper;
//using SmartMirrorInter = com::deepin::lastore::Smartmirror;
//using Appearance = com::deepin::daemon::Appearance;
//using RecoveryInter = com::deepin::ABRecovery;
//using PowerInter = com::deepin::system::Power;

class QJsonArray;


class UpdateWorker : public QObject {
    Q_OBJECT
public:
    explicit UpdateWorker(UpdateModel* model, QObject* parent = nullptr);
    ~UpdateWorker();
    void activate();
    void deactivate();
    void getLicenseState();

    Q_INVOKABLE void reStart();

Q_SIGNALS:
    void requestInit();
    void requestActive();
    void requestRefreshLicenseState();
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    void requestRefreshMirrors();
#endif
    void systemActivationChanged(bool systemActivation);

public Q_SLOTS:
    void init();
    void checkForUpdates();
    void setFunctionUpdate(bool update);
    void setSecurityUpdate(bool update);
    void setThirdPartyUpdate(bool update);
    void setUpdateMode(const quint64 updateMode);
    void setAutoCleanCache(const bool autoCleanCache);
    void setAutoDownloadUpdates(const bool& autoDownload);
    void setMirrorSource(const MirrorInfo& mirror);
    void testingChannelCheck(bool checked);
    void setTestingChannelEnable(const bool& enable);
    void checkCanExitTestingChannel();
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    void setSourceCheck(bool enable);
#endif
    void testMirrorSpeed();
    void checkNetselect();
    void setSmartMirror(bool enable);
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    void refreshMirrors();
#endif
    void licenseStateChangeSlot();
    void refreshHistoryAppsInfo();
    void refreshLastTimeAndCheckCircle();
    void setUpdateNotify(const bool notify);
    void onDownloadJobCtrl(int updateCtrlType);
    void onClassifiedUpdatablePackagesChanged(const QMap<QString, QStringList>& packages);
    void onRequestRetry(int type, int updateTypes);
    void onRequestLastoreHeartBeat();
    void startDownload(int updateTypes);
    void setIdleDownloadEnabled(bool enable);
    void setIdleDownloadBeginTime(int time);
    void setIdleDownloadEndTime(int time);
    void setIdleDownloadConfig(const IdleDownloadConfig& config);
    void setDownloadSpeedLimitEnabled(bool enable);
    void setDownloadSpeedLimitSize(const QString& size);
    void setDownloadSpeedLimitConfig(const QString& config);
    void setP2PUpdateEnabled(bool enabled);

    void doUpgrade(int updateTypes, bool doBackup);
    void stopDownload();

private Q_SLOTS:
    void setCheckUpdatesJob(const QString& jobPath);
    void onJobListChanged(const QList<QDBusObjectPath>& jobs);
    void onIconThemeChanged(const QString& theme);
    void onCheckUpdateStatusChanged(const QString& value);
    void onDownloadStatusChanged(const QString& value);
    void onBackupStatusChanged(const QString& value);
    void checkTestingChannelStatus();
    QStringList getSourcesOfPackage(const QString pkg, const QString version);
    QString getTestingChannelSource();
    void onUpdateModeChanged(qulonglong value);
    void onDistUpgradeStatusChanged(const QString& status);
    void checkPower();
    void onUpdateStatusChanged(const QString& value);

private:
    QMap<UpdateType, UpdateItemInfo*> getAllUpdateInfo(const QMap<QString, QStringList>& updatePackages);
    void setUpdateInfo();
    void setUpdateItemDownloadSize(UpdateItemInfo* updateItem);
    inline bool checkDbusIsValid();
    void onSmartMirrorServiceIsValid(bool valid);
    void createCheckUpdateJob(const QString& jobPath);
    void setDownloadJob(const QString& jobPath);
    bool checkJobIsValid(QPointer<UpdateJobDBusProxy> dbusJob);
    void deleteJob(QPointer<UpdateJobDBusProxy> dbusJob);
    void cleanLaStoreJob(QPointer<UpdateJobDBusProxy> dbusJob);
    UpdateErrorType analyzeJobErrorMessage(const QString& jobDescription, UpdatesStatus status = Default);
    bool isUpdatesAvailable(const QMap<QString, QStringList>& updatablePackages);
    int isUnstableResource() const;
    void onRequestCheckUpdateModeChanged(int type, bool isChecked);
    void setDistUpgradeJob(const QString& jobPath);
    void updateSystemVersion();
    QUrl getTestingChannelJoinURL() const;
    QString timeToString(int value);
    QString adjustTimeFunc(const QString& start, const QString& end, bool returnEndTime);
    void setBackupJob(const QString& jobPath);

private:
    UpdateModel* m_model;
//    QPointer<JobInter> m_checkUpdateJob; // 检查更新
//    QPointer<JobInter> m_downloadJob; // 下载
//    QPointer<JobInter> m_fixErrorJob; // 修复错误
//    QPointer<JobInter> m_distUpgradeJob; // 升级
//    LastoressionHelper* m_lastoreSessionHelper;
 //   UpdateInter* m_updateInter;
//    ManagerInter* m_managerInter;
//    SmartMirrorInter* m_smartMirrorInter;
//    Appearance* m_iconTheme;
//    PowerInter *m_powerInter;
    QString m_iconThemeState;
    QMutex m_downloadMutex;

    QPointer<UpdateJobDBusProxy> m_checkUpdateJob;
    QPointer<UpdateJobDBusProxy> m_fixErrorJob;

    QPointer<UpdateJobDBusProxy> m_downloadJob;
    QPointer<UpdateJobDBusProxy> m_distUpgradeJob;
    QPointer<UpdateJobDBusProxy> m_backupJob;
    // QPointer<UpdateJobDBusProxy> m_sysUpdateDownloadJob;
    // QPointer<UpdateJobDBusProxy> m_safeUpdateDownloadJob;
    // QPointer<UpdateJobDBusProxy> m_unknownUpdateDownloadJob;

    // QPointer<UpdateJobDBusProxy> m_sysUpdateInstallJob;
    // QPointer<UpdateJobDBusProxy> m_safeUpdateInstallJob;
    // QPointer<UpdateJobDBusProxy> m_unknownUpdateInstallJob;

    // QPointer<UpdateJobDBusProxy> m_releaseNoteInstallJob;

    UpdateDBusProxy *m_updateInter;

    QString m_jobPath;
};
#endif // UPDATEWORK_H
