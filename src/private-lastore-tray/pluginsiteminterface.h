// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PLUGINSITEMINTERFACE_H
#define PLUGINSITEMINTERFACE_H

#include "pluginproxyinterface.h"

#include <QIcon>
#include <QtCore>

// Dock 插件条目接口，定义条目显示和交互能力。
class PluginsItemInterface
{
public:
    enum PluginType {
        Normal,
        Fixed
    };

    // 条目尺寸跟随系统或使用自定义策略。
    enum PluginSizePolicy {
        System = 1 << 0,
        Custom = 1 << 1
    };

    // 销毁插件条目对象。
    virtual ~PluginsItemInterface() {}

    // 返回插件唯一标识。
    virtual const QString pluginName() const = 0;
    virtual const QString pluginDisplayName() const { return QString(); }

    // 初始化条目并接收 dock 代理接口。
    virtual void init(PluginProxyInterface *proxyInter) = 0;

    // 返回条目控件。
    virtual QWidget *itemWidget(const QString &itemKey) = 0;

    // 返回条目悬浮提示控件。
    virtual QWidget *itemTipsWidget(const QString &itemKey) {Q_UNUSED(itemKey); return nullptr;}

    // 返回条目点击后弹出的内容控件。
    virtual QWidget *itemPopupApplet(const QString &itemKey) {Q_UNUSED(itemKey); return nullptr;}

    // 返回点击条目时执行的命令。
    virtual const QString itemCommand(const QString &itemKey) {Q_UNUSED(itemKey); return QString();}

    // 返回条目上下文菜单定义。
    virtual const QString itemContextMenu(const QString &itemKey) {Q_UNUSED(itemKey); return QString();}

    // 处理上下文菜单点击事件。
    virtual void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) {Q_UNUSED(itemKey); Q_UNUSED(menuId); Q_UNUSED(checked);}

    // 返回条目排序位置。
    virtual int itemSortKey(const QString &itemKey) {Q_UNUSED(itemKey); return 1;}

    // 保存条目排序位置。
    virtual void setSortKey(const QString &itemKey, const int order) {Q_UNUSED(itemKey); Q_UNUSED(order);}

    // 返回条目是否允许进入容器区域。
    virtual bool itemAllowContainer(const QString &itemKey) {Q_UNUSED(itemKey); return false;}

    // 返回条目当前是否在容器中。
    virtual bool itemIsInContainer(const QString &itemKey) {Q_UNUSED(itemKey); return false;}

    // 保存条目容器状态。
    virtual void setItemIsInContainer(const QString &itemKey, const bool container) {Q_UNUSED(itemKey); Q_UNUSED(container);}

    virtual bool pluginIsAllowDisable() { return false; }
    virtual bool pluginIsDisable() { return false; }
    virtual void pluginStateSwitched() {}

    // 处理 dock 显示模式变化。
    virtual void displayModeChanged(const Dock::DisplayMode displayMode) {Q_UNUSED(displayMode);}

    // 处理 dock 停靠位置变化。
    virtual void positionChanged(const Dock::Position position) {Q_UNUSED(position);}

    // 刷新条目图标。
    virtual void refreshIcon(const QString &itemKey) { Q_UNUSED(itemKey); }

    // 返回当前 dock 显示模式。
    inline Dock::DisplayMode displayMode() const
    {
        return qApp->property(PROP_DISPLAY_MODE).value<Dock::DisplayMode>();
    }

    // 返回当前 dock 停靠位置。
    inline Dock::Position position() const
    {
        return qApp->property(PROP_POSITION).value<Dock::Position>();
    }

    // 处理插件设置变化。
    virtual void pluginSettingsChanged() {}

    // 返回插件类型。
    QT_DEPRECATED_X("Use flags()")
    virtual PluginType type() { return Normal; }

    // 返回条目尺寸策略。
    virtual PluginSizePolicy pluginSizePolicy() const { return System; }

protected:
    // 保存 dock 代理接口。
    PluginProxyInterface *m_proxyInter = nullptr;
};

QT_BEGIN_NAMESPACE

#define ModuleInterface_iid "com.deepin.dock.PluginsItemInterface"

Q_DECLARE_INTERFACE(PluginsItemInterface, ModuleInterface_iid)
QT_END_NAMESPACE

#endif // PLUGINSITEMINTERFACE_H
