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


class UpdateModel : public QObject
{
    Q_OBJECT

    // 系统激活状态
    Q_PROPERTY(bool systemActivation READ systemActivation WRITE setSystemActivation NOTIFY systemActivationChanged FINAL)

    Q_PROPERTY(int lastStatus READ lastStatus  NOTIFY lastStatusChanged FINAL)

    // 检查更新页面数据
    Q_PROPERTY(bool showCheckUpdate READ showCheckUpdate NOTIFY showCheckUpdateChanged FINAL)
    Q_PROPERTY(bool needDoCheck READ needDoCheck NOTIFY needDoCheckChanged FINAL)
    Q_PROPERTY(QString checkUpdateIcon READ checkUpdateIcon NOTIFY checkUpdateIconChanged FINAL)
    Q_PROPERTY(double checkUpdateProgress READ checkUpdateProgress NOTIFY checkUpdateProgressChanged FINAL)
    Q_PROPERTY(int checkUpdateStatus READ checkUpdateStatus NOTIFY checkUpdateStatusChanged FINAL)
    Q_PROPERTY(QString checkUpdateErrTips READ checkUpdateErrTips NOTIFY checkUpdateErrTipsChanged FINAL)
    Q_PROPERTY(QString checkBtnText READ checkBtnText NOTIFY checkBtnTextChanged FINAL)
    Q_PROPERTY(QString lastCheckUpdateTime READ lastCheckUpdateTime NOTIFY lastCheckUpdateTimeChanged FINAL)

    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged FINAL)
    Q_PROPERTY(double distUpgradeProgress READ distUpgradeProgress NOTIFY distUpgradeProgressChanged FINAL)
    Q_PROPERTY(double backupProgress READ backupProgress NOTIFY backupProgressChanged FINAL)

    Q_PROPERTY(UpdateListModel *preUpdatelistModel READ preUpdatelistModel  NOTIFY preUpdatelistModelChanged FINAL)
    Q_PROPERTY(QString preUpdateTips READ preUpdateTips WRITE setPreUpdateTips NOTIFY preUpdateTipsChanged FINAL)
    Q_PROPERTY(UpdateListModel *preInstallListModel READ preInstallListModel NOTIFY preInstallListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installinglistModel READ installinglistModel NOTIFY installinglistModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installCompleteListModel READ installCompleteListModel  NOTIFY installCompleteListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installFailedListModel READ installFailedListModel  NOTIFY installFailedListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *downloadFailedListModel READ downloadFailedListModel  NOTIFY downloadFailedListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *downloadinglistModel READ downloadinglistModel NOTIFY downloadinglistModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *backingUpListModel READ backingUpListModel NOTIFY backingUpListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *backupFailedListModel READ backupFailedListModel NOTIFY backupFailedListModelChanged FINAL)
    Q_PROPERTY(QString downloadFailedTips READ downloadFailedTips NOTIFY downloadFailedTipsChanged FINAL)
    Q_PROPERTY(QString installFailedTips READ installFailedTips NOTIFY installFailedTipsChanged FINAL)
    Q_PROPERTY(QString backUpFailedTips READ backupFailedTips NOTIFY backupFailedTipsChanged FINAL)

    // 更新设置页面数据
    Q_PROPERTY(bool securityUpdateEnabled READ securityUpdateEnabled WRITE setSecurityUpdateEnabled NOTIFY securityUpdateEnabledChanged FINAL)
    Q_PROPERTY(bool thirdPartyUpdateEnabled READ thirdPartyUpdateEnabled WRITE setThirdPartyUpdateEnabled NOTIFY thirdPartyUpdateEnabledChanged FINAL)
    Q_PROPERTY(bool functionUpdate READ functionUpdate NOTIFY functionUpdateChanged FINAL)
    Q_PROPERTY(bool securityUpdate READ securityUpdate NOTIFY securityUpdateChanged FINAL)
    Q_PROPERTY(bool thirdPartyUpdate READ thirdPartyUpdate NOTIFY thirdPartyUpdateChanged FINAL)
    Q_PROPERTY(bool downloadSpeedLimitEnabled READ downloadSpeedLimitEnabled NOTIFY downloadSpeedLimitConfigChanged FINAL)
    Q_PROPERTY(QString downloadSpeedLimitSize READ downloadSpeedLimitSize NOTIFY downloadSpeedLimitConfigChanged FINAL)
    Q_PROPERTY(bool autoDownloadUpdates READ autoDownloadUpdates WRITE setAutoDownloadUpdates NOTIFY autoDownloadUpdatesChanged FINAL)
    Q_PROPERTY(bool idleDownloadEnabled READ idleDownloadEnabled NOTIFY idleDownloadConfigChanged FINAL)
    Q_PROPERTY(int beginTime READ beginTime NOTIFY idleDownloadConfigChanged FINAL)
    Q_PROPERTY(int endTime READ endTime NOTIFY idleDownloadConfigChanged FINAL)
    Q_PROPERTY(bool updateNotify READ updateNotify WRITE setUpdateNotify NOTIFY updateNotifyChanged FINAL)
    Q_PROPERTY(bool autoCleanCache READ autoCleanCache WRITE setAutoCleanCache NOTIFY autoCleanCacheChanged FINAL)
    Q_PROPERTY(bool smartMirrorSwitch READ smartMirrorSwitch WRITE setSmartMirrorSwitch NOTIFY smartMirrorSwitchChanged FINAL)
    Q_PROPERTY(TestingChannelStatus testingChannelStatus READ testingChannelStatus WRITE setTestingChannelStatus NOTIFY testingChannelStatusChanged FINAL)


