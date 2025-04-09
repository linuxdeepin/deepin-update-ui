// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include "common.h"
#include "updatedatastructs.h"
#include "updateiteminfo.h"
#include "utils.h"
#include "mirrorinfolist.h"
#include "appupdateinfolist.h"
#include "updatelistmodel.h"

#include <QJsonDocument>
#include <QObject>

#include <DConfig>


struct UpdateJobErrorMessage {
    QString jobErrorType;
    QString jobErrorMessage;
};

class UpdateModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool showUpdateCtl READ showUpdateCtl  NOTIFY showUpdateCtlChanged FINAL)
    Q_PROPERTY(int lastStatus READ lastStatus  NOTIFY lastStatusChanged FINAL)

    // checkupdate data
    Q_PROPERTY(QString checkBtnText READ getCheckBtnText NOTIFY checkBtnTextChanged FINAL)
    Q_PROPERTY(QString checkUpdateErrTips READ getCheckUpdateErrTips NOTIFY checkUpdateErrTipsChanged FINAL)
    Q_PROPERTY(QString checkUpdateIcon READ checkUpdateIcon  NOTIFY checkUpdateIconChanged FINAL)
    Q_PROPERTY(QString lastCheckUpdateTime READ lastCheckUpdateTime  NOTIFY updateCheckUpdateTime FINAL)
    Q_PROPERTY(int checkUpdateStatus READ checkUpdateStatus NOTIFY checkUpdateStatusChanged FINAL)
    Q_PROPERTY(double checkUpdateProgress READ checkUpdateProgress NOTIFY updateProgressChanged FINAL)
    Q_PROPERTY(double downloadProgress READ downloadProgress NOTIFY downloadProgressChanged FINAL)
    Q_PROPERTY(double distUpgradeProgress READ distUpgradeProgress NOTIFY distUpgradeProgressChanged FINAL)


    Q_PROPERTY(UpdateListModel *preUpdatelistModel READ preUpdatelistModel  NOTIFY preUpdatelistModelChanged FINAL)
    Q_PROPERTY(QString preUpdateTips READ preUpdateTips WRITE setPreUpdateTips NOTIFY preUpdateTipsChanged FINAL)
    Q_PROPERTY(UpdateListModel *preInstallListModel READ preInstallListModel NOTIFY preInstallListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installinglistModel READ installinglistModel NOTIFY installinglistModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installCompleteListModel READ installCompleteListModel  NOTIFY installCompleteListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *installFailedListModel READ installFailedListModel  NOTIFY installFailedListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *downloadFailedListModel READ downloadFailedListModel  NOTIFY downloadFailedListModelChanged FINAL)
    Q_PROPERTY(UpdateListModel *downloadinglistModel READ downloadinglistModel NOTIFY downloadinglistModelChanged FINAL)
    Q_PROPERTY(QString downloadFailedTips READ downloadFailedTips NOTIFY downloadFailedTipsChanged FINAL)
    Q_PROPERTY(QString installFailedTips READ installFailedTips NOTIFY installFailedTipsChanged FINAL)


public:
    explicit UpdateModel(QObject *parent = nullptr);
    ~UpdateModel();

public:
    enum TestingChannelStatus {
        Hidden,
        NotJoined,
        WaitJoined,
        Joined,
    };

    Q_ENUM(TestingChannelStatus);

    void setMirrorInfos(const MirrorInfoList &list);

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

    double checkUpdateProgress() const { return m_checkUpdateProgress; }

    void setCheckUpdateProgress(double updateProgress);

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

    inline UiActiveState systemActivation() const { return m_systemActivation; }

    void setSystemActivation(const UiActiveState &systemActivation);

    bool isUpdatable() const { return m_isUpdatable; }

    const QString &lastCheckUpdateTime() const { return m_lastCheckUpdateTime; }

    void setLastCheckUpdateTime(const QString &lastTime);

    const QList<AppUpdateInfo> &historyAppInfos() const { return m_historyAppInfos; }

    void setHistoryAppInfos(const QList<AppUpdateInfo> &infos);

    bool enterCheckUpdate();

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

    QString getMachineID() const;

    void setTestingChannelStatus(const TestingChannelStatus status);

    TestingChannelStatus getTestingChannelStatus() const { return m_testingChannelStatus; }

    QString getTestingChannelServer() const { return m_testingChannelServer; }

    void setTestingChannelServer(const QString server);
    void setCanExitTestingChannel(const bool can);

    qlonglong downloadSize(int updateTypes) const;

    void setSpeedLimitConfig(const QByteArray &config);
    DownloadSpeedLimitConfig speedLimitConfig() const;

    void setDownloadProgress(double downloadProgress);

    double downloadProgress() const { return m_downloadProgress; }

    void setLastoreDaemonStatus(int status);

    int lastoreDaemonStatus() const { return m_lastoreDeamonStatus; }

    bool isUpdateToDate() const;
    bool isActivationValid() const;

    void resetDownloadInfo();

    void refreshUpdateStatus();
    void setUpdateStatus(const QByteArray &status);

    void setCheckUpdateMode(int value);

    int checkUpdateMode() const { return m_checkUpdateMode; }

    void setDistUpgradeProgress(double progress);

    double distUpgradeProgress() const { return m_distUpgradeProgress; }

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

    bool showUpdateCtl() const;
    void setShowUpdateCtl(bool newShowUpdateCtl);

    QString getCheckBtnText() const;
    void setCheckBtnText(const QString &newCheckBtnText);

    QString getCheckUpdateErrTips() const;
    void setCheckUpdateErrTips(const QString &newCheckUpdateErrTips);

    // checkupdate qml data
    void updateCheckUpdateUi();

    QString checkUpdateIcon() const;
    void setCheckUpdateIcon(const QString &newCheckUpdateIcon);

    int checkUpdateStatus() const;
    void setCheckUpdateStatus(int newCheckUpdateStatus);

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

    QString downloadFailedTips() const;
    void setDownloadFailedTips(const QString &newDownloadFailedTips);

    QString installFailedTips() const;
    void setInstallFailedTips(const QString &newInstallFailedTips);

