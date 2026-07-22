// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SECURITYLOADERHELPER_H
#define SECURITYLOADERHELPER_H

#include <QString>
#include <QJsonArray>

struct LoaderHandshakeInfo {
    int fd1 = -1;
    int fd2 = -1;
    bool isLoadedByLoader = false;
};

class SecurityLoaderHelper
{
public:
    bool doSecurityLoader(int argc, char *argv[]);

private:
    LoaderHandshakeInfo parseLoaderArgs(int argc, char *argv[]);
    bool loadDestList(const QString &configPath);
    bool performHandshake(const LoaderHandshakeInfo &info);

    QJsonArray m_destList;
};

#endif // SECURITYLOADERHELPER_H
