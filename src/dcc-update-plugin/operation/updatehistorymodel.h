// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef UPDATEHISTORYMODEL_H
#define UPDATEHISTORYMODEL_H

#include <QAbstractListModel>
#include <QObject>

class HistoryItemInfo;
class UpdateHistoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum updateRoles {
        Type = Qt::UserRole + 1,
        Summary,
        Details,
        UpgradeTime
    };

    explicit UpdateHistoryModel(QObject *parent = nullptr);

    Q_INVOKABLE void refreshHistory();
    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QList<HistoryItemInfo> m_data;
};

#endif // UPDATEHISTORYMODEL_H
