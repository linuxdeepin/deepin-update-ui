// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "privatelastoreplugin.h"
#include "privatelastoreitem.h"
#include "qdbusconnection.h"
#include "common/global_util/public_func.h"

#include <DStandardPaths>

#define PRIVATE_LASTORE_KEY "private-lastore-key"
#define STATE_KEY "enabled"

DCORE_USE_NAMESPACE
Q_LOGGING_CATEGORY(dockPrivateUpdatePlugin, "org.deepin.dde.dock.update")

PrivateLastorePlugin::PrivateLastorePlugin(QObject *parent)
    : QObject(parent)
    , m_item(new PrivateLastoreItem)
    , m_pluginLoaded(false)
    , m_shouldShow(false)
    , m_dconfig(DConfig::create("org.deepin.dde.lastore", "org.deepin.dde.lastore", QString(), this))
    , m_dockTrayConfig(DConfig::create("org.deepin.dde.shell", "org.deepin.ds.dock.tray", QString(), this))
{
    QTranslator *translator = new QTranslator(this);
    const bool loaded = translator->load(QLocale(), "private-lastore-tray", "_", "/usr/share/private-lastore-tray/translations");
    if (!loaded) {
        qCWarning(dockPrivateUpdatePlugin) << "Failed to load private-lastore-tray translations";
    }
    QCoreApplication::installTranslator(translator);

    connect(m_dconfig.data(), &DConfig::valueChanged, this, &PrivateLastorePlugin::onConfigChanged);
}

PrivateLastorePlugin::~PrivateLastorePlugin()
{
    if (m_item) {
        delete m_item;
        m_item = nullptr;
    }
}

const QString PrivateLastorePlugin::pluginName() const
{
    return "private-lastore";
}

const QString PrivateLastorePlugin::pluginDisplayName() const
{
    return tr("Private Lastore");
}

void PrivateLastorePlugin::init(PluginProxyInterface *proxyInter)
{
    if (!m_dconfig) {
        qCWarning(dockPrivateUpdatePlugin) << "Failed to create DConfig for org.deepin.dde.lastore";
        return;
    }
    if (!m_dockTrayConfig) {
        qCWarning(dockPrivateUpdatePlugin) << "Failed to create DConfig for org.deepin.ds.dock.tray";
    }
    m_proxyInter = proxyInter;
    loadPlugin();

    m_shouldShow = m_dconfig->value("intranet-update", false).toBool();
    updateDockHiddenSurfaceIds(!m_shouldShow);
}

QWidget *PrivateLastorePlugin::itemWidget(const QString &itemKey)
{
    return m_item;
}
    
QWidget *PrivateLastorePlugin::itemTipsWidget(const QString &itemKey)
{
    return m_item->tipsWidget();
}

void PrivateLastorePlugin::loadPlugin()
{
    if (m_pluginLoaded) {
        return;
    }
    m_proxyInter->itemAdded(this, pluginName());
    m_proxyInter->saveValue(this, STATE_KEY, true);
    m_pluginLoaded = true;

    m_item->refreshTrayIcon();
}

void PrivateLastorePlugin::onConfigChanged(const QString &key)
{
    if (key == "intranet-update") {
        m_shouldShow = m_dconfig->value("intranet-update", false).toBool();
    }
    refreshPluginItemsVisible();
}

void PrivateLastorePlugin::refreshPluginItemsVisible()
{
    if (!m_shouldShow) {
        updateDockHiddenSurfaceIds(true);
    } else {
        if (!m_pluginLoaded) {
            loadPlugin();
            return;
        }
        updateDockHiddenSurfaceIds(false);
        m_proxyInter->itemAdded(this, pluginName());
    }
}

void PrivateLastorePlugin::updateDockHiddenSurfaceIds(bool shouldHide)
{
    const QString surfaceId = "private-lastore::private-lastore";
    QStringList hiddenIds = m_dockTrayConfig->value("dockHiddenSurfaceIds").toStringList();

    if (shouldHide) {
        // 添加到隐藏列表
        if (!hiddenIds.contains(surfaceId)) {
            hiddenIds.append(surfaceId);
            m_dockTrayConfig->setValue("dockHiddenSurfaceIds", hiddenIds);
            qCInfo(dockPrivateUpdatePlugin) << "Added" << surfaceId << "to dockHiddenSurfaceIds";
        }
    } else {
        // 从隐藏列表移除
        if (hiddenIds.contains(surfaceId)) {
            hiddenIds.removeAll(surfaceId);
            m_dockTrayConfig->setValue("dockHiddenSurfaceIds", hiddenIds);
            qCInfo(dockPrivateUpdatePlugin) << "Removed" << surfaceId << "from dockHiddenSurfaceIds";
        }
    }
}