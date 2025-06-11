// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "updateworker.h"
#include "dconfig_helper.h"
#include "common/dbus/updatejobdbusproxy.h"
#include "common/commondefine.h"

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

#include <DDBusSender>

Q_LOGGING_CATEGORY(DDE_UPDATE_WORKER, "dde-update-worker")

static const QList<UpdateModel::UpdateError> CAN_BE_FIXED_ERRORS = { UpdateModel::DpkgInterrupted };

UpdateWorker::UpdateWorker(QObject *parent)
    : QObject(parent)
    , m_distUpgradeJob(nullptr)
    , m_fixErrorJob(nullptr)
    , m_checkSystemJob(nullptr)
    , m_dbusProxy(new UpdateDBusProxy(this))
    , m_waitingToCheckSystem(false)
{
}

void UpdateWorker::init()
{
    // m_managerInter->setSync(false);
    connect(m_dbusProxy, &UpdateDBusProxy::JobListChanged, this, &UpdateWorker::onJobListChanged);
    connect(m_dbusProxy, &UpdateDBusProxy::managerInterServiceValidChanged, this, [this](bool valid) {
        if (!valid) {
            const auto status = UpdateModel::instance()->updateStatus();
            qWarning() << "Lastore daemon manager service is invalid, curren status: " << status;
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
}

/**
 * @brief 开始更新流程
 * 一般流程： 检查电量 -> 备份 -> 安装 -> 重启/关机
 */
void UpdateWorker::startUpdateProgress()
{
    qInfo() << "Start update progress";

    doDistUpgradeIfCanBackup();
}

void UpdateWorker::doDistUpgrade(bool doBackup)
{
    qInfo() << "Do dist upgrade, do backup: " << doBackup;
    if (!m_dbusProxy->managerInterIsValid()) {
        UpdateModel::instance()->setLastErrorLog("org.deepin.dde.Lastore1.Manager interface is invalid.");
        UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::UpdateInterfaceError);
        UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::InstallFailed);
        return;
    }

    if (UpdateModel::instance()->updateStatus() >= UpdateModel::Installing) {
        qWarning() << "Installing, won't do it again";
        return;
    }

    cleanLaStoreJob(m_distUpgradeJob);
    cleanLaStoreJob(m_backupJob);
    UpdateModel::instance()->setHasBackup(doBackup);
    QDBusPendingReply<QDBusObjectPath> reply = m_dbusProxy->DistUpgradePartly(UpdateModel::instance()->updateMode(), doBackup);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, reply, watcher, doBackup] {
        watcher->deleteLater();
        if (reply.isValid()) {
            if (doBackup) {
                createBackupJob(reply.value().path());
            } else {
                UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::Installing);
                createDistUpgradeJob(reply.value().path());
            }
            
        } else {
            const QString &errorMessage = watcher->error().message();
            qWarning() << "Do dist upgrade failed:" << watcher->error().message();
            UpdateModel::instance()->setLastErrorLog(errorMessage);
            UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::UnKnown);
            UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::InstallFailed);
        }
    });
}

void UpdateWorker::onJobListChanged(const QList<QDBusObjectPath> &jobs)
{
    qInfo() << "Job list changed";
    for (const auto &job : jobs) {
        const QString &jobPath = job.path();
        qInfo() << "Path : " << jobPath;
        JobInter jobInter(jobPath, this);
        if (!jobInter.isValid()) {
            qWarning() << "Job is invalid";
            continue;
        }

        // id maybe scrapped
        const QString &id = jobInter.id();
        qInfo() << "Job id : " << id;
        if (id == "dist_upgrade" && m_distUpgradeJob == nullptr) {
            qInfo() << "Create dist upgrade job";
            createDistUpgradeJob(jobPath);
        } else if (id == "check_system" && m_checkSystemJob == nullptr) {
            qInfo() << "Create check system job";
            createCheckSystemJob(jobPath);
        } else if (id == "backup" && m_backupJob == nullptr) {
            qInfo() << "Create backup job";
            createBackupJob(jobPath);
        }
    }
}

