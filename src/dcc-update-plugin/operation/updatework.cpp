// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatework.h"
#include "utils.h"
#include "dconfigwatcher.h"
#include "updateloghelper.h"

#include <QDBusError>
#include <QDesktopServices>
#include <QFuture>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVariant>
#include <QtConcurrent>

#include <DDBusSender>
#include <DNotifySender>

Q_LOGGING_CATEGORY(DCC_UPDATE_WORKER, "dcc-update-worker")

using namespace DCC_NAMESPACE;

const QString TestingChannel = "testing Channel";
const QString TestingChannelPackage = "deepin-unstable-source";
const QString ServiceLink = QStringLiteral("https://insider.deepin.org");
const QString ChangeLogFile = "/usr/share/deepin/release-note/UpdateInfo.json";
const QString ChangeLogDic = "/usr/share/deepin/";
const QString UpdateLogTmpFile = "/tmp/deepin-update-log.json";

const int LOWEST_BATTERY_PERCENT = 60;

static const QStringList DCC_CONFIG_FILES {
    "/etc/deepin/dde-control-center.conf",
    "/usr/share/dde-control-center/dde-control-center.conf"
};

void notifyError(const QString &summary, const QString &body)
{
    DUtil::DNotifySender(summary)
            .appIcon("")
            .appName("org.deepin.dde.control-center")
            .appBody(body)
            .timeOut(5000)
            .call();
}

void notifyErrorWithoutBody(const QString &summary)
{
    DUtil::DNotifySender(summary)
            .appIcon("")
            .appName("org.deepin.dde.control-center")
            .timeOut(5000)
            .call();
}

static int TestMirrorSpeedInternal(const QString& url, QPointer<QObject> baseObject)
{
    if (!baseObject || QCoreApplication::closingDown()) {
        return -1;
    }

    QStringList args;
    args << url << "-s"
         << "1";

    QProcess process;
    process.start("netselect", args);

    if (!process.waitForStarted()) {
        return 10000;
    }

    do {
        if (!baseObject || QCoreApplication::closingDown()) {
            process.kill();
            process.terminate();
            process.waitForFinished(1000);

            return -1;
        }

        if (process.waitForFinished(500))
            break;
    } while (process.state() == QProcess::Running);

    const QString output = process.readAllStandardOutput().trimmed();
    const QStringList result = output.split(' ');

    if (!result.first().isEmpty()) {
        return result.first().toInt();
    }

    return 10000;
}

UpdateWorker::UpdateWorker(UpdateModel* model, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_updateInter(new UpdateDBusProxy(this))
    , m_lastoreHeartBeatTimer(new QTimer)
    , m_machineid(std::nullopt)
    , m_testingChannelUrl(std::nullopt)
    , m_checkUpdateJob(nullptr)
    , m_fixErrorJob(nullptr)
    , m_downloadJob(nullptr)
    , m_distUpgradeJob(nullptr)
    , m_backupJob(nullptr)
    , m_installPackageJob(nullptr)
    , m_removePackageJob(nullptr)
{
    qRegisterMetaType<UpdatesStatus>("UpdatesStatus");
    qRegisterMetaType<UiActiveState>("UiActiveState");
    qRegisterMetaType<ControlPanelType>("ControlPanelType");

    initConnect();
}

UpdateWorker::~UpdateWorker()
{
    deleteJob(m_checkUpdateJob);
    deleteJob(m_fixErrorJob);
    deleteJob(m_downloadJob);
    deleteJob(m_distUpgradeJob);
    deleteJob(m_backupJob);
    deleteJob(m_installPackageJob);
    deleteJob(m_removePackageJob);

    if (m_lastoreHeartBeatTimer != nullptr) {
        if (m_lastoreHeartBeatTimer->isActive()) {
            m_lastoreHeartBeatTimer->stop();
        }
        delete m_lastoreHeartBeatTimer;
        m_lastoreHeartBeatTimer = nullptr;
    }
}

void UpdateWorker::initConnect()
{
    QDBusConnection::systemBus().connect("com.deepin.license", 
                                         "/com/deepin/license/Info", 
                                         "com.deepin.license.Info", 
                                         "LicenseStateChange", this, SLOT(onLicenseStateChange()));
    // systemActivationChanged是在线程中发出
    connect(this, &UpdateWorker::systemActivationChanged, m_model, &UpdateModel::setSystemActivation, Qt::QueuedConnection);

    connect(m_updateInter, &UpdateDBusProxy::BatteryPercentageChanged, this, &UpdateWorker::onPowerChange);
    connect(m_updateInter, &UpdateDBusProxy::OnBatteryChanged, this, &UpdateWorker::onPowerChange);
    connect(m_updateInter, &UpdateDBusProxy::UpdateModeChanged, this, &UpdateWorker::onUpdateModeChanged);
    connect(m_updateInter, &UpdateDBusProxy::JobListChanged, this, &UpdateWorker::onJobListChanged);
    connect(m_updateInter, &UpdateDBusProxy::UpdateStatusChanged, this, &UpdateWorker::onUpdateStatusChanged);
    connect(m_updateInter, &UpdateDBusProxy::ClassifiedUpdatablePackagesChanged, this, &UpdateWorker::onClassifiedUpdatablePackagesChanged);

    QDBusConnection::systemBus().connect("org.deepin.dde.Lastore1",
                                         "/org/deepin/dde/Lastore1",
                                         "org.freedesktop.DBus.Properties",
                                         "PropertiesChanged", m_model, SLOT(onUpdatePropertiesChanged(QString, QVariantMap, QStringList)));
    connect(m_updateInter, &UpdateDBusProxy::UpdateNotifyChanged, m_model, &UpdateModel::setUpdateNotify);
    connect(m_updateInter, &UpdateDBusProxy::AutoCleanChanged, m_model, &UpdateModel::setAutoCleanCache);
    connect(m_updateInter, &UpdateDBusProxy::AutoDownloadUpdatesChanged, m_model, &UpdateModel::setAutoDownloadUpdates);
    connect(m_updateInter, &UpdateDBusProxy::MirrorSourceChanged, m_model, &UpdateModel::setDefaultMirror);
    if (IsCommunitySystem) {
        connect(m_updateInter, &UpdateDBusProxy::EnableChanged, m_model, &UpdateModel::setSmartMirrorSwitch);
    }

    m_lastoreHeartBeatTimer->setInterval(60000);
    m_lastoreHeartBeatTimer->start();
    connect(m_lastoreHeartBeatTimer, &QTimer::timeout, this, &UpdateWorker::refreshLastTimeAndCheckCircle);

    connect(DConfigWatcher::instance(), &DConfigWatcher::notifyDConfigChanged, [this](const QString &moduleName, const QString &configName) {
        qCDebug(DCC_UPDATE_WORKER) << "Config changed:" << moduleName << configName;

        if (moduleName != "update") {
            return;
        }

        if (configName == "updateThirdPartySource") {
            m_model->setThirdPartyUpdateEnabled(IsCommunitySystem ? true : DConfigWatcher::instance()->getValue(DConfigWatcher::update, "updateThirdPartySource").toString() != "Hidden");
        } else if (configName == "updateSafety") {
            m_model->setSecurityUpdateEnabled(DConfigWatcher::instance()->getValue(DConfigWatcher::update, configName).toString() != "Hidden");
        } else if (configName == "updateHistoryEnabled") {
            // m_model->setUpdateHistoryEnabled(DConfigWatcher::instance()->getValue(DConfigWatcher::update, configName).toBool());
        } else if (configName == "p2pUpdateEnabled") {
            // m_model->setP2PUpdateEnabled(DConfigWatcher::instance()->getValue(DConfigWatcher::update, configName).toBool());
        }
    });
}

void UpdateWorker::activate()
{
    qCInfo(DCC_UPDATE_WORKER) << "Active update worker";

    onLicenseStateChange();
    onPowerChange();
    updateSystemVersion();
    refreshLastTimeAndCheckCircle();
    initTestingChannel();

    m_model->setUpdateMode(m_updateInter->updateMode());
    m_model->setCheckUpdateMode(m_updateInter->checkUpdateMode());
    m_model->setSecurityUpdateEnabled(DConfigWatcher::instance()->getValue(DConfigWatcher::update, "updateSafety").toString() != "Hidden");
    m_model->setThirdPartyUpdateEnabled(IsCommunitySystem ? true : DConfigWatcher::instance()->getValue(DConfigWatcher::update, "updateThirdPartySource").toString() != "Hidden");
    m_model->setSpeedLimitConfig(m_updateInter->downloadSpeedLimitConfig().toUtf8());
    m_model->setAutoDownloadUpdates(m_updateInter->autoDownloadUpdates());
    QString config = m_updateInter->idleDownloadConfig();
    m_model->setIdleDownloadConfig(IdleDownloadConfig::toConfig(config.toUtf8()));
    m_model->setUpdateNotify(m_updateInter->updateNotify());
    m_model->setAutoCleanCache(m_updateInter->autoClean());
    m_model->setP2PUpdateEnabled(m_updateInter->p2pUpdateEnable());
    if (IsCommunitySystem) {
        m_model->setSmartMirrorSwitch(m_updateInter->enable());
    }
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    refreshMirrors();
#endif

    m_model->setUpdateStatus(m_updateInter->updateStatus().toUtf8());

    if (LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_model->lastoreDaemonStatus())) {
        m_model->setLastStatus(UpdatesStatus::UpdateIsDisabled, __LINE__);
    } else {
        // 获取当前的job
        const QList<QDBusObjectPath> jobs = m_updateInter->jobList();
        if (jobs.count() > 0) {
            onJobListChanged(jobs);
            // 如果处于下载中或者安装中的时候直接显示更新内容，不检查更新
            const bool isDownloading = m_downloadJob && m_downloadJob->status() != "failed";
            const bool isUpgrading = m_distUpgradeJob && m_distUpgradeJob->status() != "failed";
            if (isUpgrading || isDownloading) {
                auto watcher = new QDBusPendingCallWatcher(m_updateInter->GetUpdateLogs(SystemUpdate | SecurityUpdate), this);
                connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
                    watcher->deleteLater();
                    if (!watcher->isError()) {
                        QDBusPendingReply<QString> reply = watcher->reply();
                        UpdateLogHelper::ref().handleUpdateLog(reply.value());
                    } else {
                        qCWarning(DCC_UPDATE_WORKER) << "Get update log failed";
                    }
                    // 日志处理完了再显示更新内容界面
                    m_model->setLastStatus(CheckingSucceed, __LINE__);
                    setUpdateInfo();
                });
            }
        }
    }
}

