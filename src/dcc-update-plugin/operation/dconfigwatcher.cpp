// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dconfigwatcher.h"

#include <QListView>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVariant>
#include <QWidget>
#include <QDebug>
#include <QMetaEnum>
#include <QCoreApplication>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logDccUpdatePlugin)

using namespace DTK_NAMESPACE::Core;

/*
* 控制中心增加新的配置项时，需要注意：
* 1、配置项按照模块存放，如 更新模块的配置项存放在 dde.control-center.update.dconfig.json 文件中
* 2、二级菜单的配置项会和搜索关联，所以需要配置项的名称与二级菜单的文案保持一致
* 如： 鼠标模块 General 二级菜单，配置项目也需要定义为 General
*/


DConfigWatcher::DConfigWatcher(QObject *parent)
    : QObject(parent)
{
    qCDebug(logDccUpdatePlugin) << "Initialize DConfigWatcher";
    //通过模块枚举加载所有的文件，并从文件中获取所有的dconfig对象
    QMetaEnum metaEnum = QMetaEnum::fromType<ModuleType>();
    qCDebug(logDccUpdatePlugin) << "Loading" << metaEnum.keyCount() << "module configs";
    for (int i = 0; i <  metaEnum.keyCount(); i++) {
        const QString fileName = QString("org.deepin.dde.control-center.%1").arg(metaEnum.valueToKey(i));
        qCDebug(logDccUpdatePlugin) << "Creating config for module:" << fileName;
        DConfig *config = DConfig::create("org.deepin.dde.control-center", fileName, "", this);
        if (!config->isValid()) {
            qCWarning(logDccUpdatePlugin) << QString("DConfig is invalide, name: [%1], subpath: [%2]").arg(config->name(), config->subpath());
            continue;
        } else {
            qCDebug(logDccUpdatePlugin) << "Successfully loaded config for module:" << metaEnum.valueToKey(i);
            m_mapModulesConfig.insert(metaEnum.valueToKey(i), config);
            connect(config, &DConfig::valueChanged, this, [this, config](QString key) {
                auto moduleName = m_mapModulesConfig.key(config);
                qCDebug(logDccUpdatePlugin) << "Config value changed for module:" << moduleName << "key:" << key;
                int type = QMetaEnum::fromType<ModuleType>().keyToValue(moduleName.toStdString().c_str());
                onStatusModeChanged(static_cast<ModuleType>(type), key);
            });
        }
    }
}

DConfigWatcher *DConfigWatcher::instance()
{
    static DConfigWatcher *w = nullptr;
    if (w == nullptr) {
        qCDebug(logDccUpdatePlugin) << "Creating DConfigWatcher singleton instance";
        w = new DConfigWatcher();
        w->moveToThread(qApp->thread());
        w->setParent(qApp);
    }
    return w;
}

/**
 * @brief DConfigWatcher::bind 三级控件绑定gsettings
 * @param moduleType            模块类型
 * @param configName            key值
 * @param binder                控件指针
 */
void DConfigWatcher::bind(ModuleType moduleType, const QString &configName, QWidget *binder)
{
    qCDebug(logDccUpdatePlugin) << "Binding widget to config - module:" << moduleType << "key:" << configName;
    QString moduleName;
    if (!existKey(moduleType, configName, moduleName)) {
        qCWarning(logDccUpdatePlugin) << "Config key does not exist, cannot bind widget";
        return;
    }

    //添加key值到map中
    ModuleKey *key = new ModuleKey();
    key->key = configName;
    key->type = moduleType;
    //在包含的情况下去
    m_thirdMap.insert(key, binder);
    setStatus(moduleName, configName, binder);
    qCDebug(logDccUpdatePlugin) << "Successfully bound widget to config";

    // 自动解绑
    connect(binder, &QObject::destroyed, this, [ = ] {
        qCDebug(logDccUpdatePlugin) << "Widget destroyed, auto unbinding config";
        if (m_thirdMap.values().contains(binder))
            erase(m_thirdMap.key(binder)->type, m_thirdMap.key(binder)->key);
    });
}

/**
 * @brief DConfigWatcher::bind 二级菜单绑定gsettings
 * @param moduleType            模块类型
 * @param configName            key值
 * @param viewer                listview指针
 * @param item                  item指针
 */
void DConfigWatcher::bind(ModuleType moduleType, const QString &configName, QListView *viewer, QStandardItem *item)
{
    qCDebug(logDccUpdatePlugin) << "Binding ListView to config - module:" << moduleType << "key:" << configName;
    QString moduleName;
    if (!existKey(moduleType, configName, moduleName)) {
        qCWarning(logDccUpdatePlugin) << "Config key does not exist for ListView binding";
        return;
    }

    //添加key值到map中
    ModuleKey *key = new ModuleKey();
    key->key = configName;
    key->type = moduleType;
    //在包含的情况下去
    m_menuMap.insert(key, QPair<QListView *, QStandardItem *>(viewer, item));
    setStatus(moduleName, configName, viewer, item);
    qCDebug(logDccUpdatePlugin) << "Successfully bound ListView to config";

    // 自动解绑
    connect(viewer, &QListView::destroyed, this, [ = ] {
        qCDebug(logDccUpdatePlugin) << "ListView destroyed, auto erasing config binding";
        erase(moduleType, configName);
    });
}

