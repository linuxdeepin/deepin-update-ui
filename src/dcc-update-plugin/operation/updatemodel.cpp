// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatemodel.h"
#include "operation/common.h"
#include "utils.h"

#include <DSysInfo>

using namespace dcc::update::common;

Q_LOGGING_CATEGORY(DCC_UPDATE_MODEL, "dcc-update-model")

DCORE_USE_NAMESPACE
static const QMap<UpdatesStatus, ControlPanelType> ControlPanelTypeMapping = {
    { Default, CPT_Invalid },
    { UpdatesAvailable, CPT_Available },
    { DownloadWaiting, CPT_Available },
    { Downloading, CPT_Downloading },
    { DownloadPaused, CPT_Downloading },
    { DownloadFailed, CPT_DownloadFailed },
    { Downloaded, CPT_Downloaded },
    { UpgradeWaiting, CPT_Downloaded },
    { BackingUp, CPT_Upgrade },
    { BackupSuccess, CPT_Upgrade },
    { BackupFailed, CPT_BackupFailed },
    { UpgradeReady, CPT_Upgrade },
    { Upgrading, CPT_Upgrade },
    { UpgradeFailed, CPT_UpgradeFailed },
    { UpgradeSuccess, CPT_NeedRestart },
};

UpdateModel::UpdateModel(QObject* parent)
    : QObject(parent)
    , m_lastStatus(Default)
    , m_downloadProgress(0.0)
    , m_distUpgradeProgress(0.0)
#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
    , m_sourceCheck(false)
#endif
    , m_netselectExist(false)
    , m_autoCleanCache(false)
    , m_autoDownloadUpdates(false)
    , m_securityUpdateEnabled(false)
    , m_thirdPartyUpdateEnabled(false)
    , m_updateMode(UpdateType::Invalid)
    , m_functionUpdate(false)
    , m_securityUpdate(false)
    , m_thirdPartyUpdate(false)
    , m_updateNotify(false)
    , m_smartMirrorSwitch(false)
    , m_mirrorId(QString())
    , m_systemVersionInfo(QString())
    , m_systemActivation(false)
    , m_testingChannelServer(QString())
    , m_testingChannelStatus(TestingChannelStatus::DeActive)
    , m_isUpdatable(false)
    , lastoreDConfig(DConfig::create("org.deepin.dde.lastore", "org.deepin.dde.lastore", "", this))
    , m_lastoreDeamonStatus(0)
    , m_checkUpdateMode(0)
    , m_batterIsOK(false)
    , m_p2pUpdateEnabled(false)
    , m_showUpdateCtl(false)
    , m_checkUpdateIcon("")
    , m_checkUpdateProgress(0.0)
    , m_checkUpdateStatus(UpdatesStatus::Default)
    , m_checkUpdateErrTips("")
    , m_checkBtnText("")
    , m_lastCheckUpdateTime("")
    , m_preUpdatelistModel(new UpdateListModel(this))
    , m_downloadinglistModel(new UpdateListModel(this))
    , m_preInstallListModel(new UpdateListModel(this))
    , m_installinglistModel(new UpdateListModel(this))
    , m_installCompleteListModel(new UpdateListModel(this))
    , m_installFailedListModel(new UpdateListModel(this))
    , m_downloadFailedListModel(new UpdateListModel(this))
    , m_backingUpListModel(new UpdateListModel(this))
    , m_backupFailedListModel(new UpdateListModel(this))
{
    qRegisterMetaType<TestingChannelStatus>("TestingChannelStatus");
    qRegisterMetaType<UpdateType>("UpdateType");

    initConfig();
}

UpdateModel::~UpdateModel()
{
    qDeleteAll(m_allUpdateInfos.values());
}

void UpdateModel::initConfig()
{
    if (lastoreDConfig && lastoreDConfig->isValid()) {
        setLastoreDaemonStatus(lastoreDConfig->value("lastore-daemon-status").toInt());
        connect(lastoreDConfig, &DConfig::valueChanged, this, [this](const QString& key) {
            if ("lastore-daemon-status" == key) {
                bool ok;
                int value = lastoreDConfig->value(key).toInt(&ok);
                if (!ok || m_lastoreDeamonStatus == value)
                    return;

                setLastoreDaemonStatus(value);
            }
        });
    } else {
        qCWarning(DCC_UPDATE_MODEL) << "Lastore dconfig is nullptr or invalid";
    }
}

void UpdateModel::setSecurityUpdateEnabled(bool enable)
{
    if (m_securityUpdateEnabled == enable)
        return;

    m_securityUpdateEnabled = enable;
    Q_EMIT securityUpdateEnabledChanged(enable);
}

void UpdateModel::setThirdPartyUpdateEnabled(bool enable)
{
    if (m_thirdPartyUpdateEnabled == enable)
        return;

    m_thirdPartyUpdateEnabled = enable;
    Q_EMIT thirdPartyUpdateEnabledChanged(enable);
}

