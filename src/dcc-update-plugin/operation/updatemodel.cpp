// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatemodel.h"
#include "updatehistorymodel.h"
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
    , m_lastoreDaemonStatus(0)
    , m_updateProhibited(false)
    , m_systemActivation(false)
    , m_isUpdateDisabled(false)
    , m_updateDisabledIcon("")
    , m_updateDisabledTips("")
    , m_batterIsOK(false)
    , m_lastStatus(Default)
    , m_showCheckUpdate(false)
    , m_checkUpdateIcon("")
    , m_checkUpdateProgress(0.0)
    , m_checkUpdateStatus(UpdatesStatus::Default)
    , m_checkUpdateErrTips("")
    , m_checkBtnText("")
    , m_lastCheckUpdateTime("")
    , m_checkUpdateMode(0)
    , m_preUpdatelistModel(new UpdateListModel(this))
    , m_downloadinglistModel(new UpdateListModel(this))
    , m_downloadFailedListModel(new UpdateListModel(this))
    , m_preInstallListModel(new UpdateListModel(this))
    , m_installinglistModel(new UpdateListModel(this))
    , m_installCompleteListModel(new UpdateListModel(this))
    , m_installFailedListModel(new UpdateListModel(this))
    , m_backingUpListModel(new UpdateListModel(this))
    , m_backupFailedListModel(new UpdateListModel(this))
    , m_downloadWaiting(false)
    , m_downloadPaused(false)
    , m_upgradeWaiting(false)
    , m_downloadProgress(0.0)
    , m_distUpgradeProgress(0.0)
    , m_backupProgress(0.0)
    , m_preUpdateTips("")
    , m_downloadFailedTips("")
    , m_installFailedTips("")
    , m_backupFailedTips("")
    , m_installLog("")
    , m_isUpdatable(false)
    , m_securityUpdateEnabled(false)
    , m_thirdPartyUpdateEnabled(false)
    , m_updateMode(UpdateType::Invalid)
    , m_speedLimitConfig("")
    , m_autoDownloadUpdates(false)
    , m_updateNotify(false)
    , m_autoCleanCache(false)
    , m_smartMirrorSwitch(false)
    , m_mirrorId("")
    , m_netselectExist(false)
    , m_testingChannelStatus(TestingChannelStatus::DeActive)
    , m_systemVersionInfo("")
    , m_showVersion("")
    , m_baseline("")
    , m_p2pUpdateEnabled(false)
    , m_historyModel(new UpdateHistoryModel(this))
{
    qRegisterMetaType<UpdatesStatus>("UpdatesStatus");
    qRegisterMetaType<TestingChannelStatus>("TestingChannelStatus");
    qRegisterMetaType<UpdateType>("UpdateType");
}

UpdateModel::~UpdateModel()
{
    qDeleteAll(m_allUpdateInfos.values());
}

void UpdateModel::setLastoreDaemonStatus(int status)
{
    qCDebug(DCC_UPDATE_MODEL) << "lastore daemon status:" << status;
    m_lastoreDaemonStatus = status;

    if (LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_lastoreDaemonStatus)) {
        setUpdateProhibited(true);
    } else {
        setUpdateProhibited(false);
    }
}

void UpdateModel::setUpdateProhibited(bool prohibited)
{
    if (m_updateProhibited == prohibited) {
        return;
    }
    m_updateProhibited = prohibited;
    Q_EMIT updateProhibitedChanged(prohibited);

    refreshIsUpdateDisabled();
}

void UpdateModel::setSystemActivation(bool systemActivation)
{
    qCInfo(DCC_UPDATE_MODEL) << "System activation:" << systemActivation;
    if (m_systemActivation == systemActivation) {
        return;
    }
    m_systemActivation = systemActivation;
    Q_EMIT systemActivationChanged(systemActivation);

    refreshIsUpdateDisabled();
}

