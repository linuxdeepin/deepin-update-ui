// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "securityloaderhelper.h"

#include <QDebug>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDBusConnection>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <sys/stat.h>

static const QString DEFAULT_AUTH_RESOURCE = ":/misc/org.deepin.dde-update.json";
// Upper bound for the authorization response to guard against a misbehaving
// loader that never closes its write end or streams excessive data.
static constexpr int MAX_RESPONSE_BYTES = 1024 * 1024;  // 1 MiB
// Loader-injected fds must not collide with the standard streams.
static constexpr int MIN_VALID_FD = 3;

bool SecurityLoaderHelper::doSecurityLoader(int argc, char *argv[])
{
    auto loaderInfo = parseLoaderArgs(argc, argv);
    if (!loaderInfo.isLoadedByLoader) {
        return true;
    }

    if (!loadDestList(DEFAULT_AUTH_RESOURCE)) {
        qWarning() << "Failed to load DestList, cannot perform handshake";
        if (loaderInfo.fd1 >= 0) close(loaderInfo.fd1);
        if (loaderInfo.fd2 >= 0) close(loaderInfo.fd2);
        return false;
    }

    if (!performHandshake(loaderInfo)) {
        qWarning() << "Security loader handshake failed";
        return false;
    }
    return true;
}

LoaderHandshakeInfo SecurityLoaderHelper::parseLoaderArgs(int argc, char *argv[])
{
    LoaderHandshakeInfo info;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fd1") == 0 && i + 1 < argc) {
            const char *val = argv[++i];
            char *end = nullptr;
            errno = 0;
            long fd = strtol(val, &end, 10);
            // Reject: no digits, trailing chars, range error, negative, or
            // colliding with the standard streams.
            if (end == val || *end != '\0' || errno == ERANGE
                    || fd < 0 || fd > INT_MAX || fd < MIN_VALID_FD) {
                qWarning() << "Invalid --fd1 value:" << val << ", ignoring";
                continue;
            }
            info.fd1 = static_cast<int>(fd);
            // Verify the fd is actually a FIFO to prevent writing
            // authorization data to arbitrary open files.
            struct stat st;
            if (fstat(info.fd1, &st) != 0 || !S_ISFIFO(st.st_mode)) {
                qWarning() << "fd1 is not a FIFO, ignoring";
                info.fd1 = -1;
                continue;
            }
            info.isLoadedByLoader = true;
        } else if (strcmp(argv[i], "--fd2") == 0 && i + 1 < argc) {
            const char *val = argv[++i];
            char *end = nullptr;
            errno = 0;
            long fd = strtol(val, &end, 10);
            if (end == val || *end != '\0' || errno == ERANGE
                    || fd < 0 || fd > INT_MAX || fd < MIN_VALID_FD) {
                qWarning() << "Invalid --fd2 value:" << val << ", ignoring";
                continue;
            }
            info.fd2 = static_cast<int>(fd);
            // Verify the fd is actually a FIFO to prevent writing
            // authorization data to arbitrary open files.
            struct stat st;
            if (fstat(info.fd2, &st) != 0 || !S_ISFIFO(st.st_mode)) {
                qWarning() << "fd2 is not a FIFO, ignoring";
                info.fd2 = -1;
                continue;
            }
        }
    }

    if (info.isLoadedByLoader) {
        qInfo() << "Detected loader injection: fd1=" << info.fd1 << "fd2=" << info.fd2;
    }

    return info;
}

bool SecurityLoaderHelper::loadDestList(const QString &configPath)
{
    qInfo() << "Loading permission config from:" << configPath;

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot open file:" << configPath;
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error in" << configPath << ":" << error.errorString();
        return false;
    }

    QJsonObject root = doc.object();
    if (!root.contains("DestList") || !root["DestList"].isArray()) {
        qWarning() << "Invalid config format in" << configPath << ": missing DestList";
        return false;
    }

    m_destList = root["DestList"].toArray();
    qInfo() << "Loaded" << m_destList.size() << "D-Bus interfaces to authorize";
    return !m_destList.isEmpty();
}

