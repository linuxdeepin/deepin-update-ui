// SPDX-FileCopyrightText: 2025-2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOGWATCHERHELPER_H
#define LOGWATCHERHELPER_H

#include <QObject>
#include <QSocketNotifier>

class UpdateDBusProxy;

class LogWatcherHelper : public QObject
{
    Q_OBJECT

public:
    explicit LogWatcherHelper(UpdateDBusProxy *dbusProxy, QObject *parent = nullptr);
    ~LogWatcherHelper();

    void startWatchFile();
    void stopWatchFile();

private Q_SLOTS:
    void onDataAvailable();

Q_SIGNALS:
    void incrementalDataChanged(const QString &incrementaldata);
    void fileReset();

private:
    void setupLogPipe();
    void readAvailableData();
    void startRealtimeLogAfterHistory();
    
    QString m_data;
    UpdateDBusProxy *m_dbusProxy;
    int m_readFd;
    int m_writeFd;
    QSocketNotifier *m_socketNotifier;
};

#endif // LOGWATCHERHELPER_H
