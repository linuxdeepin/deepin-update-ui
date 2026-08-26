// SPDX-FileCopyrightText: 2022 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UPDATEMODEL_H
#define UPDATEMODEL_H

#include <QMap>
#include <QObject>
#include <QPair>
#include <QStringList>

class UpdateModel : public QObject
{
    Q_OBJECT

public:

enum UpdateStatus {
    Default = 0,
    Ready,
    PrepareFailed,
    BackingUp,
    BackupSuccess,
    BackupFailed,
    Installing,
    InstallSuccess,
    InstallFailed,
    InstallComplete
};
Q_ENUM(UpdateStatus)

enum UpdateError {
    NoError,
    LowPower,
    BackupInterfaceError,
    CanNotBackup,
    BackupNoSpace,
    BackupFailedUnknownReason,
    /* !! 以下失败是要重启电脑的，添加错误的时候请注意 */
    UpdateInterfaceError,
    InstallNoSpace,
    DependenciesBrokenError,
    DpkgInterrupted,
    UnKnown
};
Q_ENUM(UpdateError)

enum UpdateAction {
    None,
    DoBackupAgain,
    ExitUpdating,
    ContinueUpdating,
    Reboot,
    ShutDown,
    EnterDesktop
};
Q_ENUM(UpdateAction)

enum CheckStatus {
    ReadToCheck,
    Checking,
    CheckFailed,
    CheckSuccess,
    CheckEnd
};
Q_ENUM(CheckStatus)

enum CheckSystemStage {
    CSS_None = 0,
    CSS_BeforeLogin,
    CSS_AfterLogin
};

public:
    static UpdateModel *instance();

    void setUpdateStatus(UpdateModel::UpdateStatus status);
    UpdateStatus updateStatus() const { return m_updateStatus; }

    void setJobProgress(double progress);
    double jobProgress() const { return m_jobProgress; }

    void setUpdateError(UpdateError error);
    UpdateError updateError() const { return m_updateError; };
    static QString updateErrorToString(UpdateError error);
    static QPair<QString, QString> updateErrorMessage(UpdateError error);

    void setLastErrorLog(const QString &log);
    QString lastErrorLog() const { return m_lastErrorLog; }

    void setBackupConfigValidation(bool valid);
    bool isBackupConfigValid() const { return m_isBackupConfigValid; }

    bool isUpdating() { return m_isUpdating; }
    void setIsUpdating(bool isUpdating) { m_isUpdating = isUpdating; }

    void setIsReboot(bool isReboot) { m_isReboot = isReboot; }
    bool isReboot() { return m_isReboot; }

    void setDoUpgrade(bool doUpgrade) { m_doUpgrade = doUpgrade; }
    bool whetherDoUpgrade() const { return m_doUpgrade; }

    void setMajorUpgrade(bool majorUpgrade) { m_majorUpgrade = majorUpgrade; }
    bool majorUpgrade() const { return m_majorUpgrade; }

    void setUpdateMode(int updateMode) { m_updateMode = updateMode; }
    int updateMode() const { return m_updateMode; }

    static QString updateActionText(UpdateAction action);

    void setCheckStatus(CheckStatus);
    CheckStatus checkStatus() const { return m_checkStatus; }

    void setCheckSystemStage(CheckSystemStage stage) { m_checkSystemStage = stage; }
    CheckSystemStage checkSystemStage() const { return m_checkSystemStage; }

    bool hasBackup() const { return m_hasBackup; }
    void setHasBackup(bool hasBackup) { m_hasBackup = hasBackup; }

    void setUpdateLog(const QString &log);
    void appendUpdateLog(const QString &log);
    QString updateLog() const { return m_updateLog; }

private:
    explicit UpdateModel(QObject *parent = nullptr);

signals:
    void updateAvailableChanged(bool available);
    void updateStatusChanged(UpdateStatus status);
    void JobProgressChanged(double progress);
    void checkStatusChanged(CheckStatus);
    void updateLogChanged(const QString &log);
    void updateLogAppended(const QString &log);

private:
    bool m_updateAvailable;
    UpdateStatus m_updateStatus;
    double m_jobProgress;
    UpdateError m_updateError;      // 错误类型
    QString m_lastErrorLog;         // 错误日志
    bool m_isBackupConfigValid;     // 可以备份的必要条件
    bool m_isUpdating;              // 是否正在更新
    bool m_isReboot;                // true 更新并重启 false 更新并关机
    int m_updateMode; // 更新类型
    bool m_doUpgrade; // 是否进行更新操作
    bool m_majorUpgrade = false; // 是否为大版本升级 (lastore-daemon 判定并写入选项文件)
    bool m_hasBackup = false; // 是否有备份

    CheckStatus m_checkStatus;
    CheckSystemStage m_checkSystemStage;

    QString m_updateLog;
};

#endif // UPDATEMODEL_H
