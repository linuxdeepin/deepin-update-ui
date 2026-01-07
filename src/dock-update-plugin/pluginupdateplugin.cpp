// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pluginupdateplugin.h"

#include <DDBusSender>

#include <QIcon>
#include <QSettings>
#include <QJsonDocument>
#include <QProcess>
#include <QLocale>
#include <QStandardPaths>
#include <QDir>

#include <DGuiApplicationHelper>
#include <DStandardPaths>

#define PLUGIN_STATE_KEY "enable"

Q_LOGGING_CATEGORY(dockUpdatePlugin, "org.deepin.dde.dock.update")

constexpr auto UPDATE_STATE_KEY = "update-state-key";

constexpr auto MENU_OPEN_UPDATES = "OpenUpdates";
constexpr auto MENU_RESTART = "Restart";


DCORE_USE_NAMESPACE
using namespace DockUpdatePlugin;

PluginUpdatePlugin::PluginUpdatePlugin(QObject *parent)
    : QObject(parent)
    , m_pluginLoaded(false)
    , m_dockIcon(nullptr)
    , m_tipsLabel(new TipsWidget)
    , m_dconfig(DConfig::create("org.deepin.dde.lastore", "org.deepin.dde.lastore", QString(), this))
    , m_dockTrayConfig(DConfig::create("org.deepin.dde.shell", "org.deepin.ds.dock.tray", QString(), this))
    , m_currentState(UpdateState::UpdatesAvailable)
    , m_updateMode(0)
    , m_shouldShow(false)
{
    m_tipsLabel->setVisible(false);
    m_tipsLabel->setAccessibleName("plugin-update");

    QList<QString> translationDirs;
    const auto dataDirs = DStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const auto &path : dataDirs) {
        translationDirs << QDir(path).filePath("dock-update-plugin/translations");
    }

    const QList<QLocale> localeFallback{QLocale::system(), QLocale::c()};
    if (!DGuiApplicationHelper::loadTranslator("dock-update-plugin", translationDirs, localeFallback)) {
        qCWarning(dockUpdatePlugin) << "Failed to load dock-update-plugin translations via DGuiApplicationHelper";
    }
}

const QString PluginUpdatePlugin::pluginName() const
{
    return "plugin-update";
}

const QString PluginUpdatePlugin::pluginDisplayName() const
{
    return tr("Update status");
}

QWidget *PluginUpdatePlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_dockIcon.data();
}

QWidget *PluginUpdatePlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    
    switch (m_currentState) {
    case UpdateState::UpdatesAvailable:
        m_tipsLabel->setText(tr("Install updates to get the latest features and security improvements."));
        break;
    case UpdateState::UpdatesInstalled:
        m_tipsLabel->setText(tr("New updates are available, Restart to finish installing."));
        break;
    }
    
    return m_tipsLabel.data();
}

void PluginUpdatePlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    if (!m_dconfig) {
        qCWarning(dockUpdatePlugin) << "Failed to create DConfig for org.deepin.dde.lastore";
        return;
    }
    if (!m_dockTrayConfig) {
        qCWarning(dockUpdatePlugin) << "Failed to create DConfig for org.deepin.ds.dock.tray";
    }

    m_updateMode = m_dconfig->value("update-mode").toInt();
    m_updateStatus = m_dconfig->value("update-status");

    connect(m_dconfig.data(), &DConfig::valueChanged, this, &PluginUpdatePlugin::onConfigChanged);

    // 初始化时隐藏插件，将 surfaceId 添加到 dockHiddenSurfaceIds
    updateDockHiddenSurfaceIds(true);

    updateStateFromUpdateStatus();
    if (!pluginIsDisable()) {
        loadPlugin();
     }
}

void PluginUpdatePlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, PLUGIN_STATE_KEY, !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, true).toBool());
    refreshPluginItemsVisible();
}

bool PluginUpdatePlugin::pluginIsDisable()
{
    const auto lastoreDisabled = m_dconfig->value("lastore-daemon-status", 0).toInt() == 2;
    bool defaultValue = true;
    return lastoreDisabled || !m_proxyInter->getValue(this, PLUGIN_STATE_KEY, defaultValue).toBool();
}