void UpdateModel::setUpdateType(quint64 updateMode)
{
    m_functionUpdate = updateMode & UpdateType::SystemUpdate;
    emit functionUpdateChanged(m_functionUpdate);

    m_securityUpdate = updateMode & UpdateType::SecurityUpdate;
    emit securityUpdateChanged(m_securityUpdate);

    m_thirdPartyUpdate = updateMode & UpdateType::UnknownUpdate;
    emit thirdPartyUpdateChanged(m_thirdPartyUpdate);
}

void UpdateModel::setMirrorInfos(const MirrorInfoList& list)
{
    m_mirrorList = list;
}

void UpdateModel::setDefaultMirror(const QString& mirrorId)
{
    if (mirrorId == "")
        return;
    m_mirrorId = mirrorId;

    QList<MirrorInfo>::iterator it = m_mirrorList.begin();
    for (; it != m_mirrorList.end(); ++it) {
        if ((*it).m_id == mirrorId) {
            Q_EMIT defaultMirrorChanged(*it);
        }
    }
}

void UpdateModel::setMirrorSpeedInfo(const QMap<QString, int>& mirrorSpeedInfo)
{
    m_mirrorSpeedInfo = mirrorSpeedInfo;

    if (mirrorSpeedInfo.keys().length())
        Q_EMIT mirrorSpeedInfoAvailable(mirrorSpeedInfo);
}

void UpdateModel::setAutoDownloadUpdates(bool autoDownloadUpdates)
{
    if (m_autoDownloadUpdates != autoDownloadUpdates) {
        m_autoDownloadUpdates = autoDownloadUpdates;
        Q_EMIT autoDownloadUpdatesChanged(autoDownloadUpdates);
    }
}

MirrorInfo UpdateModel::defaultMirror() const
{
    QList<MirrorInfo>::const_iterator it = m_mirrorList.begin();
    for (; it != m_mirrorList.end(); ++it) {
        if ((*it).m_id == m_mirrorId) {
            return *it;
        }
    }

    return m_mirrorList.at(0);
}

void UpdateModel::setLastStatus(const UpdatesStatus& status, int line, int types)
{
    qCInfo(DCC_UPDATE_MODEL) << "Status: ======== " << status << ", types:" << types << ", line:" << line;
    if (status == UpgradeWaiting || status == DownloadWaiting) {
        m_waitingStatusMap.insert(status, types);
    }

    if (m_lastStatus != status) {
        m_lastStatus = status;
        Q_EMIT lastStatusChanged(m_lastStatus);
    }
}

void UpdateModel::setAutoCleanCache(bool autoCleanCache)
{
    if (m_autoCleanCache == autoCleanCache)
        return;

    m_autoCleanCache = autoCleanCache;
    Q_EMIT autoCleanCacheChanged(autoCleanCache);
}

void UpdateModel::setCheckUpdateProgress(double updateProgress)
{
    if (!qFuzzyCompare(m_checkUpdateProgress, updateProgress)) {
        m_checkUpdateProgress = updateProgress;
        Q_EMIT checkUpdateProgressChanged();
    }
}

void UpdateModel::setNetselectExist(bool netselectExist)
{
    if (m_netselectExist == netselectExist) {
        return;
    }

    m_netselectExist = netselectExist;

    Q_EMIT netselectExistChanged(netselectExist);
}

void UpdateModel::setUpdateMode(quint64 updateMode)
{
    qCInfo(DCC_UPDATE_MODEL) << "Set update mode:" << updateMode << ", current mode: " << m_updateMode;

    if (m_updateMode == updateMode)
        return;

    m_updateMode = updateMode;

    setUpdateType(m_updateMode);
    setUpdateItemEnabled();
    refreshUpdateStatus();
    updateAvailableState();
    if (m_lastStatus == Updated && m_isUpdatable) {
        setLastStatus(UpdatesAvailable, __LINE__);
    }
    Q_EMIT updateModeChanged(m_updateMode);
}

void UpdateModel::setSmartMirrorSwitch(bool smartMirrorSwitch)
{
    if (m_smartMirrorSwitch == smartMirrorSwitch)
        return;

    m_smartMirrorSwitch = smartMirrorSwitch;

    Q_EMIT smartMirrorSwitchChanged(smartMirrorSwitch);
}

void UpdateModel::setSystemVersionInfo(const QString& systemVersionInfo)
{
    if (m_systemVersionInfo == systemVersionInfo)
        return;

    m_systemVersionInfo = systemVersionInfo;

    Q_EMIT systemVersionChanged(systemVersionInfo);
}

void UpdateModel::setSystemActivation(bool systemActivation)
{
    qCInfo(DCC_UPDATE_MODEL) << "System activation:" << systemActivation;
    if (m_systemActivation == systemActivation) {
        return;
    }
    m_systemActivation = systemActivation;
    Q_EMIT systemActivationChanged(systemActivation);
}

void UpdateModel::setIsUpdatable(bool isUpdatable)
{
    if (m_isUpdatable == isUpdatable)
        return;

    m_isUpdatable = isUpdatable;
    Q_EMIT isUpdatableChanged(isUpdatable);
}

