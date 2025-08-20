// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "updateworker.h"
#include "dconfig_helper.h"
#include "common/commondefine.h"
#include "common/dbus/updatejobdbusproxy.h"
#include "common/global_util/public_func.h"

#include <QTimer>
#include <QDir>
#include <QDBusPendingReply>
#include <QDBusMetaType>
#include <QMap>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <QFont>
#include <QWidget>
#include <QApplication>
#include <QFile>
#include <QFileSystemWatcher>
#include <QTextStream>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <DDBusSender>
#include <DStandardPaths>

Q_DECLARE_LOGGING_CATEGORY(logUpdateModal)

static const QList<UpdateModel::UpdateError> CAN_BE_FIXED_ERRORS = { UpdateModel::DpkgInterrupted };

UpdateWorker::UpdateWorker(QObject *parent)
    : QObject(parent)
    , m_distUpgradeJob(nullptr)
    , m_fixErrorJob(nullptr)
    , m_checkSystemJob(nullptr)
    , m_dbusProxy(new UpdateDBusProxy(this))
    , m_waitingToCheckSystem(false)
    , m_logWatcherHelper(new LogWatcherHelper(m_dbusProxy, this))
{
}

void UpdateWorker::init()
{
    // m_managerInter->setSync(false);
    connect(m_dbusProxy, &UpdateDBusProxy::JobListChanged, this, &UpdateWorker::onJobListChanged);
    connect(m_dbusProxy, &UpdateDBusProxy::managerInterServiceValidChanged, this, [this](bool valid) {
        if (!valid) {
            const auto status = UpdateModel::instance()->updateStatus();
            qCWarning(logUpdateModal) << "Lastore daemon manager service is invalid, curren status: " << status;
            if (m_distUpgradeJob) {
                delete m_distUpgradeJob;
                m_distUpgradeJob = nullptr;
            }

            if (status != UpdateModel::Installing)
                return;

            UpdateModel::instance()->setLastErrorLog("org.deepin.dde.Lastore1 interface is invalid.");
            UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::UpdateInterfaceError);
            UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::InstallFailed);
        } else {
            if (m_waitingToCheckSystem) {
                m_waitingToCheckSystem = false;
                doCheckSystem(UpdateModel::instance()->updateMode(), UpdateModel::instance()->checkSystemStage());
            }
        }
    });

    getUpdateOption();
    onJobListChanged(m_dbusProxy->jobList());

    connect(m_logWatcherHelper, &LogWatcherHelper::incrementalDataChanged, this, [](const QString &data) {
        UpdateModel::instance()->appendUpdateLog(data);
    });

    connect(m_logWatcherHelper, &LogWatcherHelper::fileReset, this, []() {
        UpdateModel::instance()->setUpdateLog(QString());
    });

    QTimer::singleShot(0, this, [this]() {
        m_logWatcherHelper->startWatchFile();
    });
}

/**
 * @brief 开始更新流程
 * 一般流程： 检查电量 -> 备份 -> 安装 -> 重启/关机
 */
void UpdateWorker::startUpdateProgress()
{
    qCInfo(logUpdateModal) << "Start update progress";

    doDistUpgradeIfCanBackup();
}

