// SPDX-FileCopyrightText: 2015 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "public_func.h"
#include "common/dbus/updatedbusproxy.h"

#include <QJsonDocument>
#include <DDBusSender>
#include <QDBusReply>

QPixmap loadPixmap(const QString &file)
{
    qreal ratio = 1.0;
    qreal devicePixel = qApp->devicePixelRatio();

    QPixmap pixmap;

    if (!qFuzzyCompare(ratio, devicePixel)) {
        QImageReader reader;
        reader.setFileName(qt_findAtNxFile(file, devicePixel, &ratio));
        if (reader.canRead()) {
            reader.setScaledSize(reader.size() * (devicePixel / ratio));
            pixmap = QPixmap::fromImage(reader.read());
            pixmap.setDevicePixelRatio(devicePixel);
        }
    } else {
        pixmap.load(file);
    }

    return pixmap;
}

QString getCurrentLocale()
{
    static const QString DEFAULT_LOCALE = QStringLiteral("en_US");

    UpdateDBusProxy dbusProxy;
    const QString &currentUserJson = dbusProxy.CurrentUser();
    qInfo() << "Get current locale, current user:" << currentUserJson;

    QJsonParseError jsonParseError;
    const auto& userDoc = QJsonDocument::fromJson(currentUserJson.toUtf8(), &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError || userDoc.isEmpty()) {
        qWarning("Failed to obtain current user information from lock service");
        return DEFAULT_LOCALE;
    }

    const auto& userObj = userDoc.object();
    const auto& uid = userObj.value("Uid").toInt();
    if (uid == 0) {
        qWarning() << "Current user's uid is invalid";
        return DEFAULT_LOCALE;
    }

    const auto path(QString("/org/deepin/dde/Accounts1/User%1").arg(uid));
    qInfo() << "Current user account path: " << path;
    QDBusReply<QDBusVariant> reply = DDBusSender::system().interface("org.deepin.dde.Accounts1.User").path(path).service("org.deepin.dde.Accounts1").property("Locale").get();
    if (!reply.isValid()) {
        qWarning() << "Failed to get current user locale, error: " << reply.error().message();
        return DEFAULT_LOCALE;
    }

    auto ret = qdbus_cast<QString>(reply.value().variant());
    return ret;
}