#ifndef DISABLE_SYS_UPDATE_SOURCE_CHECK
void UpdateModel::setSourceCheck(bool sourceCheck)
{
    if (m_sourceCheck == sourceCheck)
        return;

    m_sourceCheck = sourceCheck;

    Q_EMIT sourceCheckChanged(sourceCheck);
}
#endif

void UpdateModel::setLastCheckUpdateTime(const QString& lastTime)
{
    qCDebug(DCC_UPDATE_MODEL) << "Last check time:" << lastTime;
    m_lastCheckUpdateTime = lastTime.left(QString("0000-00-00 00:00:00").size());
    emit lastCheckUpdateTimeChanged();
}

void UpdateModel::setHistoryAppInfos(const QList<AppUpdateInfo>& infos)
{
    m_historyAppInfos = infos;
}

bool UpdateModel::enterCheckUpdate()
{
    static const int AUTO_CHECK_UPDATE_CIRCLE = 3600 * 24; // 决定进入检查更新界面是否自动检查,单位：小时; 默认24小时
    return QDateTime::fromString(m_lastCheckUpdateTime, "yyyy-MM-dd hh:mm:ss").secsTo(QDateTime::currentDateTime()) > AUTO_CHECK_UPDATE_CIRCLE;
}

void UpdateModel::setUpdateNotify(const bool notify)
{
    if (m_updateNotify == notify) {
        return;
    }

    m_updateNotify = notify;
    Q_EMIT updateNotifyChanged(notify);
}

void UpdateModel::deleteUpdateInfo(UpdateItemInfo* updateItemInfo)
{
    if (updateItemInfo != nullptr) {
        updateItemInfo->deleteLater();
        updateItemInfo = nullptr;
    }
}

void UpdateModel::addUpdateInfo(UpdateItemInfo* info)
{
    if (info == nullptr)
        return;

    const auto updateType = info->updateType();
    qCDebug(DCC_UPDATE_MODEL) << "Add update info:" << info->updateType();
    info->setUpdateStatus(updateStatus(updateType));
    if (m_allUpdateInfos.contains(updateType)) {
        if (m_allUpdateInfos.value(updateType))
            deleteUpdateInfo(m_allUpdateInfos.value(updateType));
        m_allUpdateInfos.remove(updateType);
    }

    m_allUpdateInfos.insert(updateType, info);

    if (!info->isUpdateAvailable()) {
        for (auto& pair : m_controlStatusMap) {
            pair.second.removeAll(updateType);
        }
    }

    Q_EMIT updateInfoChanged(updateType);
}

void UpdateModel::setLastError(UpdatesStatus status, UpdateErrorType errorType)
{
    qCInfo(DCC_UPDATE_MODEL) << "Set last error: " << errorType;
    if (m_errorMap.value(status, NoError) == errorType) {
        return;
    }

    m_errorMap.insert(status, errorType);
    Q_EMIT lastErrorChanged(status, errorType);
}

void UpdateModel::setP2PUpdateEnabled(bool enabled)
{
    if (enabled != m_p2pUpdateEnabled) {
        m_p2pUpdateEnabled = enabled;
        Q_EMIT p2pUpdateEnableStateChanged(enabled);
    }
}

QString UpdateModel::getMachineID() const
{
    QProcess process;
    auto args = QStringList();
    args.append("-c");
    args.append("eval `apt-config shell Token Acquire::SmartMirrors::Token`; echo $Token");
    process.start("sh", args);
    process.waitForFinished();
    const auto token = QString(process.readAllStandardOutput());
    const auto list = token.split(";");
    for (const auto& line : list) {
        const auto key = line.section("=", 0, 0);
        if (key == "i") {
            const auto value = line.section("=", 1);
            return value;
        }
    }
    return "";
}

void UpdateModel::setTestingChannelStatus(TestingChannelStatus status)
{
    if (status != m_testingChannelStatus) {
        m_testingChannelStatus = status;
        Q_EMIT testingChannelStatusChanged(m_testingChannelStatus);
    }
}

void UpdateModel::setTestingChannelServer(const QString server)
{
    m_testingChannelServer = server;
}

void UpdateModel::setCanExitTestingChannel(const bool can)
{
    Q_EMIT canExitTestingChannelChanged(can);
}

void UpdateModel::onUpdatePropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties, const QStringList& invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties)

    if (interfaceName == "org.deepin.dde.Lastore1.Manager") {
        if (changedProperties.contains("UpdateStatus")) {
            setUpdateStatus(changedProperties.value("UpdateStatus").toByteArray());
        }

        if (changedProperties.contains("CheckUpdateMode")) {
            // 用户A、B都打开控制中心，用户A多次修改CheckUpdateMode后切换到用户B，控制中心会立刻收到多个改变信号
            // 增加一个100ms的防抖，只取最后一次的数值
            static int tmpValue = 0;
            static QTimer* timer = nullptr;
            tmpValue = changedProperties.value("CheckUpdateMode").toInt();
            if (!timer) {
                timer = new QTimer(this);
                timer->setInterval(100);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, this, [this] {
                    setCheckUpdateMode(tmpValue);
                    timer->deleteLater();
                    timer = nullptr;
                });
            }
            timer->start();
        }
    }

    if (interfaceName == "org.deepin.dde.Lastore1.Updater") {
        if (changedProperties.contains("IdleDownloadConfig")) {
            setIdleDownloadConfig(IdleDownloadConfig::toConfig(changedProperties.value("IdleDownloadConfig").toByteArray()));
        }

        if (changedProperties.contains("DownloadSpeedLimitConfig")) {
            setSpeedLimitConfig(changedProperties.value("DownloadSpeedLimitConfig").toByteArray());
        }

        if (changedProperties.contains("P2PUpdateEnable")) {
            setP2PUpdateEnabled(changedProperties.value("P2PUpdateEnable").toBool());
        }
    }
}

