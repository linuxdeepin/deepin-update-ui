// SPDX-FileCopyrightText: 2015 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "public_func.h"
#include "common/dbus/updatedbusproxy.h"

#include <QJsonDocument>
#include <DDBusSender>
#include <QDBusReply>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logCommon)

QPixmap loadPixmap(const QString &file)
{
    qCDebug(logCommon) << "Loading pixmap from file:" << file;
    qreal ratio = 1.0;
    qreal devicePixel = qApp->devicePixelRatio();

    QPixmap pixmap;

    if (!qFuzzyCompare(ratio, devicePixel)) {
        qCDebug(logCommon) << "Loading scaled pixmap for device pixel ratio:" << devicePixel;
        QImageReader reader;
        reader.setFileName(qt_findAtNxFile(file, devicePixel, &ratio));
        if (reader.canRead()) {
            reader.setScaledSize(reader.size() * (devicePixel / ratio));
            pixmap = QPixmap::fromImage(reader.read());
            pixmap.setDevicePixelRatio(devicePixel);
        } else {
            qCWarning(logCommon) << "Cannot read scaled pixmap file";
        }
    } else {
        qCDebug(logCommon) << "Loading standard pixmap";
        pixmap.load(file);
    }

    qCDebug(logCommon) << "Pixmap loaded, size:" << pixmap.size();
    return pixmap;
}

// uid, name
std::pair<int, QString> getCurrentUser()
{
    qCDebug(logCommon) << "Getting current user information";
    UpdateDBusProxy dbusProxy;
    const QString &currentUserJson = dbusProxy.CurrentUser();
    qInfo() << "Get current locale, current user:" << currentUserJson;

    QJsonParseError jsonParseError;
    const auto& userDoc = QJsonDocument::fromJson(currentUserJson.toUtf8(), &jsonParseError);

    if (jsonParseError.error != QJsonParseError::NoError || userDoc.isEmpty()) {
        qCWarning(logCommon) << "Failed to parse user JSON, error:" << jsonParseError.errorString();
        return {};
    }

    const auto& userObj = userDoc.object();
    const auto& uid = userObj.value("Uid").toInt();
    const auto& name = userObj.value("Name").toString();

    return {uid, name};
}

QString getCurrentLocale()
{
    qCDebug(logCommon) << "Getting current locale";
    static const QString DEFAULT_LOCALE = QStringLiteral("en_US");

    UpdateDBusProxy dbusProxy;
    const auto& [uid, _] = getCurrentUser();

    if (uid == 0) {
        qCWarning(logCommon) << "Current user's uid is invalid";
        return DEFAULT_LOCALE;
    }

    const auto path(QString("/org/deepin/dde/Accounts1/User%1").arg(uid));
    qCInfo(logCommon) << "Current user account path: " << path;
    QDBusReply<QDBusVariant> reply = DDBusSender::system().interface("org.deepin.dde.Accounts1.User").path(path).service("org.deepin.dde.Accounts1").property("Locale").get();
    if (!reply.isValid()) {
        qCWarning(logCommon) << "Failed to get current user locale, error: " << reply.error().message();
        return DEFAULT_LOCALE;
    }

    auto ret = qdbus_cast<QString>(reply.value().variant());
    return ret;
}