public:
    explicit UpdateModel(QObject *parent = nullptr);
    ~UpdateModel();

public:

    void setSecurityUpdateEnabled(bool enable);
    bool securityUpdateEnabled() const { return m_securityUpdateEnabled; }

    void setThirdPartyUpdateEnabled(bool enable);
    bool thirdPartyUpdateEnabled() const { return m_thirdPartyUpdateEnabled; }

    bool functionUpdate() const { return m_functionUpdate; }
    bool securityUpdate() const { return m_securityUpdate; }
    bool thirdPartyUpdate() const { return m_thirdPartyUpdate; }

    void setUpdateType(quint64 updateMode);

    void setMirrorInfos(const MirrorInfoList& list);
    MirrorInfoList mirrorInfos() const { return m_mirrorList; }

    int lastStatus() const { return m_lastStatus; }

    void setLastStatus(const UpdatesStatus &status, int line, int types = 0);

    MirrorInfo defaultMirror() const;
    void setDefaultMirror(const QString &mirrorId);

    QMap<UpdateType, UpdateItemInfo *> allDownloadInfo() const { return m_allUpdateInfos; }

    UpdateItemInfo *updateItemInfo(UpdateType type) const { return m_allUpdateInfos.value(type); }

    QMap<QString, int> mirrorSpeedInfo() const { return m_mirrorSpeedInfo; }

    void setMirrorSpeedInfo(const QMap<QString, int> &mirrorSpeedInfo);

    bool autoDownloadUpdates() const { return m_autoDownloadUpdates; }

    void setAutoDownloadUpdates(bool autoDownloadUpdates);

    bool autoCleanCache() const { return m_autoCleanCache; }

    void setAutoCleanCache(bool autoCleanCache);


#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    bool sourceCheck() const { return m_sourceCheck; }

    void setSourceCheck(bool sourceCheck);