void UpdateModel::setIdleDownloadConfig(const IdleDownloadConfig& config)
{
    if (m_idleDownloadConfig == config)
        return;

    m_idleDownloadConfig = config;
    Q_EMIT idleDownloadConfigChanged();
}

bool UpdateModel::idleDownloadEnabled() const
{
    return m_idleDownloadConfig.idleDownloadEnabled;
}

int UpdateModel::beginTime() const
{
    QTime time = QTime::fromString(m_idleDownloadConfig.beginTime);
    return time.hour() * 60 + time.minute();
}

int UpdateModel::endTime() const
{
    QTime time = QTime::fromString(m_idleDownloadConfig.endTime);
    return time.hour() * 60 + time.minute();
}

qlonglong UpdateModel::downloadSize(int updateTypes) const
{
    qlonglong downloadSize = 0;
    for (auto info : m_allUpdateInfos.values()) {
        if (info->updateType() & updateTypes) {
            downloadSize += info->downloadSize();
        }
    }
    return downloadSize;
}

void UpdateModel::setUpdateItemEnabled()
{
    for (const auto item : m_allUpdateInfos.values()) {
        item->setUpdateModeEnabled(m_updateMode & item->updateType());
    }
}

QString UpdateModel::errorToText(UpdateErrorType error)
{
    static QMap<UpdateErrorType, QString> errorText = {
        { UnKnown, tr("Unknown error") },
        { DownloadingNoSpace, tr("Downloading updates failed. Please free up %1 disk space first.") },
        { DependenciesBrokenError, tr("Dependency error, failed to detect the updates") },
        { NoNetwork, tr("Please check your network and try again.") },
        { DownloadingNoNetwork, tr("Downloading updates failed. Please check your network and try again.") },
        { CanNotBackup, tr("Unable to perform system backup. If you continue the updates, you cannot roll back to the old system later.") },
        { BackupFailedUnknownReason, tr("If you continue the updates, you cannot roll back to the old system later.") },
        { NoSpace, tr("Insufficient disk space") },
        { DpkgInterrupted, tr("DPKG error") },
        { DpkgError, tr("DPKG error") },
        { FileMissing, tr("File missing") },
        { PlatformUnreachable, tr("Service connection is abnormal, please check the network and try again") },
        { InvalidSourceList, tr("The repository source configuration is not valid, please check and try again.") },
    };

    return errorText.value(error);
}

void UpdateModel::setSpeedLimitConfig(const QByteArray& config)
{
    if (m_speedLimitConfig == config)
        return;

    m_speedLimitConfig = config;
    Q_EMIT downloadSpeedLimitConfigChanged();
}

DownloadSpeedLimitConfig UpdateModel::speedLimitConfig() const
{
    return DownloadSpeedLimitConfig::fromJson(m_speedLimitConfig);
}

bool UpdateModel::downloadSpeedLimitEnabled() const
{
    return DownloadSpeedLimitConfig::fromJson(m_speedLimitConfig).downloadSpeedLimitEnabled;
}

QString UpdateModel::downloadSpeedLimitSize() const
{
    return DownloadSpeedLimitConfig::fromJson(m_speedLimitConfig).limitSpeed;
}

void UpdateModel::setDownloadProgress(double downloadProgress)
{
    if (!qFuzzyCompare(m_downloadProgress, downloadProgress)) {
        m_downloadProgress = downloadProgress;
        Q_EMIT downloadProgressChanged(downloadProgress);
    }
}

void UpdateModel::setLastoreDaemonStatus(int status)
{
    if (m_lastoreDeamonStatus == status)
        return;

    m_lastoreDeamonStatus = status;
    Q_EMIT lastoreDaemonStatusChanged(status);
}

bool UpdateModel::isUpdateToDate() const
{
    for (const auto item : m_allUpdateInfos.values()) {
        if (item->isUpdateAvailable()) {
            return false;
        }
    }

    return true;
}

void UpdateModel::resetDownloadInfo()
{
    for (const auto item : m_allUpdateInfos.values()) {
        item->reset();
    }
}

