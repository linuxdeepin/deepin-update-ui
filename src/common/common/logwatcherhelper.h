// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGWATCHERHELPER_H
#define LOGWATCHERHELPER_H

#include <QObject>
#include <QFileSystemWatcher>

class LogWatcherHelper : public QObject
{
    Q_OBJECT

public:
    explicit LogWatcherHelper(QObject *parent = nullptr);
    ~LogWatcherHelper();

    void startWatchFile();
    void stopWatchFile();
    void readFileIncrement();

private slots:
    void onFileChanged(const QString &path);
    void onDirectoryChanged(const QString &path);

signals:
    void incrementalDataChanged(const QString &incrementaldata);

private:
    QFileSystemWatcher *m_fileWatcher;
    qint64 m_lastFileSize;
    QString m_data;
};

#endif // LOGWATCHERHELPER_H