bool SecurityLoaderHelper::performHandshake(const LoaderHandshakeInfo &info)
{
    // Both fds must be valid to proceed; close whichever was injected.
    if (!info.isLoadedByLoader || info.fd1 < 0 || info.fd2 < 0) {
        qInfo() << "Not loaded by loader or fds incomplete, skipping handshake";
        if (info.fd1 >= 0) close(info.fd1);
        if (info.fd2 >= 0) close(info.fd2);
        return true;
    }

    if (m_destList.isEmpty()) {
        qWarning() << "No D-Bus interfaces loaded, skipping handshake";
        close(info.fd1);
        close(info.fd2);
        return true;
    }

    qInfo() << "Performing loader handshake...";

    QDBusConnection systemBus = QDBusConnection::systemBus();
    if (!systemBus.isConnected()) {
        qWarning() << "Cannot connect to system bus";
        close(info.fd1);
        close(info.fd2);
        return false;
    }

    QString uniqueName = systemBus.baseService();
    qInfo() << "System Bus UniqueName:" << uniqueName;

    QJsonObject request;
    request["UniqueName"] = uniqueName;
    request["DestList"] = m_destList;

    QByteArray jsonData = QJsonDocument(request).toJson(QJsonDocument::Compact);
    qInfo() << "Sending request with" << m_destList.size() << "interfaces";

    // Use POSIX write()/close() on the raw fd: QFile::open(fd, ...) defaults to
    // DontCloseHandle so QFile::close() would not close the underlying fd,
    // leaking it and blocking the loader on EOF.
    {
        const char *data = jsonData.constData();
        ssize_t total = 0;
        ssize_t remaining = jsonData.size();
        while (remaining > 0) {
            ssize_t w = write(info.fd1, data + total, remaining);
            if (w < 0) {
                if (errno == EINTR) {
                    continue;
                }
                qWarning() << "write() to fd1 failed:" << strerror(errno);
                close(info.fd1);
                close(info.fd2);
                return false;
            }
            total += w;
            remaining -= w;
        }
        close(info.fd1);
    }
    qInfo() << "Sent authorization request to loader, bytes:" << jsonData.size();

    // Set fd2 non-blocking so read() never hangs; poll() enforces a total
    // deadline covering first-byte and all subsequent reads. If the loader
    // writes a partial response and stalls, we still time out instead of
    // blocking forever (which would orphan the GUI before FifoNotifier runs).
    int fd2Flags = fcntl(info.fd2, F_GETFL);
    if (fd2Flags < 0 || fcntl(info.fd2, F_SETFL, fd2Flags | O_NONBLOCK) < 0) {
        qWarning() << "Cannot set fd2 non-blocking:" << strerror(errno);
        close(info.fd2);
        return false;
    }

    // Unified 5-second deadline for the entire response.
    QByteArray response;
    char buf[4096];
    auto deadline = std::chrono::steady_clock::now()
                  + std::chrono::milliseconds(5000);
    while (true) {
        int remainingMs = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                deadline - std::chrono::steady_clock::now()).count());
        if (remainingMs <= 0) {
            qWarning() << "Timeout reading loader response (5000ms total)";
            close(info.fd2);
            return false;
        }

        struct pollfd pfd;
        pfd.fd = info.fd2;
        pfd.events = POLLIN;
        int pret = -1;
        do {
            pret = poll(&pfd, 1, remainingMs);
        } while (pret < 0 && errno == EINTR);
        if (pret < 0) {
            qWarning() << "poll() on fd2 failed:" << strerror(errno);
            close(info.fd2);
            return false;
        }
        if (pret == 0) {
            qWarning() << "Timeout reading loader response (5000ms total)";
            close(info.fd2);
            return false;
        }
        if (pfd.revents & (POLLERR | POLLNVAL)) {
            qWarning() << "poll() on fd2 returned error:" << pfd.revents;
            close(info.fd2);
            return false;
        }
        if (!(pfd.revents & (POLLIN | POLLHUP))) {
            continue;
        }

        ssize_t n = read(info.fd2, buf, sizeof(buf));
        if (n > 0) {
            response.append(buf, n);
            if (response.size() > MAX_RESPONSE_BYTES) {
                qWarning() << "Authorization response exceeded" << MAX_RESPONSE_BYTES
                           << "bytes, aborting";
                close(info.fd2);
                return false;
            }
            continue;
        }
        if (n == 0) {
            // EOF: loader closed write end, message complete.
            break;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Data not ready yet; loop back to poll with remaining deadline.
            continue;
        }
        if (errno == EINTR) {
            continue;
        }
        qWarning() << "read() on fd2 failed:" << strerror(errno);
        close(info.fd2);
        return false;
    }
    close(info.fd2);

    if (response.isEmpty()) {
        qWarning() << "Empty response from loader";
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument responseDoc = QJsonDocument::fromJson(response, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDoc.isObject()) {
        qWarning() << "Invalid JSON response from loader:" << parseError.errorString();
        return false;
    }

    QJsonObject result = responseDoc.object();
    if (!result.contains("Result")) {
        qWarning() << "Loader response missing 'Result' field";
        return false;
    }
    if (result["Result"].toBool()) {
        qInfo() << "Loader handshake completed successfully";
        return true;
    } else {
        qWarning() << "Loader authorization denied:" << result["Message"].toString();
        return false;
    }
}
