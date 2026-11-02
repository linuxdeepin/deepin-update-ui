// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FIFONOTIFIER_H
#define FIFONOTIFIER_H

#include <QString>

/*!
 * \brief RAII 退出通知器
 *
 * 构造时向 FIFO 写 `PID:<pid>`，析构时写 `EXIT`，让启动脚本
 * （/usr/bin/dde-update --wait、/usr/bin/dde-rollback --wait）能阻塞等待目标进程退出。
 * 覆盖 main() 的所有返回路径；fifoPath 为空时不做任何事。
 */
class FifoNotifier
{
public:
    explicit FifoNotifier(const QString &fifoPath);
    ~FifoNotifier();

    // Disable copy to avoid double-notify.
    FifoNotifier(const FifoNotifier &) = delete;
    FifoNotifier &operator=(const FifoNotifier &) = delete;

private:
    void reportPid();
    void notify();

    QString m_path;
};

#endif // FIFONOTIFIER_H
