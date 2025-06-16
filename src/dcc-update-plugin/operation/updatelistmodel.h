// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef UPDATELISTMODEL_H
#define UPDATELISTMODEL_H

#include "updateiteminfo.h"
#include "common.h"

#include <QAbstractListModel>

class UpdateListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool anyVisible READ anyVisible NOTIFY visibilityChanged)
    Q_PROPERTY(bool isUpdateEnable READ isUpdateEnable NOTIFY isUpdateEnableChanged)
    Q_PROPERTY(QString downloadSize READ downloadSize NOTIFY downloadSizeChanged)

public:
    enum updateRoles {
        Title = Qt::UserRole + 1,
        Version,
        TitleDescription,
        UpdateLog,
        ReleaseTime,
        Checked,
        UpdateStatus,
        IconName
    };

    explicit UpdateListModel(QObject *parent = nullptr);

    // Basic functionality:
    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addUpdateData(UpdateItemInfo *itemData);

    void clearAllData();

    Q_INVOKABLE void setChecked(int index, bool checked);

    Q_INVOKABLE int getAllUpdateType() const;

    QString getIconName(UpdateType type) const;

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[Title] = "title";
        roles[Version] = "version";
        roles[TitleDescription] = "titleDescription";
        roles[UpdateLog] = "updateLog";
        roles[ReleaseTime] = "releaseTime";
        roles[Checked] = "checked";
        roles[UpdateStatus] = "updateStatus";
        roles[IconName] = "iconName";
        return roles;
    }

    bool anyVisible() const;
    bool isUpdateEnable() const;
    QString downloadSize() const;

    Q_INVOKABLE UpdateType getUpdateType(int index) const;

public Q_SLOTS:
    void refreshDownloadSize();

signals:
    void visibilityChanged();
    void isUpdateEnableChanged();
    void downloadSizeChanged();

private:
    QList<UpdateItemInfo *> m_updateLists;
    QString m_downloadSize;
};
#endif // UPDATELISTMODEL_H