/**
 * @brief DConfigWatcher::erase 清楚map中已被析构的值
 * @param moduleType             模块类型
 * @param configName             key值
 */
void DConfigWatcher::erase(ModuleType moduleType, const QString &configName)
{
    qCDebug(logDccUpdatePlugin) << "Erasing config bindings - module:" << moduleType << "key:" << configName;
    auto lst = m_thirdMap.keys();
    for (auto k : lst) {
        if (k->key == configName && k->type == moduleType) {
            m_thirdMap.remove(k);
        }
    }

    auto lst1 = m_menuMap.keys();
    for (auto k : lst1) {
        if (k->key == configName && k->type == moduleType) {
            m_menuMap.remove(k);
        }
    }
}

/**
 * @brief DConfigWatcher::erase erase重载，指定删除特定key
 * @param moduleType             模块类型
 * @param configName             key值
 * @param binder                 控件指针
 */
void DConfigWatcher::erase(ModuleType moduleType, const QString &configName, QWidget *binder)
{
    qCDebug(logDccUpdatePlugin) << "Erasing specific widget binding - module:" << moduleType << "key:" << configName;
    auto lst = m_thirdMap.keys();
    for (auto k : lst) {
        if (k->key == configName && k->type == moduleType) {
            m_thirdMap.remove(k, binder);
        }
    }
}

/**
 * @brief DConfigWatcher::insertState 插入二级菜单初始状态
 * @param moduleType                   模块类型
 * @param key                          key值
 */
void DConfigWatcher::insertState(ModuleType moduleType, const QString &key)
{
    qCDebug(logDccUpdatePlugin) << "Inserting menu state - module:" << moduleType << "key:" << key;
    QString moduleName;
    if (!existKey(moduleType, key, moduleName)) {
        qCWarning(logDccUpdatePlugin) << "Cannot insert state, key does not exist";
        return;
    }

    ModuleKey *keys = new ModuleKey();
    keys->key = key;
    keys->type = moduleType;
    m_menuState.insert(keys, m_mapModulesConfig[moduleName]->value(key).toBool());
}

/**
 * @brief DConfigWatcher::setStatus 设置三级控件状态
 * @param moduleType                 模块类型
 * @param configName                 key值
 * @param binder                     控件指针
 */
void DConfigWatcher::setStatus(QString &moduleName, const QString &configName, QWidget *binder)
{
    qCDebug(logDccUpdatePlugin) << "Setting widget status - module:" << moduleName << "key:" << configName;
    if (!binder) {
        qCWarning(logDccUpdatePlugin) << "Cannot set status, binder is null";
        return;
    }

    const QString setting = m_mapModulesConfig[moduleName]->value(configName).toString();
    qCDebug(logDccUpdatePlugin) << "Widget setting value:" << setting;

    if ("Enabled" == setting) {
        binder->setEnabled(true);
        binder->update();
    } else if ("Disabled" == setting) {
        binder->setEnabled(false);
        binder->update();
    }

    binder->setVisible("Hidden" != setting);
}

/**
 * @brief DConfigWatcher::setStatus 设置二级菜单状态
 * @param moduleName                 模块名称
 * @param configName              key值
 * @param viewer                     listview指针
 * @param item                       item指针
 */
void DConfigWatcher::setStatus(QString &moduleName, const QString &configName, QListView *viewer, QStandardItem *item)
{
    qCDebug(logDccUpdatePlugin) << "Setting ListView status - module:" << moduleName << "key:" << configName;
    bool visible = m_mapModulesConfig[moduleName]->value(configName).toBool();
    viewer->setRowHidden(item->row(), !visible);

    if (visible)
        Q_EMIT requestShowSecondMenu(item->row());
    else
        Q_EMIT requestUpdateSecondMenu(item->row(), configName);

    Q_EMIT notifyDConfigChanged(moduleName, configName);
}

/**
 * @brief DConfigWatcher::getStatus 获取三级控件状态
 * @param moduleType                 模块类型
 * @param configName                 key值
 * @return
 */
const QString DConfigWatcher::getStatus(ModuleType moduleType, const QString &configName)
{
    qCDebug(logDccUpdatePlugin) << "Getting status - module:" << moduleType << "key:" << configName;
    QString moduleName;
    if (!existKey(moduleType, configName, moduleName)) {
        qCWarning(logDccUpdatePlugin) << "Cannot get status, key does not exist";
        return "";
    }
    return m_mapModulesConfig[QMetaEnum::fromType<ModuleType>().valueToKey(moduleType)]->value(configName).toString();
}