void UpdateWorker::doDistUpgrade(bool doBackup)
{
    qCInfo(logUpdateModal) << "Do dist upgrade, do backup: " << doBackup;
    if (!m_dbusProxy->managerInterIsValid()) {
        UpdateModel::instance()->setLastErrorLog("org.deepin.dde.Lastore1.Manager interface is invalid.");
        UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::UpdateInterfaceError);
        UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::InstallFailed);
        return;
    }

    if (UpdateModel::instance()->updateStatus() >= UpdateModel::Installing) {
        qCWarning(logUpdateModal) << "Installing, won't do it again";
        return;
    }

    cleanLaStoreJob(m_distUpgradeJob);
    cleanLaStoreJob(m_backupJob);
    UpdateModel::instance()->setHasBackup(doBackup);
    QDBusPendingReply<QDBusObjectPath> reply = m_dbusProxy->DistUpgradePartly(UpdateModel::instance()->updateMode(), doBackup);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, doBackup] {
        QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
        if (reply.isValid()) {
            if (doBackup) {
                createBackupJob(reply.value().path());   
            } else {
                UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::Installing);
                createDistUpgradeJob(reply.value().path());
            }
        } else {
            const QString &errorMessage = watcher->error().message();
            qCWarning(logUpdateModal) << "Do dist upgrade failed:" << watcher->error().message();
            UpdateModel::instance()->setLastErrorLog(errorMessage);
            UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::UnKnown);
            UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::InstallFailed);
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::onJobListChanged(const QList<QDBusObjectPath> &jobs)
{
    qCInfo(logUpdateModal) << "Job list changed";
    for (const auto &job : jobs) {
        const QString &jobPath = job.path();
        qCInfo(logUpdateModal) << "Path : " << jobPath;
        JobInter jobInter(jobPath, this);
        if (!jobInter.isValid()) {
            qCWarning(logUpdateModal) << "Job is invalid";
            continue;   
        }

        // id maybe scrapped
        const QString &id = jobInter.id();
        qCInfo(logUpdateModal) << "Job id : " << id;
        if (id == "dist_upgrade" && m_distUpgradeJob == nullptr) {
            qCInfo(logUpdateModal) << "Create dist upgrade job";
            createDistUpgradeJob(jobPath);
        } else if (id == "check_system" && m_checkSystemJob == nullptr) {
            qCInfo(logUpdateModal) << "Create check system job";
            createCheckSystemJob(jobPath);
        } else if (id == "backup" && m_backupJob == nullptr) {
            qCInfo(logUpdateModal) << "Create backup job";
            createBackupJob(jobPath);
        }
    }
}

void UpdateWorker::createDistUpgradeJob(const QString& jobPath)
{
    qCInfo(logUpdateModal) << "Job path:" << jobPath;
    if (m_distUpgradeJob) {
        qCWarning(logUpdateModal) << "Dist upgrade job is not null";
        return;
    }

    if (jobPath.isEmpty()) {
        qCWarning(logUpdateModal) << "Dist upgrade job path is empty";
        return;
    }

    m_distUpgradeJob = new JobInter(jobPath, this);
    connect(m_distUpgradeJob, &UpdateJobDBusProxy::ProgressChanged, UpdateModel::instance(), &UpdateModel::setJobProgress);
    connect(m_distUpgradeJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onDistUpgradeStatusChanged);
    UpdateModel::instance()->setJobProgress(m_distUpgradeJob->progress());
    onDistUpgradeStatusChanged(m_distUpgradeJob->status());
}

void UpdateWorker::createCheckSystemJob(const QString& jobPath)
{
    qCInfo(logUpdateModal) << "Job path:" << jobPath;
    if (m_checkSystemJob) {
        qCWarning(logUpdateModal) << "Check system job is not null";
        return;
    }

    if (jobPath.isEmpty()) {
        qCWarning(logUpdateModal) << "Check system job path is empty";
        return;
    }

    m_checkSystemJob = new JobInter(jobPath, this);
    // 低概率出现创建 job 时，系统检查已经完成的情况，此时直接退出进程即可。
    if (m_checkSystemJob->id().isEmpty()) {
        qCWarning(logUpdateModal) << "Check system job id is empty, exit application now";
        qApp->exit();
    }
    connect(m_checkSystemJob, &UpdateJobDBusProxy::ProgressChanged, UpdateModel::instance(), &UpdateModel::setJobProgress);
    connect(m_checkSystemJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onCheckSystemStatusChanged);
    UpdateModel::instance()->setJobProgress(m_checkSystemJob->progress());
    onCheckSystemStatusChanged(m_checkSystemJob->status());
}

void UpdateWorker::createBackupJob(const QString& jobPath)
{
    qCInfo(logUpdateModal) << "Create backup upgrade job, path:" << jobPath;
    if (m_backupJob || jobPath.isEmpty()) {
        qCInfo(logUpdateModal) << "Job is not null or job path is empty";
        return;
    }

    m_backupJob = new UpdateJobDBusProxy(jobPath, this);
    connect(m_backupJob, &UpdateJobDBusProxy::ProgressChanged, UpdateModel::instance(), &UpdateModel::setJobProgress);
    connect(m_backupJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onBackupStatusChanged);
    connect(m_backupJob, &UpdateJobDBusProxy::DescriptionChanged, this, [this](const QString &description) {
        if (m_backupJob->status() == "failed") {
            UpdateModel::instance()->setLastErrorLog(description);
        }
    });
    UpdateModel::instance()->setJobProgress(m_backupJob->progress());
    onBackupStatusChanged(m_backupJob->status());
}