void UpdateWorker::createDistUpgradeJob(const QString& jobPath)
{
    qInfo() << "Job path:" << jobPath;
    if (m_distUpgradeJob) {
        qWarning() << "Dist upgrade job is not null";
        return;
    }

    if (jobPath.isEmpty()) {
        qWarning() << "Dist upgrade job path is empty";
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
    qInfo() << "Job path:" << jobPath;
    if (m_checkSystemJob) {
        qWarning() << "Check system job is not null";
        return;
    }

    if (jobPath.isEmpty()) {
        qWarning() << "Check system job path is empty";
        return;
    }

    m_checkSystemJob = new JobInter(jobPath, this);
    // 低概率出现创建 job 时，系统检查已经完成的情况，此时直接退出进程即可。
    if (m_checkSystemJob->id().isEmpty()) {
        qWarning() << "Check system job id is empty, exit application now";
        qApp->exit();
    }
    connect(m_checkSystemJob, &UpdateJobDBusProxy::ProgressChanged, UpdateModel::instance(), &UpdateModel::setJobProgress);
    connect(m_checkSystemJob, &UpdateJobDBusProxy::StatusChanged, this, &UpdateWorker::onCheckSystemStatusChanged);
    UpdateModel::instance()->setJobProgress(m_checkSystemJob->progress());
    onCheckSystemStatusChanged(m_checkSystemJob->status());
}

void UpdateWorker::createBackupJob(const QString& jobPath)
{
    qCInfo(DDE_UPDATE_WORKER) << "Create backup upgrade job, path:" << jobPath;
    if (m_backupJob || jobPath.isEmpty()) {
        qCInfo(DDE_UPDATE_WORKER) << "Job is not null or job path is empty";
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

    qInfo() << "Dist upgrade status changed " << status;
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
        qWarning() << "Unknown dist upgrade status";
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

    qInfo() << "Check system status changed " << status;
    if (CHECK_UPGRADE_STATUS_MAP.contains(status)) {
        const auto updateStatus = CHECK_UPGRADE_STATUS_MAP.value(status);
        const auto stage = UpdateModel::instance()->checkSystemStage();
        qInfo() << "Check system stage:" << stage << ", update status:" << updateStatus;
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
                    qInfo() << "Exit application";
                    qApp->exit();
                    QTimer::singleShot(5000, qApp, [] {
                        qWarning() << "Something abnormal, I'm still here";
                        qApp->quit();
                    });
                }
            } else {
                UpdateModel::instance()->setCheckStatus(updateStatus);
            }
        }
    } else {
        qWarning() << "Unknown check system status";
    }
}

void UpdateWorker::onBackupStatusChanged(const QString &value)
{
    qCInfo(DDE_UPDATE_WORKER) << "backup status changed: " << value;
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
    qInfo() << "Analyze job error message: " << jobDescription;
    QJsonParseError err_rpt;
    QJsonDocument jobErrorMessage = QJsonDocument::fromJson(jobDescription.toUtf8(), &err_rpt);

    if (err_rpt.error != QJsonParseError::NoError) {
        qWarning() << "Json format error";
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
    qInfo() << "Prepare to do backup";
    // TODO: 暂未支持选择是否备份，之后会在社区版支持，现在默认备份
    bool needBackup = true;
    // TODO: 磐石暂未实现当前能否备份的接口，当前默认支持
    bool canBackup = true;
    if(canBackup) {
        doDistUpgrade(needBackup);
    } else {
        qWarning() << "Can not backup";
        UpdateModel::instance()->setLastErrorLog("");
        UpdateModel::instance()->setUpdateError(UpdateModel::UpdateError::CanNotBackup);
        UpdateModel::instance()->setUpdateStatus(UpdateModel::UpdateStatus::InstallFailed);
    }
}

void UpdateWorker::doCheckSystem(int updateMode, UpdateModel::CheckSystemStage stage)
{
    qInfo() << "Update mode:" << updateMode << ", check system stage:" << stage;
    if (!m_dbusProxy->managerInterIsValid()) {
        qWarning() << "Update mode is invalid";
        m_waitingToCheckSystem = true;
        return;
    }

    QDBusPendingReply<QDBusObjectPath> reply = m_dbusProxy->CheckUpgrade(updateMode, static_cast<int>(stage));
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, reply, watcher] {
        watcher->deleteLater();
        if (reply.isValid()) {
            UpdateModel::instance()->setCheckStatus(UpdateModel::CheckStatus::ReadToCheck);
            createCheckSystemJob(reply.value().path());
        } else {
            const QString &errorMessage = watcher->error().message();
            qWarning() << "Call `CheckUpgrade` function failed, error:" << watcher->error().message();
            UpdateModel::instance()->setLastErrorLog(errorMessage);
            UpdateModel::instance()->setCheckStatus(UpdateModel::CheckStatus::CheckFailed);
        }
    });
}

void UpdateWorker::doAction(UpdateModel::UpdateAction action)
{
    qInfo() << "Do action: " << action;
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
    qInfo() << "Check power";
    bool onBattery = m_dbusProxy->onBattery();
    if (!onBattery) {
        qInfo() << "No battery";
        return true;
    }

    double data = m_dbusProxy->batteryPercentage().value("Display", 0);
    int batteryPercentage = uint(qMin(100.0, qMax(0.0, data)));
    qInfo() << "Battery percentage: " << batteryPercentage;
    return batteryPercentage >= 60;
}

/**
 * @brief 修复更新错误（现在只能修复dpkg错误）
 *
 */
