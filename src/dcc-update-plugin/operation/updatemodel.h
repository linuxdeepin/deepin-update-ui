// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include "common.h"
#include "updatedatastructs.h"
#include "updateiteminfo.h"
#include "mirrorinfolist.h"
#include "appupdateinfolist.h"
#include "updatelistmodel.h"

#include <QJsonDocument>
#include <QObject>
#include <DConfig>

class UpdateHistoryModel;
class UpdateModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool updateProhibited READ updateProhibited NOTIFY updateProhibitedChanged FINAL)
    Q_PROPERTY(bool systemActivation READ systemActivation NOTIFY systemActivationChanged FINAL)
    Q_PROPERTY(bool isUpdateDisabled READ isUpdateDisabled NOTIFY isUpdateDisabledChanged FINAL)
    Q_PROPERTY(QString updateDisabledIcon READ updateDisabledIcon NOTIFY updateDisabledIconChanged FINAL)
    Q_PROPERTY(QString updateDisabledTips READ updateDisabledTips NOTIFY updateDisabledTipsChanged FINAL)
    Q_PROPERTY(bool batterIsOK READ batterIsOK NOTIFY batterIsOKChanged FINAL)
    Q_PROPERTY(int lastStatus READ lastStatus  NOTIFY lastStatusChanged FINAL)

    // ---------------检查更新页面数据---------------
    Q_PROPERTY(bool showCheckUpdate READ showCheckUpdate NOTIFY showCheckUpdateChanged FINAL)
    Q_PROPERTY(QString checkUpdateIcon READ checkUpdateIcon NOTIFY checkUpdateIconChanged FINAL)
    Q_PROPERTY(double checkUpdateProgress READ checkUpdateProgress NOTIFY checkUpdateProgressChanged FINAL)
    Q_PROPERTY(UpdatesStatus checkUpdateStatus READ checkUpdateStatus NOTIFY checkUpdateStatusChanged FINAL)
    Q_PROPERTY(QString checkUpdateErrTips READ checkUpdateErrTips NOTIFY checkUpdateErrTipsChanged FINAL)
    Q_PROPERTY(QString checkBtnText READ checkBtnText NOTIFY checkBtnTextChanged FINAL)
    Q_PROPERTY(QString lastCheckUpdateTime READ lastCheckUpdateTime NOTIFY lastCheckUpdateTimeChanged FINAL)

    // ---------------下载、备份、安装列表数据---------------
    Q_PROPERTY(UpdateListModel *preUpdatelistModel READ preUpdatelistModel NOTIFY preUpdatelistModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *downloadinglistModel READ downloadinglistModel NOTIFY downloadinglistModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *downloadFailedListModel READ downloadFailedListModel NOTIFY downloadFailedListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *preInstallListModel READ preInstallListModel NOTIFY preInstallListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installinglistModel READ installinglistModel NOTIFY installinglistModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installCompleteListModel READ installCompleteListModel NOTIFY installCompleteListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installFailedListModel READ installFailedListModel NOTIFY installFailedListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *backingUpListModel READ backingUpListModel NOTIFY backingUpListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *backupFailedListModel READ backupFailedListModel NOTIFY backupFailedListModelChanged FINAL)    

    Q_PROPERTY(bool downloadWaiting READ downloadWaiting NOTIFY downloadWaitingChanged FINAL)
    Q_PROPERTY(bool downloadPaused READ downloadPaused NOTIFY downloadPausedChanged FINAL)
    Q_PROPERTY(bool upgradeWaiting READ upgradeWaiting NOTIFY upgradeWaitingChanged FINAL)

    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged FINAL)
    Q_PROPERTY(double backupProgress READ backupProgress NOTIFY backupProgressChanged FINAL)
    Q_PROPERTY(double distUpgradeProgress READ distUpgradeProgress NOTIFY distUpgradeProgressChanged FINAL)

    Q_PROPERTY(QString preUpdateTips READ preUpdateTips WRITE setPreUpdateTips NOTIFY preUpdateTipsChanged FINAL)
    Q_PROPERTY(QString downloadFailedTips READ downloadFailedTips NOTIFY downloadFailedTipsChanged FINAL)
    Q_PROPERTY(QString installFailedTips READ installFailedTips NOTIFY installFailedTipsChanged FINAL)
    Q_PROPERTY(QString backUpFailedTips READ backupFailedTips NOTIFY backupFailedTipsChanged FINAL)
    Q_PROPERTY(QString updateInstallLog READ updateInstallLog NOTIFY updateInstallLogChanged FINAL)

    // ---------------更新设置页面数据---------------
    Q_PROPERTY(bool securityUpdateEnabled READ securityUpdateEnabled WRITE setSecurityUpdateEnabled NOTIFY securityUpdateEnabledChanged FINAL)
    Q_PROPERTY(bool thirdPartyUpdateEnabled READ thirdPartyUpdateEnabled WRITE setThirdPartyUpdateEnabled NOTIFY thirdPartyUpdateEnabledChanged FINAL)
    Q_PROPERTY(bool functionUpdate READ functionUpdate NOTIFY updateModeChanged FINAL)
    Q_PROPERTY(bool securityUpdate READ securityUpdate NOTIFY updateModeChanged FINAL)
    Q_PROPERTY(bool thirdPartyUpdate READ thirdPartyUpdate NOTIFY updateModeChanged FINAL)
    Q_PROPERTY(bool updateModeDisabled READ updateModeDisabled NOTIFY updateModeChanged FINAL)
    Q_PROPERTY(bool downloadSpeedLimitEnabled READ downloadSpeedLimitEnabled NOTIFY downloadSpeedLimitConfigChanged FINAL)
    Q_PROPERTY(QString downloadSpeedLimitSize READ downloadSpeedLimitSize NOTIFY downloadSpeedLimitConfigChanged FINAL)
    Q_PROPERTY(bool autoDownloadUpdates READ autoDownloadUpdates WRITE setAutoDownloadUpdates NOTIFY autoDownloadUpdatesChanged FINAL)
    Q_PROPERTY(bool idleDownloadEnabled READ idleDownloadEnabled NOTIFY idleDownloadConfigChanged FINAL)
    Q_PROPERTY(QString beginTime READ beginTime NOTIFY idleDownloadConfigChanged FINAL)
    Q_PROPERTY(QString endTime READ endTime NOTIFY idleDownloadConfigChanged FINAL)
    Q_PROPERTY(bool updateNotify READ updateNotify WRITE setUpdateNotify NOTIFY updateNotifyChanged FINAL)
    Q_PROPERTY(bool autoCleanCache READ autoCleanCache WRITE setAutoCleanCache NOTIFY autoCleanCacheChanged FINAL)
    Q_PROPERTY(bool smartMirrorSwitch READ smartMirrorSwitch WRITE setSmartMirrorSwitch NOTIFY smartMirrorSwitchChanged FINAL)
    Q_PROPERTY(TestingChannelStatus testingChannelStatus READ testingChannelStatus WRITE setTestingChannelStatus NOTIFY testingChannelStatusChanged FINAL)

    Q_PROPERTY(UpdateHistoryModel *historyModel READ historyModel NOTIFY historyModelChanged FINAL)