void UpdateModel::refreshIsUpdateDisabled()
{
    if (!m_systemActivation) {
        setIsUpdateDisabled(true);
        setUpdateDisabledIcon("update_no_active");
        setUpdateDisabledTips(tr("Your system is not activated, and it failed to connect to update services"));
    } else if (m_immutableAutoRecovery) {
        setIsUpdateDisabled(true);
        setUpdateDisabledIcon("update_prohibit");
        setUpdateDisabledTips(tr("The system has enabled auto recovery function and does not support updates. If you have any questions, please contact the enterprise administrator"));
    } else if (m_updateProhibited) {
        setIsUpdateDisabled(true);
        setUpdateDisabledIcon("update_prohibit");
        setUpdateDisabledTips(tr("The system updates are disabled. Please contact your administrator for help"));
    } else if (updateModeDisabled()) {
        setIsUpdateDisabled(true);
        setUpdateDisabledIcon("update_nice_service");
        setUpdateDisabledTips(tr("Turn on the switches under Update Content to get better experiences"));
    } else {
        setIsUpdateDisabled(false);
        setUpdateDisabledIcon("");
        setUpdateDisabledTips("");
    }
}

void UpdateModel::setIsUpdateDisabled(bool disabled)
{
    if (m_isUpdateDisabled == disabled)
        return;

    m_isUpdateDisabled = disabled;
    Q_EMIT isUpdateDisabledChanged(disabled);
}

void UpdateModel::setUpdateDisabledIcon(const QString &icon)
{
    if (m_updateDisabledIcon == icon)
        return;

    m_updateDisabledIcon = icon;
    Q_EMIT updateDisabledIconChanged();
}

void UpdateModel::setUpdateDisabledTips(const QString &tips)
{
    if (m_updateDisabledTips == tips)
        return;

    m_updateDisabledTips = tips;
    Q_EMIT updateDisabledTipsChanged();
}