const QString PluginUpdatePlugin::itemCommand(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    // 打开控制中心的更新页面
    return QString(
        "dbus-send --session --print-reply "
        "--dest=org.deepin.dde.ControlCenter1 "
        "/org/deepin/dde/ControlCenter1 "
        "org.deepin.dde.ControlCenter1.ShowModule "
        "string:update"
    );
}

const QString PluginUpdatePlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    
    QList<QVariant> items;
    
    if (m_currentState == UpdateState::UpdatesAvailable) {
        QMap<QString, QVariant> openUpdates;
        openUpdates["itemId"] = MENU_OPEN_UPDATES;
        openUpdates["itemText"] = tr("Open Update");
        openUpdates["isActive"] = true;
        items.push_back(openUpdates);
    } else if (m_currentState == UpdateState::UpdatesInstalled) {
        QMap<QString, QVariant> restart;
        restart["itemId"] = MENU_RESTART;
        restart["itemText"] = tr("Restart to finish installing");
        restart["isActive"] = true;
        items.push_back(restart);

        QMap<QString, QVariant> openUpdates;
        openUpdates["itemId"] = MENU_OPEN_UPDATES;
        openUpdates["itemText"] = tr("Open Update");
        openUpdates["isActive"] = true;
        items.push_back(openUpdates);
    }
    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;
    
    return QJsonDocument::fromVariant(menu).toJson();
}

void PluginUpdatePlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)
    if (menuId == MENU_OPEN_UPDATES) {
        // 更新设置
        DDBusSender()
            .service("org.deepin.dde.ControlCenter1")
            .interface("org.deepin.dde.ControlCenter1")
            .path("/org/deepin/dde/ControlCenter1")
            .method("ShowModule")
            .arg(QString("update"))
            .call();
    } else if (menuId == MENU_RESTART) {
        // 重启系统
        DDBusSender()
            .service("org.deepin.dde.ShutdownFront1")
            .interface("org.deepin.dde.ShutdownFront1")
            .path("/org/deepin/dde/ShutdownFront1")
            .method("Restart")
            .call();
    }
}

void PluginUpdatePlugin::displayModeChanged(const Dock::DisplayMode displayMode)
{
    Q_UNUSED(displayMode);
    
    if (!pluginIsDisable() && !m_dockIcon.isNull()) {
        m_dockIcon->update();
    }
}

int PluginUpdatePlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    return m_proxyInter->getValue(this, key, 7).toInt();
}

void PluginUpdatePlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    m_proxyInter->saveValue(this, key, order);
}

void PluginUpdatePlugin::pluginSettingsChanged()
{
    refreshPluginItemsVisible();
}

void PluginUpdatePlugin::loadPlugin()
{
    if (m_pluginLoaded) {
        return;
    }
    
    m_pluginLoaded = true;
    
    m_dockIcon.reset(new DockIconWidget);
    m_dockIcon->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    
    updateIconState();
    m_proxyInter->itemAdded(this, UPDATE_STATE_KEY);
    displayModeChanged(displayMode());
}

void PluginUpdatePlugin::refreshPluginItemsVisible()
{
    bool shouldShow = !pluginIsDisable() && m_shouldShow;

    if (!shouldShow) {
        if (m_pluginLoaded) {
            updateDockHiddenSurfaceIds(true);
        }
    } else {
        if (!m_pluginLoaded) {
            loadPlugin();
            return;
        }
        // m_shouldShow 为 true 时，从 dockHiddenSurfaceIds 移除插件 ID
        updateDockHiddenSurfaceIds(false);
        m_proxyInter->itemAdded(this, UPDATE_STATE_KEY);
    }
}

void PluginUpdatePlugin::updateIconState()
{
    if (!m_dockIcon) {
        return;
    }
    
    QString iconPath;
    switch (m_currentState) {
        case UpdateState::UpdatesAvailable:
            iconPath = "status-updates-available";
            break;
        case UpdateState::UpdatesInstalled:
            iconPath = "status-installed-restart-pending";
            break;
    }
    m_dockIcon->setIconPath(iconPath);
}