#endif

    bool netselectExist() const { return m_netselectExist; }

    void setNetselectExist(bool netselectExist);

    inline quint64 updateMode() const { return m_updateMode; }

    void setUpdateMode(quint64 updateMode);

    bool smartMirrorSwitch() const { return m_smartMirrorSwitch; }

    void setSmartMirrorSwitch(bool smartMirrorSwitch);

    inline QString systemVersionInfo() const { return m_systemVersionInfo; }

    void setSystemVersionInfo(const QString &systemVersionInfo);

    inline bool systemActivation() const { return m_systemActivation; }
    void setSystemActivation(bool systemActivation);

    bool isUpdatable() const { return m_isUpdatable; }

    const QList<AppUpdateInfo> &historyAppInfos() const { return m_historyAppInfos; }

    void setHistoryAppInfos(const QList<AppUpdateInfo> &infos);

    Q_INVOKABLE bool enterCheckUpdate();

    inline bool updateNotify() { return m_updateNotify; }

    void setUpdateNotify(const bool notify);

    void deleteUpdateInfo(UpdateItemInfo *updateItemInfo);

    void addUpdateInfo(UpdateItemInfo *info);

    QMap<UpdateType, UpdateItemInfo *> getAllUpdateInfos() const { return m_allUpdateInfos; }

    void setLastError(UpdatesStatus status, UpdateErrorType errorType);

    UpdateErrorType lastError(UpdatesStatus status) { return m_errorMap.value(status, NoError); }

    static QString errorToText(UpdateErrorType error);

    void setIdleDownloadConfig(const IdleDownloadConfig &config);

    inline IdleDownloadConfig idleDownloadConfig() const { return m_idleDownloadConfig; }
    bool idleDownloadEnabled() const;
    int beginTime() const;
    int endTime() const;

    QString getMachineID() const;

    void setTestingChannelStatus(TestingChannelStatus status);
    TestingChannelStatus testingChannelStatus() const { return m_testingChannelStatus; }

    QString getTestingChannelServer() const { return m_testingChannelServer; }
    void setTestingChannelServer(const QString server);
    void setCanExitTestingChannel(const bool can);

    qlonglong downloadSize(int updateTypes) const;

    void setSpeedLimitConfig(const QByteArray &config);
    DownloadSpeedLimitConfig speedLimitConfig() const;
    bool downloadSpeedLimitEnabled() const;
    QString downloadSpeedLimitSize() const;

    void setDownloadProgress(double downloadProgress);

    double downloadProgress() const { return m_downloadProgress; }

    void setLastoreDaemonStatus(int status);

    int lastoreDaemonStatus() const { return m_lastoreDeamonStatus; }

    bool isUpdateToDate() const;

    void resetDownloadInfo();

    void refreshUpdateStatus();
    void setUpdateStatus(const QByteArray &status);

    void setCheckUpdateMode(int value);

    int checkUpdateMode() const { return m_checkUpdateMode; }

    void setDistUpgradeProgress(double progress);

    double distUpgradeProgress() const { return m_distUpgradeProgress; }

    void setBackupProgress(double progress);

    double backupProgress() const { return m_backupProgress; }

    UpdatesStatus updateStatus(ControlPanelType type) const;
    UpdatesStatus updateStatus(UpdateType type) const;
    QList<UpdateType> updateTypesList(ControlPanelType type) const;
    int updateTypes(ControlPanelType type) const;

    QList<ControlPanelType> controlTypes() const { return m_controlStatusMap.keys(); }

    QList<UpdatesStatus> allUpdateStatus() const;

    QMap<UpdatesStatus, int> allWaitingStatus() const { return m_waitingStatusMap; };

    void updatePackages(const QMap<QString, QStringList> &packages);

    void setBatterIsOK(bool ok);

    bool batterIsOK() const { return m_batterIsOK; }

    void setShowVersion(const QString &showVersion);

    QString showVersion() const { return m_showVersion; }

    void setBaseline(const QString &baseline);

    inline QString baseline() const { return m_baseline; }

    LastoreDaemonUpdateStatus getLastoreDaemonStatus()
    {
        return LastoreDaemonUpdateStatus::fromJson(m_updateStatus);
    };

    void setLastErrorLog(UpdatesStatus status, const QString &description)
    {
        m_descriptionMap.insert(status, description);
    }

    QString lastErrorLog(UpdatesStatus status) const { return m_descriptionMap.value(status, ""); }

    void setP2PUpdateEnabled(bool enabled);

    bool isP2PUpdateEnabled() const { return m_p2pUpdateEnabled; }

    static bool isSupportedUpdateType(UpdateType type);
    static QList<UpdateType> getSupportUpdateTypes(int updateTypes);
    static QList<UpdatesStatus> getSupportUpdateTypes(ControlPanelType type);
    static ControlPanelType getControlPanelType(UpdatesStatus status);
    static QString updateErrorToString(UpdateErrorType error);
    

    // 检查更新页面数据
    bool showCheckUpdate() const { return m_showCheckUpdate; }
    void setShowCheckUpdate(bool value);

    bool needDoCheck() const { return m_needDoCheck; }
    void setNeedDoCheck(bool value);

    QString checkUpdateIcon() const { return m_checkUpdateIcon; }
    void setCheckUpdateIcon(const QString &newCheckUpdateIcon);

    double checkUpdateProgress() const { return m_checkUpdateProgress; }
    void setCheckUpdateProgress(double updateProgress);
    void updateCheckUpdateUi();

    int checkUpdateStatus() const { return m_checkUpdateStatus; }
    void setCheckUpdateStatus(int newCheckUpdateStatus);

    QString checkUpdateErrTips() const { return m_checkUpdateErrTips; }
    void setCheckUpdateErrTips(const QString &newCheckUpdateErrTips);

    QString checkBtnText() const { return m_checkBtnText; }
    void setCheckBtnText(const QString &newCheckBtnText);

    QString lastCheckUpdateTime() const { return m_lastCheckUpdateTime; }
    void setLastCheckUpdateTime(const QString &lastTime);

    void refreshUpdateUiModel();

    UpdateListModel *preUpdatelistModel() const;
    void setPreUpdatelistModel(UpdateListModel *newPreUpdatelistModel);

    QString preUpdateTips() const;
    void setPreUpdateTips(const QString &newPreUpdateTips);

    UpdateListModel *preInstallListModel() const;
    void setPreInstallListModel(UpdateListModel *newPreInstallListModel);

    UpdateListModel *installinglistModel() const;
    void setInstallinglistModel(UpdateListModel *newInstallinglistModel);

    UpdateListModel *installCompleteListModel() const;
    void setInstallCompleteListModel(UpdateListModel *newInstallCompleteListModel);

    UpdateListModel *installFailedListModel() const;
    void setInstallFailedListModel(UpdateListModel *newInstallFailedListModel);

    UpdateListModel *downloadFailedListModel() const;
    void setDownloadFailedListModel(UpdateListModel *newDownloadFailedListModel);

    UpdateListModel *downloadinglistModel() const;
    void setDownloadinglistModel(UpdateListModel *newDownloadinglistModel);

    UpdateListModel *backingUpListModel() const;
    void setBackingUpListModel(UpdateListModel *newBackingUpListModel);

    UpdateListModel *backupFailedListModel() const;
    void setBackupFailedListModel(UpdateListModel *newBackupFailedListModel);

    QString downloadFailedTips() const;
    void setDownloadFailedTips(const QString &newDownloadFailedTips);

    QString installFailedTips() const;
    void setInstallFailedTips(const QString &newInstallFailedTips);

    QString backupFailedTips() const;
    void setBackupFailedTips(const QString &newBackupFailedTips);

    Q_INVOKABLE bool isCommunitySystem() const;

    void updateAvailableState();

