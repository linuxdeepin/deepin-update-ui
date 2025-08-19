// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updatestatus.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

UpdateStatus::UpdateStatus(QObject *parent)
    : QObject(parent)
    , m_ABStatus("")
    , m_ABError("")
    , m_TriggerBackingUpType(0)
    , m_TriggerBackupFailedType(0)
    , m_statusData(new UpdateStatusData(this))
{
    qCDebug(logDccUpdatePlugin) << "Initialize UpdateStatus";
}

UpdateStatus::UpdateStatus(const UpdateStatus &other)
    : QObject(other.parent()),
      m_ABStatus(other.m_ABStatus),
      m_ABError(other.m_ABError),
      m_TriggerBackingUpType(other.m_TriggerBackingUpType),
      m_TriggerBackupFailedType(other.m_TriggerBackupFailedType),
      m_statusData(other.m_statusData ? new UpdateStatusData(*other.m_statusData) : nullptr)
{
    qCDebug(logDccUpdatePlugin) << "Copy construct UpdateStatus";
}

UpdateStatus::~UpdateStatus()
{
    qCDebug(logDccUpdatePlugin) << "Destroying UpdateStatus";
    if (m_statusData) {
        delete m_statusData;
        m_statusData = nullptr;
    }
}

QString UpdateStatus::ABStatus() const
{
    return m_ABStatus;
}

QString UpdateStatus::ABError() const
{
    return m_ABError;
}

int UpdateStatus::TriggerBackingUpType() const
{
    return m_TriggerBackingUpType;
}

int UpdateStatus::TriggerBackupFailedType() const
{
    return m_TriggerBackupFailedType;
}

void UpdateStatus::setABStatus(const QString &ABStatus)
{
    qCDebug(logDccUpdatePlugin) << "Set AB status: " << ABStatus << "old value: " << m_ABStatus;
    if (m_ABStatus != ABStatus) {
        m_ABStatus = ABStatus;
        Q_EMIT ABStatusChanged(m_ABStatus);
    }
}

void UpdateStatus::setABError(const QString &ABError)
{
    qCDebug(logDccUpdatePlugin) << "Set AB error:" << ABError << "old value: " << m_ABError;
    if (m_ABError != ABError) {
        m_ABError = ABError;
        Q_EMIT ABErrorChanged(m_ABError);
    }
}

void UpdateStatus::setTriggerBackingUpType(int TriggerBackingUpType)
{
    qCDebug(logDccUpdatePlugin) << "Set trigger backing up type:" << TriggerBackingUpType << "old value: " << m_TriggerBackingUpType;
    if (m_TriggerBackingUpType != TriggerBackingUpType) {
        m_TriggerBackingUpType = TriggerBackingUpType;
        Q_EMIT TriggerBackingUpTypeChanged(m_TriggerBackingUpType);
    }
}

void UpdateStatus::setTriggerBackupFailedType(int TriggerBackupFailedType)
{
    qCDebug(logDccUpdatePlugin) << "Set trigger backup failed type:" << TriggerBackupFailedType << "old value: " << m_TriggerBackupFailedType;
    if (m_TriggerBackupFailedType != TriggerBackupFailedType) {
        m_TriggerBackupFailedType = TriggerBackupFailedType;
        Q_EMIT TriggerBackupFailedTypeChanged(m_TriggerBackupFailedType);
    }
}

UpdateStatusData* UpdateStatus::statusData() const
{
    return m_statusData;
}
