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

#include "common/dbus/updatedbusproxy.h"
#include "common/dbus/updatejobdbusproxy.h"
#include "common/logwatcherhelper.h"

using JobInter = UpdateJobDBusProxy;

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
    void exportUpdateLogFinished(bool success);

public slots:
    bool exportUpdateLog();

private:
    explicit UpdateWorker(QObject *parent = nullptr);
    UpdateModel::UpdateError analyzeJobErrorMessage(QString jobDescription);
    bool fixError(UpdateModel::UpdateError error, const QString &description, UpdateModel::UpdateStatus status);
    void checkStatusAfterSessionActive();
    bool syncStartService(const QString &serviceName);
    void createDistUpgradeJob(const QString& jobPath);
    void createCheckSystemJob(const QString& jobPath);
    void createBackupJob(const QString& jobPath);
    void cleanLaStoreJob(QPointer<JobInter> dbusJob);
    void getUpdateOption();

private slots:
    void onJobListChanged(const QList<QDBusObjectPath> &jobs);
    void onDistUpgradeStatusChanged(const QString &status);
    void onCheckSystemStatusChanged(const QString &status);
    void onBackupStatusChanged(const QString& value);

private:
    QPointer<JobInter> m_distUpgradeJob; // 更新job
    QPointer<JobInter> m_backupJob; // 备份job
    QPointer<JobInter> m_fixErrorJob; // 修复错误job
    QPointer<JobInter> m_checkSystemJob; // 修复错误job
    UpdateDBusProxy *m_dbusProxy;
    bool m_waitingToCheckSystem;
    LogWatcherHelper *m_logWatcherHelper;
    QTimer *m_waitServiceTimer = nullptr;
};

#endif // UPDATECTRL_H