public:
    explicit UpdateModel(QObject *parent = nullptr);
    ~UpdateModel();

public:
    int lastoreDaemonStatus() const { return m_lastoreDaemonStatus; }
    void setLastoreDaemonStatus(int status);

    bool updateProhibited() const { return m_updateProhibited; }
    void setUpdateProhibited(bool prohibited);

    bool systemActivation() const { return m_systemActivation; }
    void setSystemActivation(bool systemActivation);
    
    void refreshIsUpdateDisabled();

    bool isUpdateDisabled() const { return m_isUpdateDisabled; }
    void setIsUpdateDisabled(bool disabled);

    QString updateDisabledIcon() const { return m_updateDisabledIcon; }
    void setUpdateDisabledIcon(const QString &icon);

    QString updateDisabledTips() const { return m_updateDisabledTips; }
    void setUpdateDisabledTips(const QString &tips);

    bool batterIsOK() const { return m_batterIsOK; }
    void setBatterIsOK(bool ok);

    int lastStatus() const { return m_lastStatus; }
    void setLastStatus(const UpdatesStatus &status, int line, int types = 0);

    bool immutableAutoRecovery() const { return m_immutableAutoRecovery; }
    void setImmutableAutoRecovery(bool value);

    // ---------------检查更新页面数据---------------
    bool showCheckUpdate() const { return m_showCheckUpdate; }
    void setShowCheckUpdate(bool value);

    QString checkUpdateIcon() const { return m_checkUpdateIcon; }
    void setCheckUpdateIcon(const QString &newCheckUpdateIcon);

    double checkUpdateProgress() const { return m_checkUpdateProgress; }
    void setCheckUpdateProgress(double updateProgress);

    UpdatesStatus checkUpdateStatus() const { return m_checkUpdateStatus; }
    void setCheckUpdateStatus(UpdatesStatus newCheckUpdateStatus);
    void updateCheckUpdateUi();

    QString checkUpdateErrTips() const { return m_checkUpdateErrTips; }
    void setCheckUpdateErrTips(const QString &newCheckUpdateErrTips);

    QString checkBtnText() const { return m_checkBtnText; }
    void setCheckBtnText(const QString &newCheckBtnText);

    QString lastCheckUpdateTime() const { return m_lastCheckUpdateTime; }
    void setLastCheckUpdateTime(const QString &lastTime);

    quint64 checkUpdateMode() const { return m_checkUpdateMode; }
    void setCheckUpdateMode(quint64 value);
    void refreshUpdateItemsChecked();

    // ---------------下载、备份、安装列表数据---------------
    UpdateListModel *preUpdatelistModel() const { return m_preUpdatelistModel; }
    void setPreUpdatelistModel(UpdateListModel *newPreUpdatelistModel);

    UpdateListModel *downloadinglistModel() const { return m_downloadinglistModel; }
    void setDownloadinglistModel(UpdateListModel *newDownloadinglistModel);

    UpdateListModel *downloadFailedListModel() const { return m_downloadFailedListModel; }
    void setDownloadFailedListModel(UpdateListModel *newDownloadFailedListModel);

    UpdateListModel *preInstallListModel() const { return m_preInstallListModel; }
    void setPreInstallListModel(UpdateListModel *newPreInstallListModel);

    UpdateListModel *installinglistModel() const { return m_installinglistModel; }
    void setInstallinglistModel(UpdateListModel *newInstallinglistModel);

    UpdateListModel *installCompleteListModel() const { return m_installCompleteListModel; }
    void setInstallCompleteListModel(UpdateListModel *newInstallCompleteListModel);

    UpdateListModel *installFailedListModel() const { return m_installFailedListModel; }
    void setInstallFailedListModel(UpdateListModel *newInstallFailedListModel);

    UpdateListModel *backingUpListModel() const { return m_backingUpListModel; }
    void setBackingUpListModel(UpdateListModel *newBackingUpListModel);

    UpdateListModel *backupFailedListModel() const { return m_backupFailedListModel; }
    void setBackupFailedListModel(UpdateListModel *newBackupFailedListModel);

    bool downloadWaiting() const { return m_downloadWaiting; }
    void setDownloadWaiting(bool waiting);

    bool downloadPaused() const { return m_downloadPaused; }
    void setDownloadPaused(bool paused);

    bool upgradeWaiting() const { return m_upgradeWaiting; }
    void setUpgradeWaiting(bool waiting);

    double downloadProgress() const { return m_downloadProgress; }
    void setDownloadProgress(double downloadProgress);

    double distUpgradeProgress() const { return m_distUpgradeProgress; }
    void setDistUpgradeProgress(double progress);

    double backupProgress() const { return m_backupProgress; }
    void setBackupProgress(double progress);

    QString preUpdateTips() const { return m_preUpdateTips; }
    void setPreUpdateTips(const QString &newPreUpdateTips);

    QString downloadFailedTips() const { return m_downloadFailedTips; }
    void setDownloadFailedTips(const QString &newDownloadFailedTips);

    QString installFailedTips() const { return m_installFailedTips; }
    void setInstallFailedTips(const QString &newInstallFailedTips);

    QString backupFailedTips() const { return m_backupFailedTips; }
    void setBackupFailedTips(const QString &newBackupFailedTips);

    QString updateInstallLog() const { return m_installLog; }
    void setUpdateLog(const QString &log);
    void appendUpdateLog(const QString &log);

    QMap<UpdateType, UpdateItemInfo *> getAllUpdateInfos() const { return m_allUpdateInfos; }
    UpdateItemInfo *updateItemInfo(UpdateType type) const { return m_allUpdateInfos.value(type); }
    void addUpdateInfo(UpdateItemInfo *info);
    void deleteUpdateInfo(UpdateItemInfo *updateItemInfo);
    void resetDownloadInfo();
    void updatePackages(const QMap<QString, QStringList> &packages);

    static QString errorToText(UpdateErrorType error);
    UpdateErrorType lastError(UpdatesStatus status) { return m_errorMap.value(status, NoError); }
    void setLastError(UpdatesStatus status, UpdateErrorType errorType);
    QString lastErrorLog(UpdatesStatus status) const { return m_descriptionMap.value(status, ""); }
    void setLastErrorLog(UpdatesStatus status, const QString &description);

    static ControlPanelType getControlPanelType(UpdatesStatus status);
    static QString updateErrorToString(UpdateErrorType error);

    void setUpdateStatus(const QByteArray &status);
    void refreshUpdateStatus();
    void refreshUpdateUiModel();
    void updateAvailableState();
    void modifyUpdateStatusByBackupStatus(LastoreDaemonUpdateStatus &);
    void updateWaitingStatus(UpdateType updateType, UpdatesStatus status);

    bool isUpdatable() const { return m_isUpdatable; }
    void setIsUpdatable(bool isUpdatable);

    UpdatesStatus updateStatus(ControlPanelType type) const;
    UpdatesStatus updateStatus(UpdateType type) const;
    QList<UpdateType> updateTypesList(ControlPanelType type) const;
    int updateTypes(ControlPanelType type) const;
    QList<UpdatesStatus> allUpdateStatus() const;
    QMap<UpdatesStatus, int> allWaitingStatus() const { return m_waitingStatusMap; };


    // ---------------更新设置页面数据---------------
    bool securityUpdateEnabled() const { return m_securityUpdateEnabled; }
    void setSecurityUpdateEnabled(bool enable);

    bool thirdPartyUpdateEnabled() const { return m_thirdPartyUpdateEnabled; }
    void setThirdPartyUpdateEnabled(bool enable);

    bool functionUpdate() const;
    bool securityUpdate() const;
    bool thirdPartyUpdate() const;
    bool updateModeDisabled() const;
    quint64 updateMode() const { return m_updateMode; }
    void setUpdateMode(quint64 updateMode);
    void setUpdateItemEnabled();

    bool downloadSpeedLimitEnabled() const;
    QString downloadSpeedLimitSize() const;
    DownloadSpeedLimitConfig speedLimitConfig() const;
    void setSpeedLimitConfig(const QByteArray &config);

    bool autoDownloadUpdates() const { return m_autoDownloadUpdates; }
    void setAutoDownloadUpdates(bool autoDownloadUpdates);

    bool idleDownloadEnabled() const;
    QString beginTime() const;
    QString endTime() const;
    IdleDownloadConfig idleDownloadConfig() const { return m_idleDownloadConfig; }
    void setIdleDownloadConfig(const IdleDownloadConfig &config);

    bool updateNotify() { return m_updateNotify; }
    void setUpdateNotify(const bool notify);

    bool autoCleanCache() const { return m_autoCleanCache; }
    void setAutoCleanCache(bool autoCleanCache);

    const QList<AppUpdateInfo> &historyAppInfos() const { return m_historyAppInfos; }
    void setHistoryAppInfos(const QList<AppUpdateInfo> &infos);

    bool smartMirrorSwitch() const { return m_smartMirrorSwitch; }
    void setSmartMirrorSwitch(bool smartMirrorSwitch);

    MirrorInfoList mirrorInfos() const { return m_mirrorList; }
    void setMirrorInfos(const MirrorInfoList& list);

    MirrorInfo defaultMirror() const;
    void setDefaultMirror(const QString &mirrorId);
    
    QMap<QString, int> mirrorSpeedInfo() const { return m_mirrorSpeedInfo; }
    void setMirrorSpeedInfo(const QMap<QString, int> &mirrorSpeedInfo);

    bool netselectExist() const { return m_netselectExist; }
    void setNetselectExist(bool netselectExist);

    TestingChannelStatus testingChannelStatus() const { return m_testingChannelStatus; }
    void setTestingChannelStatus(TestingChannelStatus status);

    QString systemVersionInfo() const { return m_systemVersionInfo; }
    void setSystemVersionInfo(const QString &systemVersionInfo);

    QString showVersion() const { return m_showVersion; }
    void setShowVersion(const QString &showVersion);

    QString baseline() const { return m_baseline; }
    void setBaseline(const QString &baseline);

    bool isP2PUpdateEnabled() const { return m_p2pUpdateEnabled; }
    void setP2PUpdateEnabled(bool enabled);


    Q_INVOKABLE bool isCommunitySystem() const;
    Q_INVOKABLE QString privacyAgreementText() const;

    UpdateHistoryModel *historyModel() const;

