// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updatelistmodel.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

UpdateListModel::UpdateListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qCDebug(logDccUpdatePlugin) << "Initialize UpdateListModel";
}

int UpdateListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_updateLists.count();
}

QVariant UpdateListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qCDebug(logDccUpdatePlugin) << "Invalid index requested";
        return QVariant();
    }

    if (index.row() >= m_updateLists.size()) {
        qCWarning(logDccUpdatePlugin) << "Index out of range:" << index.row() << "max:" << m_updateLists.size();
        return QVariant();
    }

    UpdateItemInfo* data = m_updateLists[index.row()];
    qCDebug(logDccUpdatePlugin) << "Getting data for role:" << role << "item:" << data->name();

    switch (role) {
    case Title:
        return data->name();
    case Version:
        return data->currentVersion();
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
        qCDebug(logDccUpdatePlugin) << "Unknown role requested:" << role;
        break;
    }
     return QVariant();
}

void UpdateListModel::addUpdateData(UpdateItemInfo* itemData)
{
    qCDebug(logDccUpdatePlugin) << "Adding update data:" << itemData->name() << "type:" << itemData->updateType();
    int row = rowCount();
    beginInsertRows(QModelIndex(), row, row);
    m_updateLists.append(itemData);
    connect(itemData, &UpdateItemInfo::downloadSizeChanged, this, &UpdateListModel::refreshDownloadSize);
    endInsertRows();

    refreshDownloadSize();
    emit visibilityChanged();
    qCDebug(logDccUpdatePlugin) << "Update data added, total items:" << m_updateLists.size();
}

QString UpdateListModel::getIconName(UpdateType type) const
{
    qCDebug(logDccUpdatePlugin) << "Getting icon name for type:" << type;
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

void UpdateListModel::refreshDownloadSize()
{
    qCDebug(logDccUpdatePlugin) << "Refreshing download size calculation";
    double downloadSize = 0;
    for (int i = 0; i < m_updateLists.size(); ++i) {
        if (!m_updateLists[i]->isChecked()) {
            continue;
        }

        downloadSize += m_updateLists[i]->downloadSize();
    }

    const int oneGB = 1024 * 1024 * 1024;
    const int oneMB = 1024 * 1024;
    const int oneKB = 1024;

    QString sizeUnit;
    if (downloadSize >= oneGB) { // more than 1 GB
        sizeUnit = QString("%1G").arg(downloadSize /= oneGB, 0, 'f', 2);
    } else if (downloadSize >= oneMB) { // more than 1 MB
        sizeUnit = QString("%1MB").arg(downloadSize /= oneMB, 0, 'f', 2);
    } else if (downloadSize >= oneKB) { // more than 1 KB
        sizeUnit = QString("%1KB").arg(downloadSize /= oneKB, 0, 'f', 2);
    } else { // less than 1 KB
        sizeUnit = QString("%1B").arg(downloadSize, 0, 'f', 2);
    }

    if (qFuzzyCompare(downloadSize, 0)) {
        sizeUnit = QString("0B");
    }

    m_downloadSize = sizeUnit;
    emit downloadSizeChanged();
}

bool UpdateListModel::anyVisible() const
{
    return m_updateLists.count();
}

bool UpdateListModel::isUpdateEnable() const
{
    for (int i = 0; i < m_updateLists.count(); ++i) {
        if (m_updateLists[i]->isChecked()) {
            return true;
        }
    }
    return false;
}

QString UpdateListModel::downloadSize() const
{
    return m_downloadSize;
}

UpdateType UpdateListModel::getUpdateType(int index) const
{
    if (index >= 0 && index < m_updateLists.count()) {
        return m_updateLists[index]->updateType();
    }
    qCWarning(logDccUpdatePlugin) << "Invalid index" << index << "max:" << m_updateLists.count();
    return Invalid;
}

void UpdateListModel::clearAllData()
{
    qCDebug(logDccUpdatePlugin) << "Clearing all data, current count:" << m_updateLists.size();
    beginResetModel();
    m_updateLists.clear();
    endResetModel();

    emit visibilityChanged();
}

void UpdateListModel::setChecked(int index, bool checked)
{
    qCDebug(logDccUpdatePlugin) << "Setting checked state for index:" << index << "checked:" << checked;
    if (index >= 0 && index < m_updateLists.count()) {
        const QString itemName = m_updateLists[index]->name();
        m_updateLists[index]->setIsChecked(checked);
        qCDebug(logDccUpdatePlugin) << "Updated check state for item:" << itemName;

        QModelIndex changedIndex = this->index(index);
        emit dataChanged(changedIndex, changedIndex, { Checked });

        refreshDownloadSize();
        emit isUpdateEnableChanged();
    } else {
        qCWarning(logDccUpdatePlugin) << "Invalid index for setChecked:" << index;
    }
}

int UpdateListModel::getAllUpdateType() const
{
    qCDebug(logDccUpdatePlugin) << "Getting combined update type for all checked items";
    int updateType = 0;
    for (int i = 0; i < m_updateLists.count(); ++i) {
        if (m_updateLists[i]->isChecked()) {
            updateType |= m_updateLists[i]->updateType();
        }
    }
    return updateType;
}
