// // SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updateassistant.h"

UpdateAssistant::UpdateAssistant(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent)
    , d_ptr(new UpdateAssistantPrivate)
{
    QDBusConnection::systemBus().connect("org.deepin.upgradedelivery", "/org/deepin/upgradedelivery", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(onPropertyChanged(QString, QVariantMap, QStringList)));
}

UpdateAssistant::~UpdateAssistant()
{
}

void UpdateAssistant::onPropertyChanged(const QString& interfaceName,
                                   const QVariantMap& changedProperties,
                                   const QStringList& invalidatedProperties)
{
    if (interfaceName != staticInterfaceName())
        return;

    if (changedProperties.contains("UploadLimitSpeed")) {
        Q_EMIT UploadLimitSpeedChanged(uploadLimitSpeed());
    }
    if (changedProperties.contains("DownloadLimitSpeed")) {
        Q_EMIT DownloadLimitSpeedChanged(downloadLimitSpeed());
    }

    return;
}

void UpdateAssistant::CallQueued(const QString &callName, const QList<QVariant> &args)
{
    if (d_ptr->m_waittingCalls.contains(callName)) {
        d_ptr->m_waittingCalls[callName] = args;
        return;
    }
    if (d_ptr->m_processingCalls.contains(callName)) {
        d_ptr->m_waittingCalls.insert(callName, args);
    } else {
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(asyncCallWithArgumentList(callName, args));
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &UpdateAssistant::onPendingCallFinished);
        d_ptr->m_processingCalls.insert(callName, watcher);
    }
}

void UpdateAssistant::onPendingCallFinished(QDBusPendingCallWatcher *w)
{
    if (!w)
        return;
    w->deleteLater();
    const auto callName = d_ptr->m_processingCalls.key(w);
    Q_ASSERT(!callName.isEmpty());
    if (callName.isEmpty())
        return;
    d_ptr->m_processingCalls.remove(callName);
    if (!d_ptr->m_waittingCalls.contains(callName))
        return;
    const auto args = d_ptr->m_waittingCalls.take(callName);
    CallQueued(callName, args);
}