void UpdateModel::setBatterIsOK(bool ok)
{
    if (m_batterIsOK == ok) {
        return;
    }

    m_batterIsOK = ok;
    Q_EMIT batterIsOKChanged(ok);
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

void UpdateModel::setImmutableAutoRecovery(bool value)
{
    qCInfo(DCC_UPDATE_MODEL) << "Set immutable auto recovery: " << value;
    if (m_immutableAutoRecovery == value)
        return;

    m_immutableAutoRecovery = value;
    Q_EMIT immutableAutoRecoveryChanged(value);
    refreshIsUpdateDisabled();
}

void UpdateModel::setShowCheckUpdate(bool value)
{
    if (m_showCheckUpdate == value)
        return;

    m_showCheckUpdate = value;
    emit showCheckUpdateChanged();
}

void UpdateModel::setCheckUpdateIcon(const QString &newCheckUpdateIcon)
{
    if (m_checkUpdateIcon == newCheckUpdateIcon)
        return;
    m_checkUpdateIcon = newCheckUpdateIcon;
    emit checkUpdateIconChanged();
}

void UpdateModel::setCheckUpdateProgress(double updateProgress)
{
    if (!qFuzzyCompare(m_checkUpdateProgress, updateProgress)) {
        m_checkUpdateProgress = updateProgress;
        Q_EMIT checkUpdateProgressChanged();
    }
}

void UpdateModel::setCheckUpdateStatus(UpdatesStatus newCheckUpdateStatus)
{
    if (m_checkUpdateStatus == newCheckUpdateStatus)
        return;

    m_checkUpdateStatus = newCheckUpdateStatus;
    emit checkUpdateStatusChanged();

    updateCheckUpdateUi();
}

void UpdateModel::updateCheckUpdateUi()
{
    switch (m_checkUpdateStatus) {
        case Checking:
            setCheckUpdateErrTips(tr("Checking for updates, please wait…"));
            setCheckUpdateIcon("updating");
            setCheckBtnText("");
            break;
        case CheckingFailed:
            setCheckUpdateErrTips(errorToText(lastError(CheckingFailed)));
            setCheckUpdateIcon("update_failure");
            setCheckBtnText(tr("Check Again"));
            break;
        case Updated:
            setCheckBtnText(tr("Check Again"));
            setCheckUpdateErrTips(tr("Your system is up to date"));
            setCheckUpdateIcon("update_abreast_of_time");
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

void UpdateModel::setLastCheckUpdateTime(const QString& lastTime)
{
    qCInfo(DCC_UPDATE_MODEL) << "Last check time:" << lastTime;
    m_lastCheckUpdateTime = lastTime.left(QString("0000-00-00 00:00:00").size());
    emit lastCheckUpdateTimeChanged();
}

void UpdateModel::setCheckUpdateMode(quint64 value)
{
    qCInfo(DCC_UPDATE_MODEL) << "Set check update mode: " << value;
    if (m_checkUpdateMode == value)
        return;

    m_checkUpdateMode = value;
    Q_EMIT checkUpdateModeChanged(value);

    // 升级时切换用户，再切回来的时候收到的信号时乱序，可能会先收到updateStatusChanged再收到checkUpdateModeChanged
    refreshUpdateItemsChecked();
    refreshUpdateStatus();
}

void UpdateModel::refreshUpdateItemsChecked()
{
    for (const auto item : m_allUpdateInfos.values()) {
        item->setIsChecked(m_checkUpdateMode & item->updateType());
    }
}

void UpdateModel::setPreUpdatelistModel(UpdateListModel *newPreUpdatelistModel)
{
    if (m_preUpdatelistModel == newPreUpdatelistModel)
        return;

    m_preUpdatelistModel = newPreUpdatelistModel;
    emit preUpdatelistModelChanged();
}

void UpdateModel::setDownloadinglistModel(UpdateListModel *newDownloadinglistModel)
{
    if (m_downloadinglistModel == newDownloadinglistModel)
        return;

    m_downloadinglistModel = newDownloadinglistModel;
    emit downloadinglistModelChanged();
}

void UpdateModel::setDownloadFailedListModel(UpdateListModel *newDownloadFailedListModel)
{
    if (m_downloadFailedListModel == newDownloadFailedListModel)
        return;

    m_downloadFailedListModel = newDownloadFailedListModel;
    emit downloadFailedListModelChanged();
}

void UpdateModel::setPreInstallListModel(UpdateListModel *newPreInstallListModel)
{
    if (m_preInstallListModel == newPreInstallListModel)
        return;

    m_preInstallListModel = newPreInstallListModel;
    emit preInstallListModelChanged();
}

void UpdateModel::setInstallinglistModel(UpdateListModel *newInstallinglistModel)
{
    if (m_installinglistModel == newInstallinglistModel)
        return;

    m_installinglistModel = newInstallinglistModel;
    emit installinglistModelChanged();
}

void UpdateModel::setInstallCompleteListModel(UpdateListModel *newInstallCompleteListModel)
{
    if (m_installCompleteListModel == newInstallCompleteListModel)
        return;

    m_installCompleteListModel = newInstallCompleteListModel;
    emit installCompleteListModelChanged();
}

void UpdateModel::setInstallFailedListModel(UpdateListModel *newInstallFailedListModel)
{
    if (m_installFailedListModel == newInstallFailedListModel)
        return;

    m_installFailedListModel = newInstallFailedListModel;
    emit installFailedListModelChanged();
}

void UpdateModel::setBackingUpListModel(UpdateListModel *newBackingUpListModel)
{
    if (m_backingUpListModel == newBackingUpListModel)
        return;

    m_backingUpListModel = newBackingUpListModel;
    emit backingUpListModelChanged();
}

void UpdateModel::setBackupFailedListModel(UpdateListModel *newBackupFailedListModel)
{
    if (m_backupFailedListModel == newBackupFailedListModel)
        return;

    m_backupFailedListModel = newBackupFailedListModel;
    emit backupFailedListModelChanged();
}

void UpdateModel::setDownloadWaiting(bool waiting)
{
    if (m_downloadWaiting == waiting)
        return;

    m_downloadWaiting = waiting;
    Q_EMIT downloadWaitingChanged(waiting);
}

void UpdateModel::setDownloadPaused(bool paused)
{
    if (m_downloadPaused == paused)
        return;

    m_downloadPaused = paused;
    Q_EMIT downloadPausedChanged(paused);
}

void UpdateModel::setUpgradeWaiting(bool waiting)
{
    if (m_upgradeWaiting == waiting)
        return;

    m_upgradeWaiting = waiting;
    Q_EMIT upgradeWaitingChanged(waiting);
}

void UpdateModel::setDownloadProgress(double downloadProgress)
{
    if (qFuzzyCompare(downloadProgress, m_downloadProgress))
        return;
        
    m_downloadProgress = downloadProgress;
    Q_EMIT downloadProgressChanged(downloadProgress);
}

void UpdateModel::setDistUpgradeProgress(double progress)
{
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

void UpdateModel::setPreUpdateTips(const QString &newPreUpdateTips)
{
    if (m_preUpdateTips == newPreUpdateTips)
        return;

    m_preUpdateTips = newPreUpdateTips;
    emit preUpdateTipsChanged();
}

void UpdateModel::setDownloadFailedTips(const QString &newDownloadFailedTips)
{
    if (m_downloadFailedTips == newDownloadFailedTips)
        return;

    m_downloadFailedTips = newDownloadFailedTips;
    emit downloadFailedTipsChanged();
}

void UpdateModel::setInstallFailedTips(const QString &newInstallFailedTips)
{
    if (m_installFailedTips == newInstallFailedTips)
        return;

    m_installFailedTips = newInstallFailedTips;
    emit installFailedTipsChanged();
}

void UpdateModel::setBackupFailedTips(const QString &newBackupFailedTips)
{
    if (m_backupFailedTips == newBackupFailedTips)
        return;

    m_backupFailedTips = newBackupFailedTips;
    emit backupFailedTipsChanged();
}

void UpdateModel::setUpdateLog(const QString &log)
{
    m_installLog = log;
    emit updateInstallLogChanged(m_installLog);
}

void UpdateModel::appendUpdateLog(const QString &log)
{
    m_installLog += log;
    emit updateInstallLogChanged(m_installLog);
}

void UpdateModel::addUpdateInfo(UpdateItemInfo* info)
{
    if (info == nullptr)
        return;

    const auto updateType = info->updateType();
    info->setUpdateStatus(updateStatus(updateType));
    if (m_allUpdateInfos.contains(updateType)) {
        if (m_allUpdateInfos.value(updateType))
            deleteUpdateInfo(m_allUpdateInfos.value(updateType));
        m_allUpdateInfos.remove(updateType);
    }

    qCInfo(DCC_UPDATE_MODEL) << "Add update info:" << info->updateType() << info->updateStatus();
    m_allUpdateInfos.insert(updateType, info);

    if (!info->isUpdateAvailable()) {
        for (auto& pair : m_controlStatusMap) {
            pair.second.removeAll(updateType);
        }
    }

    Q_EMIT updateInfoChanged(updateType);
}

void UpdateModel::deleteUpdateInfo(UpdateItemInfo* updateItemInfo)
{
    if (updateItemInfo != nullptr) {
        updateItemInfo->deleteLater();
        updateItemInfo = nullptr;
    }
}

void UpdateModel::resetDownloadInfo()
{
    for (const auto item : m_allUpdateInfos.values()) {
        item->reset();
    }
}

void UpdateModel::updatePackages(const QMap<QString, QStringList>& packages)
{
    for (const auto item : m_allUpdateInfos.values()) {
        item->setPackages(packages.value(item->typeString()));
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

void UpdateModel::setLastError(UpdatesStatus status, UpdateErrorType errorType)
{
    qCInfo(DCC_UPDATE_MODEL) << "Set last error: " << errorType;
    if (m_errorMap.value(status, NoError) == errorType) {
        return;
    }

    m_errorMap.insert(status, errorType);
    Q_EMIT lastErrorChanged(status, errorType);
}

void UpdateModel::setLastErrorLog(UpdatesStatus status, const QString &description)
{
    m_descriptionMap.insert(status, description);
}

ControlPanelType UpdateModel::getControlPanelType(UpdatesStatus status)
{
    return ControlPanelTypeMapping.value(status, CPT_Invalid);
}

QString UpdateModel::updateErrorToString(UpdateErrorType error)
{
    if (error == UpdateErrorType::DependenciesBrokenError)
        return "dependenciesBroken";

    if (error == UpdateErrorType::DpkgInterrupted)
        return "dpkgInterrupted";

    return "";
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

        if (updateStatus >= Downloading && updateStatus <= DownloadFailed) {
            setDownloadWaiting(false);
        }

        if (updateStatus >= BackingUp && updateStatus <= UpgradeComplete) {
            setUpgradeWaiting(false);
        }
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
        qCDebug(DCC_UPDATE_MODEL) << "refresh Update Ui:" << item->updateType() << item->updateStatus() << item->isUpdateModeEnabled();
        if (!item->isUpdateModeEnabled())
            continue;

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
        case BackupSuccess:
            m_backingUpListModel->addUpdateData(item);
            break;
        case BackupFailed:
            m_backupFailedListModel->addUpdateData(item);
        default:
            break;
        }
    }
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


// lastore返回的update status没有将备份的状态匹配给具体的更新类型,需要根据备份状态修正一下更新状态
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

void UpdateModel::setIsUpdatable(bool isUpdatable)
{
    if (m_isUpdatable == isUpdatable)
        return;

    m_isUpdatable = isUpdatable;
    Q_EMIT isUpdatableChanged(isUpdatable);
}

UpdatesStatus UpdateModel::updateStatus(ControlPanelType type) const
{
    if (!m_controlStatusMap.contains(type)) {
        return UpdatesStatus::Default;
    }

    return m_controlStatusMap.value(type).first;
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

QList<UpdatesStatus> UpdateModel::allUpdateStatus() const
{
    QList<UpdatesStatus> list;
    for (const auto& pair : m_controlStatusMap.values()) {
        list.append(pair.first);
    }

    return list;
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

bool UpdateModel::functionUpdate() const 
{ 
    return m_updateMode & UpdateType::SystemUpdate;
}

bool UpdateModel::securityUpdate() const 
{ 
    return m_updateMode & UpdateType::SecurityUpdate;
}

bool UpdateModel::thirdPartyUpdate() const 
{ 
    return m_updateMode & UpdateType::UnknownUpdate; 
}

bool UpdateModel::updateModeDisabled() const
{
    return m_updateMode == UpdateType::Invalid;
}

void UpdateModel::setUpdateMode(quint64 updateMode)
{
    qCInfo(DCC_UPDATE_MODEL) << "Set update mode:" << updateMode << ", current mode: " << m_updateMode;
    if (m_updateMode == updateMode)
        return;

    m_updateMode = updateMode;

    refreshIsUpdateDisabled();
    setUpdateItemEnabled();
    refreshUpdateStatus();
    updateAvailableState();
    if (m_lastStatus == Updated && m_isUpdatable) {
        setLastStatus(UpdatesAvailable, __LINE__);
    }
    Q_EMIT updateModeChanged(m_updateMode);
}

void UpdateModel::setUpdateItemEnabled()
{
    for (const auto item : m_allUpdateInfos.values()) {
        item->setUpdateModeEnabled(m_updateMode & item->updateType());
    }
}

bool UpdateModel::downloadSpeedLimitEnabled() const
{
    return DownloadSpeedLimitConfig::fromJson(m_speedLimitConfig).downloadSpeedLimitEnabled;
}

QString UpdateModel::downloadSpeedLimitSize() const
{
    return DownloadSpeedLimitConfig::fromJson(m_speedLimitConfig).limitSpeed;
}

DownloadSpeedLimitConfig UpdateModel::speedLimitConfig() const
{
    return DownloadSpeedLimitConfig::fromJson(m_speedLimitConfig);
}

void UpdateModel::setSpeedLimitConfig(const QByteArray& config)
{
    if (m_speedLimitConfig == config)
        return;

    m_speedLimitConfig = config;
    Q_EMIT downloadSpeedLimitConfigChanged();
}

void UpdateModel::setAutoDownloadUpdates(bool autoDownloadUpdates)
{
    if (m_autoDownloadUpdates == autoDownloadUpdates) 
        return;

    m_autoDownloadUpdates = autoDownloadUpdates;
    Q_EMIT autoDownloadUpdatesChanged(autoDownloadUpdates);
}

bool UpdateModel::idleDownloadEnabled() const
{
    return m_idleDownloadConfig.idleDownloadEnabled;
}

QString UpdateModel::beginTime() const
{
    return m_idleDownloadConfig.beginTime;
}

QString UpdateModel::endTime() const
{
    return m_idleDownloadConfig.endTime;
}

void UpdateModel::setIdleDownloadConfig(const IdleDownloadConfig& config)
{
    if (m_idleDownloadConfig == config)
        return;

    m_idleDownloadConfig = config;
    Q_EMIT idleDownloadConfigChanged();
}

void UpdateModel::setUpdateNotify(const bool notify)
{
    if (m_updateNotify == notify) {
        return;
    }

    m_updateNotify = notify;
    Q_EMIT updateNotifyChanged(notify);
}

void UpdateModel::setAutoCleanCache(bool autoCleanCache)
{
    if (m_autoCleanCache == autoCleanCache)
        return;

    m_autoCleanCache = autoCleanCache;
    Q_EMIT autoCleanCacheChanged(autoCleanCache);
}

void UpdateModel::setHistoryAppInfos(const QList<AppUpdateInfo>& infos)
{
    m_historyAppInfos = infos;
}

void UpdateModel::setSmartMirrorSwitch(bool smartMirrorSwitch)
{
    if (m_smartMirrorSwitch == smartMirrorSwitch)
        return;

    m_smartMirrorSwitch = smartMirrorSwitch;

    Q_EMIT smartMirrorSwitchChanged(smartMirrorSwitch);
}

void UpdateModel::setMirrorInfos(const MirrorInfoList& list)
{
    m_mirrorList = list;
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

void UpdateModel::setNetselectExist(bool netselectExist)
{
    if (m_netselectExist == netselectExist) {
        return;
    }

    m_netselectExist = netselectExist;
    Q_EMIT netselectExistChanged(netselectExist);
}

void UpdateModel::setTestingChannelStatus(TestingChannelStatus status)
{
    if (status == m_testingChannelStatus) 
        return;

    m_testingChannelStatus = status;
    Q_EMIT testingChannelStatusChanged(m_testingChannelStatus);
}

void UpdateModel::setSystemVersionInfo(const QString& systemVersionInfo)
{
    if (m_systemVersionInfo == systemVersionInfo)
        return;

    m_systemVersionInfo = systemVersionInfo;
    Q_EMIT systemVersionChanged(systemVersionInfo);
}

void UpdateModel::setShowVersion(const QString &showVersion)
{
    if (m_showVersion == showVersion)
        return;

    m_showVersion = showVersion;
    emit showVersionChanged(m_showVersion);
}

void UpdateModel::setBaseline(const QString &baseline)
{
    if (m_baseline == baseline)
        return;

    m_baseline = baseline;
    Q_EMIT baselineChanged(m_baseline);
}

void UpdateModel::setP2PUpdateEnabled(bool enabled)
{
    if (enabled != m_p2pUpdateEnabled) {
        m_p2pUpdateEnabled = enabled;
        Q_EMIT p2pUpdateEnableStateChanged(enabled);
    }
}

bool UpdateModel::isCommunitySystem() const
{
    return Dtk::Core::DSysInfo::UosCommunity == Dtk::Core::DSysInfo::DSysInfo::uosEditionType();
}

QString UpdateModel::privacyAgreementText() const
{
    const QString& systemLocaleName = QLocale::system().name();
    if (systemLocaleName.length() != QString("zh_CN").length()) {
        qCDebug(DCC_UPDATE_MODEL) << "Get system locale name failed:" << systemLocaleName;
    }
    QString region = systemLocaleName.right(2).toLower();

    QString addr;
    if (DCC_NAMESPACE::IsCommunitySystem) {
        QString communityRegion = "en";
        QStringList chineseRegion = { "cn", "hk", "tw" };
        if (chineseRegion.contains(region)) {
            communityRegion = "zh";
        }
        addr = QString("https://www.deepin.org/%1/agreement/privacy/").arg(communityRegion);
    } else {
        QStringList supportedRegion = { "cn", "en", "tw", "hk", "ti", "uy" };
        if (!supportedRegion.contains(region)) {
            region = "en";
        }
        addr = QString("https://www.uniontech.com/agreement/privacy-%1").arg(region);
    }

    QString link = QString("<a style=\"text-decoration: none\" href=\"%1\">%2</a>").arg(addr).arg(tr("Privacy Policy"));
    return tr("To use this software, you must accept the %1 that accompanies software updates.").arg(link);
}

void UpdateModel::onUpdatePropertiesChanged(const QString& interfaceName, const QVariantMap& changedProperties, const QStringList& invalidatedProperties)
{
    Q_UNUSED(invalidatedProperties)

    if (interfaceName == "org.deepin.dde.Lastore1.Manager") {
        if (changedProperties.contains("CheckUpdateMode")) {
            // 用户A、B都打开控制中心，用户A多次修改CheckUpdateMode后切换到用户B，控制中心会立刻收到多个改变信号
            // 增加一个100ms的防抖，只取最后一次的数值
            static quint64 tmpValue = 0;
            static QTimer* timer = nullptr;
            tmpValue = changedProperties.value("CheckUpdateMode").toULongLong();
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

UpdateHistoryModel *UpdateModel::historyModel() const
{
    return m_historyModel;
}
