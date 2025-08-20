// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updatedbusproxy.h"

#include <QDBusArgument>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QDBusUnixFileDescriptor>
#include <QLoggingCategory>

#include "common/commondefine.h"

Q_DECLARE_LOGGING_CATEGORY(logCommon)

UpdateDBusProxy::UpdateDBusProxy(QObject *parent)
    : QObject(parent)
    , m_hostname1Inter(new DDBusInterface(
              HostnameService, HostnamePath, HostnameInterface, QDBusConnection::systemBus(), this))
    , m_updateInter(new DDBusInterface(
              UpdaterService, UpdaterPath, UpdaterInterface, QDBusConnection::systemBus(), this))
    , m_managerInter(new DDBusInterface(
              ManagerService, ManagerPath, ManagerInterface, QDBusConnection::systemBus(), this))
    , m_powerInter(new DDBusInterface(
              PowerService, PowerPath, PowerInterface, QDBusConnection::sessionBus(), this))
    , m_atomicUpgradeInter(new DDBusInterface(AtomicUpdaterService,
                                              AtomicUpdaterPath,
                                              AtomicUpdaterJobInterface,
                                              QDBusConnection::systemBus(),
                                              this))
    , m_smartMirrorInter(new DDBusInterface("org.deepin.dde.Lastore1.Smartmirror",
                                        "/org/deepin/dde/Lastore1/Smartmirror",
                                          "org.deepin.dde.Lastore1.Smartmirror",
                                          QDBusConnection::systemBus(),
                                          this))
    , m_lockServiceInter(new DDBusInterface(
        LockService, LockPath, LockInterface, QDBusConnection::systemBus(), this))
    , m_shutdownFrontInter(new DDBusInterface(
        ShutdownFront1Service, ShutdownFront1Path, ShutdownFront1Interface, QDBusConnection::sessionBus(), this))
    , m_interWatcher(new QDBusServiceWatcher(UpdaterService, QDBusConnection::systemBus()))

{
    qCDebug(logCommon) << "Initialize UpdateDBusProxy with multiple services";
    qRegisterMetaType<LastoreUpdatePackagesInfo>("LastoreUpdatePackagesInfo");
    qDBusRegisterMetaType<LastoreUpdatePackagesInfo>();

    qRegisterMetaType<BatteryPercentageInfo>("BatteryPercentageInfo");
    qDBusRegisterMetaType<BatteryPercentageInfo>();

    m_interWatcher->setWatchedServices({UpdaterService, ManagerInterface, HostnameService});

    connect(m_interWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &serviceName) {
        qCDebug(logCommon) << "DBus service registered:" << serviceName;
        if (serviceName == ManagerService) {
            emit managerInterServiceValidChanged(true);
        }
    });

    connect(m_interWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        qCDebug(logCommon) << "DBus service unregistered:" << serviceName;
        if (serviceName == ManagerService) {
            emit managerInterServiceValidChanged(false);
        }
    });
}

UpdateDBusProxy::~UpdateDBusProxy()
{
    qCDebug(logCommon) << "Destroying UpdateDBusProxy, cleaning up interfaces";
    m_hostname1Inter->deleteLater();
    m_updateInter->deleteLater();
    m_managerInter->deleteLater();
    m_powerInter->deleteLater();
    m_atomicUpgradeInter->deleteLater();
}

QString UpdateDBusProxy::staticHostname() const
{
    return qvariant_cast<QString>(m_hostname1Inter->property("StaticHostname"));
}

bool UpdateDBusProxy::updateNotify()
{
    return qvariant_cast<bool>(m_updateInter->property("UpdateNotify"));
}

void UpdateDBusProxy::SetUpdateNotify(bool in0)
{
    qCDebug(logCommon) << "Setting update notify:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    m_updateInter->asyncCallWithArgumentList(QStringLiteral("SetUpdateNotify"), argumentList);
}

LastoreUpdatePackagesInfo UpdateDBusProxy::classifiedUpdatablePackages()
{
    qCDebug(logCommon) << "Getting classified updatable packages";
    QDBusInterface updateInter(m_updateInter->service(),
                               m_updateInter->path(),
                               PropertiesInterface,
                               m_updateInter->connection());
    QDBusMessage mess = updateInter.call(QStringLiteral("Get"),
                                         m_updateInter->interface(),
                                         QStringLiteral("ClassifiedUpdatablePackages"));
    QVariant v = mess.arguments().first();
    const QDBusArgument arg = v.value<QDBusVariant>().variant().value<QDBusArgument>();
    LastoreUpdatePackagesInfo packagesInfo;
    arg >> packagesInfo;
    qCDebug(logCommon) << "Got packages info with" << packagesInfo.size() << "categories";
    return packagesInfo;
}

