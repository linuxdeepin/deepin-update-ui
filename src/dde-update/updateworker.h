// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UPDATECTRL_H
#define UPDATECTRL_H

#include "updatemodel.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QtDBus>

#include <com_deepin_abrecovery.h>
#include <com_deepin_lastore_job.h>
#include <com_deepin_lastore_jobmanager.h>
#include <com_deepin_lastore_updater.h>
#include <com_deepin_system_systempower.h>
#include <org_freedesktop_login1.h>
#include <org_freedesktop_dbus.h>

// using UpdateInter = com::deepin::lastore::Updater;
// using JobInter = com::deepin::lastore::Job;
// using ManagerInter = com::deepin::lastore::Manager;
// using RecoveryInter = com::deepin::ABRecovery;
// using PowerInter = com::deepin::system::Power;
// using Login1Manager = org::freedesktop::login1::Manager;
// using DBusManager = org::freedesktop::DBus;

class UpdateWorker : public QObject
{
    Q_OBJECT

public:
    static UpdateWorker *instance() {
        static UpdateWorker *pIns = nullptr;
        if (!pIns) {
            pIns = new UpdateWorker();
        }
        return pIns;
    };

    void init();
    void doDistUpgrade(bool doBackup);
    void doDistUpgradeIfCanBackup();
    void doCheckSystem(int updateMode, UpdateModel::CheckSystemStage stage);
    void doAction(UpdateModel::UpdateAction action);
    void startUpdateProgress();
    bool checkPower();
    void enableShortcuts(bool enable);
    void doPowerAction(bool reboot);
    void forceReboot(bool reboot);

Q_SIGNALS:
    void requestExitUpdating();

private:
    explicit UpdateWorker(QObject *parent = nullptr);
    UpdateModel::UpdateError analyzeJobErrorMessage(QString jobDescription);
    bool fixError(UpdateModel::UpdateError error, const QString &description, UpdateModel::UpdateStatus status);
    void checkStatusAfterSessionActive();
    bool syncStartService(DBusExtendedAbstractInterface *interface);
    void createDistUpgradeJob(const QString& jobPath);
    void createCheckSystemJob(const QString& jobPath);
    void cleanLaStoreJob(QPointer<JobInter> dbusJob);
    void getUpdateOption();

private slots:
    void onJobListChanged(const QList<QDBusObjectPath> &jobs);
    void onDistUpgradeStatusChanged(const QString &status);
    void onCheckSystemStatusChanged(const QString &status);

private:
    PowerInter *m_powerInter;
    ManagerInter *m_managerInter;
    RecoveryInter *m_abRecoveryInter;
    Login1Manager* m_login1Manager;
    QPointer<JobInter> m_distUpgradeJob; // 更新job
    QPointer<JobInter> m_fixErrorJob; // 修复错误job
    QPointer<JobInter> m_checkSystemJob; // 修复错误job
    bool m_waitingToCheckSystem;
};

#endif // UPDATECTRL_H
