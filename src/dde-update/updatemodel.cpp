// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef __UPDATEMODEL_H__
#define __UPDATEMODEL_H__

#include "updatemodel.h"

#include <QDebug>
#include <QMetaEnum>
#include <QList>
#include <QMap>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logUpdateModal)

UpdateModel::UpdateModel(QObject *parent)
    : QObject{parent}
    , m_updateAvailable(false)
    , m_updateStatus(UpdateStatus::Default)
    , m_jobProgress(0)
    , m_isBackupConfigValid(true)
    , m_isUpdating(false)
    , m_isReboot(false)
    , m_updateMode(0)
    , m_doUpgrade(false)
    , m_checkStatus(ReadToCheck)
    , m_checkSystemStage(CSS_None)
{
    qCDebug(logUpdateModal) << "Initialize UpdateModel";
}

UpdateModel* UpdateModel::instance()
{
    qCDebug(logUpdateModal) << "Getting UpdateModel instance";
    static UpdateModel *updateModelInstance = nullptr;
    // 对于单线程来讲，安全性足够
    if (!updateModelInstance) {
        qCDebug(logUpdateModal) << "Creating new UpdateModel instance";
        updateModelInstance = new UpdateModel();
    }

    return updateModelInstance;
}

void UpdateModel::setUpdateStatus(UpdateModel::UpdateStatus status)
{
    qCInfo(logUpdateModal) << "Set update status:" << status << "current:" << m_updateStatus;
    if (m_updateStatus == status)
        return;

    m_updateStatus = status;
    Q_EMIT updateStatusChanged(m_updateStatus);
}

void UpdateModel::setJobProgress(double progress)
{
    qCDebug(logUpdateModal) << "Set job progress:" << progress << "current:" << m_jobProgress;
    if (qFuzzyCompare(progress, m_jobProgress))
        return;

    m_jobProgress = progress;
    Q_EMIT JobProgressChanged(m_jobProgress);
}

void UpdateModel::setUpdateError(UpdateError error)
{
    qCWarning(logUpdateModal) << "Set update error:" << error << "current:" << m_updateError;
    if (m_updateError == error)
        return;

    m_updateError = error;
}

QString UpdateModel::updateErrorToString(UpdateError error)
{
    qCDebug(logUpdateModal) << "Converting error to string:" << error;
    if (error == UpdateError::DependenciesBrokenError)
        return "dependenciesBroken";

    if (error == UpdateError::DpkgInterrupted)
        return "dpkgInterrupted";

    qCDebug(logUpdateModal) << "Unknown error, returning empty string";
    return "";
}

QPair<QString, QString> UpdateModel::updateErrorMessage(UpdateError error)
{
    qCDebug(logUpdateModal) << "Getting error message for error:" << error;
    static const QMap<UpdateError, QPair<QString, QString>> ErrorMessage = {
        {UpdateError::UnKnown, qMakePair(tr("Update failed"), tr("Unknown error"))},
        {UpdateError::CanNotBackup, qMakePair(tr("Backup failed"), tr("Unable to perform system backup. If you continue the updates, you cannot roll back to the old system later."))},
        {UpdateError::BackupNoSpace, qMakePair(tr("Backup failed"), QString(tr("Insufficient disk space. Please clean up your disk and try again.")))},
        {UpdateError::BackupInterfaceError, qMakePair(tr("Backup failed"), tr("Failed to connect to backup services. Please check and retry again."))},
        {UpdateError::BackupFailedUnknownReason, qMakePair(tr("Backup failed"), tr("Unknown error"))},
        {UpdateError::UpdateInterfaceError, qMakePair(tr("Update failed"), tr("Failed to connect to update services. Please check and retry again."))},
        {UpdateError::InstallNoSpace, qMakePair(tr("Update failed"), tr("Insufficient disk space. Please clean up your disk and try again."))},
        {UpdateError::DependenciesBrokenError, qMakePair(tr("Update failed"), tr("Dependency error"))},
        {UpdateError::DpkgInterrupted, qMakePair(tr("Update failed"), tr("DPKG error"))}
    };

    if (ErrorMessage.contains(error)) {
        const auto& result = ErrorMessage.value(error);
        qCDebug(logUpdateModal) << "Found error message:" << result.first;
        return result;
    }

    qCWarning(logUpdateModal) << "Unknown error, returning empty pair";
    return qMakePair(QString(), QString());
}

void UpdateModel::setLastErrorLog(const QString &log)
{
    qCWarning(logUpdateModal) << "Set last error log, length:" << log.length();
    m_lastErrorLog = log;
}

void UpdateModel::setBackupConfigValidation(bool valid)
{
    qCDebug(logUpdateModal) << "Set backup config validation:" << valid;
    m_isBackupConfigValid = valid;
}

QString UpdateModel::updateActionText(UpdateAction action)
{
    qCDebug(logUpdateModal) << "Getting action text for:" << action;
    static const QMap<UpdateAction, QString> ActionsText = {
        {None, tr("")},
        {DoBackupAgain, tr("Back Up Again")},
        {ExitUpdating, tr("Abort")},
        {ContinueUpdating, tr("Proceed to Update")},
        {Reboot, tr("Reboot")},
        {ShutDown, tr("Shut Down")},
        {EnterDesktop, tr("Go to Desktop")},
    };

    return ActionsText.value(action);
}

void UpdateModel::setCheckStatus(CheckStatus status)
{
    qCDebug(logUpdateModal) << "Set check status:" << status;
    if (m_checkStatus != status) {
        m_checkStatus = status;
        Q_EMIT checkStatusChanged(m_checkStatus);
    }
}

void UpdateModel::setUpdateLog(const QString &log)
{
    qCDebug(logUpdateModal) << "Set update log" << log;
    m_updateLog = log;
    Q_EMIT updateLogChanged(m_updateLog);
}

void UpdateModel::appendUpdateLog(const QString &log)
{
    qCDebug(logUpdateModal) << "Append update log:" << log;
    m_updateLog += log;
    Q_EMIT updateLogAppended(log);
}

#endif // __UPDATEMODEL_H__
