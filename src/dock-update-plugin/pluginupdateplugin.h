// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PLUGINUPDATEPLUGIN_H
#define PLUGINUPDATEPLUGIN_H

#include "constants.h"
#include "widget/tipswidget.h"
#include "widget/dockiconwidget.h"
#include "pluginsiteminterface_v2.h"

#include <QSharedPointer>
#include <QTimer>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <DConfig>
#include <qwidget.h>

DCORE_USE_NAMESPACE

enum class UpdateState {
    UpdatesAvailable, // 有可用更新
    UpdatesInstalled // 更新已安装需重启
};

class PluginUpdatePlugin : public QObject, PluginsItemInterfaceV2
{
    Q_OBJECT
    Q_INTERFACES(PluginsItemInterfaceV2)
    Q_PLUGIN_METADATA(IID "com.deepin.dock.PluginsItemInterface" FILE "plugin-update.json")

public:
    explicit PluginUpdatePlugin(QObject *parent = nullptr);

    const QString pluginName() const override;
    const QString pluginDisplayName() const override;
    void init(PluginProxyInterface *proxyInter) override;

    void pluginStateSwitched() override;
    bool pluginIsAllowDisable() override { return true; }
    bool pluginIsDisable() override;

    QWidget *itemWidget(const QString &itemKey) override;
    QWidget *itemTipsWidget(const QString &itemKey) override;
    const QString itemCommand(const QString &itemKey) override;
    const QString itemContextMenu(const QString &itemKey) override;
    void invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked) override;
    void displayModeChanged(const Dock::DisplayMode displayMode) override;

    int itemSortKey(const QString &itemKey) override;
    void setSortKey(const QString &itemKey, const int order) override;
    void pluginSettingsChanged() override;

private:
    void loadPlugin();
    void refreshPluginItemsVisible();
    void updateIconState();
    void saveUpdateState(UpdateState state);
    
    void onConfigChanged(const QString &key);
    
    void updateStateFromUpdateStatus();

private:
    bool m_pluginLoaded;
    QScopedPointer<DockIconWidget> m_dockIcon;
    QScopedPointer<DockUpdatePlugin::TipsWidget> m_tipsLabel;
    QSharedPointer<DConfig> m_dconfig;
    
    UpdateState m_currentState;
    int m_updateMode;
    QVariant m_updateStatus;
    bool m_shouldShow;
};

Q_DECLARE_METATYPE(UpdateState)

#endif // PLUGINUPDATEPLUGIN_H