void UpdateModel::refreshUpdateStatus()
{
    if (m_updateStatus.isEmpty()) {
        return;
    }

    auto lastoreUpdateStatus = LastoreDaemonUpdateStatus::fromJson(m_updateStatus);
    modifyUpdateStatusByBackupStatus(lastoreUpdateStatus);
    if (lastoreUpdateStatus.backupStatus == BackupSuccess) {
        Q_EMIT notifyBackupSuccess();
    }
    for (auto info : m_allUpdateInfos.values()) {
        info->setUpdateStatus(lastoreUpdateStatus.m_statusMap.value(info->updateType(), Default));

    }

    auto it = lastoreUpdateStatus.m_statusMap.begin();
    for (; it != lastoreUpdateStatus.m_statusMap.end(); it++) {
        const auto updateType = it.key();
        const auto updateStatus = it.value();
        const auto controlType = getControlPanelType(updateStatus);

        if (it.value() == Default || (updateItemInfo(updateType) && !updateItemInfo(updateType)->isUpdateAvailable())) {
            qCInfo(DCC_UPDATE_MODEL) << updateType << " is not available";
            continue;
        }
        if (!m_controlStatusMap.contains(controlType)) {
            qCInfo(DCC_UPDATE_MODEL) << "Insert control type:" << controlType;
            m_controlStatusMap.insert( controlType, qMakePair<UpdatesStatus, QList<UpdateType>>(std::move(const_cast<UpdatesStatus&>(updateStatus)), { updateType }));
            Q_EMIT updateStatusChanged(controlType, updateStatus);
        }

        // 判断updateType在对应的control中，修改status即可
        // 如果updateType不在对应的control中，从其他control中移除updateType
        auto controlIt = m_controlStatusMap.begin();
        for (; controlIt != m_controlStatusMap.end(); controlIt++) {
            if (controlIt.key() == controlType) {
                if (controlIt.value().second.contains(updateType)) {
                    if (controlIt.value().first != updateStatus) {
                        qCInfo(DCC_UPDATE_MODEL) << controlType << " change status from " << controlIt.value().first << " to " << updateStatus;
                        controlIt.value().first = updateStatus;
                        Q_EMIT updateStatusChanged(controlIt.key(), updateStatus);
                    }
                } else {
                    qCInfo(DCC_UPDATE_MODEL) << "Append " << updateType << " to " << controlType;
                    controlIt.value().second.append(updateType);
                }
            } else {
                if (controlIt.value().second.contains(updateType)) {
                    qCInfo(DCC_UPDATE_MODEL) << "Remove " << updateType << " from " <<controlIt.key();
                    controlIt.value().second.removeOne(updateType);
                }

                // 将Ready状态还原为本来的状态
                if (Downloading == updateStatus && CPT_Available == controlIt.key()) {
                    controlIt.value().first = UpdatesAvailable;
                } else if ((Upgrading == updateStatus || BackingUp == updateStatus) && CPT_Downloaded == controlIt.key()) {
                    controlIt.value().first = Downloaded;
                }
            }
        }

        updateWaitingStatus(updateType, updateStatus);
     }

    // 清理m_controlStatusMap中无用的control
    for (auto key : m_controlStatusMap.keys()) {
        bool exist = false;
        for (auto status : lastoreUpdateStatus.m_statusMap.values()) {
            if (getControlPanelType(status) == key) {
                exist = true;
                break;
            }
        }

        if (!exist) {
            qCInfo(DCC_UPDATE_MODEL) << "Remove control type:" << key;
            m_controlStatusMap.remove(key);
        }
    }

    refreshUpdateUiModel();
    Q_EMIT controlTypeChanged();
}

void UpdateModel::updateWaitingStatus(UpdateType updateType, UpdatesStatus updateStatus)
{
    int downloadWaitingTypes = m_waitingStatusMap.value(DownloadWaiting, 0);
    if (updateStatus > DownloadWaiting && updateStatus <= DownloadFailed && downloadWaitingTypes & updateType) {
        m_waitingStatusMap.remove(DownloadWaiting);
        return;
    }

    int upgradeWaitingTypes = m_waitingStatusMap.value(UpgradeWaiting, 0);
    if (updateStatus > UpgradeWaiting && updateStatus <= UpgradeComplete && upgradeWaitingTypes & updateType) {
        m_waitingStatusMap.remove(UpgradeWaiting);
    }
}

QString UpdateModel::installFailedTips() const
{
    return m_installFailedTips;
}

void UpdateModel::setInstallFailedTips(const QString &newInstallFailedTips)
{
    if (m_installFailedTips == newInstallFailedTips)
        return;
    m_installFailedTips = newInstallFailedTips;
    emit installFailedTipsChanged();
}

QString UpdateModel::downloadFailedTips() const
{
    return m_downloadFailedTips;
}

void UpdateModel::setDownloadFailedTips(const QString &newDownloadFailedTips)
{
    if (m_downloadFailedTips == newDownloadFailedTips)
        return;
    m_downloadFailedTips = newDownloadFailedTips;
    emit downloadFailedTipsChanged();
}

QString UpdateModel::backupFailedTips() const
{
    return m_backupFailedTips;
}

void UpdateModel::setBackupFailedTips(const QString &newBackupFailedTips)
{
    if (m_backupFailedTips == newBackupFailedTips)
        return;
    m_backupFailedTips = newBackupFailedTips;
    emit backupFailedTipsChanged();
}

