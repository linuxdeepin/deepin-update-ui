// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PLUGINPROXYINTERFACE_H
#define PLUGINPROXYINTERFACE_H

#include "constants.h"

#include <QtCore>

class PluginsItemInterface;
class PluginProxyInterface
{
public:
    // 向 dock 添加条目。
    virtual void itemAdded(PluginsItemInterface * const itemInter, const QString &itemKey) = 0;

    // 通知 dock 刷新条目。
    virtual void itemUpdate(PluginsItemInterface * const itemInter, const QString &itemKey) = 0;

    // 从 dock 移除条目。
    virtual void itemRemoved(PluginsItemInterface * const itemInter, const QString &itemKey) = 0;

    // 请求显示条目上下文菜单。
    //virtual void requestContextMenu(PluginsItemInterface * const itemInter, const QString &itemKey) = 0;

    virtual void requestWindowAutoHide(PluginsItemInterface * const itemInter, const QString &itemKey, const bool autoHide) = 0;
    virtual void requestRefreshWindowVisible(PluginsItemInterface * const itemInter, const QString &itemKey) = 0;

    virtual void requestSetAppletVisible(PluginsItemInterface * const itemInter, const QString &itemKey, const bool visible) = 0;

    // 保存插件配置。
    virtual void saveValue(PluginsItemInterface * const itemInter, const QString &key, const QVariant &value) = 0;

    // 读取插件配置。
    virtual const QVariant getValue(PluginsItemInterface *const itemInter, const QString &key, const QVariant& fallback = QVariant()) = 0;

    // 删除插件配置。
    virtual void removeValue(PluginsItemInterface *const itemInter, const QStringList &keyList) = 0;
};

#endif // PLUGINPROXYINTERFACE_H