void UpdateWorker::onDistUpgradeStatusChanged(const QString &status)
{
    // 无需处理ready状态
    static const QMap<QString, UpdateModel::UpdateStatus> DIST_UPGRADE_STATUS_MAP = {
        {"running", UpdateModel::UpdateStatus::Installing},
        {"failed", UpdateModel::UpdateStatus::InstallFailed},
        {"succeed", UpdateModel::UpdateStatus::InstallSuccess},
        {"end", UpdateModel::UpdateStatus::InstallComplete}
    };

    qCInfo(logUpdateModal) << "Dist upgrade status changed " << status;
    if (DIST_UPGRADE_STATUS_MAP.contains(status)) {
        const UpdateModel::UpdateStatus updateStatus = DIST_UPGRADE_STATUS_MAP.value(status);
        if (updateStatus == UpdateModel::UpdateStatus::InstallComplete) {
            cleanLaStoreJob(m_distUpgradeJob);
        } else {
            if (updateStatus == UpdateModel::InstallFailed && m_distUpgradeJob) {
                const auto &description = m_distUpgradeJob->description();
                const auto error = analyzeJobErrorMessage(description);
                // 如果错误修复可以被修复，那么自动走修复流程
                // 界面状态冻结，根据修复结果再做处理
                if (CAN_BE_FIXED_ERRORS.contains(error) && fixError(error, description, UpdateModel::InstallFailed))
                    return;

                UpdateModel::instance()->setLastErrorLog(description);
                UpdateModel::instance()->setUpdateError(error);
            }
            UpdateModel::instance()->setUpdateStatus(updateStatus);
        }
    } else {
        qCWarning(logUpdateModal) << "Unknown dist upgrade status";
    }
}

void UpdateWorker::onCheckSystemStatusChanged(const QString &status)
{
    // 无需处理ready状态
    static const QMap<QString, UpdateModel::CheckStatus> CHECK_UPGRADE_STATUS_MAP = {
        {"running", UpdateModel::Checking},
        {"failed", UpdateModel::CheckFailed},
        {"succeed", UpdateModel::CheckSuccess},
        {"end", UpdateModel::CheckEnd}
    };

    qCInfo(logUpdateModal) << "Check system status changed " << status;
    if (CHECK_UPGRADE_STATUS_MAP.contains(status)) {
        const auto updateStatus = CHECK_UPGRADE_STATUS_MAP.value(status);
        const auto stage = UpdateModel::instance()->checkSystemStage();
        qCInfo(logUpdateModal) << "Check system stage:" << stage << ", update status:" << updateStatus;
        if (UpdateModel::CSS_AfterLogin == stage && updateStatus == UpdateModel::CheckEnd) {
            cleanLaStoreJob(m_checkSystemJob);
        } else {
            if (updateStatus == UpdateModel::CheckFailed && m_checkSystemJob) {
                UpdateModel::instance()->setLastErrorLog(m_checkSystemJob->description());
            }
            if (UpdateModel::CSS_BeforeLogin == stage) {
                if (updateStatus == UpdateModel::CheckFailed) {
                    doAction(UpdateModel::Reboot);
                } else if (updateStatus == UpdateModel::CheckSuccess || updateStatus == UpdateModel::CheckEnd) {
                    cleanLaStoreJob(m_checkSystemJob);
                    qCInfo(logUpdateModal) << "Exit application";
                    qApp->exit();
                    QTimer::singleShot(5000, qApp, [] {
                        qCWarning(logUpdateModal) << "Something abnormal, I'm still here";
                        qApp->quit();
                    });
                }
            } else {
                UpdateModel::instance()->setCheckStatus(updateStatus);
            }
        }
    } else {
        qCWarning(logUpdateModal) << "Unknown check system status";
    }
}