void UpdateWorker::getLicenseState()
{
    if (IsCommunitySystem) {
        emit systemActivationChanged(true);
        return;
    }

    QDBusInterface licenseInfo("com.deepin.license",
                               "/com/deepin/license/Info",
                               "com.deepin.license.Info",
                               QDBusConnection::systemBus());
    if (!licenseInfo.isValid()) {
        qCWarning(DCC_UPDATE_WORKER) << "License info dbus is invalid.";
        return;
    }
    UiActiveState reply =
            static_cast<UiActiveState>(licenseInfo.property("AuthorizationState").toInt());
    emit systemActivationChanged(reply == UiActiveState::Authorized || reply == UiActiveState::TrialAuthorized);
}

bool UpdateWorker::checkDbusIsValid()
{
    return checkJobIsValid(m_checkUpdateJob) && checkJobIsValid(m_downloadJob);
}

bool UpdateWorker::checkJobIsValid(QPointer<UpdateJobDBusProxy> dbusJob)
{
    if (dbusJob.isNull())
        return false;

    if (dbusJob->isValid())
        return true;

    dbusJob->deleteLater();
    return false;
}

void UpdateWorker::deleteJob(QPointer<UpdateJobDBusProxy> dbusJob)
{
    if (!dbusJob.isNull()) {
        dbusJob->deleteLater();
        dbusJob = nullptr;
    }
}

void UpdateWorker::cleanLaStoreJob(QPointer<UpdateJobDBusProxy> dbusJob)
{
    if (dbusJob != nullptr) {
        m_updateInter->CleanJob(dbusJob->id());
        deleteJob(dbusJob);
    }
}

UpdateErrorType UpdateWorker::analyzeJobErrorMessage(const QString& jobDescription, UpdatesStatus status)
{
    qCWarning(DCC_UPDATE_WORKER) << "Job description:" << jobDescription;

    QJsonParseError err_rpt;
    QJsonDocument jobErrorMessage = QJsonDocument::fromJson(jobDescription.toUtf8(), &err_rpt);

    if (err_rpt.error != QJsonParseError::NoError) {
        qCWarning(DCC_UPDATE_WORKER) << "Parse json failed";
        return UnKnown;
    }
    const QJsonObject& object = jobErrorMessage.object();
    QString errorType = object.value("ErrType").toString();
    if (errorType.contains("fetchFailed", Qt::CaseInsensitive) || errorType.contains("IndexDownloadFailed", Qt::CaseInsensitive)) {
        if (status == DownloadFailed) {
            return DownloadingNoNetwork;
        }

        return NoNetwork;
    }
    if (errorType.contains("unmetDependencies", Qt::CaseInsensitive) || errorType.contains("dependenciesBroken", Qt::CaseInsensitive)) {
        return DependenciesBrokenError;
    }
    if (errorType.contains("insufficientSpace", Qt::CaseInsensitive)) {
        if (status == DownloadFailed) {
            return DownloadingNoSpace;
        }
        return NoSpace;
    }
    if (errorType.contains("interrupted", Qt::CaseInsensitive)) {
        return DpkgInterrupted;
    }

    if (errorType.contains("platformUnreachable", Qt::CaseInsensitive)) {
        return PlatformUnreachable;
    }

    if (errorType.contains("invalidSourceList", Qt::CaseInsensitive)) {
        return InvalidSourceList;
    }

    return UnKnown;
}