UpdateListModel *UpdateModel::downloadinglistModel() const
{
    return m_downloadinglistModel;
}

void UpdateModel::setDownloadinglistModel(UpdateListModel *newDownloadinglistModel)
{
    if (m_downloadinglistModel == newDownloadinglistModel)
        return;
    m_downloadinglistModel = newDownloadinglistModel;
    emit downloadinglistModelChanged();
}

UpdateListModel *UpdateModel::backingUpListModel() const
{
    return m_backingUpListModel;
}

void UpdateModel::setBackingUpListModel(UpdateListModel *newBackingUpListModel)
{
    if (m_backingUpListModel == newBackingUpListModel)
        return;
    m_backingUpListModel = newBackingUpListModel;
    emit backingUpListModelChanged();
}

UpdateListModel *UpdateModel::backupFailedListModel() const
{
    return m_backupFailedListModel;
}

void UpdateModel::setBackupFailedListModel(UpdateListModel *newBackupFailedListModel)
{
    if (m_backupFailedListModel == newBackupFailedListModel)
        return;
    m_backupFailedListModel = newBackupFailedListModel;
    emit backupFailedListModelChanged();
}

UpdateListModel *UpdateModel::downloadFailedListModel() const
{
    return m_downloadFailedListModel;
}

void UpdateModel::setDownloadFailedListModel(UpdateListModel *newDownloadFailedListModel)
{
    if (m_downloadFailedListModel == newDownloadFailedListModel)
        return;
    m_downloadFailedListModel = newDownloadFailedListModel;
    emit downloadFailedListModelChanged();
}

UpdateListModel *UpdateModel::installFailedListModel() const
{
    return m_installFailedListModel;
}

void UpdateModel::setInstallFailedListModel(UpdateListModel *newInstallFailedListModel)
{
    if (m_installFailedListModel == newInstallFailedListModel)
        return;
    m_installFailedListModel = newInstallFailedListModel;
    emit installFailedListModelChanged();
}

UpdateListModel *UpdateModel::installCompleteListModel() const
{
    return m_installCompleteListModel;
}

void UpdateModel::setInstallCompleteListModel(UpdateListModel *newInstallCompleteListModel)
{
    if (m_installCompleteListModel == newInstallCompleteListModel)
        return;
    m_installCompleteListModel = newInstallCompleteListModel;
    emit installCompleteListModelChanged();
}

UpdateListModel *UpdateModel::installinglistModel() const
{
    return m_installinglistModel;
}

void UpdateModel::setInstallinglistModel(UpdateListModel *newInstallinglistModel)
{
    if (m_installinglistModel == newInstallinglistModel)
        return;
    m_installinglistModel = newInstallinglistModel;
    emit installinglistModelChanged();
}

UpdateListModel *UpdateModel::preInstallListModel() const
{
    return m_preInstallListModel;
}

void UpdateModel::setPreInstallListModel(UpdateListModel *newPreInstallListModel)
{
    if (m_preInstallListModel == newPreInstallListModel)
        return;
    m_preInstallListModel = newPreInstallListModel;
    emit preInstallListModelChanged();
}

QString UpdateModel::preUpdateTips() const
{
    return m_preUpdateTips;
}

void UpdateModel::setPreUpdateTips(const QString &newPreUpdateTips)
{
    if (m_preUpdateTips == newPreUpdateTips)
        return;
    m_preUpdateTips = newPreUpdateTips;
    emit preUpdateTipsChanged();
}

UpdateListModel *UpdateModel::preUpdatelistModel() const
{
    return m_preUpdatelistModel;
}

void UpdateModel::setPreUpdatelistModel(UpdateListModel *newPreUpdatelistModel)
{
    if (m_preUpdatelistModel == newPreUpdatelistModel)
        return;
    m_preUpdatelistModel = newPreUpdatelistModel;
    emit preUpdatelistModelChanged();
}

void UpdateModel::setCheckUpdateStatus(int newCheckUpdateStatus)
{
    if (m_checkUpdateStatus == newCheckUpdateStatus)
        return;
    m_checkUpdateStatus = newCheckUpdateStatus;
    emit checkUpdateStatusChanged();

    updateCheckUpdateUi();
}

