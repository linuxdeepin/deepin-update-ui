#!/bin/bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 登录后系统检查（AfterGreeterCheck）—— treeland/Wayland 会话
#
# 对应 X11 的 /etc/X11/Xsession.d/98deepin-upgrade-check：
# X11 在 xsession 启动过程中同步执行 `dde-update --wait`，检查完成才 exec 会话命令，
# 桌面因此不会先起来。Wayland 下没有 Xsession.d，改用 user unit：
#
#   dde-update-after-login-check.service  →  WantedBy/PartOf=dde-session-pre.target
#                                             Before=dde-session-core.target
#
# 而 dde-shell@DDE.service 是 `After=dde-session-pre.target` / `Before=dde-session-core.target`，
# 于是 Type=oneshot 的本脚本会阻塞桌面 shell，直到 dde-update 退出。
#
# 本脚本的判定与 X11 完全一致：Wayland 会话且选项文件存在即执行。
# 具体做哪一件事（DoUpgrade / 登录前检查 / 登录后检查）由 dde-update 自己读同一份
# /tmp/deepin_update_option.json 决定（见 UpdateWorker::getUpdateOption()），
# 这里不再重复判断字段，避免与进程内的优先级逻辑产生分歧。
set -u

# 日志统一走 journal，tag 用 unit 名，现场直接
# journalctl --user -t dde-update-after-login-check -b 定位。
TAG="dde-update-after-login-check"

# X11 由 Xsession.d 负责，这里不重复执行
if [ "${XDG_SESSION_TYPE:-}" != "wayland" ]; then
    logger -t "$TAG" -p daemon.info \
        "skip: XDG_SESSION_TYPE=${XDG_SESSION_TYPE:-unset}, not a wayland session"
    exit 0
fi

OPT=/tmp/deepin_update_option.json
if [ ! -f "$OPT" ]; then
    logger -t "$TAG" -p daemon.info "skip: $OPT not found"
    exit 0
fi

# /usr/bin/dde-update 是包装脚本（内含 deepin-security-loader）：
# 会话用户身份不在 lastore 的 trusted 名单里，必须靠握手注册 allow-caller。
# 必须带 --wait：非 wait 模式下包装脚本只是 exec loader（loader 会 setsid 脱离），
# 会立即返回，桌面就不再等检查结束。--wait 与 X11 的 /etc/X11/Xsession.d/98deepin-upgrade-check 保持一致。
logger -t "$TAG" -p daemon.info "run after-login check: exec dde-update --wait"
exec /usr/bin/dde-update --wait
