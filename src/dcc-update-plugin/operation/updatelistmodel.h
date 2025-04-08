// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef UPDATELISTMODEL_H
#define UPDATELISTMODEL_H

#include "updateiteminfo.h"

#include <QAbstractListModel>

// struct UpdateItemData
// {
//     UpdateItemData() {}
//
//     QString title;
//     QString titleDescription;
//     QString updateLog;
//     QString releaseTime;
//
//     bool checked;
//
//     int updateStatus;
// };
//

class UpdateListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool anyVisible READ anyVisible NOTIFY visibilityChanged)

public:
    enum updateRoles {
        Title = Qt::UserRole + 1,
        TitleDescription,
        UpdateLog,
        ReleaseTime,
        Checked,
        UpdateStatus,
        Do
    };

    explicit UpdateListModel(QObject *parent = nullptr);

    // Basic functionality:
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addUpdateData(UpdateItemInfo *itemData);

    void clearAllData();

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[Title] = "title";
        roles[TitleDescription] = "titleDescription";
        roles[UpdateLog] = "updateLog";
        roles[ReleaseTime] = "releaseTime";
        roles[Checked] = "checked";
        roles[UpdateStatus] = "updateStatus";
        return roles;
    }

    bool anyVisible() const {
        return m_updateLists.count();
    }

signals:
    void visibilityChanged();

private:
    QList<UpdateItemInfo *> m_updateLists;
};
#endif // UPDATELISTMODEL_H