void UpdateWorker::updateNeedDoCheck()
{
    if (!m_model->systemActivation())
        return;

    // 如果打开控制中心后第一次进入检查更新界面,则显示页面并进行检查
    static bool doCheckFirst = true;
    if (doCheckFirst) {
        doCheckFirst = false;
        m_model->setShowCheckUpdate(true);
        m_model->setNeedDoCheck(true);
        return;
    }

    // 如果非第一次进入检查更新界面，则需要判断上次检查时间和当前时间查决定是否需要再次检查, 默认24小时内不再检查
    static const int AUTO_CHECK_UPDATE_CIRCLE = 3600 * 24;
    qint64 checkTimeInterval = QDateTime::fromString(m_model->lastCheckUpdateTime(), "yyyy-MM-dd hh:mm:ss").secsTo(QDateTime::currentDateTime());
    bool bEnter = checkTimeInterval > AUTO_CHECK_UPDATE_CIRCLE;
    qCDebug(DCC_UPDATE_WORKER) << "check time interval:" << checkTimeInterval << " need to check:" << bEnter;
    if (bEnter) {
        m_model->setShowCheckUpdate(true);
        m_model->setNeedDoCheck(true);
    } else {
        m_model->setNeedDoCheck(false);
    }
}

void UpdateWorker::checkForUpdates()
{
    qCInfo(DCC_UPDATE_WORKER) << "Check for updates";
    if (LastoreDaemonDConfigStatusHelper::isUpdateDisabled(m_model->lastoreDaemonStatus())) {
        qCWarning(DCC_UPDATE_WORKER) << "Update is disabled";
        return;
    }

    if (!m_model->systemActivation()) {
        qCWarning(DCC_UPDATE_WORKER) << "System activation is invalid: " << m_model->systemActivation();
        return;
    }

    if (checkDbusIsValid()) {
        qCWarning(DCC_UPDATE_WORKER) << "Check Dbus's validation failed do nothing";
        return;
    }

    if (m_checkUpdateJob) {
        qCWarning(DCC_UPDATE_WORKER) << "Is checking update, won't do it again";
        return;
    }

    const auto& allUpdateStatuses = m_model->allUpdateStatus();
    if (allUpdateStatuses.contains(BackingUp)
        || allUpdateStatuses.contains(Upgrading)
        || allUpdateStatuses.contains(Downloading)
        || allUpdateStatuses.contains(DownloadPaused)) {
        qCInfo(DCC_UPDATE_WORKER) << "Lastore daemon is busy now, current statuses:" << allUpdateStatuses;
        m_model->setShowCheckUpdate(false);
        return;
    }

    m_model->resetDownloadInfo(); // 在检查更新前重置数据，避免有上次检查的数据残留

    QDBusPendingCall call = m_updateInter->UpdateSource();
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, call, watcher] {
        watcher->deleteLater();
        if (!call.isError()) {
            QDBusReply<QDBusObjectPath> reply = call.reply();
            const QString jobPath = reply.value().path();
            setCheckUpdatesJob(jobPath);
        } else {
            qCWarning(DCC_UPDATE_WORKER) << "Check update failed, error: " << call.error().message();
            m_model->setLastStatus(UpdatesStatus::CheckingFailed, __LINE__);
            cleanLaStoreJob(m_checkUpdateJob);
        }
    });
}

void UpdateWorker::setCheckUpdatesJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE_WORKER) << "Set check updates job";
    UpdatesStatus state = m_model->updateStatus(CPT_Downloading);
    if (UpdatesStatus::Downloading != state && UpdatesStatus::DownloadPaused != state) {
        m_model->setLastStatus(UpdatesStatus::Checking, __LINE__);
    }

    m_model->setCheckUpdateStatus(UpdatesStatus::Checking);
    createCheckUpdateJob(jobPath);
}

void UpdateWorker::createCheckUpdateJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE_WORKER) << "Create check update job: " << jobPath;

    if (m_checkUpdateJob != nullptr) {
        qCInfo(DCC_UPDATE_WORKER) << "Check update job existed";
        return;
    }
    m_checkUpdateJob = new UpdateJobDBusProxy(jobPath, this);

    connect(m_checkUpdateJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onCheckUpdateStatusChanged);
    connect(m_checkUpdateJob, &UpdateJobDBusProxy::ProgressChanged, m_model, &UpdateModel::setCheckUpdateProgress, Qt::QueuedConnection);


    m_checkUpdateJob->ProgressChanged(m_checkUpdateJob->progress());
    m_checkUpdateJob->StatusChanged(m_checkUpdateJob->status());
}

void UpdateWorker::refreshLastTimeAndCheckCircle()
{
    QString checkTime;
    m_updateInter->GetCheckIntervalAndTime(checkTime);
    m_model->setLastCheckUpdateTime(checkTime);
}

void UpdateWorker::setUpdateInfo()
{
    const QMap<QString, QStringList>& updatePackages = m_updateInter->classifiedUpdatablePackages();

    QMap<UpdateType, UpdateItemInfo*> updateInfoMap = getAllUpdateInfo(updatePackages);
    bool isUpdated = true;
    for (auto info : updateInfoMap.values()) {
        m_model->addUpdateInfo(info);
        if (info->isUpdateAvailable()) {
            isUpdated = false;
        }
    }
    m_model->refreshUpdateStatus();
    m_model->updateAvailableState();
    m_model->setLastStatus(isUpdated ? Updated : UpdatesAvailable, __LINE__);
}

/**
 * @brief 获取更新信息，包大小、日志等内容
 *
 * @param updatePackages 例如：{{“system”, {"dde-session-ui, code"}}, {"security", {"dde-session-shell"}}}
 * @return QMap<ClassifyUpdateType, UpdateItemInfo *>
 */
QMap<UpdateType, UpdateItemInfo*> UpdateWorker::getAllUpdateInfo(const QMap<QString, QStringList>& updatePackages)
{
    const QStringList& systemPackages = updatePackages.value(SYSTEM_UPGRADE_TYPE_STRING);
    const QStringList& securityPackages = updatePackages.value(SECURITY_UPGRADE_TYPE_STRING);
    const QStringList& unknownPackages = updatePackages.value(UNKNOWN_UPGRADE_STRING);
    const quint64 updateMode = m_model->updateMode();
    QMap<UpdateType, UpdateItemInfo*> resultMap;

    UpdateItemInfo* systemItemInfo = new UpdateItemInfo(SystemUpdate);
    systemItemInfo->setTypeString(SYSTEM_UPGRADE_TYPE_STRING);
    systemItemInfo->setName(tr("System Updates"));
    systemItemInfo->setExplain(tr("Fixed some known bugs and security vulnerabilities"));
    systemItemInfo->setPackages(systemPackages);
    setUpdateItemDownloadSize(systemItemInfo);
    systemItemInfo->setUpdateModeEnabled(updateMode & UpdateType::SystemUpdate);
    resultMap.insert(UpdateType::SystemUpdate, systemItemInfo);

    UpdateItemInfo* securityItemInfo = new UpdateItemInfo(SecurityUpdate);
    securityItemInfo->setTypeString(SECURITY_UPGRADE_TYPE_STRING);
    securityItemInfo->setName(tr("Security Updates"));
    securityItemInfo->setExplain(tr("Fixed some known bugs and security vulnerabilities"));
    securityItemInfo->setPackages(securityPackages);
    setUpdateItemDownloadSize(securityItemInfo);
    securityItemInfo->setUpdateModeEnabled(updateMode & UpdateType::SecurityUpdate);
    resultMap.insert(UpdateType::SecurityUpdate, securityItemInfo);

    UpdateItemInfo* unkownItemInfo = new UpdateItemInfo(UnknownUpdate);
    unkownItemInfo->setTypeString(UNKNOWN_UPGRADE_STRING);
    unkownItemInfo->setName(tr("Third-party Repositories"));
    unkownItemInfo->setPackages(unknownPackages);
    setUpdateItemDownloadSize(unkownItemInfo);
    unkownItemInfo->setUpdateModeEnabled(updateMode & UpdateType::UnknownUpdate);
    resultMap.insert(UpdateType::UnknownUpdate, unkownItemInfo);

    // 将更新日志根据`系统更新`or`安全更新`进行分类，并保存留用
    for (UpdateType type : resultMap.keys()) {
        UpdateLogHelper::ref().updateItemInfo(resultMap.value(type));
    }

    return resultMap;
}