public slots:
    void onUpdatePropertiesChanged(const QString &interfaceName,
                                   const QVariantMap &changedProperties,
                                   const QStringList &invalidatedProperties);

Q_SIGNALS:
    void updateProhibitedChanged(bool prohibited);
    void systemActivationChanged(bool systemActivation);
    void isUpdateDisabledChanged(bool isDisabled);
    void updateDisabledIconChanged();
    void updateDisabledTipsChanged();

    void batterIsOKChanged(bool isOK);
    void lastStatusChanged(int status);
    void immutableAutoRecoveryChanged(bool value);

    // 检查更新页面数据
    void showCheckUpdateChanged();
    void checkUpdateIconChanged();
    void checkUpdateProgressChanged();
    void checkUpdateStatusChanged();
    void checkUpdateErrTipsChanged();
    void checkBtnTextChanged();
    void lastCheckUpdateTimeChanged();
    void checkUpdateModeChanged(quint64);

    // 下载、备份、安装列表数据
    void preUpdatelistModelChanged();
    void downloadinglistModelChanged();
    void downloadFailedListModelChanged();
    void preInstallListModelChanged();
    void installinglistModelChanged();
    void installCompleteListModelChanged();
    void installFailedListModelChanged();
    void backingUpListModelChanged();
    void backupFailedListModelChanged();

    void downloadWaitingChanged(bool waiting);
    void downloadPausedChanged(bool paused);
    void upgradeWaitingChanged(bool waiting);

    void downloadProgressChanged(const double &progress);
    void backupProgressChanged(double progress);
    void distUpgradeProgressChanged(double progress);

    void preUpdateTipsChanged();
    void downloadFailedTipsChanged();
    void installFailedTipsChanged();
    void backupFailedTipsChanged();
    void updateInstallLogChanged(const QString &log);

    void updateInfoChanged(UpdateType);
    void isUpdatableChanged(const bool isUpdatablePackages);
    void updateStatusChanged(ControlPanelType, UpdatesStatus);
    void controlTypeChanged();
    void lastErrorChanged(UpdatesStatus, UpdateErrorType);
    void notifyBackupSuccess();

    // 更新设置页面数据
    void securityUpdateEnabledChanged(bool enable);
    void thirdPartyUpdateEnabledChanged(bool enable);
    void updateModeChanged(quint64 updateMode);
    void downloadSpeedLimitConfigChanged();
    void autoDownloadUpdatesChanged(bool autoDownloadUpdates);
    void idleDownloadConfigChanged();
    void updateNotifyChanged(const bool notify);
    void autoCleanCacheChanged(const bool autoCleanCache);
    void smartMirrorSwitchChanged(bool smartMirrorSwitch);
    void defaultMirrorChanged(const MirrorInfo& mirror);
    void mirrorSpeedInfoAvailable(const QMap<QString, int> &mirrorSpeedInfo);
    void netselectExistChanged(const bool netselectExist);
    void testingChannelStatusChanged(TestingChannelStatus status);
    void systemVersionChanged(QString version);
    void showVersionChanged(QString version);
    void baselineChanged(const QString &baseline);
    void p2pUpdateEnableStateChanged(bool enabled);
    void historyModelChanged();

