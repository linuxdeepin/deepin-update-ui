// // SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef UPDATEASSISTANT_H
#define UPDATEASSISTANT_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

class UpdateAssistantPrivate
{
public:
    UpdateAssistantPrivate() = default;

public:
    QMap<QString, QDBusPendingCallWatcher *> m_processingCalls;
    QMap<QString, QList<QVariant>> m_waittingCalls;
};

/*
 * Proxy class for interface org.deepin.updateassistant
 */
class UpdateAssistant: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.deepin.upgradedelivery"; }

public:
    UpdateAssistant(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);

    ~UpdateAssistant();

    Q_PROPERTY(QString UploadLimitSpeed READ uploadLimitSpeed NOTIFY UploadLimitSpeedChanged)
    inline QString uploadLimitSpeed() const
    { return qvariant_cast<QString>(internalPropGet("UploadLimitSpeed")); }

    Q_PROPERTY(QString DownloadLimitSpeed READ downloadLimitSpeed NOTIFY DownloadLimitSpeedChanged)
    inline QString downloadLimitSpeed() const
    { return qvariant_cast<QString>(internalPropGet("DownloadLimitSpeed")); }

public Q_SLOTS: // METHODS

    inline QDBusPendingReply<void> SetUploadRateLimit(int speed)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(speed);
        return asyncCallWithArgumentList(QStringLiteral("SetUploadRateLimit"), argumentList);
    }

    inline QDBusPendingReply<void> SetDownloadRateLimit(int speed)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(speed);
        return asyncCallWithArgumentList(QStringLiteral("SetDownloadRateLimit"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    // begin property changed signals
    void propertyChanged(const QString &propertyName, const QVariant &value);
    void UploadLimitSpeedChanged(const QString & speed) const;
    void DownloadLimitSpeedChanged(const QString & speed) const;

public Q_SLOTS:
    void CallQueued(const QString &callName, const QList<QVariant> &args);

private Q_SLOTS:
    void onPendingCallFinished(QDBusPendingCallWatcher *w);
    void onPropertyChanged(const QString& interfaceName,
                           const QVariantMap& changedProperties,
                           const QStringList& invalidatedProperties);

private:
    UpdateAssistantPrivate *d_ptr;
};

#endif // UPDATEASSISTANT_H