QString UpdateDBusProxy::GetCheckIntervalAndTime()
{
    qCDebug(logCommon) << "Getting check interval and time";
    QList<QVariant> argumentList;
    QDBusMessage reply =
            m_updateInter->callWithArgumentList(QDBus::Block,
                                                QStringLiteral("GetCheckIntervalAndTime"),
                                                argumentList);
    if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().count() == 2) {
        QString checkTime = qdbus_cast<QString>(reply.arguments().at(1));
        qCDebug(logCommon) << "Check time:" << checkTime;
        return checkTime;
    } else if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(logCommon) << "GetCheckIntervalAndTime failed: " << reply.errorMessage();
    }
    return {};
}

bool UpdateDBusProxy::autoDownloadUpdates()
{
    return qvariant_cast<bool>(m_updateInter->property("AutoDownloadUpdates"));
}

bool UpdateDBusProxy::autoInstallUpdates()
{
    return qvariant_cast<bool>(m_updateInter->property("AutoInstallUpdates"));
}

qulonglong UpdateDBusProxy::autoInstallUpdateType()
{
    return qvariant_cast<qulonglong>(m_updateInter->property("AutoInstallUpdateType"));
}

bool UpdateDBusProxy::autoCheckUpdates()
{
    return qvariant_cast<bool>(m_updateInter->property("AutoCheckUpdates"));
}

void UpdateDBusProxy::SetAutoCheckUpdates(bool in0)
{
    qCDebug(logCommon) << "Setting auto check updates:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    m_updateInter->asyncCallWithArgumentList(QStringLiteral("SetAutoCheckUpdates"), argumentList);
}

void UpdateDBusProxy::SetAutoDownloadUpdates(bool in0)
{
    qCDebug(logCommon) << "Setting auto download updates:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    m_updateInter->asyncCallWithArgumentList(QStringLiteral("SetAutoDownloadUpdates"),
                                             argumentList);
}

void UpdateDBusProxy::setAutoInstallUpdates(bool value)
{
    qCDebug(logCommon) << "Setting auto install updates property:" << value;
    m_updateInter->setProperty("AutoInstallUpdates", QVariant::fromValue(value));
}

void UpdateDBusProxy::SetMirrorSource(const QString &in0)
{
    qCDebug(logCommon) << "Setting mirror source:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    m_updateInter->asyncCallWithArgumentList(QStringLiteral("SetMirrorSource"), argumentList);
}

bool UpdateDBusProxy::autoClean()
{
    return qvariant_cast<bool>(m_managerInter->property("AutoClean"));
}

quint64 UpdateDBusProxy::updateMode()
{
    return qvariant_cast<quint64>(m_managerInter->property("UpdateMode"));
}

void UpdateDBusProxy::setUpdateMode(quint64 value)
{
    qCDebug(logCommon) << "Setting update mode:" << value;
    m_managerInter->setProperty("UpdateMode", QVariant::fromValue(value));
}

QList<QDBusObjectPath> UpdateDBusProxy::jobList()
{
    return qvariant_cast<QList<QDBusObjectPath>>(m_managerInter->property("JobList"));
}

QString UpdateDBusProxy::updateStatus()
{
    return qvariant_cast<QString>(m_managerInter->property("UpdateStatus"));
}

bool UpdateDBusProxy::immutableAutoRecovery()
{
    return qvariant_cast<bool>(m_managerInter->property("ImmutableAutoRecovery"));
}

QString UpdateDBusProxy::hardwareId()
{
    return qvariant_cast<QString>(m_managerInter->property("HardwareId"));
}

quint64 UpdateDBusProxy::checkUpdateMode()
{
    return qvariant_cast<quint64>(m_managerInter->property("CheckUpdateMode"));
}

void UpdateDBusProxy::setCheckUpdateMode(quint64 value)
{
    qCDebug(logCommon) << "Setting check update mode:" << value;
    m_managerInter->setProperty("CheckUpdateMode", QVariant::fromValue(value));
}

QString UpdateDBusProxy::idleDownloadConfig()
{
    return qvariant_cast<QString>(m_updateInter->property("IdleDownloadConfig"));
}

QString UpdateDBusProxy::downloadSpeedLimitConfig()
{
    return qvariant_cast<QString>(m_updateInter->property("DownloadSpeedLimitConfig"));
}

bool UpdateDBusProxy::p2pUpdateEnable()
{
    return qvariant_cast<bool>(m_updateInter->property("P2PUpdateEnable"));
}