public slots:
    void onUpdatePropertiesChanged(const QString &interfaceName,
                                   const QVariantMap &changedProperties,
                                   const QStringList &invalidatedProperties);

Q_SIGNALS:
    void securityUpdateEnabledChanged(bool enable);
    void thirdPartyUpdateEnabledChanged(bool enable);
    void functionUpdateChanged(bool update);
    void securityUpdateChanged(bool update);
    void thirdPartyUpdateChanged(bool update);
    void autoDownloadUpdatesChanged(bool autoDownloadUpdates);
    void defaultMirrorChanged(const MirrorInfo& mirror);
    void smartMirrorSwitchChanged(bool smartMirrorSwitch);
    void lastStatusChanged(int status);
    void notifyDownloadSizeChanged();
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    void sourceCheckChanged(bool sourceCheck);
#endif
    void mirrorSpeedInfoAvailable(const QMap<QString, int> &mirrorSpeedInfo);
    void autoCleanCacheChanged(const bool autoCleanCache);
    void netselectExistChanged(const bool netselectExist);
    void systemVersionChanged(QString version);
    void systemActivationChanged(bool systemActivation);
    void beginCheckUpdate();
    void updateHistoryAppInfos();
    void updateNotifyChanged(const bool notify);
    void isUpdatableChanged(const bool isUpdatablePackages);
    void testingChannelStatusChanged(TestingChannelStatus status);
    void canExitTestingChannelChanged(const bool can);
    void idleDownloadConfigChanged();
    void updateModeChanged(quint64 updateMode);
    void downloadSpeedLimitConfigChanged();
    void downloadProgressChanged(const double &progress);
    void lastoreDaemonStatusChanged(int status);
    void checkUpdateModeChanged(int);
    void updateInfoChanged(UpdateType);
    void distUpgradeProgressChanged(double progress);
    void updateStatusChanged(ControlPanelType, UpdatesStatus);
    void controlTypeChanged();
    void lastErrorChanged(UpdatesStatus, UpdateErrorType);
    void batterStatusChanged(bool isOK);
    void notifyBackupSuccess();
    void p2pUpdateEnableStateChanged(bool enabled);
    void baselineChanged(const QString &baseline);
    void backupProgressChanged(double progress);

    // 检查更新页面数据
    void showCheckUpdateChanged();
    void needDoCheckChanged();
    void checkUpdateIconChanged();
    void checkUpdateProgressChanged();
    void checkUpdateStatusChanged();
    void checkUpdateErrTipsChanged();
    void checkBtnTextChanged();
    void lastCheckUpdateTimeChanged();

    void preUpdatelistModelChanged();

    void preUpdateTipsChanged();

    void preInstallListModelChanged();

    void installinglistModelChanged();

    void installCompleteListModelChanged();

    void installFailedListModelChanged();

    void downloadFailedListModelChanged();

    void downloadinglistModelChanged();

    void backingUpListModelChanged();

    void backupFailedListModelChanged();

    void downloadFailedTipsChanged();

    void installFailedTipsChanged();

    void backupFailedTipsChanged();