void UpdateWorker::onBackupStatusChanged(const QString &value)
{
    qCInfo(logUpdateModal) << "backup status changed: " << value;
    if (value == "failed") {
        const auto& description = m_backupJob->description();

        auto error = analyzeJobErrorMessage(description);
        if (error == UpdateModel::UpdateError::InstallNoSpace) {
            UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::BackupNoSpace);
        } else if (error == UpdateModel::UpdateError::UnKnown) {
            UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::BackupFailedUnknownReason);
        } else {
            UpdateModel::instance()->setUpdateError(error);
        }

        UpdateModel::instance()->setLastErrorLog(description);
        UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::BackupFailed);
    } else if (value == "end") {
        cleanLaStoreJob(m_backupJob);
    } else if (value == "running") {
        UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::BackingUp);
    }
}

UpdateModel::UpdateError UpdateWorker::analyzeJobErrorMessage(QString jobDescription)
{
    qCInfo(logUpdateModal) << "Analyze job error message: " << jobDescription;
    QJsonParseError err_rpt;
    QJsonDocument jobErrorMessage = QJsonDocument::fromJson(jobDescription.toUtf8(), &err_rpt);

    if (err_rpt.error != QJsonParseError::NoError) {
        qCWarning(logUpdateModal) << "Json format error";
        return UpdateModel::UpdateError::UnKnown;
    }
    const QJsonObject &object = jobErrorMessage.object();
    QString errorType =  object.value("ErrType").toString();

    if (errorType.contains("unmetDependencies", Qt::CaseInsensitive) || errorType.contains("dependenciesBroken", Qt::CaseInsensitive)) {
        return UpdateModel::UpdateError::DependenciesBrokenError;
    }
    if (errorType.contains("insufficientSpace", Qt::CaseInsensitive)) {
        return UpdateModel::UpdateError::InstallNoSpace;
    }
    if (errorType.contains("interrupted", Qt::CaseInsensitive)) {
        return UpdateModel::UpdateError::DpkgInterrupted;
    }

    return UpdateModel::UpdateError::UnKnown;
}

void UpdateWorker::doDistUpgradeIfCanBackup()
{
// TODO 进入更新的接口
    qCInfo(logUpdateModal) << "Prepare to do backup";
    // TODO: 暂未支持选择是否备份，之后会在社区版支持，现在默认备份
    bool needBackup = true;
    // TODO: 磐石暂未实现当前能否备份的接口，当前默认支持
    bool canBackup = true;
    if(canBackup) {
        doDistUpgrade(needBackup);
    } else {
        qCWarning(logUpdateModal) << "Can not backup";
        UpdateModel::instance()->setLastErrorLog("");
        UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::CanNotBackup);
        UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::InstallFailed);
    }
}

void UpdateWorker::doCheckSystem(int updateMode, UpdateModel::CheckSystemStage stage)
{
    qCInfo(logUpdateModal) << "Update mode:" << updateMode << ", check system stage:" << stage;
    if (!m_dbusProxy->managerInterIsValid()) {
        qCWarning(logUpdateModal) << "Update mode is invalid";
        m_waitingToCheckSystem = true;
        return;
    }

    QDBusPendingReply<QDBusObjectPath> reply = m_dbusProxy->CheckUpgrade(updateMode, static_cast<int>(stage));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        QDBusPendingReply<QDBusObjectPath> reply = watcher->reply();
        if (reply.isValid()) {
            UpdateModel::instance()->setCheckStatus(UpdateModel::CheckStatus::ReadToCheck);
            createCheckSystemJob(reply.value().path());
        } else {
            const QString &errorMessage = watcher->error().message();
            qCWarning(logUpdateModal) << "Call `CheckUpgrade` function failed, error:" << watcher->error().message();
            UpdateModel::instance()->setLastErrorLog(errorMessage);
            UpdateModel::instance()->setCheckStatus(UpdateModel::CheckStatus::CheckFailed);
        }
        watcher->deleteLater();
    });
}

void UpdateWorker::doAction(UpdateModel::UpdateAction action)
{
    qCInfo(logUpdateModal) << "Do action: " << action;
    switch (action) {
        case UpdateModel::DoBackupAgain:
            startUpdateProgress();
            break;
        case UpdateModel::ContinueUpdating:
            doDistUpgrade(false);
            break;
        case UpdateModel::Reboot:
            doPowerAction(true);
            break;
        case UpdateModel::ExitUpdating:
            doPowerAction(UpdateModel::instance()->isReboot());
            break;
        case UpdateModel::ShutDown:
            doPowerAction(false);
            break;
        case UpdateModel::EnterDesktop:
            qApp->exit();
            break;
        default:
            break;
    }
}

