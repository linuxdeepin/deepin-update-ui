// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updatestatusdata.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

UpdateStatusData::UpdateStatusData(QObject *parent)
    : QObject{ parent },
      m_securityUpgrade(""),
      m_systemUpgrade(""),
      m_unknowUpgrade("")
{
    qCDebug(logDccUpdatePlugin) << "Initialize UpdateStatusData";
}

UpdateStatusData::UpdateStatusData(const UpdateStatusData &other)
    : QObject(other.parent()),
      m_securityUpgrade(other.m_securityUpgrade),
      m_systemUpgrade(other.m_systemUpgrade),
      m_unknowUpgrade(other.m_unknowUpgrade)
{
    qCDebug(logDccUpdatePlugin) << "Copy construct UpdateStatusData";
}

QString UpdateStatusData::securityUpgrade() const
{
    return m_securityUpgrade;
}

QString UpdateStatusData::systemUpgrade() const
{
    return m_systemUpgrade;
}

QString UpdateStatusData::unknowUpgrade() const
{
    return m_unknowUpgrade;
}

void UpdateStatusData::setSecurityUpgrade(const QString &securityUpgrade)
{
    qCDebug(logDccUpdatePlugin) << "Set security upgrade:" << securityUpgrade << "old value: " << m_securityUpgrade;
    if (m_securityUpgrade != securityUpgrade) {
        m_securityUpgrade = securityUpgrade;
        Q_EMIT securityUpgradeChanged(m_securityUpgrade);
    }
}

void UpdateStatusData::setSystemUpgrade(const QString &systemUpgrade)
{
    qCDebug(logDccUpdatePlugin) << "Set system upgrade:" << systemUpgrade << "old value: " << m_systemUpgrade;
    if (m_systemUpgrade != systemUpgrade) {
        m_systemUpgrade = systemUpgrade;
        Q_EMIT systemUpgradeChanged(m_systemUpgrade);
    }
}

void UpdateStatusData::setUnknowUpgrade(const QString &unknowUpgrade)
{
    qCDebug(logDccUpdatePlugin) << "Set unknown upgrade:" << unknowUpgrade << "old value: " << m_unknowUpgrade;
    if (m_unknowUpgrade != unknowUpgrade) {
        m_unknowUpgrade = unknowUpgrade;
        Q_EMIT unknowUpgradeChanged(m_unknowUpgrade);
    }
}