private:
    void setUpdateItemEnabled();
    void initConfig();
    void modifyUpdateStatusByBackupStatus(LastoreDaemonUpdateStatus &);
    void setIsUpdatable(bool isUpdatable);
    void updateWaitingStatus(UpdateType updateType, UpdatesStatus status);

private:
    int m_lastStatus;
    QMap<UpdateType, UpdateItemInfo *> m_allUpdateInfos;
    
    double m_downloadProgress;
    double m_distUpgradeProgress;
    double m_backupProgress;
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    bool m_sourceCheck;
#endif
    bool m_netselectExist;
    bool m_autoCleanCache;
    bool m_autoDownloadUpdates;
    bool m_securityUpdateEnabled;
    bool m_thirdPartyUpdateEnabled;
    quint64 m_updateMode;
    bool m_functionUpdate;
    bool m_securityUpdate;
    bool m_thirdPartyUpdate;
    bool m_updateNotify;
    bool m_smartMirrorSwitch;
    QString m_mirrorId;
    MirrorInfoList m_mirrorList;
    QMap<QString, int> m_mirrorSpeedInfo;
    QString m_systemVersionInfo;
    bool m_systemActivation;
    
    QList<AppUpdateInfo> m_historyAppInfos; // 历史更新应用列表
    QString m_testingChannelServer;
    TestingChannelStatus m_testingChannelStatus;
    bool m_isUpdatable; // 是否有包可更新
    IdleDownloadConfig m_idleDownloadConfig;
    QByteArray m_speedLimitConfig;
    Dtk::Core::DConfig *lastoreDConfig;
    int m_lastoreDeamonStatus; // 比较重要的数值，每个位标识不同的含义，使用LastoreDaemonStatusHelper对它进行解析
    QByteArray m_updateStatus; // lastore daemon发上来的原始json数据
    QMap<ControlPanelType, QPair<UpdatesStatus, QList<UpdateType>>> m_controlStatusMap;
    QMap<UpdatesStatus, UpdateErrorType> m_errorMap;
    QMap<UpdatesStatus, QString> m_descriptionMap;
    QMap<UpdatesStatus, int> m_waitingStatusMap;
    int m_checkUpdateMode;
    bool m_batterIsOK;
    bool m_p2pUpdateEnabled;
    QString m_showVersion;
    QString m_baseline;

    // 检查更新页面数据
    bool m_showCheckUpdate;
    bool m_needDoCheck;
    QString m_checkUpdateIcon;
    double m_checkUpdateProgress;
    int m_checkUpdateStatus;
    QString m_checkUpdateErrTips;
    QString m_checkBtnText;
    QString m_lastCheckUpdateTime;

    // preUpdateList qml data
    UpdateListModel *m_preUpdatelistModel;
    QString m_preUpdateTips;

    // downloadingList qml data
    UpdateListModel *m_downloadinglistModel;
    QString m_downloadTips;

    // preInstallList qml data
    UpdateListModel *m_preInstallListModel;

    // installingList qml data
    UpdateListModel *m_installinglistModel;

    // installCompleteList qml data
    UpdateListModel *m_installCompleteListModel;

    // installFailedList qml data
    UpdateListModel *m_installFailedListModel;
    QString m_installFailedTips;

    // downloadFailedList qml data
    UpdateListModel *m_downloadFailedListModel;
    QString m_downloadFailedTips;

    // backing up qml data
    UpdateListModel *m_backingUpListModel;

    // backup failed qml data
    UpdateListModel *m_backupFailedListModel;
    QString m_backupFailedTips;

};

#endif // UPDATEMODEL_H