bool UpdateWorker::fixError(UpdateModel::UpdateError error, const QString &description, UpdateModel::UpdateStatus updateStatus)
{
    qInfo() << "Try to fix error:" << error;
    if (m_fixErrorJob) {
        qWarning() << "Fix error job is not nullptr, won't fix error again";
        return false;
    }

    if(!CAN_BE_FIXED_ERRORS.contains(error)) {
        qWarning() << "Not supported for fixing" << error;
        return false;
    }

    const auto &errorString = UpdateModel::updateErrorToString(error);
    if (errorString.isEmpty()) {
        qWarning() << "Error message is empty";
        return false;
    }

    QDBusReply<QDBusObjectPath> reply = m_dbusProxy->fixError(errorString);
    if (!reply.isValid()) {
        qWarning() << "Call `FixError` reply is invalid, error: " << reply.error().message();
        return false;
    }

    m_fixErrorJob = new JobInter(reply.value().path(), this);
    if (!m_fixErrorJob->isValid()) {
        qWarning() << "Fix error job is invalid";
        delete m_fixErrorJob;
        m_fixErrorJob = nullptr;
        return false;
    }

    connect(m_fixErrorJob, &JobInter::StatusChanged, this, [this, error, description, updateStatus](const QString status) {
        qInfo() << "Fix error job status changed :" << status;
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
    qInfo() << "Update worker do power action, is reboot: " << reboot;

    auto powerActionReply = m_dbusProxy->Poweroff(reboot);
    powerActionReply.waitForFinished();
    if (powerActionReply.isError()) {
        qWarning() << "Do power action failed: " << powerActionReply.error().message();
    } else {
        qInfo() << "Do power action successfully ";
    }
}

void UpdateWorker::enableShortcuts(bool enable)
{
    qInfo() << "Enable shortcuts: " << enable;
    QDBusPendingCall reply = DDBusSender()
        .service("com.deepin.dde.osd")
        .path("/")
        .interface("com.deepin.dde.osd")
        .property("OSDEnabled")
        .set(enable);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [reply, watcher] {
        watcher->deleteLater();
        if (!reply.isValid()) {
            qWarning() << "Set `OSDEnabled` property failed, error: " << reply.error().message();
        }
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
        qInfo() << QString("Start %1 service result: ").arg(serviceName) << reply.value();
        return reply.value() == 1;
    } else {
        qWarning() << QString("Start %1 service failed, error: ").arg(serviceName) << reply.error().message();
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
    qInfo() << "Check installation status";
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
    qInfo() << "Lastore daemon status: " << lastoreDaemonStatus;
    static const int IS_UPDATE_READY = 1; // 第一位表示更新是否可用
    const bool isUpdateReady = lastoreDaemonStatus & IS_UPDATE_READY;
    if (!isUpdateReady) {
        // 更新成功
        qInfo() << "Update successfully";
        UpdateModel::instance()->setUpdateStatus(UpdateModel::InstallSuccess);
    }
}

void UpdateWorker::cleanLaStoreJob(QPointer<JobInter> dbusJob)
{
    qInfo() << "Clean job";
    if (dbusJob != nullptr) {
        qInfo() << "Job path" << dbusJob->path();
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
            qWarning() << "Parse " << UPDATE_OPTION_FILE << " failed, use default power action:" << defaultPowerAction;
            return;
        }

        auto obj = doc.object();
        if (obj.contains("IsPowerOff")) {
            const bool isReboot = !(obj.value("IsPowerOff").toBool(false));
            qInfo() << "Power action:" << (isReboot ? "Reboot" : "Power off");
            UpdateModel::instance()->setIsReboot(isReboot);
        }
        if (obj.contains("DoUpgradeMode")) {
            const int upgradeMode = obj.value("DoUpgradeMode").toInt(5); // system:1 security:4
            qInfo() << "Upgrade mode:" << upgradeMode;
            UpdateModel::instance()->setUpdateMode(upgradeMode);
        }
        if (obj.contains("DoUpgrade")) {
            const bool whetherDoUpgrade = obj.value("DoUpgrade").toBool(false); // system:1 security:4
            qInfo() << "Whether do upgrade:" << whetherDoUpgrade;
            UpdateModel::instance()->setDoUpgrade(whetherDoUpgrade);
            if (whetherDoUpgrade)
                return;
        }

        if (obj.value("PreGreeterCheck").toBool(false)) {
            qInfo() << "Do pre greeter check";
            UpdateModel::instance()->setCheckSystemStage(UpdateModel::CSS_BeforeLogin);
        } else if (obj.value("AfterGreeterCheck").toBool(false)) {
            qInfo() << "Do after greeter check";
            UpdateModel::instance()->setCheckSystemStage(UpdateModel::CSS_AfterLogin);
        }
    } else {
        qWarning() << "Open " << UPDATE_OPTION_FILE << " failed, use default power action:" << defaultPowerAction;
    }
}

void UpdateWorker::forceReboot(bool reboot)
{
    qInfo() << "Force reboot:" << reboot;
    QDBusPendingReply<void> reply = m_dbusProxy->Poweroff(reboot);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [reply, watcher] {
        watcher->deleteLater();
        if (reply.isError()) {
            qWarning() << "Power off failed:" << reply.error().message();
        }
    });
}
