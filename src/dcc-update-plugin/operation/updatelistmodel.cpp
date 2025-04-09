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
    case IconName:
        return getIconName(data->updateType());
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
    emit downloadSizeChanged();
}

bool UpdateListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();

    emit visibilityChanged();
    emit downloadSizeChanged();
    return true;
}

bool UpdateListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();

    emit visibilityChanged();
    emit downloadSizeChanged();
    return true;
}

QString UpdateListModel::getIconName(UpdateType type) const
{
    QString path = "qrc:/icons/deepin/builtin/icons/";
    switch (type) {
    case Invalid:
    case UnknownUpdate:
        return path + "dcc_unknown_update.svg";
    case SystemUpdate:
        return path + "dcc_system_update.svg";
    case AppStoreUpdate:
        return path + "dcc_app_update.svg";
    case SecurityUpdate:
    case OnlySecurityUpdate:
        return path + "dcc_safe_update";
    }

    return path + "dcc_unknown_update.svg";
}

double UpdateListModel::downloadSize() const
{
    int size = 0;
    for (int i = 0; i < m_updateLists.size(); ++i) {
        qDebug() <<"========== " << m_updateLists[i]->downloadSize();
        size += m_updateLists[i]->downloadSize();
    }

    return size;
}

void UpdateListModel::clearAllData()
{
    beginResetModel();
    m_updateLists.clear();
    endResetModel();

    emit visibilityChanged();
}

void UpdateListModel::setChecked(int index, bool checked)
{
    if (index >= 0 && index < m_updateLists.count()) {
        m_updateLists[index]->setIsChecked(checked);

        QModelIndex changedIndex = this->index(index);
        emit dataChanged(changedIndex, changedIndex, {});
    }
}

int UpdateListModel::getAllUpdateType() const
{
    int updateType = 0;
    for (int i = 0; i < m_updateLists.count(); ++i) {
        if (m_updateLists[i]->isChecked()) {
            updateType |= m_updateLists[i]->updateType();
        }
    }
    return updateType;
}
