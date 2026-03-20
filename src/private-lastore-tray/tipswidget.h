// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef TIPSWIDGET_H
#define TIPSWIDGET_H

#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include "common/dbus/updatedbusproxy.h"
#include "common/dbus/updatejobdbusproxy.h"

inline QString checkHasSystemUpdate(const QString& updateStatus)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(updateStatus.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return "";
    }
    if (!doc.isObject()) {
        qWarning() << "JSON is not an object";
        return "";
    }
    QJsonObject rootObj = doc.object();
    QJsonObject updateStatusObj = rootObj.value("UpdateStatus").toObject();
    QString systemUpgradeStatus = updateStatusObj.value("system_upgrade").toString();
    return systemUpgradeStatus;
}

class TipsWidget : public QFrame
{
    Q_OBJECT
    enum ShowType
    {
        SingleLine,
        MultiLine
    };
public:
    explicit TipsWidget(QWidget *parent = nullptr);

    void setText(const QString &text);
    void setTextList(const QStringList &textList);
    void refreshContent();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    bool checkShutdownUpdate();
    bool checkRegularlyUpdate();
    QString regulateSpeed();

private slots:
    void onDownloadLimitChanged(bool value);
    void onRefreshJobList(const QList<QDBusObjectPath> &jobs);
    void onUpdatePropertiesChanged(const QString& interfaceName,
                                   const QVariantMap& changedProperties,
                                   const QStringList& invalidatedProperties);
    void onSetUpdateProgress(double progress);
    void onSetBackUpProgress(double progress);

private:
    UpdateDBusProxy *m_managerInter = nullptr;
    UpdateJobDBusProxy *m_updateJobInter = nullptr;
    UpdateJobDBusProxy *m_backupJobInter = nullptr;
    QString m_text;
    QStringList m_textList;
    ShowType m_type;
    qulonglong m_speed = 0;
    QString m_proto = "";
    bool m_downloadLimitOnChanging = false;
    double m_updateProgress = 0.0;
    double m_backupProgress = 0.0;
};

#endif // TIPSWIDGET_H