void UpdateWorker::setUpdateItemDownloadSize(UpdateItemInfo* updateItem)
{
    if (updateItem->packages().isEmpty()) {
        return;
    }

    QDBusPendingCall call = m_updateInter->QueryAllSizeWithSource(updateItem->updateType());
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, updateItem, [updateItem, call, watcher] {
        watcher->deleteLater();
        if (!call.isError()) {
            QDBusReply<qlonglong> reply = call.reply();
            qlonglong value = reply.value();
            qCInfo(DCC_UPDATE_WORKER) << "Packages' size:" << value << ", name:" << updateItem->name();
            updateItem->setDownloadSize(value);
        } else {
            qCWarning(DCC_UPDATE_WORKER) << "Get packages size error:" << call.error().message();
        }
    });
}

void UpdateWorker::startDownload(int updateTypes)
{
    qCInfo(DCC_UPDATE_WORKER) << "Start download, update types: " << updateTypes;
    cleanLaStoreJob(m_downloadJob);

    // 直接设置为正在下载状态, 否则切换下载界面等待时间稍长,体验不好
    m_model->setLastStatus(UpdatesStatus::DownloadWaiting, __LINE__, updateTypes);

    // 开始下载
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(m_updateInter->PrepareDistUpgradePartly(updateTypes), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            const QString& errorMessage = reply.error().message();
            qCWarning(DCC_UPDATE_WORKER) << "Start download failed, error:" << errorMessage;
            m_model->setLastErrorLog(DownloadFailed, errorMessage);
            m_model->setLastError(DownloadFailed, analyzeJobErrorMessage(errorMessage, DownloadFailed));
            cleanLaStoreJob(m_downloadJob);
        } else {
            const QString jobPath = reply.value().path();
            if (jobPath.isEmpty()) {
                qCWarning(DCC_UPDATE_WORKER) << "Download job path is empty, error:" << reply.error().message();
                return;
            }
            setDownloadJob(jobPath);
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::stopDownload()
{
    if (!m_downloadJob) {
        qCWarning(DCC_UPDATE_WORKER) << "Download job is null";
        return;
    }

    cleanLaStoreJob(m_downloadJob);
}

void UpdateWorker::onDownloadJobCtrl(int updateCtrlType)
{
    if (m_downloadJob == nullptr) {
        qCWarning(DCC_UPDATE_WORKER) << "Download job is nullptr";
        return;
    }

    switch (updateCtrlType) {
    case UpdateCtrlType::Start:
        qCInfo(DCC_UPDATE_WORKER) << "Start download job";
        m_updateInter->StartJob(m_downloadJob->id());
        break;
    case UpdateCtrlType::Pause:
        qCInfo(DCC_UPDATE_WORKER) << "Pause download job";
        m_updateInter->PauseJob(m_downloadJob->id());
        break;
    }
}

void UpdateWorker::setDownloadJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE_WORKER) << "Set download job: " << jobPath;
    QMutexLocker locker(&m_downloadMutex);
    if (m_downloadJob) {
        qCInfo(DCC_UPDATE_WORKER) << "Download job existed, do not create again";
        return;
    }

    m_downloadJob = new UpdateJobDBusProxy(jobPath,  this);
    connect(m_downloadJob, &UpdateJobDBusProxy::ProgressChanged, m_model, &UpdateModel::setDownloadProgress);
    connect(m_downloadJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onDownloadStatusChanged);
    connect(m_downloadJob, &UpdateJobDBusProxy::DescriptionChanged, this, [this](const QString &description) {
        if (m_downloadJob && m_downloadJob->status() == "failed") {
            m_model->setLastErrorLog(DownloadFailed, description);
        }
    });
    onDownloadStatusChanged(m_downloadJob->status());
    m_model->setDownloadProgress(m_downloadJob->progress());
}

void UpdateWorker::doUpgrade(int updateTypes, bool doBackup)
{
    qCInfo(DCC_UPDATE_WORKER) << "Do upgrade, update types:" << updateTypes << ", whether do backup:" << doBackup;
    cleanLaStoreJob(m_distUpgradeJob);

    qCInfo(DCC_UPDATE_WORKER) << "Update types:" << updateTypes << ", do backup:" << doBackup;
    QDBusPendingCall call = m_updateInter->DistUpgradePartly(updateTypes, doBackup);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, updateTypes, call, watcher, doBackup] {
        watcher->deleteLater();
        if (!call.isError()) {
            m_model->setLastStatus(UpgradeWaiting, __LINE__, updateTypes);
            QList<QVariant> outArgs = call.reply().arguments();
            if (outArgs.count() > 0) {
                if (doBackup) {
                    setBackupJob(outArgs.at(0).toString());
                } else {
                    setDistUpgradeJob(outArgs.at(0).toString());
                }
            }
        } else {
            qCInfo(DCC_UPDATE_WORKER) << "Call `DistUpgradePartly` failed, error:" << call.error().message();
        }
    });
}

void UpdateWorker::reStart()
{
    DDBusSender()
    .service("com.deepin.dde.shutdownFront")
    .interface("com.deepin.dde.shutdownFront")
    .path("/com/deepin/dde/shutdownFront")
    .method("Restart")
    .call();
}

void UpdateWorker::setBackupJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE_WORKER) << "Create backup upgrade job, path:" << jobPath;
    if (m_backupJob || jobPath.isEmpty()) {
        qCInfo(DCC_UPDATE_WORKER) << "Job is not null or job path is empty";
        return;
    }

    m_backupJob = new UpdateJobDBusProxy(jobPath, this);
    connect(m_backupJob, &UpdateJobDBusProxy::ProgressChanged, m_model, &UpdateModel::setBackupProgress);
    connect(m_backupJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onBackupStatusChanged);
    connect(m_backupJob, &UpdateJobDBusProxy::DescriptionChanged, this, [this](const QString &description) {
        if (m_backupJob->status() == "failed") {
            m_model->setLastErrorLog(BackupFailed, description);
        }
    });
    m_model->setBackupProgress(m_backupJob->progress());
    onBackupStatusChanged(m_backupJob->status());
}