void UpdateModel::refreshUpdateUiModel()
{
    if (m_preUpdatelistModel) {
        m_preUpdatelistModel->clearAllData();
    }

    if (m_downloadinglistModel) {
        m_downloadinglistModel->clearAllData();
    }

    if (m_installinglistModel) {
        m_installinglistModel->clearAllData();
    }

    if (m_installCompleteListModel) {
        m_installCompleteListModel->clearAllData();
    }

    if (m_installFailedListModel) {
        m_installFailedListModel->clearAllData();
    }

    if (m_downloadFailedListModel) {
        m_downloadFailedListModel->clearAllData();
    }

    if (m_preInstallListModel) {
        m_preInstallListModel->clearAllData();
    }

    if (m_backingUpListModel) {
        m_backingUpListModel->clearAllData();
    }

    if (m_backupFailedListModel) {
        m_backupFailedListModel->clearAllData();
    }

    for (auto item : m_allUpdateInfos.values()) {
        switch (item->updateStatus()) {
        case Updated:
            m_installCompleteListModel->addUpdateData(item);
            break;
        case UpdatesAvailable:
            m_preUpdatelistModel->addUpdateData(item);
            break;
        case DownloadWaiting:
        case Downloading:
        case DownloadPaused:
        case UpgradeWaiting:
            m_downloadinglistModel->addUpdateData(item);
            break;
        case Downloaded:
            m_preInstallListModel->addUpdateData(item);
            break;
        case DownloadFailed:
            m_downloadFailedListModel->addUpdateData(item);
            break;
        case UpgradeReady:
        case Upgrading:
            m_installinglistModel->addUpdateData(item);
            break;
        case  UpgradeFailed:
            m_installFailedListModel->addUpdateData(item);
            break;
        case UpgradeSuccess:
        case UpgradeComplete:
            m_installCompleteListModel->addUpdateData(item);
            break;
        case BackingUp:
            m_backingUpListModel->addUpdateData(item);
            break;
        case BackupFailed:
            m_backupFailedListModel->addUpdateData(item);
        default:
            break;
        }
    }
}

void UpdateModel::setCheckUpdateIcon(const QString &newCheckUpdateIcon)
{
    if (m_checkUpdateIcon == newCheckUpdateIcon)
        return;
    m_checkUpdateIcon = newCheckUpdateIcon;
    emit checkUpdateIconChanged();
}

void UpdateModel::updateCheckUpdateUi()
{
    switch (m_checkUpdateStatus) {
        case Checking:
            setCheckUpdateErrTips(tr("Checking for updates, please wait…"));
            setCheckUpdateIcon("updating");
            setCheckBtnText(tr(""));
            break;
        case UpdatesAvailable:
            break;
        case Updated:
            setCheckBtnText(tr("Check Again"));
            setCheckUpdateErrTips(tr("Your system is up to date"));
            setCheckUpdateIcon("update_abreast_of_time");
            break;
        case CheckingFailed:
            setCheckUpdateErrTips(errorToText(lastError(CheckingFailed)));
            setCheckUpdateIcon("update_failure");
            setCheckBtnText(tr("Check Again"));
            break;
        default:
            setCheckBtnText(tr(""));
            return;
    }
}

void UpdateModel::setCheckUpdateErrTips(const QString &newCheckUpdateErrTips)
{
    if (m_checkUpdateErrTips == newCheckUpdateErrTips)
        return;

    m_checkUpdateErrTips = newCheckUpdateErrTips;
    emit checkUpdateErrTipsChanged();
}

void UpdateModel::setCheckBtnText(const QString &newCheckBtnText)
{
    if (m_checkBtnText == newCheckBtnText)
        return;

    m_checkBtnText = newCheckBtnText;
    emit checkBtnTextChanged();
}

/**
 * @brief lastore返回的update status没有将备份的状态匹配给具体的更新类型,需要根据备份状态修正一下更新状态
 *
 */
void UpdateModel::modifyUpdateStatusByBackupStatus(LastoreDaemonUpdateStatus& lastoreUpdateStatus)
{
    // Dirty work：当备份失败 & 当前类型的更新状态是已下载 & 是选中状态时，将此更新类型的更新状态改为备份失败
    auto const backupStatus = lastoreUpdateStatus.backupStatus;
    if (backupStatus != BackupFailed
        && backupStatus != BackingUp
        && backupStatus != BackupSuccess) {
        return;
    }

    auto it = lastoreUpdateStatus.m_statusMap.begin();
    for (; it != lastoreUpdateStatus.m_statusMap.end(); it++) {
        // UpgradeReady状态时不处理备份失败的情况，否则重试的时候会闪现一下继续更新的按钮
        const bool updateStatusNeedHandle = it.value() == Downloaded || (it.value() == UpgradeReady && backupStatus != BackupFailed);
        if (lastoreUpdateStatus.backupFailedType & it.key() && updateStatusNeedHandle) {
            it.value() = BackupFailed;
            setLastError(BackupFailed, lastoreUpdateStatus.backupError);
        }

        const bool updateTypeNeedHandle = lastoreUpdateStatus.triggerBackingUpType & m_checkUpdateMode & it.key();
        if (updateTypeNeedHandle && updateStatusNeedHandle) {
            it.value() = backupStatus;
            if (backupStatus == BackupFailed) {
                setLastError(BackupFailed, lastoreUpdateStatus.backupError);
            }
        }
    }
}

void UpdateModel::setUpdateStatus(const QByteArray& status)
{
    qCInfo(DCC_UPDATE_MODEL) << "Lastore update status:" << status;
    if (m_updateStatus == status)
        return;

    m_updateStatus = status;
    refreshUpdateStatus();
    updateAvailableState();
}

QList<UpdatesStatus> UpdateModel::getSupportUpdateTypes(ControlPanelType type)
{
    QList<UpdatesStatus> list;
    auto it = ControlPanelTypeMapping.cbegin();
    for (; it != ControlPanelTypeMapping.cend(); it++) {
        if (it.value() == type)
            list.append(it.key());
    }

    return list;
}