/**
 * @brief DConfigWatcher::getValue   获取三级控件状态
 * @param moduleType                 模块类型
 * @param configName                 key值
 * @return
 */
const QVariant DConfigWatcher::getValue(DConfigWatcher::ModuleType moduleType, const QString &configName)
{
    qCDebug(logDccUpdatePlugin) << "Getting value - module:" << moduleType << "key:" << configName;
    QString moduleName;
    if (!existKey(moduleType, configName, moduleName)) {
        qCWarning(logDccUpdatePlugin) << "Cannot get value, key does not exist";
        return QVariant();
    }
    return m_mapModulesConfig[QMetaEnum::fromType<ModuleType>().valueToKey(moduleType)]->value(configName);
}

/**
 * @brief DConfigWatcher::getMenuState
 * @return second menu state
 */
QMap<DConfigWatcher::ModuleKey *, bool> DConfigWatcher::getMenuState()
{
    qCDebug(logDccUpdatePlugin) << "Getting menu state, count:" << m_menuState.size();
    return m_menuState;
}

/**
 * @brief DConfigWatcher::setValue   获取三级控件状态
 * @param moduleType                 模块类型
 * @param configName                 key值
 * @param data                       设置的值
 * @return
 */
void DConfigWatcher::setValue(DConfigWatcher::ModuleType moduleType, const QString &configName, QVariant data)
{
    qCDebug(logDccUpdatePlugin) << "Setting value - module:" << moduleType << "key:" << configName << "value:" << data;
    QString moduleName;
    if (!existKey(moduleType, configName, moduleName)) {
        qCWarning(logDccUpdatePlugin) << "Cannot set value, key does not exist";
        return;
    }
    m_mapModulesConfig[QMetaEnum::fromType<ModuleType>().valueToKey(moduleType)]->setValue(configName, data);
}

/**
 * @brief 设置控件对应的显示类型
 * @param moduleType                 模块类型
 * @param key                        key值
 */
void DConfigWatcher::onStatusModeChanged(ModuleType moduleType, const QString &key)
{
    qCDebug(logDccUpdatePlugin) << "Status mode changed - module:" << moduleType << "key:" << key;
    QString moduleName;
    if (!existKey(moduleType, key, moduleName)) {
        qCWarning(logDccUpdatePlugin) << "Cannot handle status change, key does not exist";
        return;
    }

    // 重新设置控件对应的显示类型
    for (auto mapUnit = m_thirdMap.begin(); mapUnit != m_thirdMap.end(); ++mapUnit) {
        if (key == mapUnit.key()->key && moduleType == mapUnit.key()->type) {
            setStatus(moduleName, key, mapUnit.value());
        }
    }

    for (auto nameKey : m_menuMap.keys()) {
        if (key == nameKey->key && moduleType == nameKey->type) {
            setStatus(moduleName, key, m_menuMap.value(nameKey).first, m_menuMap.value(nameKey).second);
            break;
        }
    }
    insertState(moduleType, key);
    ModuleKey *keys = new ModuleKey();
    keys->key = key;
    keys->type = moduleType;
    Q_EMIT requestUpdateSearchMenu(moduleName + key, m_menuState.value(keys));
    Q_EMIT notifyDConfigChanged(moduleName, key);
}


/**
 * @brief 判断对应的key值是否存在
 * @param moduleType                 模块类型
 * @param key                        key值
 * @param moduleName                 模块名称
 * @return key is exist
 */
bool DConfigWatcher::existKey(ModuleType moduleType, const QString &key, QString &moduleName)
{
    qCDebug(logDccUpdatePlugin) << "Checking key existence - module:" << moduleType << "key:" << key << "module name: " << moduleName;
    moduleName = QMetaEnum::fromType<ModuleType>().valueToKey(moduleType);
    if (m_mapModulesConfig.keys().contains(moduleName)) {
        if (m_mapModulesConfig[moduleName]->keyList().contains(key)) {
            return true;
        } else {
            qCDebug(logDccUpdatePlugin) << "Key not found in module config";
        }
    } else {
        qCWarning(logDccUpdatePlugin) << "Module config not found in map";
    }
    return false;
}

DConfig *DConfigWatcher::getModulesConfig(ModuleType moduleType)
{
    qCDebug(logDccUpdatePlugin) << "Getting modules config for type:" << moduleType;
    QMetaEnum metaEnum = QMetaEnum::fromType<ModuleType>();
    QString key = metaEnum.key(moduleType);
    qCDebug(logDccUpdatePlugin) << "Module key:" << key;
    
    if (m_mapModulesConfig.contains(key)) {
        return m_mapModulesConfig.value(key);
    }
    qCWarning(logDccUpdatePlugin) << "No config found for module:" << key;
    return nullptr;
}