private:
    int m_lastoreDaemonStatus; // 比较重要的数值，每个位标识不同的含义，使用 LastoreDaemonDConfigStatusHelper 对它进行解析
    bool m_updateProhibited;
    bool m_systemActivation;
    bool m_isUpdateDisabled;
    QString m_updateDisabledIcon;
    QString m_updateDisabledTips;
    bool m_batterIsOK;
    int m_lastStatus;
    bool m_immutableAutoRecovery;

    // 检查更新页面数据
    bool m_showCheckUpdate;
    QString m_checkUpdateIcon;
    double m_checkUpdateProgress;
    UpdatesStatus m_checkUpdateStatus;
    QString m_checkUpdateErrTips;
    QString m_checkBtnText;
    QString m_lastCheckUpdateTime;
    quint64 m_checkUpdateMode;

    // 下载、备份、安装列表数据
    UpdateListModel *m_preUpdatelistModel; // preUpdateList qml data
    UpdateListModel *m_downloadinglistModel; // downloadingList qml data
    UpdateListModel *m_downloadFailedListModel; // downloadFailedList qml data
    UpdateListModel *m_preInstallListModel; // preInstallList qml data
    UpdateListModel *m_installinglistModel; // installingList qml data
    UpdateListModel *m_installCompleteListModel; // installCompleteList qml data
    UpdateListModel *m_installFailedListModel; // installFailedList qml data
    UpdateListModel *m_backingUpListModel; // backing up qml data
    UpdateListModel *m_backupFailedListModel; // backup failed qml data
    bool m_downloadWaiting;
    bool m_downloadPaused;
    bool m_upgradeWaiting;
    double m_downloadProgress;
    double m_distUpgradeProgress;
    double m_backupProgress;
    QString m_preUpdateTips;
    QString m_downloadFailedTips;
    QString m_installFailedTips;
    QString m_backupFailedTips;
    QString m_installLog;

    QMap<UpdateType, UpdateItemInfo *> m_allUpdateInfos;
    QMap<UpdatesStatus, UpdateErrorType> m_errorMap;
    QMap<UpdatesStatus, QString> m_descriptionMap;
    QByteArray m_updateStatus; // lastore daemon发上来的原始json数据

    bool m_isUpdatable; // 是否有包可更新
    QMap<ControlPanelType, QPair<UpdatesStatus, QList<UpdateType>>> m_controlStatusMap;
    QMap<UpdatesStatus, int> m_waitingStatusMap;

    // 更新设置页面数据
    bool m_securityUpdateEnabled;
    bool m_thirdPartyUpdateEnabled;
    quint64 m_updateMode;
    QByteArray m_speedLimitConfig;
    bool m_autoDownloadUpdates;
    IdleDownloadConfig m_idleDownloadConfig;
    bool m_updateNotify;
    bool m_autoCleanCache;
    QList<AppUpdateInfo> m_historyAppInfos;
    bool m_smartMirrorSwitch;
    MirrorInfoList m_mirrorList;
    QString m_mirrorId;
    QMap<QString, int> m_mirrorSpeedInfo;
    bool m_netselectExist;
    TestingChannelStatus m_testingChannelStatus;
    QString m_systemVersionInfo;
    QString m_showVersion;
    QString m_baseline;
    bool m_p2pUpdateEnabled;

    // update history qml data
    UpdateHistoryModel *m_historyModel;
};

#endif // UPDATEMODEL_H