void PluginUpdatePlugin::saveUpdateState(UpdateState state)
{
    if (m_proxyInter) {
        m_proxyInter->saveValue(this, UPDATE_STATE_KEY, static_cast<int>(state));
    }
}

void PluginUpdatePlugin::onConfigChanged(const QString &key)
{    
    if (key == "update-mode") {
        m_updateMode = m_dconfig->value("update-mode", 0).toInt();
    } else if (key == "update-status") {
        m_updateStatus = m_dconfig->value("update-status", QVariant());
        qCInfo(dockUpdatePlugin) << "UpdateStatus changed to:" << m_updateStatus;
    }
    //对dconfig获取的数据进行解析，从而处理更新的状态和是否显示
    updateStateFromUpdateStatus();
    
    //根据是否显示，刷新插件的显示状态
    refreshPluginItemsVisible();
}

void PluginUpdatePlugin::updateStateFromUpdateStatus()
{
    if (m_updateStatus.isNull() || !m_updateStatus.isValid()) {
        qCWarning(dockUpdatePlugin) << "UpdateStatus data is null or invalid, cannot update state";
        return;
    }
    
    QJsonDocument jsonDoc;
    QJsonObject jsonObj;
    QJsonParseError error;
    jsonDoc = QJsonDocument::fromJson(m_updateStatus.toString().toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(dockUpdatePlugin) << "Failed to parse UpdateStatus JSON:" << error.errorString();
        return;
    }
    jsonObj = jsonDoc.object();
    
    if (!jsonObj.contains("UpdateStatus")) {
        qCWarning(dockUpdatePlugin) << "UpdateStatus field not found in data";
        return;
    }
    
    QJsonObject updateStatusObj = jsonObj["UpdateStatus"].toObject();
    
    QString systemUpgrade = updateStatusObj["system_upgrade"].toString();
    QString securityUpgrade = updateStatusObj["security_upgrade"].toString();
    
    qCInfo(dockUpdatePlugin) << "Parsing UpdateStatus - system_upgrade:" << systemUpgrade << "security_upgrade:" << securityUpgrade;
    
    bool shouldShow = (m_updateMode == 1 || m_updateMode == 5 || m_updateMode == 9 || m_updateMode == 13 || m_updateMode == 4) &&
     (systemUpgrade == "needReboot" || securityUpgrade == "needReboot" || systemUpgrade == "notDownload" || securityUpgrade == "notDownload");
    
    m_shouldShow = shouldShow;

    UpdateState newState = m_currentState;  
    if (systemUpgrade == "needReboot" || securityUpgrade == "needReboot") {
        newState = UpdateState::UpdatesInstalled;
    } else if (systemUpgrade == "notDownload" || securityUpgrade == "notDownload") {
        newState = UpdateState::UpdatesAvailable;
    }
    
    if (m_currentState != newState) {
        m_currentState = newState;
        saveUpdateState(m_currentState);
        updateIconState();
    }
    
    qCInfo(dockUpdatePlugin) << "Plugin should show:" << shouldShow;
    return;
}

void PluginUpdatePlugin::updateDockHiddenSurfaceIds(bool shouldHide)
{
    if (!m_dockTrayConfig) {
        qCWarning(dockUpdatePlugin) << "m_dockTrayConfig is null, cannot update dockHiddenSurfaceIds";
        return;
    }

    const QString surfaceId = "plugin-update::update-state-key";
    QStringList hiddenIds = m_dockTrayConfig->value("dockHiddenSurfaceIds").toStringList();

    if (shouldHide) {
        // 添加到隐藏列表
        if (!hiddenIds.contains(surfaceId)) {
            hiddenIds.append(surfaceId);
            m_dockTrayConfig->setValue("dockHiddenSurfaceIds", hiddenIds);
            qCInfo(dockUpdatePlugin) << "Added" << surfaceId << "to dockHiddenSurfaceIds";
        }
    } else {
        // 从隐藏列表移除
        if (hiddenIds.contains(surfaceId)) {
            hiddenIds.removeAll(surfaceId);
            m_dockTrayConfig->setValue("dockHiddenSurfaceIds", hiddenIds);
            qCInfo(dockUpdatePlugin) << "Removed" << surfaceId << "from dockHiddenSurfaceIds";
        }
    }
}