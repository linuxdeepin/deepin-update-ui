// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "logwatcherhelper.h"

#include <QDebug>
#include <QDir>

#define UPDATE_LOG_FILE "/tmp/lastore_update_detail.log"

LogWatcherHelper::LogWatcherHelper(QObject *parent)
    : QObject(parent)
    , m_fileWatcher(nullptr)
    , m_lastFileSize(0)
    , m_data(QString())
{

}

LogWatcherHelper::~LogWatcherHelper()
{
    stopWatchFile();
}

void LogWatcherHelper::startWatchFile()
{
    qInfo() << "Start watch update log file";
    if (m_fileWatcher) {
        qWarning() << "Log file watcher already exists";
        return;
    }

    m_fileWatcher = new QFileSystemWatcher(this);
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this, &LogWatcherHelper::onFileChanged);
    connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged, this, &LogWatcherHelper::onDirectoryChanged);

    // 总是监控 /tmp 目录以检测文件创建/删除
    const QString logDir = "/tmp";
    if (QDir(logDir).exists()) {
        m_fileWatcher->addPath(logDir);
    } else {
        qWarning() << "tmp directory does not exist:" << logDir;
    }

    // 如果日志文件已存在，也监控它
    if (QFile::exists(UPDATE_LOG_FILE)) {
        m_fileWatcher->addPath(UPDATE_LOG_FILE);

        // 立即读取现有文件内容
        m_lastFileSize = 0;
        m_data = QString();
        readFileIncrement();
    }
}

void LogWatcherHelper::stopWatchFile()
{
    qInfo() << "Stop watch update log file";
    if (m_fileWatcher) {
        m_fileWatcher->removePaths(m_fileWatcher->files());
        delete m_fileWatcher;
        m_fileWatcher = nullptr;
    }

    // 重置文件大小记录
    m_lastFileSize = 0;
    m_data = QString();
}

void LogWatcherHelper::onFileChanged(const QString &path)
{
    if (path != UPDATE_LOG_FILE) {
        return;
    }

    readFileIncrement();

    // 重新添加文件到监控列表（QFileSystemWatcher 在文件变化后可能会自动移除监控）
    if (m_fileWatcher && !m_fileWatcher->files().contains(path)) {
        m_fileWatcher->addPath(path);
    }
}

void LogWatcherHelper::onDirectoryChanged(const QString &path)
{
    if (path != "/tmp") {
        return;
    }

    const bool fileExists = QFile::exists(UPDATE_LOG_FILE);
    const bool fileWatched = m_fileWatcher && m_fileWatcher->files().contains(UPDATE_LOG_FILE);

    if (fileExists && !fileWatched) {
        m_fileWatcher->addPath(UPDATE_LOG_FILE);
        m_lastFileSize = 0;
        m_data = QString();
        readFileIncrement();
    } else if (!fileExists && fileWatched) {
        qInfo() << "Update log file was deleted:" << UPDATE_LOG_FILE;
        m_lastFileSize = 0;
        m_data = QString();
    }
}

void LogWatcherHelper:: readFileIncrement()
{
    QFile logFile(UPDATE_LOG_FILE);

    if (!logFile.exists()) {
        qWarning() << "Log file does not exist:" << UPDATE_LOG_FILE;
        return;
    }

    const qint64 currentFileSize = logFile.size();

    // 如果文件变小了，说明文件被重新创建或截断，重新开始读取
    if (currentFileSize < m_lastFileSize) {
        m_lastFileSize = 0;
        m_data = QString();
    }

    // 如果文件大小没变，没有新内容
    if (currentFileSize == m_lastFileSize) {
        return;
    }

    if (!logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open log file for reading:" << logFile.errorString();
        return;
    }

    // 定位到上次读取的位置
    if (!logFile.seek(m_lastFileSize)) {
        qWarning() << "Failed to seek to position:" << m_lastFileSize;
        logFile.close();
        return;
    }

    const QByteArray newData = logFile.readAll();
    logFile.close();

    m_lastFileSize = currentFileSize;

    const QString incrementalContent = QString::fromUtf8(newData);
    m_data.append(incrementalContent);
    emit incrementalDataChanged(incrementalContent);
}
