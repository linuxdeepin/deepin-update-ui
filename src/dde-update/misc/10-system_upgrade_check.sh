#!/bin/sh
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# treeland prestart hook —— 系统更新 / 登录前检查
#
# 对应 X11 的上车点：/usr/share/dde-session-shell/greeters.d/launch.conf.d/
#                   99_system_upgrade_conf.json
#                     NeedExecScript = /usr/bin/system_upgrade_check.sh
#                     ExecPath       = /usr/libexec/deepin/dde-update
#
# 放在 <AppDataLocation>/prestart.d/blocking/ 下。treeland 在钩子执行期间保持渲染，
# 全部钩子退出后才加载锁屏，因此这里前台执行、等 GUI 退出再返回，与 X11 的
# "阻塞执行 ExecPath，之后才出现登录界面"语义一致。
# 是否执行钩子由 treeland 判定（已有非 dde 用户登录时整体跳过），脚本不再做门控。
#
# 条件判定复用 X11 的同一个脚本，保证两条链路行为一致：
#   /tmp/deepin_update_option.json 存在 且（DoUpgrade=true 或 PreGreeterCheck=true）

if ! /usr/bin/system_upgrade_check.sh >/dev/null 2>&1; then
    logger -t dde-update -p daemon.info "10-system_upgrade_check: skip, no update option"
    exit 0
fi

logger -t dde-update -p daemon.info "10-system_upgrade_check: start dde-update"

# 必须用 /usr/bin/dde-update 包装脚本（内含 deepin-security-loader）：
# treeland 下进程身份是 dde，裸二进制拿不到 lastore 的 allow-caller 授权。
# 必须带 --wait：loader 用 setsid 把目标拉起后自身立即退出，不带 --wait 时包装
# 脚本会提前返回，锁屏可能在更新界面还在运行时就开始加载。
QT_QPA_PLATFORM=wayland exec /usr/bin/dde-update --wait