ControlPanelType UpdateModel::getControlPanelType(UpdatesStatus status)
{
    return ControlPanelTypeMapping.value(status, CPT_Invalid);
}

UpdatesStatus UpdateModel::updateStatus(UpdateType type) const
{
    for (const auto& pair : m_controlStatusMap.values()) {
        if (pair.second.contains(type)) {
            return pair.first;
        }
    }

    return UpdatesStatus::Default;
}

UpdatesStatus UpdateModel::updateStatus(ControlPanelType type) const
{
    if (!m_controlStatusMap.contains(type)) {
        return UpdatesStatus::Default;
    }

    return m_controlStatusMap.value(type).first;
}

QList<UpdateType> UpdateModel::updateTypesList(ControlPanelType type) const
{
    if (!m_controlStatusMap.contains(type)) {
        return {};
    }

    return m_controlStatusMap.value(type).second;
}

int UpdateModel::updateTypes(ControlPanelType type) const
{
    QList<UpdateType> list = updateTypesList(type);
    int types = 0;
    for (const auto& item : list) {
        types |= item;
    }
    return types;
}

void UpdateModel::setCheckUpdateMode(int value)
{
    qCInfo(DCC_UPDATE_MODEL) << "Set check update mode: " << value;
    if (m_checkUpdateMode == value)
        return;

    m_checkUpdateMode = value;
    Q_EMIT checkUpdateModeChanged(value);

    // 升级时切换用户，再切回来的时候收到的信号时乱序，可能会先收到updateStatusChanged再收到checkUpdateModeChanged
    refreshUpdateStatus();
}

void UpdateModel::setDistUpgradeProgress(double progress)
{
    qDebug() << " setDistUpgradeProgress ============ " << progress;
    if (qFuzzyCompare(progress, m_distUpgradeProgress))
        return;

    m_distUpgradeProgress = progress;
    Q_EMIT distUpgradeProgressChanged(m_distUpgradeProgress);
}

void UpdateModel::setBackupProgress(double progress)
{
    if (qFuzzyCompare(progress, m_backupProgress))
        return;

    m_backupProgress = progress;
    Q_EMIT backupProgressChanged(m_backupProgress);
}

QList<UpdatesStatus> UpdateModel::allUpdateStatus() const
{
    QList<UpdatesStatus> list;
    for (const auto& pair : m_controlStatusMap.values()) {
        list.append(pair.first);
    }

    return list;
}

void UpdateModel::updatePackages(const QMap<QString, QStringList>& packages)
{
    for (const auto item : m_allUpdateInfos.values()) {
        item->setPackages(packages.value(item->typeString()));
    }
}

bool UpdateModel::isSupportedUpdateType(UpdateType type)
{
    const static QList<UpdateType> supportedTypes = { SystemUpdate, SecurityUpdate, UnknownUpdate };
    return supportedTypes.contains(type);
}

QList<UpdateType> UpdateModel::getSupportUpdateTypes(int updateTypes)
{
    const static QList<UpdateType> supportedTypes = { SystemUpdate, SecurityUpdate, UnknownUpdate };
    QList<UpdateType> list;
    for (const auto type : supportedTypes) {
        if (type & updateTypes) {
            list.append(type);
        }
    }

    return list;
}

QString UpdateModel::updateErrorToString(UpdateErrorType error)
{
    if (error == UpdateErrorType::DependenciesBrokenError)
        return "dependenciesBroken";

    if (error == UpdateErrorType::DpkgInterrupted)
        return "dpkgInterrupted";

    return "";
}

void UpdateModel::setBatterIsOK(bool ok)
{
    if (m_batterIsOK == ok) {
        return;
    }

    m_batterIsOK = ok;
    Q_EMIT batterStatusChanged(ok);
}

void UpdateModel::setShowVersion(const QString &showVersion)
{
    m_showVersion = showVersion;
}

void UpdateModel::setBaseline(const QString &baseline)
{
    if (m_baseline == baseline)
        return;

    m_baseline = baseline;

    Q_EMIT baselineChanged(m_baseline);
}

void UpdateModel::updateAvailableState()
{
    auto it = m_controlStatusMap.begin();
    for (; it != m_controlStatusMap.end(); it++) {
        auto pair = it.value();
        if ((pair.first >= UpdatesAvailable && pair.first <= UpgradeComplete && (m_updateMode & updateTypes(it.key())))) {
            setIsUpdatable(true);
            return;
        }
    }

    setIsUpdatable(false);
}

void UpdateModel::setShowUpdateCtl(bool newShowUpdateCtl)
{
    if (m_showUpdateCtl == newShowUpdateCtl)
        return;

    m_showUpdateCtl = newShowUpdateCtl;
    emit showUpdateCtlChanged();
}

bool UpdateModel::isCommunitySystem() const
{
    return Dtk::Core::DSysInfo::UosCommunity == Dtk::Core::DSysInfo::DSysInfo::uosEditionType();
}
