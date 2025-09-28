// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatework.h"
#include "common/common/logwatcherhelper.h"
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
#include <QLoggingCategory>

#include <memory>

#include <DDBusSender>
#include <DNotifySender>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

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

void notifyInfo(const QString &summary, const QString &body)
{
    qCDebug(logDccUpdatePlugin) << "Send notification:" << summary;
    DUtil::DNotifySender(summary)
            .appIcon("")
            .appName("org.deepin.dde.control-center")
            .appBody(body)
            .timeOut(5000)
            .call();
}

void notifyInfoWithoutBody(const QString &summary)
{
    qCDebug(logDccUpdatePlugin) << "Send simple notification:" << summary;
    DUtil::DNotifySender(summary)
            .appIcon("")
            .appName("org.deepin.dde.control-center")
            .timeOut(5000)
            .call();
}

static int TestMirrorSpeedInternal(const QString& url, QPointer<QObject> baseObject)
{
    qCDebug(logDccUpdatePlugin) << "Testing mirror speed for:" << url;
    if (!baseObject || QCoreApplication::closingDown()) {
        qCDebug(logDccUpdatePlugin) << "Object invalid or app closing, abort test";
        return -1;
    }

    QStringList args;
    args << url << "-s"
         << "1";

    qCDebug(logDccUpdatePlugin) << "start process netselect";
    QProcess process;
    process.start("netselect", args);

    if (!process.waitForStarted()) {
        qCWarning(logDccUpdatePlugin) << "Failed to start netselect process";
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
    , m_lastoreDConfig(DConfig::create("org.deepin.dde.lastore", "org.deepin.dde.lastore", "", this))
    , m_model(model)
    , m_updateInter(new UpdateDBusProxy(this))
    , m_lastoreHeartBeatTimer(new QTimer(this))
    , m_logWatcherHelper(new LogWatcherHelper(m_updateInter, this))
    , m_machineid(std::nullopt)
    , m_testingChannelUrl(std::nullopt)
    , m_doCheckUpdates(false)
    , m_checkUpdateJob(nullptr)
    , m_fixErrorJob(nullptr)
    , m_downloadJob(nullptr)
    , m_distUpgradeJob(nullptr)
    , m_backupJob(nullptr)
    , m_installPackageJob(nullptr)
    , m_removePackageJob(nullptr)
{
    qCDebug(logDccUpdatePlugin) << "Initializing UpdateWorker";
    qRegisterMetaType<UpdatesStatus>("UpdatesStatus");
    qRegisterMetaType<UiActiveState>("UiActiveState");
    qRegisterMetaType<ControlPanelType>("ControlPanelType");

    initConnect();
}

UpdateWorker::~UpdateWorker()
{
    qCDebug(logDccUpdatePlugin) << "Destroying UpdateWorker, cleaning up jobs";
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
    qCDebug(logDccUpdatePlugin) << "Setting up DBus connections and signals";
    QDBusConnection::systemBus().connect("com.deepin.license", 
                                         "/com/deepin/license/Info", 
                                         "com.deepin.license.Info", 
                                         "LicenseStateChange", this, SLOT(onLicenseStateChange()));

    connect(m_updateInter, &UpdateDBusProxy::BatteryPercentageChanged, this, &UpdateWorker::onPowerChange);
    connect(m_updateInter, &UpdateDBusProxy::OnBatteryChanged, this, &UpdateWorker::onPowerChange);
    connect(m_updateInter, &UpdateDBusProxy::UpdateModeChanged, this, &UpdateWorker::onUpdateModeChanged);
    connect(m_updateInter, &UpdateDBusProxy::JobListChanged, this, &UpdateWorker::onJobListChanged);
    connect(m_updateInter, &UpdateDBusProxy::UpdateStatusChanged, this, &UpdateWorker::onUpdateStatusChanged);
    connect(m_updateInter, &UpdateDBusProxy::ClassifiedUpdatablePackagesChanged, this, &UpdateWorker::onClassifiedUpdatablePackagesChanged);
    connect(m_updateInter, &UpdateDBusProxy::ImmutableAutoRecoveryChanged, m_model, &UpdateModel::setImmutableAutoRecovery);

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

    connect(m_logWatcherHelper, &LogWatcherHelper::incrementalDataChanged, m_model, &UpdateModel::appendUpdateLog);

    connect(DConfigWatcher::instance(), &DConfigWatcher::notifyDConfigChanged, [this](const QString &moduleName, const QString &configName) {
        qCDebug(logDccUpdatePlugin) << "Config changed:" << moduleName << configName;

        if (moduleName != "update") {
            return;
        }

        if (configName == "updateThirdPartySource") {
            m_model->setThirdPartyUpdateEnabled(DConfigWatcher::instance()->getValue(DConfigWatcher::update, configName).toString() != "Hidden");
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
    qCInfo(logDccUpdatePlugin) << "Active update worker";

    initConfig();
    getLicenseState();
    onPowerChange();
    updateSystemVersion();
    refreshLastTimeAndCheckCircle();
    initTestingChannel();

    m_model->setUpdateMode(m_updateInter->updateMode());
    m_model->setCheckUpdateMode(m_updateInter->checkUpdateMode());
    m_model->setSecurityUpdateEnabled(DConfigWatcher::instance()->getValue(DConfigWatcher::update, "updateSafety").toString() != "Hidden");
    m_model->setThirdPartyUpdateEnabled(DConfigWatcher::instance()->getValue(DConfigWatcher::update, "updateThirdPartySource").toString() != "Hidden");
    m_model->setSpeedLimitConfig(m_updateInter->downloadSpeedLimitConfig().toUtf8());
    m_model->setAutoDownloadUpdates(m_updateInter->autoDownloadUpdates());
    QString config = m_updateInter->idleDownloadConfig();
    m_model->setIdleDownloadConfig(IdleDownloadConfig::toConfig(config.toUtf8()));
    m_model->setUpdateNotify(m_updateInter->updateNotify());
    m_model->setAutoCleanCache(m_updateInter->autoClean());
    m_model->setP2PUpdateEnabled(m_updateInter->p2pUpdateEnable());
    m_model->setImmutableAutoRecovery(m_updateInter->immutableAutoRecovery());
    if (IsCommunitySystem) {
        qCDebug(logDccUpdatePlugin) << "community system, enable smarrt mirror switch";
        m_model->setSmartMirrorSwitch(m_updateInter->enable());
    }
#ifndef DISABLE_SYS_UPDATE_MIRRORS
    refreshMirrors();
#endif

    m_model->setUpdateStatus(m_updateInter->updateStatus().toUtf8());

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
                if (!watcher->isError()) {
                    QDBusPendingReply<QString> reply = watcher->reply();
                    UpdateLogHelper::ref().handleUpdateLog(reply.value());
                    auto resultMap = m_model->getAllUpdateInfos();
                    for (UpdateType type : resultMap.keys()) {
                        UpdateLogHelper::ref().updateItemInfo(resultMap.value(type));
                    }
                } else {
                    qCWarning(logDccUpdatePlugin) << "Get update log failed";
                }
                // 日志处理完了再显示更新内容界面
                m_model->setLastStatus(CheckingSucceed, __LINE__);
                setUpdateInfo();
                watcher->deleteLater();
            });
        }
    }
    
    QTimer::singleShot(0, this, [this]() {
        m_logWatcherHelper->startWatchFile();
    });
}

void UpdateWorker::initConfig()
{
    qCDebug(logDccUpdatePlugin) << "Initialize lastore daemon configuration";
    if (m_lastoreDConfig && m_lastoreDConfig->isValid()) {
        m_model->setLastoreDaemonStatus(m_lastoreDConfig->value("lastore-daemon-status").toInt());
        connect(m_lastoreDConfig, &DConfig::valueChanged, this, [this](const QString& key) {
            if ("lastore-daemon-status" == key) {
                bool ok;
                int value = m_lastoreDConfig->value(key).toInt(&ok);
                if (ok) {
                    qCDebug(logDccUpdatePlugin) << "Lastore daemon status changed:" << value;
                    m_model->setLastoreDaemonStatus(value);
                }
            }
        });
    } else {
        qCWarning(logDccUpdatePlugin) << "Lastore dconfig is nullptr or invalid";
    }
}

void UpdateWorker::getLicenseState()
{
    qCDebug(logDccUpdatePlugin) << "Getting system license activation state";
    if (IsCommunitySystem) {
        qCDebug(logDccUpdatePlugin) << "Community system, activation always true";
        m_model->setSystemActivation(true);
        return;
    }

    QDBusInterface licenseInfo("com.deepin.license",
                               "/com/deepin/license/Info",
                               "com.deepin.license.Info",
                               QDBusConnection::systemBus());
    if (!licenseInfo.isValid()) {
        qCWarning(logDccUpdatePlugin) << "License info dbus is invalid.";
        return;
    }
    UiActiveState reply = static_cast<UiActiveState>(licenseInfo.property("AuthorizationState").toInt());
    const auto isActivated = reply == UiActiveState::Authorized || reply == UiActiveState::TrialAuthorized;
    qCDebug(logDccUpdatePlugin) << "System activation state:" << isActivated;
    m_model->setSystemActivation(isActivated);
}

bool UpdateWorker::checkDbusIsValid()
{
    const auto result = checkJobIsValid(m_checkUpdateJob) && checkJobIsValid(m_downloadJob);
    qCDebug(logDccUpdatePlugin) << "DBus jobs validity check result:" << result;
    return result;
}

bool UpdateWorker::checkJobIsValid(QPointer<UpdateJobDBusProxy> dbusJob)
{
    if (dbusJob.isNull()) {
        qCDebug(logDccUpdatePlugin) << "DBus job is null";
        return false;
    }

    if (dbusJob->isValid()) {
        qCDebug(logDccUpdatePlugin) << "DBus job is valid";
        return true;
    }

    qCDebug(logDccUpdatePlugin) << "DBus job invalid, deleting";
    dbusJob->deleteLater();
    return false;
}

void UpdateWorker::deleteJob(QPointer<UpdateJobDBusProxy> dbusJob)
{
    if (!dbusJob.isNull()) {
        qCDebug(logDccUpdatePlugin) << "Deleting job:" << dbusJob->id();
        dbusJob->deleteLater();
        dbusJob = nullptr;
    }
}

void UpdateWorker::cleanLaStoreJob(QPointer<UpdateJobDBusProxy> dbusJob)
{
    if (dbusJob != nullptr) {
        qCDebug(logDccUpdatePlugin) << "Cleaning lastore job:" << dbusJob->id();
        m_updateInter->CleanJob(dbusJob->id());
        deleteJob(dbusJob);
    }
}

UpdateErrorType UpdateWorker::analyzeJobErrorMessage(const QString& jobDescription, UpdatesStatus status)
{
    qCWarning(logDccUpdatePlugin) << "Analyzing job error message, status:" << status;

    QJsonParseError err_rpt;
    QJsonDocument jobErrorMessage = QJsonDocument::fromJson(jobDescription.toUtf8(), &err_rpt);

    if (err_rpt.error != QJsonParseError::NoError) {
        qCWarning(logDccUpdatePlugin) << "Parse json failed, error:" << err_rpt.errorString();
        return UnKnown;
    }
    const QJsonObject& object = jobErrorMessage.object();
    QString errorType = object.value("ErrType").toString();
    qCDebug(logDccUpdatePlugin) << "errorType:" << errorType;
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

    if (errorType.contains("dpkgError", Qt::CaseInsensitive)) {
        return DpkgError;
    }

    if (errorType.contains("platformUnreachable", Qt::CaseInsensitive)) {
        return PlatformUnreachable;
    }

    if (errorType.contains("invalidSourceList", Qt::CaseInsensitive)) {
        return InvalidSourceList;
    }

    return UnKnown;
}

void UpdateWorker::checkNeedDoUpdates()
{
    qCInfo(logDccUpdatePlugin) << "check need do updates";

    if (m_model->isUpdateDisabled()) {
        qCWarning(logDccUpdatePlugin) << "update disabled";
        m_model->setShowCheckUpdate(false);
        return;
    }

    // 如果当前正在检查更新，则不再检查
    if (m_doCheckUpdates) {
        qCDebug(logDccUpdatePlugin) << "Is doing check updates";
        return;
    }

    // 如果打开控制中心后第一次进入检查更新界面,则显示页面并进行检查
    static bool doCheckFirst = true;
    if (doCheckFirst) {
        doCheckFirst = false;
        m_model->setShowCheckUpdate(true);
        doCheckUpdates();
        return;
    }

    // 如果非第一次进入检查更新界面，则需要判断上次检查时间和当前时间查决定是否需要再次检查, 默认24小时内不再检查
    static const int AUTO_CHECK_UPDATE_CIRCLE = 3600 * 24;
    qint64 checkTimeInterval = QDateTime::fromString(m_model->lastCheckUpdateTime(), "yyyy-MM-dd hh:mm:ss").secsTo(QDateTime::currentDateTime());
    bool bEnter = checkTimeInterval > AUTO_CHECK_UPDATE_CIRCLE;
    qCDebug(logDccUpdatePlugin) << "check time interval:" << checkTimeInterval << " need to check:" << bEnter;
    if (bEnter) {
        m_model->setShowCheckUpdate(true);
        doCheckUpdates();
        return;
    }

    m_model->setShowCheckUpdate(m_model->checkUpdateStatus() == UpdatesStatus::CheckingFailed || !m_model->isUpdatable());
}

void UpdateWorker::doCheckUpdates()
{
    qCInfo(logDccUpdatePlugin) << "do check updates";
    if (checkDbusIsValid()) {
        qCWarning(logDccUpdatePlugin) << "Check Dbus's validation failed do nothing";
        return;
    }

    if (m_checkUpdateJob) {
        qCWarning(logDccUpdatePlugin) << "Is checking update, won't do it again";
        return;
    }

    const auto& allUpdateStatuses = m_model->allUpdateStatus();
    if (allUpdateStatuses.contains(BackingUp)
        || allUpdateStatuses.contains(Upgrading)
        || allUpdateStatuses.contains(Downloading)
        || allUpdateStatuses.contains(DownloadPaused)) {
        qCInfo(logDccUpdatePlugin) << "Lastore daemon is busy now, current statuses:" << allUpdateStatuses;
        m_model->setShowCheckUpdate(false);
        return;
    }

    m_model->resetDownloadInfo(); // 在检查更新前重置数据，避免有上次检查的数据残留

    m_doCheckUpdates = true;
    QDBusPendingCall call = m_updateInter->UpdateSource();
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            qCWarning(logDccUpdatePlugin) << "Check update failed, error: " << reply.error().message();
            m_model->setLastStatus(UpdatesStatus::CheckingFailed, __LINE__);
            cleanLaStoreJob(m_checkUpdateJob);
            m_doCheckUpdates = false;
        } else {
            const QString jobPath = reply.value().path();
            qCInfo(logDccUpdatePlugin) << "jobpath:" << jobPath;
            setCheckUpdatesJob(jobPath);
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::setCheckUpdatesJob(const QString& jobPath)
{
    qCInfo(logDccUpdatePlugin) << "Set check updates job";
    UpdatesStatus state = m_model->updateStatus(CPT_Downloading);
    if (UpdatesStatus::Downloading != state && UpdatesStatus::DownloadPaused != state) {
        m_model->setLastStatus(UpdatesStatus::Checking, __LINE__);
    }

    m_model->setCheckUpdateStatus(UpdatesStatus::Checking);
    createCheckUpdateJob(jobPath);
}

void UpdateWorker::createCheckUpdateJob(const QString& jobPath)
{
    qCInfo(logDccUpdatePlugin) << "Create check update job: " << jobPath;

    if (m_checkUpdateJob != nullptr) {
        qCInfo(logDccUpdatePlugin) << "Check update job existed";
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
    qCDebug(logDccUpdatePlugin) << "Refresh last check time and check circle";
    const QString checkTime = m_updateInter->GetCheckIntervalAndTime();
    if (checkTime.isEmpty()) {
        qCWarning(logDccUpdatePlugin) << "Get check time failed, return empty string";
        return;
    }
    m_model->setLastCheckUpdateTime(checkTime);
}

void UpdateWorker::setUpdateInfo()
{
    qCDebug(logDccUpdatePlugin) << "Setting update information from packages";
    const QMap<QString, QStringList>& updatePackages = m_updateInter->classifiedUpdatablePackages();

    QMap<UpdateType, UpdateItemInfo*> updateInfoMap = getAllUpdateInfo(updatePackages);
    bool isUpdated = true;
    for (auto info : updateInfoMap.values()) {
        m_model->addUpdateInfo(info);
        if (info->isUpdateAvailable()) {
            isUpdated = false;
        }
    }
    qCDebug(logDccUpdatePlugin) << "All packages up to date:" << isUpdated;
    m_model->refreshUpdateItemsChecked();
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
    qCDebug(logDccUpdatePlugin) << "Getting all update info from packages";
    const QStringList& systemPackages = updatePackages.value(SYSTEM_UPGRADE_TYPE_STRING);
    const QStringList& securityPackages = updatePackages.value(SECURITY_UPGRADE_TYPE_STRING);
    const QStringList& unknownPackages = updatePackages.value(UNKNOWN_UPGRADE_STRING);
    const quint64 updateMode = m_model->updateMode();
    qCDebug(logDccUpdatePlugin) << "System packages count:" << systemPackages.size()
                               << "Security packages count:" << securityPackages.size()
                               << "Unknown packages count:" << unknownPackages.size();
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
    qCDebug(logDccUpdatePlugin) << "Query download size for:" << updateItem->name();
    if (updateItem->packages().isEmpty()) {
        qCDebug(logDccUpdatePlugin) << "No packages to query size for";
        return;
    }

    QDBusPendingCall call = m_updateInter->QueryAllSizeWithSource(updateItem->updateType());
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, updateItem, [updateItem, watcher] {
        QDBusPendingReply<qlonglong> reply = *watcher;
        if (reply.isError()) {
            qCWarning(logDccUpdatePlugin) << "Get packages size error:" << reply.error().message();
        } else {
            qlonglong value = reply.value();
            qCInfo(logDccUpdatePlugin) << "Packages' size:" << value << ", name:" << updateItem->name();
            updateItem->setDownloadSize(value);
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::startDownload(int updateTypes)
{
    qCInfo(logDccUpdatePlugin) << "Start download, update types: " << updateTypes;
    cleanLaStoreJob(m_downloadJob);

    // 直接设置为正在下载状态, 否则切换下载界面等待时间稍长,体验不好
    m_model->setLastStatus(UpdatesStatus::DownloadWaiting, __LINE__, updateTypes);
    m_model->setDownloadWaiting(true);

    // 开始下载
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(m_updateInter->PrepareDistUpgradePartly(updateTypes), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            const QString& errorMessage = reply.error().message();
            qCWarning(logDccUpdatePlugin) << "Start download failed, error:" << errorMessage;
            m_model->setLastErrorLog(DownloadFailed, errorMessage);
            m_model->setLastError(DownloadFailed, analyzeJobErrorMessage(errorMessage, DownloadFailed));
            cleanLaStoreJob(m_downloadJob);
        } else {
            const QString jobPath = reply.value().path();
            qCInfo(logDccUpdatePlugin) << "jobpath:" << jobPath;
            setDownloadJob(jobPath);
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::stopDownload()
{
    qCDebug(logDccUpdatePlugin) << "Stopping download operation";
    if (!m_downloadJob) {
        qCWarning(logDccUpdatePlugin) << "Download job is null";
        return;
    }

    cleanLaStoreJob(m_downloadJob);
}

void UpdateWorker::downloadJobCtrl(UpdateCtrlType type)
{
    qCDebug(logDccUpdatePlugin) << "download job control, type: " << type;
    if (m_downloadJob == nullptr) {
        qCWarning(logDccUpdatePlugin) << "Download job is nullptr";
        return;
    }

    switch (type) {
    case UpdateCtrlType::Start:
        qCInfo(logDccUpdatePlugin) << "Start download job";
        m_updateInter->StartJob(m_downloadJob->id());
        break;
    case UpdateCtrlType::Pause:
        qCInfo(logDccUpdatePlugin) << "Pause download job";
        m_updateInter->PauseJob(m_downloadJob->id());
        break;
    }
}

void UpdateWorker::setDownloadJob(const QString& jobPath)
{
    qCInfo(logDccUpdatePlugin) << "Set download job: " << jobPath;
    QMutexLocker locker(&m_downloadMutex);
    if (m_downloadJob) {
        qCInfo(logDccUpdatePlugin) << "Download job existed, do not create again";
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
    qCDebug(logDccUpdatePlugin) << "do upgrade, updateTypes:" << updateTypes << ", doBackup: " << doBackup;
    if (!m_model->batterIsOK()) {
        notifyInfo(tr("Update"), tr("Please plug in and then install updates."));
        return;
    }

    emit startDoUpgrade();

    qCInfo(logDccUpdatePlugin) << "Do upgrade, update types:" << updateTypes << ", whether do backup:" << doBackup;
    cleanLaStoreJob(m_distUpgradeJob);
    cleanLaStoreJob(m_backupJob);

    QDBusPendingCall call = m_updateInter->DistUpgradePartly(updateTypes, doBackup);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, updateTypes, watcher, doBackup] {
        QDBusPendingReply<QDBusObjectPath> reply = *watcher;
        if (reply.isError()) {
            qCWarning(logDccUpdatePlugin) << "Call `DistUpgradePartly` failed, error:" << reply.error().message();
        } else {
            m_model->setLastStatus(UpgradeWaiting, __LINE__, updateTypes);
            m_model->setUpgradeWaiting(true);

            const QString jobPath = reply.value().path();
            qCInfo(logDccUpdatePlugin) << "jobpath:" << jobPath;
            if (doBackup) {
                setBackupJob(jobPath);
            } else {
                setDistUpgradeJob(jobPath);
            }
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::reStart()
{
    qCInfo(logDccUpdatePlugin) << "request restart";
    m_updateInter->Restart();
}

void UpdateWorker::modalUpgrade(bool rebootAfterUpgrade)
{
    qCInfo(logDccUpdatePlugin) << "request modal upgrade, reboot after upgrade:" << rebootAfterUpgrade;
    if (!m_model->batterIsOK()) {
        notifyInfo(tr("Update"), tr("Please plug in and then install updates."));
        qCWarning(logDccUpdatePlugin) << "battery is not ok, don't upgrade.";
        return;
    }

    if (rebootAfterUpgrade) {
        m_updateInter->UpdateAndReboot();
    } else {
        m_updateInter->UpdateAndShutdown();
    }
}

void UpdateWorker::setBackupJob(const QString& jobPath)
{
    qCInfo(logDccUpdatePlugin) << "Create backup upgrade job, path:" << jobPath;
    if (m_backupJob || jobPath.isEmpty()) {
        qCInfo(logDccUpdatePlugin) << "Job is not null or job path is empty";
        return;
    }

    m_backupJob = new UpdateJobDBusProxy(jobPath, this);
    connect(m_backupJob, &UpdateJobDBusProxy::ProgressChanged, m_model, &UpdateModel::setBackupProgress);
    connect(m_backupJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onBackupStatusChanged);
    connect(m_backupJob, &UpdateJobDBusProxy::DescriptionChanged, this, [this](const QString &description) {
        qCDebug(logDccUpdatePlugin) << "backupJob DescriptionChanged: " << description << ", status:" << m_backupJob->status();
        if (m_backupJob->status() == "failed") {
            m_model->setLastErrorLog(BackupFailed, description);
        }
    });
    m_model->setBackupProgress(m_backupJob->progress());
    onBackupStatusChanged(m_backupJob->status());
}

void UpdateWorker::setDistUpgradeJob(const QString& jobPath)
{
    qCInfo(logDccUpdatePlugin) << "Create dist upgrade job, path:" << jobPath;
    if (m_distUpgradeJob || jobPath.isEmpty()) {
        qCInfo(logDccUpdatePlugin) << "Job is not null or job path is empty";
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
    qCDebug(logDccUpdatePlugin) << "Updating system version information";
    const DConfig* dconfig = DConfigWatcher::instance()->getModulesConfig(DConfigWatcher::update);
    if (dconfig && dconfig->isValid()) {
        const auto showVersion = dconfig->value("showVersion").toString();
        qCDebug(logDccUpdatePlugin) << "Show version config:" << showVersion;
        m_model->setShowVersion(showVersion);
    }

    QString sVersion = QString("%1 %2").arg(DSysInfo::uosProductTypeName()).arg(DSysInfo::majorVersion());
    if (!IsServerSystem)
        sVersion.append(" " + DSysInfo::uosEditionName());
    qCDebug(logDccUpdatePlugin) << "System version:" << sVersion;
    m_model->setSystemVersionInfo(sVersion);

    QSettings settings("/etc/os-baseline", QSettings::IniFormat);
    const auto baseline = settings.value("Baseline").toString();
    qCDebug(logDccUpdatePlugin) << "System baseline:" << baseline;
    m_model->setBaseline(baseline);
}

void UpdateWorker::setFunctionUpdate(bool update)
{
    qCDebug(logDccUpdatePlugin) << "Set function update enabled:" << update;
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
    qCDebug(logDccUpdatePlugin) << "Set security update enabled:" << update;
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
    qCDebug(logDccUpdatePlugin) << "Set third party update enabled:" << update;
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
    qCDebug(logDccUpdatePlugin) << "Set download speed limit enabled:" << enable;
    auto config = m_model->speedLimitConfig();
    config.downloadSpeedLimitEnabled = enable;
    // dbus返回需要1s，导致界面更新慢，这里直接先更新model
    m_model->setSpeedLimitConfig(config.toJson().toUtf8());
    setDownloadSpeedLimitConfig(config.toJson());
}

void UpdateWorker::setDownloadSpeedLimitSize(const QString& size)
{
    qCDebug(logDccUpdatePlugin) << "set download speed limit size" << size;
    auto config = m_model->speedLimitConfig();
    config.limitSpeed = size;
    setDownloadSpeedLimitConfig(config.toJson());
}

void UpdateWorker::setDownloadSpeedLimitConfig(const QString& config)
{
    qCDebug(logDccUpdatePlugin) << "set download speed limit config" << config;
    QDBusPendingCall call = m_updateInter->SetDownloadSpeedLimit(config);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher] {
        QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
        if (reply.isError()) {
            qCWarning(logDccUpdatePlugin) << "Set download speed limit config error: " << reply.error().message();
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::setAutoDownloadUpdates(const bool& autoDownload)
{
    qCDebug(logDccUpdatePlugin) << "Set auto download updates:" << autoDownload;
    m_updateInter->SetAutoDownloadUpdates(autoDownload);
    if (autoDownload == false) {
        qCDebug(logDccUpdatePlugin) << "Auto download disabled, disable auto install";
        m_updateInter->setAutoInstallUpdates(false);
    }
}

void UpdateWorker::setIdleDownloadEnabled(bool enable)
{
    qCDebug(logDccUpdatePlugin) << "Set idle download enabled:" << enable;
    auto config = m_model->idleDownloadConfig();
    config.idleDownloadEnabled = enable;
    setIdleDownloadConfig(config);
}

void UpdateWorker::setIdleDownloadBeginTime(QString time)
{
    qCDebug(logDccUpdatePlugin) << "Set idle download begin time:" << time;
    auto config = m_model->idleDownloadConfig();
    config.beginTime = time;
    config.endTime = adjustTimeFunc(config.beginTime, config.endTime, true);
    setIdleDownloadConfig(config);
}

void UpdateWorker::setIdleDownloadEndTime(QString time)
{
    qCDebug(logDccUpdatePlugin) << "Set idle download end time:" << time;
    auto config = m_model->idleDownloadConfig();
    config.endTime = time;
    config.beginTime = adjustTimeFunc(config.beginTime, config.endTime, false);
    setIdleDownloadConfig(config);
}

void UpdateWorker::setIdleDownloadConfig(const IdleDownloadConfig& config)
{
    qCDebug(logDccUpdatePlugin) << "set idleDownloadConfig";
    // 避免dbus延时返回，导致界面更新慢，这里直接先更新model
    m_model->setIdleDownloadConfig(config);
    QDBusPendingCall call = m_updateInter->SetIdleDownloadConfig(QString(config.toJson()));
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher] {
        QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
        if (reply.isError()) {
            qCWarning(logDccUpdatePlugin) << "Set idle download config error:" << reply.error().message();
        }
        watcher->deleteLater();
    });
}

// 规则：开始时间和结束时间不能相等，否则默认按相隔五分钟处理
// 修改开始时间时，如果不满足规则，那么自动调整结束时间，结束时间=开始时间+5分钟
// 修改结束时间时，如果不满足规则，那么自动调整开始时间，开始时间=结束时间-5分钟
QString UpdateWorker::adjustTimeFunc(const QString& start, const QString& end, bool returnEndTime)
{
    qCDebug(logDccUpdatePlugin) << "Adjusting time - start:" << start << "end:" << end << "returnEndTime:" << returnEndTime;
    if (start != end) {
        qCDebug(logDccUpdatePlugin) << "Times are different, returning original";
        return returnEndTime ? end : start;
    }

    static const int MIN_INTERVAL_SECS = 5 * 60;
    QDateTime dateTime(QDate::currentDate(), QTime::fromString(start));
    return returnEndTime ? dateTime.addSecs(MIN_INTERVAL_SECS).time().toString("hh:mm")
                         : dateTime.addSecs(-MIN_INTERVAL_SECS).time().toString("hh:mm");
}

void UpdateWorker::setUpdateNotify(const bool notify)
{
    qCDebug(logDccUpdatePlugin) << "Set update notification enabled:" << notify;
    m_updateInter->SetUpdateNotify(notify);
}

void UpdateWorker::setAutoCleanCache(const bool autoCleanCache)
{
    qCDebug(logDccUpdatePlugin) << "Set auto clean cache enabled:" << autoCleanCache;
    m_updateInter->SetAutoClean(autoCleanCache);
}

void UpdateWorker::setSmartMirror(bool enable)
{
    qCDebug(logDccUpdatePlugin) << "Set smart mirror enabled:" << enable;
    m_updateInter->SetEnable(enable);
}

void UpdateWorker::setMirrorSource(const MirrorInfo& mirror)
{
    qCDebug(logDccUpdatePlugin) << "Set mirror source:" << mirror.m_id;
    m_updateInter->SetMirrorSource(mirror.m_id);
}

void UpdateWorker::testMirrorSpeed()
{
    qCDebug(logDccUpdatePlugin) << "Starting mirror speed test";
    QList<MirrorInfo> mirrors = m_model->mirrorInfos();

    QStringList urlList;
    for (MirrorInfo& info : mirrors) {
        urlList << info.m_url;
    }

    qCDebug(logDccUpdatePlugin) << "Testing" << urlList.size() << "mirror URLs";
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
    qCDebug(logDccUpdatePlugin) << "Checking netselect tool availability";
    QProcess* process = new QProcess;
    process->start("netselect", QStringList() << "127.0.0.1");
    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError error) {
        if ((error == QProcess::FailedToStart) || (error == QProcess::Crashed)) {
            qCDebug(logDccUpdatePlugin) << "Netselect not available, error:" << error;
            m_model->setNetselectExist(false);
            process->deleteLater();
        }
    });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int result) {
        bool isNetselectExist = 0 == result;
        qCDebug(logDccUpdatePlugin) << "Netselect availability check result:" << isNetselectExist;
        if (!isNetselectExist) {
            qCDebug(logDccUpdatePlugin) << "Netselect 127.0.0.1 :" << isNetselectExist;
        }
        m_model->setNetselectExist(isNetselectExist);
        process->deleteLater();
    });
}

#ifndef DISABLE_SYS_UPDATE_MIRRORS
void UpdateWorker::refreshMirrors()
{
    qCDebug(logDccUpdatePlugin) << QDir::currentPath();
    QFile file(":/update/themes/common/config/mirrors.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCDebug(logDccUpdatePlugin) << file.errorString();
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
    qCDebug(logDccUpdatePlugin) << "Testing:" << "setTestingChannelEnable" << enable;
    if (enable) {
        m_model->setTestingChannelStatus(TestingChannelStatus::WaitJoined);
    } else {
        m_model->setTestingChannelStatus(TestingChannelStatus::WaitToLeave);
    }

    auto machineidopt = getMachineId();
    if (!machineidopt.has_value()) {
        // INFO: 99lastore-token.conf is not generated, need wait for lastore to generating it, if
        // it is not generated for a long time, please post a issue to lastore
        qCWarning(logDccUpdatePlugin)
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
        qCDebug(logDccUpdatePlugin) << "http request finished";
        reply->deleteLater();
        http->deleteLater();
        loop.quit();
    });
    http->deleteResource(request);
    loop.exec();

    // Disable Testing Channel
    if (!enable) {
        if (m_updateInter->PackageExists(TestingChannelPackage)) {
            qCDebug(logDccUpdatePlugin) << "Testing:" << "Uninstall testing channel package";
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
    qCDebug(logDccUpdatePlugin) << "Opening testing channel URL";
    auto testChannelUrlOpt = getTestingChannelUrl();
    if (!testChannelUrlOpt.has_value()) {
        qCWarning(logDccUpdatePlugin) << "Failed to get testing channel URL";
        m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
        return false;
    }
    QUrl testChannelUrl = testChannelUrlOpt.value();
    qCDebug(logDccUpdatePlugin) << "Testing:" << "open join page" << testChannelUrl.toString();
    return openUrl(testChannelUrl.toString());
}

void UpdateWorker::exitTestingChannel(bool value)
{
    qCDebug(logDccUpdatePlugin) << "Exit testing channel confirmed:" << value;
    if (value) {
        qCDebug(logDccUpdatePlugin) << "Removing testing channel package";
        QDBusPendingCall call = m_updateInter->RemovePackage(TestingChannel, TestingChannelPackage);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
            QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
            if (reply.isError()) {
                qCWarning(logDccUpdatePlugin) << "dbus call failed: " << reply.error();
                m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
            } else {
                const QString jobPath = reply.value().path();
                setRemovePackageJob(jobPath);                
            }
            watcher->deleteLater();
        });
    } else {
        qCDebug(logDccUpdatePlugin) << "User cancelled exit, stay in testing channel";
        m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
    }
}

std::optional<QString> UpdateWorker::getMachineId()
{
    qCDebug(logDccUpdatePlugin) << "Getting machine ID";
    if (m_machineid.has_value()) {
        qCDebug(logDccUpdatePlugin) << "Using cached machine ID";
        return m_machineid.value();
    }
    QString machineid = m_updateInter->hardwareId();
    if (!machineid.isEmpty()) {
        qCDebug(logDccUpdatePlugin) << "Got machine ID from hardware";
        m_machineid = machineid;
        return machineid;
    }
    qCWarning(logDccUpdatePlugin) << "Failed to get machine ID";
    return std::nullopt;
}

std::optional<QUrl> UpdateWorker::getTestingChannelUrl()
{
    qCDebug(logDccUpdatePlugin) << "Getting testing channel URL";
    if (!m_testingChannelUrl.has_value()) {
        QString hostname = m_updateInter->staticHostname();
        auto machineid = getMachineId();
        if (!machineid.has_value()) {
            qCWarning(logDccUpdatePlugin) << "Cannot get machine ID for testing channel";
            return std::nullopt;
        }
        qCDebug(logDccUpdatePlugin) << "Building testing URL with hostname:" << hostname;
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
    qCDebug(logDccUpdatePlugin) << "Initializing testing channel";
    if (!IsCommunitySystem) {
        qCDebug(logDccUpdatePlugin) << "Not community system, testing channel deactive";
        m_model->setTestingChannelStatus(TestingChannelStatus::DeActive);
        return;
    }
    qCDebug(logDccUpdatePlugin) << "Checking testing channel package existence";
    QDBusPendingCall call = m_updateInter->PackageExists(TestingChannelPackage);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher, this] {
        QDBusPendingReply<bool> reply = watcher->reply();
        if (!reply.isError()) {
            const auto packageExists = reply.value();
            qCDebug(logDccUpdatePlugin) << "Testing channel package exists:" << packageExists;
            if (packageExists) {
                m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
            };
        } else {
            qCWarning(logDccUpdatePlugin) << "Failed to check testing channel package:" << reply.error().message();
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

    qCDebug(logDccUpdatePlugin) << "Testing:" << "check testing join status";
    QString machineid = m_machineid.value();
    auto http = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl(ServiceLink + "/api/v2/public/testing/machine/status/" + machineid));
    request.setRawHeader("content-type", "application/json");
    connect(http, &QNetworkAccessManager::finished, this, [=](QNetworkReply* reply) {
        reply->deleteLater();
        http->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            qCDebug(logDccUpdatePlugin) << "Testing:" << "Network Error" << reply->errorString();
            return;
        }
        auto data = reply->readAll();
        qCDebug(logDccUpdatePlugin) << "Testing:" << "machine status body" << data;
        auto doc = QJsonDocument::fromJson(data);
        auto obj = doc.object();
        auto status = obj["data"].toObject()["status"].toString();
        // Exit the loop if switch status is disable
        if (m_model->testingChannelStatus() != TestingChannelStatus::WaitJoined) {
            return;
        }
        // If user has joined then install testing source package;
        if (status == "joined") {
            qCDebug(logDccUpdatePlugin) << "Testing:" << "Install testing channel package";
            QDBusPendingCall call = m_updateInter->InstallPackage(TestingChannel, TestingChannelPackage);
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
            connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
                QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
                if (reply.isError()) {
                    qCWarning(logDccUpdatePlugin) << "dbus call failed: " << reply.error();
                    m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
                } else {
                    const QString jobPath = reply.value().path();
                    setInstallPackageJob(jobPath);
                }
                watcher->deleteLater();
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
    qCInfo(logDccUpdatePlugin) << "Create install package job, path:" << jobPath;
    if (m_installPackageJob || jobPath.isEmpty()) {
        qCInfo(logDccUpdatePlugin) << "Job is not null or job path is empty";
        return;
    }

    m_installPackageJob = new UpdateJobDBusProxy(jobPath, this);
    connect(m_installPackageJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onInstallPackageStatusChanged);
    onInstallPackageStatusChanged(m_installPackageJob->status());
}

void UpdateWorker::setRemovePackageJob(const QString& jobPath)
{
    qCInfo(logDccUpdatePlugin) << "Create remove package job, path:" << jobPath;
    if (m_removePackageJob || jobPath.isEmpty()) {
        qCInfo(logDccUpdatePlugin) << "Job is not null or job path is empty";
        return;
    }

    m_removePackageJob = new UpdateJobDBusProxy(jobPath, this);
    connect(m_removePackageJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onRemovePackageStatusChanged);
    onRemovePackageStatusChanged(m_removePackageJob->status());
}

bool UpdateWorker::openUrl(const QString& url)
{
    qCDebug(logDccUpdatePlugin) << "open url: " << url;
    return QDesktopServices::openUrl(QUrl(url));
}

void UpdateWorker::onRequestRetry(int type, int updateTypes)
{
    const auto controlType = static_cast<ControlPanelType>(type);
    const auto updateStatus = m_model->updateStatus(controlType);
    const auto lastError = m_model->lastError(m_model->updateStatus(controlType));

    qCWarning(logDccUpdatePlugin) << "Control type:" << controlType
               << ", update status:" << updateStatus
               << ", update types:" << updateTypes;

    if (updateStatus == UpgradeFailed && lastError == DpkgInterrupted) {
        if (m_fixErrorJob != nullptr) {
            qCWarning(logDccUpdatePlugin) << "Fix error job is nullptr";
            return;
        }

        QDBusInterface lastoreManager("org.deepin.dde.Lastore1",
                              "/org/deepin/dde/Lastore1",
                              "org.deepin.dde.Lastore1.Manager",
                              QDBusConnection::systemBus());
        if (!lastoreManager.isValid()) {
            qCDebug(logDccUpdatePlugin) << "com.deepin.license error ," << lastoreManager.lastError().name();
            return;
        }

        const QString& errorTypeString = UpdateModel::updateErrorToString(lastError);
        qCInfo(logDccUpdatePlugin) << "Call `FixError` function, error type:" << errorTypeString;
        QDBusReply<QDBusObjectPath> reply = lastoreManager.call("FixError", errorTypeString);
        if (reply.isValid()) {
            QString path = reply.value().path();
            m_fixErrorJob = new UpdateJobDBusProxy(path, this);
            connect(m_fixErrorJob, &UpdateJobDBusProxy::StatusChanged, this, [updateTypes, lastError, this](const QString status) {
                qCInfo(logDccUpdatePlugin) << "Fix error job status changed :" << status;
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
            qCWarning(logDccUpdatePlugin) << "Call `FixError` reply is invalid, error: " << reply.error().message();
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
        doCheckUpdates();
        return;
    }

    if (lastError == UnKnown || lastError == NoError) {
        qCWarning(logDccUpdatePlugin) << "Unknown error, recheck update";
        doCheckUpdates();
    }
}

void UpdateWorker::setCheckUpdateMode(int type, bool isChecked)
{
    qCDebug(logDccUpdatePlugin) << "Set check update mode, type:" << type << "checked:" << isChecked;
    quint64 currentMode = m_model->checkUpdateMode();
    quint64 outMode = isChecked ? (currentMode | type) : (currentMode & ~type);

    m_updateInter->setCheckUpdateMode(outMode);
}

void UpdateWorker::exportLogToDesktop()
{
    qCDebug(logDccUpdatePlugin) << "Exporting update log to desktop";
    // 获取桌面路径
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (desktopPath.isEmpty()) {
        // 如果无法获取桌面路径，使用主目录
        qCDebug(logDccUpdatePlugin) << "Desktop path empty, using home directory";
        desktopPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }

    // 生成文件名：更新日志_yyyy_mm_dd_HH.MM.SS.txt
    QString timestamp = QDateTime::currentDateTime().toString("yyyy_MM_dd_HH.mm.ss");
    QString fileName = tr("updatelog") + QString("_%1.txt").arg(timestamp);
    QString filePath = QDir(desktopPath).absoluteFilePath(fileName);
    qCDebug(logDccUpdatePlugin) << "Export log file path:" << filePath;

    // 直接创建桌面文件并获取文件描述符
    auto logFile = std::make_shared<QFile>(filePath);
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCWarning(logDccUpdatePlugin) << "Failed to create log file:" << filePath;
        notifyInfo(tr("Update"), tr("Log export failed, please try again"));
        return;
    }

    int fd = logFile->handle();
    if (fd == -1) {
        qCWarning(logDccUpdatePlugin) << "Failed to get file descriptor for:" << filePath;
        notifyInfo(tr("Update"), tr("Log export failed, please try again"));
        return;
    }

    QDBusPendingCall call = m_updateInter->GetUpdateDetails(fd, false);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher, logFile] {
        QDBusPendingReply<void> reply = *watcher;
        if (reply.isError()) {
            qCWarning(logDccUpdatePlugin) << "Export update details failed, error: " << reply.error().message();
            notifyInfo(tr("Update"), tr("Log export failed, please try again"));
        } else {
            notifyInfo(tr("Update"), tr("The log has been exported to the desktop"));
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::onLicenseStateChange()
{
    qCDebug(logDccUpdatePlugin) << "License state changed, checking activation";
    getLicenseState();
}

/**
 * @brief 使用电源或者电量大于60认为电量满足条件
 *
 */
void UpdateWorker::onPowerChange()
{
    qCInfo(logDccUpdatePlugin) << "Check power";
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
    qCDebug(logDccUpdatePlugin) << "Update mode changed to:" << value;
    m_model->setUpdateMode(value);
}

void UpdateWorker::onJobListChanged(const QList<QDBusObjectPath>& jobs)
{
    qCInfo(logDccUpdatePlugin) << "Job list changed, size:" << jobs.size();

    for (const auto& job : jobs) {
        QString jobPath = job.path();
        UpdateJobDBusProxy jobInter(jobPath, this);

        // id maybe scrapped
        const QString& id = jobInter.id();
        if (!jobInter.isValid() || id.isEmpty())
            continue;

        qCInfo(logDccUpdatePlugin) << "Job id: " << id << ", job path: " << jobPath;
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
    qCDebug(logDccUpdatePlugin) << "Update status changed to:" << value;
    m_model->setUpdateStatus(value.toUtf8());
}

void UpdateWorker::onClassifiedUpdatablePackagesChanged(const QMap<QString, QStringList>& packages)
{
    qCDebug(logDccUpdatePlugin) << "Classified updatable packages changed";
    m_model->updatePackages(packages);
}

void UpdateWorker::onCheckUpdateStatusChanged(const QString& value)
{
    qCInfo(logDccUpdatePlugin) << "Check update status changed: " << value;
    if (value == "failed" || value.isEmpty()) {
        if (m_checkUpdateJob != nullptr) {
            m_updateInter->CleanJob(m_checkUpdateJob->id());
            const auto& description = m_checkUpdateJob->description();
            m_model->setLastErrorLog(CheckingFailed, description);
            m_model->setLastError(CheckingFailed, analyzeJobErrorMessage(description, CheckingFailed));
            m_model->setLastStatus(CheckingFailed, __LINE__);
            m_model->setCheckUpdateStatus(CheckingFailed);
            deleteJob(m_checkUpdateJob);
            m_doCheckUpdates = false;
        }
    } else if (value == "success" || value == "succeed") {
        auto watcher = new QDBusPendingCallWatcher(m_updateInter->GetUpdateLogs(SystemUpdate | SecurityUpdate), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
            if (!watcher->isError()) {
                QDBusPendingReply<QString> reply = watcher->reply();
                UpdateLogHelper::ref().handleUpdateLog(reply.value());
                auto resultMap = m_model->getAllUpdateInfos();
                for (UpdateType type : resultMap.keys()) {
                    UpdateLogHelper::ref().updateItemInfo(resultMap.value(type));
                }
            } else {
                qCWarning(logDccUpdatePlugin) << "Get update log failed";
            }
            watcher->deleteLater();
            // 日志处理完了再显示更新内容界面
        });
        m_model->setLastStatus(CheckingSucceed, __LINE__);
        m_model->setCheckUpdateStatus(CheckingSucceed);
        setUpdateInfo();
        m_model->setShowCheckUpdate(!m_model->isUpdatable());
    } else if (value == "end") {
        refreshLastTimeAndCheckCircle();
        m_model->setCheckUpdateStatus(UpdatesStatus(m_model->lastStatus()));
        m_model->updateCheckUpdateUi();
        deleteJob(m_checkUpdateJob);
        m_doCheckUpdates = false;
    }
}

void UpdateWorker::onDownloadStatusChanged(const QString& value)
{
    qCInfo(logDccUpdatePlugin) << "Download status changed: " << value;
    if (value == "failed") {
        const auto& description = m_downloadJob->description();
        m_model->setLastErrorLog(DownloadFailed, description);
        m_model->setLastError(DownloadFailed, analyzeJobErrorMessage(description, DownloadFailed));
        m_model->setDownloadFailedTips(m_model->errorToText(m_model->lastError(DownloadFailed)));
    } else if (value == "end") {
        // 有多个下载项时,每下载完一个就会收到`end`,全部下载完毕后再析构job
        if (m_model->allUpdateStatus().contains(Downloading)) {
            qCInfo(logDccUpdatePlugin) << "Downloading, do not handle `end` status";
            return;
        }
        deleteJob(m_downloadJob);
    } else if (value == "paused") {
        m_model->setDownloadPaused(true);
    } else if (value == "running") {
        m_model->setDownloadPaused(false);
    }
}

void UpdateWorker::onBackupStatusChanged(const QString &value)
{
    qCInfo(logDccUpdatePlugin) << "backup status changed: " << value;
    if (value == "failed") {
        const auto& description = m_backupJob->description();
        m_model->setLastErrorLog(BackupFailed, description);
        m_model->setLastError(BackupFailed, analyzeJobErrorMessage(description, BackupFailed));
        m_model->setBackupFailedTips(m_model->errorToText(m_model->lastError(BackupFailed)));
    } else if (value == "end") {
        deleteJob(m_backupJob);
    }
}

void UpdateWorker::onDistUpgradeStatusChanged(const QString& status)
{
    qCInfo(logDccUpdatePlugin) << "Dist upgrade status changed:" << status;
    // 无需处理ready状态
    static const QMap<QString, UpdatesStatus> DIST_UPGRADE_STATUS_MAP = {
        { "running", Upgrading },
        { "failed", UpgradeFailed },
        { "succeed", UpgradeSuccess },
        { "end", UpgradeComplete }
    };

    qCInfo(logDccUpdatePlugin) << "Dist upgrade status changed:" << status;
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
        qCWarning(logDccUpdatePlugin) << "Unknown dist upgrade status";
    }
}

void UpdateWorker::onInstallPackageStatusChanged(const QString& value)
{
    qCInfo(logDccUpdatePlugin) << "Install package status changed: " << value;
    if (value == "failed") {
        if (m_installPackageJob != nullptr) {
            const auto& description = m_installPackageJob->description();
            qCWarning(logDccUpdatePlugin) << "Cannot install package" << TestingChannelPackage << ": " << description;
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
    qCInfo(logDccUpdatePlugin) << "Remove package status changed: " << value;
    if (value == "failed") {
        if (m_removePackageJob != nullptr) {
            const auto& description = m_removePackageJob->description();
            qCWarning(logDccUpdatePlugin) << "Cannot uninstall package" << TestingChannelPackage << ": " << description;
            m_model->setTestingChannelStatus(TestingChannelStatus::Joined);
            cleanLaStoreJob(m_removePackageJob);
        }
    } else if (value == "succeed") {
        m_model->setTestingChannelStatus(TestingChannelStatus::NotJoined);
    } else if (value == "end") {
        deleteJob(m_removePackageJob);
    }
}