void UpdateWorker::setDistUpgradeJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE_WORKER) << "Create dist upgrade job, path:" << jobPath;
    if (m_distUpgradeJob || jobPath.isEmpty()) {
        qCInfo(DCC_UPDATE_WORKER) << "Job is not null or job path is empty";
        return;
    }

    m_distUpgradeJob = new UpdateJobDBusProxy(jobPath, this);
    connect(m_distUpgradeJob, &UpdateJobDBusProxy::ProgressChanged, m_model, &UpdateModel::setDistUpgradeProgress);
    connect(m_distUpgradeJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onDistUpgradeStatusChanged);
    connect(m_distUpgradeJob, &UpdateJobDBusProxy::DescriptionChanged, this, [this](const QString &description) {
        if (m_distUpgradeJob->status() == "failed") {
            m_model->setLastErrorLog(UpgradeFailed, description);
        }
    });
    m_model->setDistUpgradeProgress(m_distUpgradeJob->progress());
    onDistUpgradeStatusChanged(m_distUpgradeJob->status());
}

void UpdateWorker::updateSystemVersion()
{
    const DConfig* dconfig = DConfigWatcher::instance()->getModulesConfig(DConfigWatcher::update);
    if (dconfig && dconfig->isValid()) {
        m_model->setShowVersion(dconfig->value("showVersion").toString());
    }

    QString sVersion = QString("%1 %2").arg(DSysInfo::uosProductTypeName()).arg(DSysInfo::majorVersion());
    if (!IsServerSystem)
        sVersion.append(" " + DSysInfo::uosEditionName());
    m_model->setSystemVersionInfo(sVersion);

    QSettings settings("/etc/os-baseline", QSettings::IniFormat);
    m_model->setBaseline(settings.value("Baseline").toString());
}

void UpdateWorker::setFunctionUpdate(bool update)
{
    quint64 updateMode = m_model->updateMode();
    if (update) {
        updateMode |= UpdateType::SystemUpdate;
    } else {
        updateMode &= ~UpdateType::SystemUpdate;
    }
    m_updateInter->setUpdateMode(updateMode);
}

void UpdateWorker::setSecurityUpdate(bool update)
{
    quint64 updateMode = m_model->updateMode();
    if (update) {
        updateMode |= UpdateType::SecurityUpdate;
    } else {
        updateMode &= ~UpdateType::SecurityUpdate;
    }
    m_updateInter->setUpdateMode(updateMode);
}

void UpdateWorker::setThirdPartyUpdate(bool update)
{
    quint64 updateMode = m_model->updateMode();
    if (update) {
        updateMode |= UpdateType::UnknownUpdate;
    } else {
        updateMode &= ~UpdateType::UnknownUpdate;
    }
    m_updateInter->setUpdateMode(updateMode);
}

void UpdateWorker::setDownloadSpeedLimitEnabled(bool enable)
{
    auto config = m_model->speedLimitConfig();
    config.downloadSpeedLimitEnabled = enable;
    setDownloadSpeedLimitConfig(config.toJson());
}

void UpdateWorker::setDownloadSpeedLimitSize(const QString& size)
{
    auto config = m_model->speedLimitConfig();
    config.limitSpeed = size;
    setDownloadSpeedLimitConfig(config.toJson());
}

void UpdateWorker::setDownloadSpeedLimitConfig(const QString& config)
{
    QDBusPendingCall call = m_updateInter->SetDownloadSpeedLimit(config);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [call, watcher] {
        watcher->deleteLater();
        if (call.isError()) {
            qCWarning(DCC_UPDATE_WORKER) << "Set download speed limit config error: " << call.error().message();
        }
    });
}

void UpdateWorker::setAutoDownloadUpdates(const bool& autoDownload)
{
    m_updateInter->SetAutoDownloadUpdates(autoDownload);
    if (autoDownload == false) {
        m_updateInter->setAutoInstallUpdates(false);
    }
}

void UpdateWorker::setIdleDownloadEnabled(bool enable)
{
    auto config = m_model->idleDownloadConfig();
    config.idleDownloadEnabled = enable;
    setIdleDownloadConfig(config);
}

void UpdateWorker::setIdleDownloadBeginTime(int time)
{
    auto config = m_model->idleDownloadConfig();
    config.beginTime = timeToString(time);
    config.endTime = adjustTimeFunc(config.beginTime, config.endTime, true);
    setIdleDownloadConfig(config);
}

void UpdateWorker::setIdleDownloadEndTime(int time)
{
    auto config = m_model->idleDownloadConfig();
    config.endTime = timeToString(time);
    config.beginTime = adjustTimeFunc(config.beginTime, config.endTime, false);
    setIdleDownloadConfig(config);
}

void UpdateWorker::setIdleDownloadConfig(const IdleDownloadConfig& config)
{
    QDBusPendingCall call = m_updateInter->SetIdleDownloadConfig(QString(config.toJson()));
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [call, watcher] {
        watcher->deleteLater();
        if (call.isError()) {
            qCWarning(DCC_UPDATE_WORKER) << "Set idle download config error:" << call.error().message();
        }
    });
}

QString UpdateWorker::timeToString(int value)
{
    int time = value / 60;
    QString timeStr = time < 10 ? ("0" + QString::number(time)) : QString::number(time);
    int minute = value % 60;
    QString minuteStr = minute < 10 ? ("0" + QString::number(minute)) : QString::number(minute);
    return timeStr + ":" + minuteStr;
}

// 规则：开始时间和结束时间不能相等，否则默认按相隔五分钟处理
// 修改开始时间时，如果不满足规则，那么自动调整结束时间，结束时间=开始时间+5分钟
// 修改结束时间时，如果不满足规则，那么自动调整开始时间，开始时间=结束时间-5分钟
QString UpdateWorker::adjustTimeFunc(const QString& start, const QString& end, bool returnEndTime)
{
    if (start != end)
        return returnEndTime ? end : start;

    static const int MIN_INTERVAL_SECS = 5 * 60;
    QDateTime dateTime(QDate::currentDate(), QTime::fromString(start));
    return returnEndTime ? dateTime.addSecs(MIN_INTERVAL_SECS).time().toString("hh:mm")
                         : dateTime.addSecs(-MIN_INTERVAL_SECS).time().toString("hh:mm");
}

void UpdateWorker::setUpdateNotify(const bool notify)
{
    m_updateInter->SetUpdateNotify(notify);
}

void UpdateWorker::setAutoCleanCache(const bool autoCleanCache)
{
    m_updateInter->SetAutoClean(autoCleanCache);
}

void UpdateWorker::setSmartMirror(bool enable)
{
    m_updateInter->SetEnable(enable);
}

void UpdateWorker::setMirrorSource(const MirrorInfo& mirror)
{
    m_updateInter->SetMirrorSource(mirror.m_id);
}

