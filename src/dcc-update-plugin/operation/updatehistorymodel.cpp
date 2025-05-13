// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updatehistorymodel.h"

#include "updateloghelper.h"

#include <QDBusInterface>
#include <QDBusPendingReply>

UpdateHistoryModel::UpdateHistoryModel(QObject *parent)
    : QAbstractListModel{parent}
{
    qRegisterMetaType<HistoryItemDetail>("HistoryItemDetail");
    qRegisterMetaType<QList<HistoryItemDetail>>("QList<HistoryItemDetail>");
    refreshHistory();
}

void UpdateHistoryModel::refreshHistory()
{
    QDBusInterface managerInter("org.deepin.dde.Lastore1",
                                "/org/deepin/dde/Lastore1",
                                "org.deepin.dde.Lastore1.Manager",
                                QDBusConnection::systemBus());

    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(managerInter.asyncCall("GetHistoryLogs"), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
        beginResetModel();
        if (!watcher->isError()) {
            QDBusPendingReply<QString> reply = watcher->reply();
            m_data = UpdateLogHelper::ref().handleHistoryUpdateLog(reply.value());
        } else {
            m_data = UpdateLogHelper::ref().handleHistoryUpdateLog("{}");
            qWarning() << watcher->error().message();
        }
        watcher->deleteLater();
        endResetModel();
    });
}


int UpdateHistoryModel::rowCount(const QModelIndex &parent) const
{
    return m_data.count();
}

QVariant UpdateHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    const HistoryItemInfo &data = m_data[index.row()];
    switch (role) {
    case Type:
        return data.type;
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
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> UpdateHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Type] = "Type";
    roles[Summary] = "Summary";
    roles[Details] = "Details";
    roles[UpgradeTime] = "UpgradeTime";
    return roles;
}