bool UpdateWorker::checkPower()
{
    qCInfo(logUpdateModal) << "Check power";
    bool onBattery = m_dbusProxy->onBattery();
    if (!onBattery) {
        qCInfo(logUpdateModal) << "No battery";
        return true;
    }

    double data = m_dbusProxy->batteryPercentage().value("Display", 0);
    int batteryPercentage = uint(qMin(100.0, qMax(0.0, data)));
    qCInfo(logUpdateModal) << "Battery percentage: " << batteryPercentage;
    return batteryPercentage >= 60;
}

/**
 * @brief 修复更新错误（现在只能修复dpkg错误）
 *
 */
bool UpdateWorker::fixError(UpdateModel::UpdateError error, const QString &description, UpdateModel::UpdateStatus updateStatus)
{
    qCInfo(logUpdateModal) << "Try to fix error:" << error;
    if (m_fixErrorJob) {
        qCWarning(logUpdateModal) << "Fix error job is not nullptr, won't fix error again";
        return false;
    }

    if(!CAN_BE_FIXED_ERRORS.contains(error)) {
        qCWarning(logUpdateModal) << "Not supported for fixing" << error;
        return false;
    }

    const auto &errorString = UpdateModel::updateErrorToString(error);
    if (errorString.isEmpty()) {
        qCWarning(logUpdateModal) << "Error message is empty";
        return false;
    }

    QDBusReply<QDBusObjectPath> reply = m_dbusProxy->fixError(errorString);
    if (!reply.isValid()) {
        qCWarning(logUpdateModal) << "Call `FixError` reply is invalid, error: " << reply.error().message();
        return false;
    }

    m_fixErrorJob = new JobInter(reply.value().path(), this);
    if (!m_fixErrorJob->isValid()) {
        qCWarning(logUpdateModal) << "Fix error job is invalid";
        delete m_fixErrorJob;
        m_fixErrorJob = nullptr;
        return false;
    }

    connect(m_fixErrorJob, &JobInter::StatusChanged, this, [this, error, description, updateStatus](const QString status) {
        qCInfo(logUpdateModal) << "Fix error job status changed :" << status;
        if (status == "succeed" || status == "failed" || status == "end") {
            if (m_fixErrorJob) {
                cleanLaStoreJob(m_fixErrorJob);
            }

            if (status == "succeed") {
                // 重新开始安装流程
                if (updateStatus >= UpdateModel::UpdateStatus::Installing) {
                    startUpdateProgress();
                }
            } else if(status == "failed") {
                UpdateModel::instance()->setUpdateError(error);
                UpdateModel::instance()->setUpdateStatus(updateStatus);
            }
        }
    });

    return true;
}

void UpdateWorker::doPowerAction(bool reboot)
{
    qCInfo(logUpdateModal) << "Update worker do power action, is reboot: " << reboot;

    auto powerActionReply = m_dbusProxy->Poweroff(reboot);
    powerActionReply.waitForFinished();
    if (powerActionReply.isError()) {
        qCWarning(logUpdateModal) << "Do power action failed: " << powerActionReply.error().message();
    } else {
        qCInfo(logUpdateModal) << "Do power action successfully ";
    }
}

void UpdateWorker::enableShortcuts(bool enable)
{
    qCInfo(logUpdateModal) << "Enable shortcuts: " << enable;
    QDBusPendingCall reply = DDBusSender()
        .service("com.deepin.dde.osd")
        .path("/")
        .interface("com.deepin.dde.osd")
        .property("OSDEnabled")
        .set(enable);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher] {
        QDBusPendingReply<void> reply = watcher->reply();
        if (reply.isError()) {
            qCWarning(logUpdateModal) << "Set `OSDEnabled` property failed, error: " << reply.error().message();
        }
        watcher->deleteLater();
    });
}

/**
 * @brief 同步调用的方式拉起服务
 *
 * @param interface 需要拉起的服务
 * @return true service is valid
 * @return false service is invalid
 */