QDBusPendingCall UpdateDBusProxy::CanRollback()
{
    qCDebug(logCommon) << "Calling CanRollback async";
    return m_managerInter->asyncCall(QStringLiteral("CanRollback"));
}

QDBusPendingCall UpdateDBusProxy::ConfirmRollback(bool confirm)
{
    qCDebug(logCommon) << "Calling ConfirmRollback with confirm:" << confirm;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(confirm);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("ConfirmRollback"), argumentList);
}

QDBusPendingCall UpdateDBusProxy::Poweroff(bool reboot)
{
    qCDebug(logCommon) << "Calling Poweroff with reboot:" << reboot;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(reboot);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("PowerOff"), argumentList);
}

QDBusPendingReply<QDBusObjectPath> UpdateDBusProxy::UpdateSource()
{
    qCDebug(logCommon) << "Calling UpdateSource async";
    QList<QVariant> argumentList;
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("UpdateSource"), argumentList);
}

void UpdateDBusProxy::CleanJob(const QString &in0)
{
    qCDebug(logCommon) << "Calling CleanJob for job:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    m_managerInter->asyncCallWithArgumentList(QStringLiteral("CleanJob"), argumentList);
}

void UpdateDBusProxy::SetAutoClean(bool in0)
{
    qCDebug(logCommon) << "Setting auto clean:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    m_managerInter->asyncCallWithArgumentList(QStringLiteral("SetAutoClean"), argumentList);
}

void UpdateDBusProxy::StartJob(const QString &in0)
{
    qCDebug(logCommon) << "Starting job:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    m_managerInter->asyncCallWithArgumentList(QStringLiteral("StartJob"), argumentList);
}

void UpdateDBusProxy::PauseJob(const QString &in0)
{
    qCDebug(logCommon) << "Pausing job:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    m_managerInter->asyncCallWithArgumentList(QStringLiteral("PauseJob"), argumentList);
}

QDBusPendingReply<QDBusObjectPath> UpdateDBusProxy::InstallPackage(const QString &jobname,
                                                                   const QString &packages)
{
    qCDebug(logCommon) << "Installing package with job:" << jobname << "packages:" << packages;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(jobname) << QVariant::fromValue(packages);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("InstallPackage"),
                                                     argumentList);
}

QDBusPendingReply<QDBusObjectPath> UpdateDBusProxy::RemovePackage(const QString &jobname,
                                                                  const QString &packages)
{
    qCDebug(logCommon) << "Removing package with job:" << jobname << "packages:" << packages;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(jobname) << QVariant::fromValue(packages);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("RemovePackage"), argumentList);
}

QDBusPendingReply<QList<QDBusObjectPath>> UpdateDBusProxy::ClassifiedUpgrade(qulonglong in0)
{
    qCDebug(logCommon) << "Starting classified upgrade with types:" << in0;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("ClassifiedUpgrade"),
                                                     argumentList);
}

QDBusPendingReply<qlonglong> UpdateDBusProxy::PackagesDownloadSize(const QStringList &in0)
{
    qCDebug(logCommon) << "Getting download size for" << in0.size() << "packages";
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(in0);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("PackagesDownloadSize"),
                                                     argumentList);
}

QDBusPendingReply<bool> UpdateDBusProxy::PackageExists(const QString &pkgid)
{
    qCDebug(logCommon) << "Checking if package exists:" << pkgid;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(pkgid);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("PackageExists"), argumentList);
}

QDBusPendingReply<QDBusObjectPath> UpdateDBusProxy::DistUpgrade()
{
    qCDebug(logCommon) << "Starting full distribution upgrade";
    return m_managerInter->asyncCall(QStringLiteral("DistUpgrade"));
}

QDBusPendingReply<QDBusObjectPath> UpdateDBusProxy::DistUpgradePartly(int updateTypes, bool doBackup)
{
    qCDebug(logCommon) << "Starting partial distribution upgrade, types:" << updateTypes << "backup:" << doBackup;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(updateTypes) << QVariant::fromValue(doBackup);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("DistUpgradePartly"),argumentList);
}

QDBusPendingReply<void> UpdateDBusProxy::SetDownloadSpeedLimit(const QString& config)
{
    qCDebug(logCommon) << "Setting download speed limit config:" << config;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(config);
    return m_updateInter->asyncCallWithArgumentList(QStringLiteral("SetDownloadSpeedLimit"), argumentList);
}

QDBusPendingReply<qlonglong> UpdateDBusProxy::QueryAllSizeWithSource(int updatetype)
{
    qCDebug(logCommon) << "Querying size for update type:" << updatetype;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(updatetype);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("QueryAllSizeWithSource"), argumentList);
}

