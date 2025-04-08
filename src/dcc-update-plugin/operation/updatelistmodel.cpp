// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updatelistmodel.h"

UpdateListModel::UpdateListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int UpdateListModel::rowCount(const QModelIndex &parent) const
{
    // For list models only the root node (an invalid parent) should return the list's size. For all
    // other (valid) parents, rowCount() should return 0 so that it does not become a tree model.
    if (parent.isValid())
        return 0;

    // FIXME: Implement me!
    return m_updateLists.count();
}

QVariant UpdateListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    UpdateItemInfo* data = m_updateLists[index.row()];

    switch (role) {
    case Title:
        return data->name();
    case TitleDescription:
        return data->explain();
    case UpdateLog:
        return "";
    case ReleaseTime:
        return data->updateTime();
    case Checked:
        return data->isChecked();
    case UpdateStatus:
        return data->updateStatus();
    default:
        break;
    }
     return QVariant();
}

void UpdateListModel::addUpdateData(UpdateItemInfo* itemData)
{
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    // FIXME: Implement me!
    m_updateLists.append(itemData);
    endInsertRows();

    emit visibilityChanged();
}

bool UpdateListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();

    emit visibilityChanged();
    return true;
}

bool UpdateListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();

    emit visibilityChanged();
    return true;
}

void UpdateListModel::clearAllData()
{
    beginResetModel();
    m_updateLists.clear();
    endResetModel();

    emit visibilityChanged();
}