bool UpdateWorker::syncStartService(const QString &serviceName)
{
    QDBusInterface inter("org.freedesktop.DBus", "/", "org.freedesktop.DBus", QDBusConnection::systemBus());
    QDBusReply<uint32_t> reply = inter.call("StartServiceByName", serviceName, quint32(0));
    if (reply.isValid()) {
        qCInfo(logUpdateModal) << QString("Start %1 service result: ").arg(serviceName) << reply.value();
        return reply.value() == 1;
    } else {
        qCWarning(logUpdateModal) << QString("Start %1 service failed, error: ").arg(serviceName) << reply.error().message();
        return false;
    }
}

/**
 * @brief 此函数用来在切换session后检查备份和更新的状态
 * 避免用户切换到其他的tty再切换回来无法正确获取状态导致进度卡住
 *
 */
void UpdateWorker::checkStatusAfterSessionActive()
{
    qCInfo(logUpdateModal) << "Check installation status";
    if (!m_dbusProxy->managerInterIsValid() && m_distUpgradeJob) {
        delete m_distUpgradeJob;
        m_distUpgradeJob = nullptr;
        syncStartService(ManagerService);
    }

    if (m_dbusProxy->managerInterIsValid()) {
        onJobListChanged(m_dbusProxy->jobList());
        if (m_distUpgradeJob) {
            return;
        }
    }
    const int lastoreDaemonStatus = DConfigHelper::instance()->getConfig("org.deepin.dde.lastore", "org.deepin.dde.lastore", "","lastore-daemon-status", 0).toInt();
    qCInfo(logUpdateModal) << "Lastore daemon status: " << lastoreDaemonStatus;
    static const int IS_UPDATE_READY = 1; // 第一位表示更新是否可用
    const bool isUpdateReady = lastoreDaemonStatus & IS_UPDATE_READY;
    if (!isUpdateReady) {
        // 更新成功
        qCInfo(logUpdateModal) << "Update successfully";
        UpdateModel::instance()->setUpdateStatus(UpdateModel::InstallSuccess);
    }
}

void UpdateWorker::cleanLaStoreJob(QPointer<JobInter> dbusJob)
{
    qCInfo(logUpdateModal) << "Clean job";
    if (dbusJob != nullptr) {
        qCInfo(logUpdateModal) << "Job path" << dbusJob->path();
        m_dbusProxy->CleanJob(dbusJob->id());
        delete dbusJob;
        dbusJob = nullptr;
    }
}

void UpdateWorker::getUpdateOption()
{
    static const QString UPDATE_OPTION_FILE = "/tmp/deepin_update_option.json";

    const auto defaultPowerAction = UpdateModel::instance()->isReboot() ? "Reboot" : "PowerOff";
    QFile f(UPDATE_OPTION_FILE);
    if (f.open(QIODevice::ReadOnly)) {
        auto data = f.readAll();
        f.close();
        QJsonParseError parseErr;
        auto doc =  QJsonDocument::fromJson(data, &parseErr);
        if (parseErr.error != QJsonParseError::NoError || doc.isEmpty()) {
            qCWarning(logUpdateModal) << "Parse " << UPDATE_OPTION_FILE << " failed, use default power action:" << defaultPowerAction;
            return;
        }

        auto obj = doc.object();
        if (obj.contains("IsPowerOff")) {
            const bool isReboot = !(obj.value("IsPowerOff").toBool(false));
            qCInfo(logUpdateModal) << "Power action:" << (isReboot ? "Reboot" : "Power off");
            UpdateModel::instance()->setIsReboot(isReboot);
        }
        if (obj.contains("DoUpgradeMode")) {
            const int upgradeMode = obj.value("DoUpgradeMode").toInt(5); // system:1 security:4
            qCInfo(logUpdateModal) << "Upgrade mode:" << upgradeMode;
            UpdateModel::instance()->setUpdateMode(upgradeMode);
        }
        if (obj.contains("DoUpgrade")) {
            const bool whetherDoUpgrade = obj.value("DoUpgrade").toBool(false); // system:1 security:4
            qCInfo(logUpdateModal) << "Whether do upgrade:" << whetherDoUpgrade;
            UpdateModel::instance()->setDoUpgrade(whetherDoUpgrade);
            if (whetherDoUpgrade)
                return;
        }

        if (obj.value("PreGreeterCheck").toBool(false)) {
            qCInfo(logUpdateModal) << "Do pre greeter check";
            UpdateModel::instance()->setCheckSystemStage(UpdateModel::CSS_BeforeLogin);
        } else if (obj.value("AfterGreeterCheck").toBool(false)) {
            qCInfo(logUpdateModal) << "Do after greeter check";
            UpdateModel::instance()->setCheckSystemStage(UpdateModel::CSS_AfterLogin);
        }
    } else {
        qCWarning(logUpdateModal) << "Open " << UPDATE_OPTION_FILE << " failed, use default power action:" << defaultPowerAction;
    }
}

