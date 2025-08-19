//SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
//SPDX-License-Identifier: GPL-3.0-or-later
#include "mirrorinfolist.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

MirrorInfo::MirrorInfo()
{
    qCDebug(logDccUpdatePlugin) << "Initialize MirrorInfo";
}

const QDBusArgument &operator>>(const QDBusArgument &argument, MirrorInfo &info)
{
    qCDebug(logDccUpdatePlugin) << "Deserializing MirrorInfo from DBus";
    argument.beginStructure();
    argument >> info.m_id;
    argument >> info.m_url;
    argument >> info.m_name;
    argument.endStructure();
    qCDebug(logDccUpdatePlugin) << "MirrorInfo deserialized - ID:" << info.m_id << "name:" << info.m_name;

    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const MirrorInfo &info)
{
    qCDebug(logDccUpdatePlugin) << "Serializing MirrorInfo to DBus - ID:" << info.m_id << "name:" << info.m_name;
    argument.beginStructure();
    argument << info.m_id;
    argument << info.m_url;
    argument << info.m_name;
    argument.endStructure();

    return argument;
}

QDebug operator<<(QDebug argument, const MirrorInfo &info)
{
    qCDebug(logDccUpdatePlugin) << "Debug output for MirrorInfo";
    argument << "mirror id: " << info.m_id;
    argument << "mirror url: " << info.m_url;
    argument << "mirror name: " << info.m_name;

    return argument;
}

void registerMirrorInfoListMetaType()
{
    qCDebug(logDccUpdatePlugin) << "Registering MirrorInfo meta types";
    qRegisterMetaType<MirrorInfo>();
    qDBusRegisterMetaType<MirrorInfo>();
    qRegisterMetaType<MirrorInfoList>();
    qDBusRegisterMetaType<MirrorInfoList>();
}
