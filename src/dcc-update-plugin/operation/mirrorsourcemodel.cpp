// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mirrorsourcemodel.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

MirrorSourceModel::MirrorSourceModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MirrorSourceModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_mirrors.count();
}

QVariant MirrorSourceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_mirrors.size())
        return QVariant();

    const MirrorData &item = m_mirrors[index.row()];

    switch (role) {
    case IdRole:
        return item.id;
    case NameRole:
        return item.name;
    case UrlRole:
        return item.url;
    case SpeedRole:
        return item.speed;
    default:
        break;
    }
    return QVariant();
}

void MirrorSourceModel::setMirrorList(const MirrorInfoList &list)
{
    beginResetModel();
    m_mirrors.clear();
    for (const MirrorInfo &info : list) {
        m_mirrors.append({info.m_id, info.m_name, info.m_url, SpeedStatus::Untested});
    }
    endResetModel();
}

void MirrorSourceModel::updateMirrorSpeed(const QString &mirrorId, int speed)
{
    for (int i = 0; i < m_mirrors.size(); ++i) {
        if (m_mirrors[i].id == mirrorId) {
            m_mirrors[i].speed = speed;
            QModelIndex changedIndex = index(i);
            Q_EMIT dataChanged(changedIndex, changedIndex, {SpeedRole});
            return;
        }
    }
}

void MirrorSourceModel::resetSpeedInfo()
{
    if (m_mirrors.isEmpty())
        return;

    for (auto &item : m_mirrors) {
        item.speed = SpeedStatus::Testing;
    }
    Q_EMIT dataChanged(index(0), index(m_mirrors.count() - 1), {SpeedRole});
}
