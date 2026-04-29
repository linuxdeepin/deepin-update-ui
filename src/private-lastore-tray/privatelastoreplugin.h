// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PRIVATELASTOREPLUGIN_H
#define PRIVATELASTOREPLUGIN_H

#include "pluginsiteminterface_v2.h"
#include "plugins-logging-category.h"

#include <QScopedPointer>
#include <DConfig>

#include "dtkcore_global.h"

class PrivateLastoreItem;
class PrivateLastorePlugin : public QObject, public PluginsItemInterfaceV2
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterfaceV2)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "privatelastoremode.json")

public:
    explicit PrivateLastorePlugin(QObject *parent = nullptr);
    ~PrivateLastorePlugin();

    const QString pluginName() const Q_DECL_OVERRIDE;
    const QString pluginDisplayName() const Q_DECL_OVERRIDE;
    void init(PluginProxyInterface *proxyInter) override;

    bool pluginIsAllowDisable() override { return true; }
    QWidget *itemWidget(const QString &itemKey) Q_DECL_OVERRIDE;
    QWidget *itemTipsWidget(const QString &itemKey) Q_DECL_OVERRIDE;
    Dock::PluginFlags flags() const Q_DECL_OVERRIDE;

private:
    void loadPlugin();
    void refreshPluginItemsVisible();
    void updateDockHiddenSurfaceIds(bool shouldHide);
    void onConfigChanged(const QString &key);

private:
    PrivateLastoreItem *m_item;
    QSharedPointer<Dtk::Core::DConfig> m_dconfig;
    QSharedPointer<Dtk::Core::DConfig> m_dockTrayConfig;
    bool m_shouldShow;
    bool m_pluginLoaded;
};

#endif // PRIVATELASTOREPLUGIN_H