void UpdateWorker::forceReboot(bool reboot)
{
    qCInfo(logUpdateModal) << "Force reboot:" << reboot;
    QDBusPendingReply<void> reply = m_dbusProxy->Poweroff(reboot);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [watcher] {
        QDBusPendingReply<void> reply = watcher->reply();
        if (reply.isError()) {
            qCWarning(logUpdateModal) << "Power off failed:" << reply.error().message();
        }
        watcher->deleteLater();
    });
}

bool UpdateWorker::exportUpdateLog()
{
    const auto& [uid, name] = getCurrentUser();
    if (uid == 0) {
        qCWarning(logUpdateModal) << "Current user's uid is invalid";
        emit exportUpdateLogFinished(false);
        return false;
    }

    QString desktopPath = Dtk::Core::DStandardPaths::homePath(uid) + "/Desktop";
    if (desktopPath.isEmpty()) {
        qCWarning(logUpdateModal) << "Cannot get desktop path for user:" << name << "uid:" << uid;
        emit exportUpdateLogFinished(false);
        return false;
    }

    // 创建固定的安全临时目录
    const QString tempDirPath = "/tmp/deepin-update-ui";
    QDir tempDir;
    if (!tempDir.exists(tempDirPath)) {
        if (!tempDir.mkpath(tempDirPath)) {
            qCWarning(logUpdateModal) << "Failed to create temp directory:" << tempDirPath;
            emit exportUpdateLogFinished(false);
            return false;
        }
    }

    // 只有lightdm用户和lightdm组可以访问
    if (chmod(tempDirPath.toUtf8().constData(), 0750) == -1) {
        qCWarning(logUpdateModal) << "Failed to set directory permissions:" << strerror(errno);
        emit exportUpdateLogFinished(false);
        return false;
    }

    const QString fileName = QString("%1_%2.txt")
        .arg(tr("updatelog"))
        .arg(QDateTime::currentDateTime().toString("yyyy_MM_dd_HH.mm.ss"));
    const QString tempLogPath = tempDirPath + "/" + fileName;

    qCInfo(logUpdateModal) << "Export update log to temp file:" << tempLogPath;
    qCInfo(logUpdateModal) << "Target desktop path:" << desktopPath;

    int fd = open(tempLogPath.toUtf8().constData(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd == -1) {
        qCWarning(logUpdateModal) << "Failed to create temp log file:" << tempLogPath << "error:" << strerror(errno);
        emit exportUpdateLogFinished(false);
        return false;
    }

    QDBusPendingCall call = m_dbusProxy->GetUpdateDetails(fd, false);
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(call, this);
    
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher, tempLogPath, desktopPath, fd] {
        close(fd);

        QDBusPendingReply<void> reply = *watcher;
        if (reply.isError()) {
            qCWarning(logUpdateModal) << "Export update details failed:" << reply.error().message();
            // 清理失败的临时文件
            QFile::remove(tempLogPath);
            emit exportUpdateLogFinished(false);
        } else {
            qCInfo(logUpdateModal) << "Export update log successfully to temp file:" << tempLogPath;

            // 使用systemd模板服务传递十六进制编码的参数（避免特殊字符问题）
            QString pathsParam = tempLogPath + ":" + desktopPath;
            QByteArray encodedParam = pathsParam.toUtf8().toHex();
            QString serviceName = QString("deepin-update-log-copy@%1.service").arg(QString::fromLatin1(encodedParam));
            
            QProcess::startDetached("systemctl", QStringList() << "start" << serviceName);
            qCInfo(logUpdateModal) << "Started copy service with hex encoded parameters:" << pathsParam;
            
            emit exportUpdateLogFinished(true);
        }
        watcher->deleteLater();
    });

    return true;
}
