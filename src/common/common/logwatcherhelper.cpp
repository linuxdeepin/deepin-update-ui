// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logwatcherhelper.h"

#include <QDebug>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingCall>
#include <QLoggingCategory>

#include <unistd.h>
#include <fcntl.h>

#include "../dbus/updatedbusproxy.h"

Q_DECLARE_LOGGING_CATEGORY(logCommon)

LogWatcherHelper::LogWatcherHelper(UpdateDBusProxy *dbusProxy, QObject *parent)
    : QObject(parent)
    , m_data(QString())
    , m_dbusProxy(dbusProxy)
    , m_readFd(-1)
    , m_writeFd(-1)
    , m_socketNotifier(nullptr)
{
    qCDebug(logCommon) << "Initialize LogWatcherHelper";
}

LogWatcherHelper::~LogWatcherHelper()
{
    qCDebug(logCommon) << "Destroying LogWatcherHelper";
    stopWatchFile();
}

void LogWatcherHelper::startWatchFile()
{
    qCInfo(logCommon) << "Starting to watch update log via DBus interface";
    
    if (!m_dbusProxy) {
        qCWarning(logCommon) << "DBus proxy is null, cannot start watching";
        return;
    }
    
    if (m_readFd != -1) {
        qCWarning(logCommon) << "Log watcher already started";
        return;
    }

    // 设置管道并调用 DBus 接口
    setupLogPipe();
}

void LogWatcherHelper::stopWatchFile()
{
    qCDebug(logCommon) << "Stopping update log watching";
    
    // 停止文件描述符监听
    if (m_socketNotifier) {
        qCDebug(logCommon) << "Disabling and deleting socket notifier";
        m_socketNotifier->setEnabled(false);
        delete m_socketNotifier;
        m_socketNotifier = nullptr;
    }
    
    // 关闭文件描述符
    if (m_readFd != -1) {
        qCDebug(logCommon) << "Closing read file descriptor";
        close(m_readFd);
        m_readFd = -1;
    }
    
    if (m_writeFd != -1) {
        qCDebug(logCommon) << "Closing write file descriptor";
        close(m_writeFd);
        m_writeFd = -1;
    }

    // 重置数据
    qCDebug(logCommon) << "Resetting log data";
    m_data = QString();
}

void LogWatcherHelper::setupLogPipe()
{
    qCDebug(logCommon) << "Setting up log monitoring pipe";
    // 创建管道
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        qCWarning(logCommon) << "Failed to create pipe for log reading";
        return;
    }

    m_readFd = pipefd[0];
    m_writeFd = pipefd[1];

    // 设置读端为非阻塞
    int flags = fcntl(m_readFd, F_GETFL, 0);
    fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

    // 设置 QSocketNotifier 来监听读端文件描述符
    qCDebug(logCommon) << "Creating socket notifier for pipe monitoring";
    m_socketNotifier = new QSocketNotifier(m_readFd, QSocketNotifier::Read, this);
    connect(m_socketNotifier, &QSocketNotifier::activated, this, &LogWatcherHelper::onDataAvailable);

    // 第一步：先获取历史日志 (realtime=false)
    qCDebug(logCommon) << "First getting historical logs...";
    QDBusPendingCall historyCall = m_dbusProxy->GetUpdateDetails(m_writeFd, false);
    QDBusPendingCallWatcher* historyWatcher = new QDBusPendingCallWatcher(historyCall, this);
    
    connect(historyWatcher, &QDBusPendingCallWatcher::finished, this, [this, historyWatcher](void) {
        // 可能获取历史日志失败（比如没有历史日志），但是不影响实时日志的获取
        if (historyWatcher->isError()) {
            qCWarning(logCommon) << "GetUpdateDetails for history failed:" << historyWatcher->error().message();
        }

        qCDebug(logCommon) << "Historical logs completed, starting realtime monitoring...";
        
        // 历史日志获取完成，直接启动实时日志监听
        startRealtimeLogAfterHistory();
        
        historyWatcher->deleteLater();
    });
}

void LogWatcherHelper::startRealtimeLogAfterHistory()
{
    qCDebug(logCommon) << "Starting realtime log monitoring after history";
    // 启用文件描述符监听
    if (m_socketNotifier) {
        qCDebug(logCommon) << "Enabling socket notifier for realtime monitoring";
        m_socketNotifier->setEnabled(true);
    }
    
    // 开始获取实时增量日志 (realtime=true)
    qCDebug(logCommon) << "Starting realtime log monitoring via DBus";
    QDBusPendingCall realtimeCall = m_dbusProxy->GetUpdateDetails(m_writeFd, true);
    QDBusPendingCallWatcher* realtimeWatcher = new QDBusPendingCallWatcher(realtimeCall, this);
    
    connect(realtimeWatcher, &QDBusPendingCallWatcher::finished, this, [this, realtimeWatcher](void) {
        
        if (realtimeWatcher->isError()) {
            qCWarning(logCommon) << "GetUpdateDetails for realtime failed:" << realtimeWatcher->error().message();
            stopWatchFile();
        } else {
            qCDebug(logCommon) << "Realtime log monitoring started successfully";
        }
        
        realtimeWatcher->deleteLater();
    });
}

void LogWatcherHelper::onDataAvailable()
{
    qCDebug(logCommon) << "Data available on log pipe, reading...";
    readAvailableData();
}

void LogWatcherHelper::readAvailableData()
{
    qCDebug(logCommon) << "readAvailableData, readFd:" << m_readFd;
    if (m_readFd == -1) {
        qCDebug(logCommon) << "Read fd invalid, skipping data read";
        return;
    }

    QByteArray buffer;
    char readBuffer[4096];
    ssize_t bytesRead;
    
    while ((bytesRead = read(m_readFd, readBuffer, sizeof(readBuffer))) > 0) {
        buffer.append(readBuffer, bytesRead);
    }
    
    if (!buffer.isEmpty()) {
        QString newContent = QString::fromUtf8(buffer);
        qCDebug(logCommon) << "Read" << buffer.size() << "bytes from log pipe";

        m_data.append(newContent);
        emit incrementalDataChanged(newContent);
    } else {
        qCDebug(logCommon) << "No new data available from pipe";
    }
}