public slots:
    void onUpdatePropertiesChanged(const QString &interfaceName,
                                   const QVariantMap &changedProperties,
                                   const QStringList &invalidatedProperties);

Q_SIGNALS:
    void autoDownloadUpdatesChanged(const bool &autoDownloadUpdates);
    void defaultMirrorChanged(const MirrorInfo &mirror);
    void smartMirrorSwitchChanged(bool smartMirrorSwitch);
    void lastStatusChanged(int status);
    void notifyDownloadSizeChanged();
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    void sourceCheckChanged(bool sourceCheck);
#endif
    void mirrorSpeedInfoAvailable(const QMap<QString, int> &mirrorSpeedInfo);
    void updateProgressChanged(const double &updateProgress);
    void autoCleanCacheChanged(const bool autoCleanCache);
    void netselectExistChanged(const bool netselectExist);
    void systemVersionChanged(QString version);
    void systemActivationChanged(UiActiveState systemActivation);
    void beginCheckUpdate();
    void updateCheckUpdateTime();
    void updateHistoryAppInfos();
    void updateNotifyChanged(const bool notify);
    void isUpdatableChanged(const bool isUpdatablePackages);
    void testingChannelStatusChanged(const TestingChannelStatus status);
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

    void showUpdateCtlChanged();

    void checkBtnTextChanged();

    void checkUpdateErrTipsChanged();

    void checkUpdateIconChanged();

    void checkUpdateStatusChanged();

    void preUpdatelistModelChanged();

    void preUpdateTipsChanged();

    void preInstallListModelChanged();

    void installinglistModelChanged();

    void installCompleteListModelChanged();

    void installFailedListModelChanged();

    void downloadFailedListModelChanged();

    void downloadinglistModelChanged();



    void downloadFailedTipsChanged();

    void installFailedTipsChanged();

private:
    void setUpdateItemEnabled();
    void initConfig();
    void modifyUpdateStatusByBackupStatus(LastoreDaemonUpdateStatus &);
    void updateAvailableState();
    void setIsUpdatable(bool isUpdatable);
    void updateWaitingStatus(UpdateType updateType, UpdatesStatus status);

private:
    int m_lastStatus;
    QMap<UpdateType, UpdateItemInfo *> m_allUpdateInfos;
    double m_checkUpdateProgress;
    double m_downloadProgress;
    double m_distUpgradeProgress;
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    bool m_sourceCheck;
#endif
    bool m_netselectExist;
    bool m_autoCleanCache;
    bool m_autoDownloadUpdates;
    quint64 m_updateMode;
    bool m_updateNotify;
    bool m_smartMirrorSwitch;
    QString m_mirrorId;
    MirrorInfoList m_mirrorList;
    QMap<QString, int> m_mirrorSpeedInfo;
    QString m_systemVersionInfo;
    UiActiveState m_systemActivation;
    QString m_lastCheckUpdateTime;          // 上次检查更新时间
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

    bool m_showUpdateCtl;
    // checkupdate qml data
    QString checkBtnText;
    QString checkUpdateErrTips;
    QString m_checkUpdateIcon;
    int m_checkUpdateStatus;

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

};

#endif // UPDATEMODEL_H