void UpdateWorker::testMirrorSpeed()
{
    QList<MirrorInfo> mirrors = m_model->mirrorInfos();

    QStringList urlList;
    for (MirrorInfo& info : mirrors) {
        urlList << info.m_url;
    }

    // reset the data;
    m_model->setMirrorSpeedInfo(QMap<QString, int>());

    QFutureWatcher<int>* watcher = new QFutureWatcher<int>();
    connect(watcher, &QFutureWatcher<int>::resultReadyAt, [this, urlList, watcher, mirrors](int index) {
        QMap<QString, int> speedInfo = m_model->mirrorSpeedInfo();

        int result = watcher->resultAt(index);
        QString mirrorId = mirrors.at(index).m_id;
        speedInfo[mirrorId] = result;

        m_model->setMirrorSpeedInfo(speedInfo);
    });

    QPointer<QObject> guest(this);
    QFuture<int> future = QtConcurrent::mapped(urlList, std::bind(TestMirrorSpeedInternal, std::placeholders::_1, guest));
    watcher->setFuture(future);
}

void UpdateWorker::checkNetselect()
{
    QProcess* process = new QProcess;
    process->start("netselect", QStringList() << "127.0.0.1");
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if ((error == QProcess::FailedToStart) || (error == QProcess::Crashed)) {
            m_model->setNetselectExist(false);
            process->deleteLater();
        }
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int result) {
        bool isNetselectExist = 0 == result;
        if (!isNetselectExist) {
            qCDebug(DCC_UPDATE_WORKER) << "Netselect 127.0.0.1 :" << isNetselectExist;
        }
        m_model->setNetselectExist(isNetselectExist);
        process->deleteLater();
    });
}

#ifndef DISABLE_SYS_UPDATE_MIRRORS
void UpdateWorker::refreshMirrors()
{
    qCDebug(DCC_UPDATE_WORKER) << QDir::currentPath();
    QFile file(":/update/themes/common/config/mirrors.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCDebug(DCC_UPDATE_WORKER) << file.errorString();
        return;
    }
    QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
    QList<MirrorInfo> list;
    for (auto item : array) {
        QJsonObject obj = item.toObject();
        MirrorInfo info;
        info.m_id = obj.value("id").toString();
        QString locale = QLocale::system().name();
        if (!(QLocale::system().name() == "zh_CN" || QLocale::system().name() == "zh_TW")) {
            locale = "zh_CN";
        }
        info.m_name = obj.value(QString("name_locale.%1").arg(locale)).toString();
        info.m_url = obj.value("url").toString();
        list << info;
    }
    m_model->setMirrorInfos(list);
    m_model->setDefaultMirror(list[0].m_id);
}
#endif

void UpdateWorker::setTestingChannelEnable(const bool& enable)
{
    qCDebug(DCC_UPDATE_WORKER) << "Testing:" << "setTestingChannelEnable" << enable;
    if (enable) {
        m_model->setTestingChannelStatus(TestingChannelStatus::WaitJoined);
    } else {
        m_model->setTestingChannelStatus(TestingChannelStatus::WaitToLeave);
    }

    auto machineidopt = getMachineId();
    if (!machineidopt.has_value()) {
        // INFO: 99lastore-token.conf is not generated, need wait for lastore to generating it, if
        // it is not generated for a long time, please post a issue to lastore
        qCWarning(DCC_UPDATE_WORKER)
                << "machineid need to read /etc/apt/apt.conf.d/99lastore-token.conf, the file is "
                   "generated by lastore. Maybe you need wait for the file to be generated.";
        m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
        return;
    }

    // every time, clear the machineid in server
    auto http = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl(ServiceLink + "/api/v2/public/testing/machine/" + machineidopt.value()));
    request.setRawHeader("content-type", "application/json");
    QEventLoop loop;
    connect(http, &QNetworkAccessManager::finished, this, [http, &loop](QNetworkReply *reply) {
        reply->deleteLater();
        http->deleteLater();
        loop.quit();
    });
    http->deleteResource(request);
    loop.exec();

    // Disable Testing Channel
    if (!enable) {
        if (m_updateInter->PackageExists(TestingChannelPackage)) {
            qCDebug(DCC_UPDATE_WORKER) << "Testing:" << "Uninstall testing channel package";
            emit requestCloseTestingChannel();
        } else {
            m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
        }
        return;
    }

    if (!openTestingChannelUrl()) 
        return;

    // Loop to check if user hava joined
    QTimer::singleShot(1000, this, &UpdateWorker::checkTestingChannelStatus);
}

bool UpdateWorker::openTestingChannelUrl()
{
    auto testChannelUrlOpt = getTestingChannelUrl();
    if (!testChannelUrlOpt.has_value()) {
        m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
        return false;
    }
    QUrl testChannelUrl = testChannelUrlOpt.value();
    qCDebug(DCC_UPDATE_WORKER) << "Testing:" << "open join page" << testChannelUrl.toString();
    return openUrl(testChannelUrl.toString());
}

void UpdateWorker::exitTestingChannel(bool value)
{
    if (value) {
        QDBusPendingCall call = m_updateInter->RemovePackage(TestingChannel, TestingChannelPackage);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, call, watcher] {
            watcher->deleteLater();
            if (call.isError()) {
                qCWarning(DCC_UPDATE_WORKER) << "dbus call failed: " << call.error();
                m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
            } else {
                QDBusReply<QDBusObjectPath> reply = call.reply();
                const QString jobPath = reply.value().path();
                setRemovePackageJob(jobPath);                
            }
        });
    } else {
        m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
    }
}

std::optional<QString> UpdateWorker::getMachineId()
{
    if (m_machineid.has_value()) {
        return m_machineid.value();
    }
    QString machineid = m_updateInter->hardwareId();
    if (!machineid.isEmpty()) {
        m_machineid = machineid;
        return machineid;
    }
    return std::nullopt;
}

std::optional<QUrl> UpdateWorker::getTestingChannelUrl()
{
    if (!m_testingChannelUrl.has_value()) {
        QString hostname = m_updateInter->staticHostname();
        auto machineid = getMachineId();
        if (!machineid.has_value()) {
            return std::nullopt;
        }
        QUrl testingUrl = QUrl(ServiceLink + "/internal-testing");
        auto query = QUrlQuery(testingUrl.query());
        query.addQueryItem("h", hostname);
        query.addQueryItem("m", machineid.value());
        query.addQueryItem("v", DSysInfo::minorVersion());
        testingUrl.setQuery(query);
        m_testingChannelUrl = testingUrl;
    }
    return m_testingChannelUrl;
}

