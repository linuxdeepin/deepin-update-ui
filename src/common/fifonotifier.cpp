// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "fifonotifier.h"

#include <QByteArray>
#include <QLoggingCategory>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

Q_LOGGING_CATEGORY(logFifoNotifier, "dde.update.fifo")

// Write all of `data` to `fd` opened O_NONBLOCK, using poll(2) for POLLOUT
// on EAGAIN/EWOULDBLOCK instead of busy-looping. Returns true iff every byte
// was written within `timeoutMs`. Handles EINTR for both write() and poll().
static bool writeAllFifo(int fd, const QByteArray &data, int timeoutMs)
{
    ssize_t total = 0;
    while (total < data.size()) {
        ssize_t w = write(fd, data.constData() + total, data.size() - total);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd;
                pfd.fd = fd;
                pfd.events = POLLOUT;
                int pr = -1;
                do {
                    pr = poll(&pfd, 1, timeoutMs);
                } while (pr < 0 && errno == EINTR);
                if (pr <= 0) {
                    return false;
                }
                continue;
            }
            return false;
        }
        total += w;
    }
    return total == data.size();
}

FifoNotifier::FifoNotifier(const QString &fifoPath)
    : m_path(fifoPath)
{
    reportPid();
}

FifoNotifier::~FifoNotifier()
{
    notify();
}

void FifoNotifier::reportPid()
{
    if (m_path.isEmpty()) {
        return;
    }
    // Open first, then verify it's a FIFO via fstat().
    // This avoids the TOCTOU race of stat() before open().
    int fd = open(m_path.toUtf8().constData(), O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        qCWarning(logFifoNotifier) << "cannot report PID, open failed:" << strerror(errno);
        return;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISFIFO(st.st_mode)) {
        qCWarning(logFifoNotifier) << "path is not a FIFO, cannot report PID";
        close(fd);
        return;
    }
    // Write our PID so the wrapper can track this exact process via kill -0.
    QByteArray msg = "PID:" + QByteArray::number(getpid()) + "\n";
    bool ok = writeAllFifo(fd, msg, 5000);
    close(fd);
    if (ok) {
        qCInfo(logFifoNotifier) << "reported PID" << getpid() << "to FIFO";
    } else {
        qCWarning(logFifoNotifier) << "failed to report PID (write incomplete)";
    }
}

void FifoNotifier::notify()
{
    if (m_path.isEmpty()) {
        return;
    }
    qCInfo(logFifoNotifier) << "notifying FIFO" << m_path;
    // Open first, then verify it's a FIFO via fstat().
    // This avoids the TOCTOU race of stat() before open().
    // O_NONBLOCK so we don't hang if the reader already closed;
    // the script opens fd 3<> before launching us so a reader exists.
    int fd = open(m_path.toUtf8().constData(), O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        qCWarning(logFifoNotifier) << "cannot open" << m_path << ":" << strerror(errno);
        return;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISFIFO(st.st_mode)) {
        qCWarning(logFifoNotifier) << "path is not a FIFO, cannot notify";
        close(fd);
        return;
    }
    // Write "EXIT\n" to signal exit to the wrapper.
    QByteArray msg("EXIT\n");
    bool ok = writeAllFifo(fd, msg, 2000);
    close(fd);
    m_path.clear();
    if (ok) {
        qCInfo(logFifoNotifier) << "exit notify sent (EXIT)";
    } else {
        qCWarning(logFifoNotifier) << "failed to notify EXIT (write incomplete)";
    }
}
