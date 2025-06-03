// SPDX-FileCopyrightText: 2016 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef COMMON_H
#define COMMON_H

#include <QObject>
#include <QString>
#include <QLoggingCategory>
#include <QtQml/qqml.h>

#include <QRegularExpression>
#include <vector>

using namespace std;


namespace dcc {
namespace update {
namespace common {

Q_NAMESPACE
QML_NAMED_ELEMENT(Common)

const double Epsion = 1e-6;
const QString SYSTEM_UPGRADE_TYPE_STRING = "system_upgrade";
const QString SECURITY_UPGRADE_TYPE_STRING = "security_upgrade";
const QString UNKNOWN_UPGRADE_STRING = "unknown_upgrade";

enum UpdatesStatus {
    Default,
    Checking,
    CheckingFailed,
    CheckingSucceed,
    Updated,
    UpdatesAvailable,
    DownloadWaiting,
    Downloading,
    DownloadPaused,
    Downloaded,
    DownloadFailed,

    // 点击下载按钮后的中间状态
    UpgradeWaiting,

    // 备份相关
    BackingUp,
    BackupFailed,
    BackupSuccess,

    // 升级
    UpgradeReady, // 备份阶段状态更新类型的状态保持UpgradeReady，直到开始安装
    Upgrading,
    UpgradeFailed,
    UpgradeSuccess,
    UpgradeComplete,
};
Q_ENUM_NS(UpdatesStatus)

enum UpdateErrorType {
    NoError,
    UnKnown,
    NoNetwork,
    DownloadingNoNetwork,
    DownloadingNoSpace,
    NoSpace,
    DependenciesBrokenError,
    DpkgInterrupted,
    CanNotBackup,
    BackupFailedUnknownReason,
    DpkgError,
    FileMissing,
    PlatformUnreachable,
    InvalidSourceList,
};
Q_ENUM_NS(UpdateErrorType)

enum UpdateType {
    Invalid = 0, // 无效
    SystemUpdate = 1 << 0, // 系统
    AppStoreUpdate = 1 << 1, // 应用商店（1050版本弃用）
    SecurityUpdate = 1 << 2, // 安全
    UnknownUpdate = 1 << 3, // 未知来源
    OnlySecurityUpdate = 1 << 4 // 仅安全更新（1060版本弃用）
};
Q_ENUM_NS(UpdateType)

enum UpdateCtrlType {
    Start = 0,
    Pause
};
Q_ENUM_NS(UpdateCtrlType)

enum UiActiveState {
    Unknown = -1,     // 未知
    Unauthorized = 0, // 未授权
    Authorized,       // 已授权
    AuthorizedLapse,  // 授权失效
    TrialAuthorized,  // 试用期已授权
    TrialExpired      // 试用期已过期
};

enum TestingChannelStatus {
    DeActive,
    NotJoined,
    WaitJoined,
    WaitToLeave,
    Joined,
};
Q_ENUM_NS(TestingChannelStatus)

enum ControlPanelType {
    CPT_Invalid,
    CPT_NeedRestart,
    CPT_UpgradeFailed,
    CPT_BackupFailed,
    CPT_Upgrade,
    CPT_Downloaded,
    CPT_DownloadFailed,
    CPT_Downloading,
    CPT_Available,
    CPT_Checking
};
Q_ENUM_NS(ControlPanelType)

enum CtrlWidgetType {
    CtrlState_None,
    CtrlState_Check,
    CtrlState_Update,
};
Q_ENUM_NS(CtrlWidgetType)



static inline QString formatCap(qulonglong cap, const int size = 1024)
{
    static QString type[] = { "B", "KB", "MB", "GB", "TB" };

    if (cap < qulonglong(size)) {
        return QString::number(cap) + type[0];
    }
    if (cap < qulonglong(size) * size) {
        return QString::number(double(cap) / size, 'f', 2) + type[1];
    }
    if (cap < qulonglong(size) * size * size) {
        return QString::number(double(cap) / size / size, 'f', 2) + type[2];
    }
    if (cap < qulonglong(size) * size * size * size) {
        return QString::number(double(cap) / size / size / size, 'f', 2) + type[3];
    }

    return QString::number(double(cap) / size / size / size / size, 'f', 2) + type[4];
}

static inline vector<double> getNumListFromStr(const QString& str)
{
    //筛选出字符串中的数字
    // 使用 QRegularExpression 替代 QRegExp
    QRegularExpression rx(R"(-?[1-9]\d*\.\d*|0+\.[0-9]+|-?0\.\d*[1-9]\d*|-?\d+)");

    vector<double> v;
    QRegularExpressionMatchIterator iter = rx.globalMatch(str);

    while (iter.hasNext()) {
        QRegularExpressionMatch match = iter.next();
        v.push_back(match.captured(0).toDouble());
    }

    return v;
}

} // end namespace common
} // end namespace update
} // end namespace dcc

#endif // COMMON_H