void UpdateWorker::initTestingChannel()
{
    if (!IsCommunitySystem) {
        m_model->setTestingChannelStatus(TestingChannelStatus::DeActive);
        return;
    }
    QDBusPendingCall call = m_updateInter->PackageExists(TestingChannelPackage);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher, call, this] {
        if (!call.isError()) {
            QDBusPendingReply<bool> reply = call.reply();
            if (reply.value()) {
                m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
            };
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::checkTestingChannelStatus()
{
    // Leave page
    if (m_model->testingChannelStatus() == TestingChannelStatus::DeActive) {
        return;
    }
    if (!m_machineid.has_value()) {
        return;
    }

    qCDebug(DCC_UPDATE_WORKER) << "Testing:" << "check testing join status";
    QString machineid = m_machineid.value();
    auto http = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl(ServiceLink + "/api/v2/public/testing/machine/status/" + machineid));
    request.setRawHeader("content-type", "application/json");
    connect(http, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
        reply->deleteLater();
        http->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(DCC_UPDATE_WORKER) << "Testing:" << "Network Error" << reply->errorString();
            return;
        }
        auto data = reply->readAll();
        qCDebug(DCC_UPDATE_WORKER) << "Testing:" << "machine status body" << data;
        auto doc = QJsonDocument::fromJson(data);
        auto obj = doc.object();
        auto status = obj["data"].toObject()["status"].toString();
        // Exit the loop if switch status is disable
        if (m_model->testingChannelStatus() != TestingChannelStatus::WaitJoined) {
            return;
        }
        // If user has joined then install testing source package;
        if (status == "joined") {
            qCDebug(DCC_UPDATE_WORKER) << "Testing:" << "Install testing channel package";
            QDBusPendingCall call = m_updateInter->InstallPackage(TestingChannel, TestingChannelPackage);
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
            connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, call, watcher] {
                watcher->deleteLater();
                if (call.isError()) {
                    qCWarning(DCC_UPDATE_WORKER) << "dbus call failed: " << call.error();
                    m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
                } else {
                    QDBusReply<QDBusObjectPath> reply = call.reply();
                    const QString jobPath = reply.value().path();
                    setInstallPackageJob(jobPath);
                }
            });
            return;
        }
        // Run again after sleep
        QTimer::singleShot(5000, this, &UpdateWorker::checkTestingChannelStatus);
    });
    http->get(request);
}

void UpdateWorker::setInstallPackageJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE_WORKER) << "Create install package job, path:" << jobPath;
    if (m_installPackageJob || jobPath.isEmpty()) {
        qCInfo(DCC_UPDATE_WORKER) << "Job is not null or job path is empty";
        return;
    }

    m_installPackageJob = new UpdateJobDBusProxy(jobPath, this);
    connect(m_installPackageJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onInstallPackageStatusChanged);
    onInstallPackageStatusChanged(m_installPackageJob->status());
}

void UpdateWorker::setRemovePackageJob(const QString& jobPath)
{
    qCInfo(DCC_UPDATE_WORKER) << "Create remove package job, path:" << jobPath;
    if (m_removePackageJob || jobPath.isEmpty()) {
        qCInfo(DCC_UPDATE_WORKER) << "Job is not null or job path is empty";
        return;
    }

    m_removePackageJob = new UpdateJobDBusProxy(jobPath, this);
    connect(m_removePackageJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onRemovePackageStatusChanged);
    onRemovePackageStatusChanged(m_removePackageJob->status());
}

bool UpdateWorker::openUrl(const QString& url)
{
    return QDesktopServices::openUrl(QUrl(url));
}

void UpdateWorker::onRequestRetry(int type, int updateTypes)
{
    const auto controlType = static_cast<ControlPanelType>(type);
    const auto updateStatus = m_model->updateStatus(controlType);
    const auto lastError = m_model->lastError(m_model->updateStatus(controlType));

    qCWarning(DCC_UPDATE_WORKER) << "Control type:" << controlType
               << ", update status:" << updateStatus
               << ", update types:" << updateTypes;

    if (updateStatus == UpgradeFailed && lastError == DpkgInterrupted) {
        if (m_fixErrorJob != nullptr) {
            qCWarning(DCC_UPDATE_WORKER) << "Fix error job is nullptr";
            return;
        }

        QDBusInterface lastoreManager("org.deepin.dde.Lastore1",
                              "/org/deepin/dde/Lastore1",
                              "org.deepin.dde.Lastore1.Manager",
                              QDBusConnection::systemBus());
        if (!lastoreManager.isValid()) {
            qDebug() << "com.deepin.license error ," << lastoreManager.lastError().name();
            return;
        }

        const QString& errorTypeString = UpdateModel::updateErrorToString(lastError);
        qCInfo(DCC_UPDATE_WORKER) << "Call `FixError` function, error type:" << errorTypeString;
        QDBusReply<QDBusObjectPath> reply = lastoreManager.call("FixError", errorTypeString);
        if (reply.isValid()) {
            QString path = reply.value().path();
            m_fixErrorJob = new UpdateJobDBusProxy(path, this);
            connect(m_fixErrorJob, &UpdateJobDBusProxy::StatusChanged, this, [updateTypes, lastError, this](const QString status) {
                qCInfo(DCC_UPDATE_WORKER) << "Fix error job status changed :" << status;
                if (status == "succeed" || status == "failed" || status == "end") {
                    deleteJob(m_fixErrorJob);
                    if (status == "succeed") {
                        doUpgrade(updateTypes, false);
                    } else if (status == "failed") {
                        m_model->setLastError(UpgradeFailed, lastError);
                    }
                }
            });
        } else {
            qCWarning(DCC_UPDATE_WORKER) << "Call `FixError` reply is invalid, error: " << reply.error().message();
        }
        return;
    }

    if (updateStatus == UpgradeFailed || updateStatus == BackupFailed) {
        doUpgrade(updateTypes, updateStatus == BackupFailed);
        return;
    }

    if (updateStatus == DownloadFailed) {
        startDownload(updateTypes);
        return;
    }

    if (updateStatus == CheckingFailed) {
        checkForUpdates();
        return;
    }

    if (lastError == UnKnown || lastError == NoError) {
        qCWarning(DCC_UPDATE_WORKER) << "Unknown error, recheck update";
        checkForUpdates();
    }
}

void UpdateWorker::onLicenseStateChange()
{
    QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
    connect(watcher, &QFutureWatcher<void>::finished, watcher, &QFutureWatcher<void>::deleteLater);

    QFuture<void> future = QtConcurrent::run([this]() {
        this->getLicenseState();  // 调用成员函数
    });
    watcher->setFuture(future);
}

/**
 * @brief 使用电源或者电量大于60认为电量满足条件
 *
 */
void UpdateWorker::onPowerChange()
{
    qCInfo(DCC_UPDATE_WORKER) << "Check power";
    bool onBattery = m_updateInter->onBattery();
    if (!onBattery) {
        m_model->setBatterIsOK(true);
        return;
    }

    double data = m_updateInter->batteryPercentage().value("Display", 0);
    int batteryPercentage = uint(qMin(100.0, qMax(0.0, data)));
    m_model->setBatterIsOK(batteryPercentage >= LOWEST_BATTERY_PERCENT);
}

void UpdateWorker::onUpdateModeChanged(qulonglong value)
{
    m_model->setUpdateMode(value);
}
 
