// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updatehistorymodel.h"

#include "updateloghelper.h"

#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

UpdateHistoryModel::UpdateHistoryModel(QObject *parent)
    : QAbstractListModel{parent}
{
    qCDebug(logDccUpdatePlugin) << "Initialize UpdateHistoryModel";
    qRegisterMetaType<HistoryItemDetail>("HistoryItemDetail");
    qRegisterMetaType<QList<HistoryItemDetail>>("QList<HistoryItemDetail>");
    refreshHistory();
}

void UpdateHistoryModel::refreshHistory()
{
    qCDebug(logDccUpdatePlugin) << "Refreshing update history from DBus";
    QDBusInterface managerInter("org.deepin.dde.Lastore1",
                                "/org/deepin/dde/Lastore1",
                                "org.deepin.dde.Lastore1.Manager",
                                QDBusConnection::systemBus());

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(managerInter.asyncCall("GetHistoryLogs"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
        qCDebug(logDccUpdatePlugin) << "GetHistoryLogs call finished";
        beginResetModel();
        if (!watcher->isError()) {
            QDBusPendingReply<QString> reply = watcher->reply();
            qCDebug(logDccUpdatePlugin) << "Processing history logs, data length:" << reply.value().length();
            m_data = UpdateLogHelper::ref().handleHistoryUpdateLog(reply.value());
        } else {
            qCWarning(logDccUpdatePlugin) << "GetHistoryLogs failed:" << watcher->error().message();
            m_data = UpdateLogHelper::ref().handleHistoryUpdateLog("{}");
        }
        watcher->deleteLater();
        endResetModel();
    });
}

int UpdateHistoryModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.count();
}

QVariant UpdateHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qCDebug(logDccUpdatePlugin) << "Invalid index requested";
        return QVariant();
    }
    const HistoryItemInfo &data = m_data[index.row()];
    switch (role) {
    case Type:
        return data.type;
    case Version:
        return data.version;
    case Summary:
        return data.summary;
    case Details: {
        QVariantList list;
        for (const auto &detail : data.details)
            list << QVariant::fromValue(detail);
        return list;
    }
    case UpgradeTime:
        return data.upgradeTime;
    case Expanded:
        return data.expanded;
    default:
        return QVariant();
    }
}

void UpdateHistoryModel::setExpanded(int index, bool expanded)
{
    if (index < 0 || index >= m_data.count()) {
        qCWarning(logDccUpdatePlugin) << "Invalid index for setExpanded:" << index;
        return;
    }
    if (m_data[index].expanded == expanded)
        return;

    m_data[index].expanded = expanded;
    const QModelIndex changedIndex = this->index(index);
    emit dataChanged(changedIndex, changedIndex, { Expanded });
}

void UpdateHistoryModel::collapseAll()
{
    for (int i = 0; i < m_data.count(); ++i) {
        if (!m_data[i].expanded)
            continue;

        m_data[i].expanded = false;
        emit dataChanged(this->index(i), this->index(i), { Expanded });
    }
}

QHash<int, QByteArray> UpdateHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Type] = "Type";
    roles[Version] = "Version";
    roles[Summary] = "Summary";
    roles[Details] = "Details";
    roles[UpgradeTime] = "UpgradeTime";
    roles[Expanded] = "expanded";
    return roles;
}