QDBusPendingReply<QString> UpdateDBusProxy::GetUpdateLogs(int updateType)
{
    qCDebug(logCommon) << "Getting update logs for type:" << updateType;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(updateType);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("GetUpdateLogs"), argumentList);
}

QDBusPendingReply<void> UpdateDBusProxy::SetIdleDownloadConfig(const QString& config)
{
    qCDebug(logCommon) << "Setting idle download config:" << config;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(config);
    return m_updateInter->asyncCallWithArgumentList(QStringLiteral("SetIdleDownloadConfig"), argumentList);
}

QDBusPendingReply<QDBusObjectPath> UpdateDBusProxy::PrepareDistUpgradePartly(int updateMode)
{
    qCDebug(logCommon) << "Preparing partial distribution upgrade with mode:" << updateMode;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(updateMode);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("PrepareDistUpgradePartly"), argumentList);
}

QDBusPendingReply<QDBusObjectPath> UpdateDBusProxy::fixError(const QString &errorType)
{
    qCDebug(logCommon) << "Fixing error of type:" << errorType;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(errorType);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("FixError"), argumentList);
}

QDBusPendingCall UpdateDBusProxy::CheckUpgrade(int checkMode, int checkOrder)
{
    qCDebug(logCommon) << "Checking upgrade with mode:" << checkMode << "order:" << checkOrder;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(checkMode) << QVariant::fromValue(checkOrder);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("CheckUpgrade"), argumentList);
}

QDBusPendingReply<void> UpdateDBusProxy::GetUpdateDetails(int fd, bool realtime)
{
    qCDebug(logCommon) << "Getting update details, fd:" << fd << "realtime:" << realtime;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(QDBusUnixFileDescriptor(fd));
    argumentList << QVariant::fromValue(realtime);
    return m_managerInter->asyncCallWithArgumentList(QStringLiteral("GetUpdateDetails"), argumentList);
}

bool UpdateDBusProxy::onBattery()
{
    return qvariant_cast<bool>(m_powerInter->property("OnBattery"));
}

BatteryPercentageInfo UpdateDBusProxy::batteryPercentage()
{
    qCDebug(logCommon) << "Getting battery percentage info";
    QDBusInterface powerInter(m_powerInter->service(),
                              m_powerInter->path(),
                              PropertiesInterface,
                              m_powerInter->connection());
    QDBusMessage mess = powerInter.call(QStringLiteral("Get"),
                                        m_powerInter->interface(),
                                        QStringLiteral("BatteryPercentage"));
    QVariant v = mess.arguments().first();
    const QDBusArgument arg = v.value<QDBusVariant>().variant().value<QDBusArgument>();
    BatteryPercentageInfo packagesInfo;
    arg >> packagesInfo;
    qCDebug(logCommon) << "Battery percentage info retrieved";
    return packagesInfo;
}

void UpdateDBusProxy::commit(const QString &commitDate)
{
    qCDebug(logCommon) << "Committing atomic upgrade with date:" << commitDate;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(commitDate);
    m_atomicUpgradeInter->asyncCallWithArgumentList(QStringLiteral("Commit"), argumentList);
}

bool UpdateDBusProxy::atomBackupIsRunning()
{
    return qvariant_cast<bool>(m_atomicUpgradeInter->property("Running"));
}

bool UpdateDBusProxy::enable() const
{
    return qvariant_cast<bool>(m_smartMirrorInter->property("Enable"));
}

void UpdateDBusProxy::SetEnable(bool enable)
{
    qCDebug(logCommon) << "Setting smart mirror enable:" << enable;
    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(enable);
    m_smartMirrorInter->asyncCallWithArgumentList(QStringLiteral("SetEnable"), argumentList);
}

QString UpdateDBusProxy::CurrentUser()
{
    return QDBusPendingReply<QString>(m_lockServiceInter->asyncCall(QStringLiteral("CurrentUser")));
}

void UpdateDBusProxy::Restart()
{
    qCInfo(logCommon) << "Calling system restart";
    m_shutdownFrontInter->asyncCall(QStringLiteral("Restart"));
}
void UpdateDBusProxy::UpdateAndReboot()
{
    qCInfo(logCommon) << "Calling update and reboot";
    m_shutdownFrontInter->asyncCall(QStringLiteral("UpdateAndReboot"));
}
void UpdateDBusProxy::UpdateAndShutdown()
{
    qCInfo(logCommon) << "Calling update and shutdown";
    m_shutdownFrontInter->asyncCall(QStringLiteral("UpdateAndShutdown"));
}