void UpdateWorker::onJobListChanged(const QList<QDBusObjectPath>& jobs)
{
    qCInfo(DCC_UPDATE_WORKER) << "Job list changed, size:" << jobs.size();

    for (const auto& job : jobs) {
        QString jobPath = job.path();
        UpdateJobDBusProxy jobInter(jobPath, this);

        // id maybe scrapped
        const QString& id = jobInter.id();
        if (!jobInter.isValid() || id.isEmpty())
            continue;

        qCInfo(DCC_UPDATE_WORKER) << "Job id: " << id << ", job path: " << jobPath;
        if ((id == "update_source" || id == "custom_update") && m_checkUpdateJob == nullptr) {
            setCheckUpdatesJob(jobPath);
        } else if (id == "dist_upgrade" && m_distUpgradeJob == nullptr) {
            setDistUpgradeJob(jobPath);
        } else if (id == "prepare_dist_upgrade" && m_downloadJob == nullptr) {
            setDownloadJob(jobPath);
        } else if (id == "backup" && m_backupJob == nullptr) {
            setBackupJob(jobPath);
        } else if (jobInter.name() == TestingChannel && jobInter.id().contains("install") && m_installPackageJob == nullptr) {
            setInstallPackageJob(jobPath);
        } else if (jobInter.name() == TestingChannel && jobInter.id().contains("remove") && m_removePackageJob == nullptr) {
            setRemovePackageJob(jobPath);
        }
    }
}

void UpdateWorker::onUpdateStatusChanged(const QString &value)
{
    m_model->setUpdateStatus(value.toUtf8());
}

void UpdateWorker::onClassifiedUpdatablePackagesChanged(const QMap<QString, QStringList>& packages)
{
    m_model->updatePackages(packages);
}

void UpdateWorker::onRequestCheckUpdateModeChanged(int type, bool isChecked)
{
    const int currentMode = m_model->checkUpdateMode();
    const int outMode = isChecked ? (currentMode | type) : (currentMode & ~type);

    m_updateInter->setCheckUpdateMode(outMode);
}

void UpdateWorker::onCheckUpdateStatusChanged(const QString& value)
{
    qCInfo(DCC_UPDATE_WORKER) << "Check update status changed: " << value;
    if (value == "failed" || value.isEmpty()) {
        if (m_checkUpdateJob != nullptr) {
            m_updateInter->CleanJob(m_checkUpdateJob->id());
            const auto& description = m_checkUpdateJob->description();
            m_model->setLastErrorLog(CheckingFailed, description);
            m_model->setLastError(CheckingFailed, analyzeJobErrorMessage(description, CheckingFailed));
            m_model->setLastStatus(CheckingFailed, __LINE__);
            m_model->setCheckUpdateStatus(CheckingFailed);
            deleteJob(m_checkUpdateJob);
        }
    } else if (value == "success" || value == "succeed") {
        auto watcher = new QDBusPendingCallWatcher(m_updateInter->GetUpdateLogs(SystemUpdate | SecurityUpdate), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
            watcher->deleteLater();
            if (!watcher->isError()) {
                QDBusPendingReply<QString> reply = watcher->reply();
                UpdateLogHelper::ref().handleUpdateLog(reply.value());
            } else {
                qCWarning(DCC_UPDATE_WORKER) << "Get update log failed";
            }
            // 日志处理完了再显示更新内容界面
        });
        m_model->setLastStatus(CheckingSucceed, __LINE__);
        m_model->setCheckUpdateStatus(CheckingSucceed);
        setUpdateInfo();
        m_model->setShowCheckUpdate(!m_model->isUpdatable());
    } else if (value == "end") {
        refreshLastTimeAndCheckCircle();
        m_model->setCheckUpdateStatus(m_model->lastStatus());
        m_model->updateCheckUpdateUi();
        deleteJob(m_checkUpdateJob);
    }
}

void UpdateWorker::onDownloadStatusChanged(const QString& value)
{
    qCInfo(DCC_UPDATE_WORKER) << "Download status changed: " << value;
    if (value == "failed") {
        const auto& description = m_downloadJob->description();
        m_model->setLastErrorLog(DownloadFailed, description);
        m_model->setLastError(DownloadFailed, analyzeJobErrorMessage(description, DownloadFailed));
        m_model->setDownloadFailedTips(m_model->errorToText(m_model->lastError(DownloadFailed)));
    } else if (value == "end") {
        // 有多个下载项时,每下载完一个就会收到`end`,全部下载完毕后再析构job
        if (m_model->allUpdateStatus().contains(Downloading)) {
            qCInfo(DCC_UPDATE_WORKER) << "Downloading, do not handle `end` status";
            return;
        }
        deleteJob(m_downloadJob);
    }
}

void UpdateWorker::onBackupStatusChanged(const QString &value)
{
    qCInfo(DCC_UPDATE_WORKER) << "backup status changed: " << value;
    if (value == "failed") {
        const auto& description = m_backupJob->description();
        m_model->setLastErrorLog(BackupFailed, description);
        m_model->setLastError(BackupFailed, analyzeJobErrorMessage(description, BackupFailed));
        m_model->setBackupFailedTips(m_model->errorToText(m_model->lastError(BackupFailed)));
    } else if (value == "end") {
        deleteJob(m_downloadJob);
    }
}

void UpdateWorker::onDistUpgradeStatusChanged(const QString& status)
{
    // 无需处理ready状态
    static const QMap<QString, UpdatesStatus> DIST_UPGRADE_STATUS_MAP = {
        { "running", Upgrading },
        { "failed", UpgradeFailed },
        { "succeed", UpgradeSuccess },
        { "end", UpgradeComplete }
    };

    qCInfo(DCC_UPDATE_WORKER) << "Dist upgrade status changed:" << status;
    if (DIST_UPGRADE_STATUS_MAP.contains(status)) {
        const auto updateStatus = DIST_UPGRADE_STATUS_MAP.value(status);
        if (updateStatus == UpgradeComplete) {
            cleanLaStoreJob(m_distUpgradeJob);
            updateSystemVersion();
        } else {
            if (updateStatus == UpgradeFailed && m_distUpgradeJob) {
                const QString& description = m_distUpgradeJob->description();
                m_model->setLastErrorLog(UpgradeFailed, description);
                m_model->setLastError(UpgradeFailed, analyzeJobErrorMessage(description));
                m_model->setInstallFailedTips(m_model->errorToText(m_model->lastError(UpgradeFailed)));
            }
        }
    } else {
        qCWarning(DCC_UPDATE_WORKER) << "Unknown dist upgrade status";
    }
}

void UpdateWorker::onInstallPackageStatusChanged(const QString& value)
{
    qCInfo(DCC_UPDATE_WORKER) << "Install package status changed: " << value;
    if (value == "failed") {
        if (m_installPackageJob != nullptr) {
            const auto& description = m_installPackageJob->description();
            qCWarning(DCC_UPDATE_WORKER) << "Cannot install package" << TestingChannelPackage << ": " << description;
            m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
            cleanLaStoreJob(m_installPackageJob);            
        }
    } else if (value == "succeed") {
        m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
    } else if (value == "end") {
        deleteJob(m_installPackageJob);
    }
}

void UpdateWorker::onRemovePackageStatusChanged(const QString& value)
{
    qCInfo(DCC_UPDATE_WORKER) << "Remove package status changed: " << value;
    if (value == "failed") {
        if (m_removePackageJob != nullptr) {
            const auto& description = m_removePackageJob->description();
            qCWarning(DCC_UPDATE_WORKER) << "Cannot uninstall package" << TestingChannelPackage << ": " << description;
            m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
            cleanLaStoreJob(m_removePackageJob);
        }
    } else if (value == "succeed") {
        m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
    } else if (value == "end") {
        deleteJob(m_removePackageJob);
    }
}

