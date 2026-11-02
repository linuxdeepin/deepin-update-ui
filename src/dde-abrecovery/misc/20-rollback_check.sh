#!/bin/sh
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# treeland prestart hook —— 系统回滚
#
# 对应 X11 的上车点：/usr/share/dde-session-shell/greeters.d/launch.conf.d/
#                   98_rollback_conf.json
#                     NeedExecScript = /usr/bin/rollback_check.sh
#                     ExecPath       = /usr/bin/dde-rollback
#
# 放在 <AppDataLocation>/prestart.d/blocking/ 下，与更新的 10-system_upgrade_check.sh 同级；
# 编号 20 排在更新钩子之后（treeland 按数值排序）。treeland 在钩子执行期间保持渲染，
# 全部钩子退出后才加载锁屏，因此这里前台执行、等回滚框关闭再返回。
# 是否执行钩子由 treeland 判定（已有非 dde 用户登录时整体跳过），脚本不做门控。

# 更新优先：X11 的 pre-greeter 按 99 → 98 顺序遍历，命中 99 就 exit 0，
# 永远不会再评估 98。这里把同一条判定前置，复刻"命中即终止"，
# 避免更新待办与可回滚同时成立时拉起两个界面。
# （system_upgrade_check.sh 只读 /tmp 下的选项文件，不涉及 D-Bus）
if /usr/bin/system_upgrade_check.sh >/dev/null 2>&1; then
    logger -t dde-rollback -p daemon.info "20-rollback_check: skip, update takes priority"
    exit 0
fi

# 回滚条件（lastore CanRollback）必须在受信任的进程里判定：
# rollback_check.sh 用的是裸 dbus-send，treeland 下身份是 dde（非 root/lightdm），
# 会被 The Lastore1 的 polkit 检查拒绝，导致判定恒为假。
# 因此这里经 /usr/bin/dde-rollback --wait 启动：包装脚本走 security-loader 完成
# fd1/fd2 握手（allow-caller 注册），由 dde-rollback 自己调 CanRollback；
# 不满足时它会立即退出、不显示任何窗口，包装脚本收到 EXIT 后返回。
logger -t dde-rollback -p daemon.info "20-rollback_check: start dde-rollback"

QT_QPA_PLATFORM=wayland exec /usr/bin/dde-rollback --wait
