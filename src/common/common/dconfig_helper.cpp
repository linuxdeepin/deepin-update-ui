// SPDX-FileCopyrightText: 2015 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dconfig_helper.h"

#include <QDebug>
#include <QMetaMethod>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logCommon, "dde.update.common")

DCORE_USE_NAMESPACE

Q_GLOBAL_STATIC(DConfigHelper, dConfigWatcher)

DConfigHelper::DConfigHelper(QObject *parent)
    : QObject(parent)
{
    qCDebug(logCommon) << "Initialize DConfigHelper";
}

DConfigHelper *DConfigHelper::instance()
{
    return dConfigWatcher;
}

DConfig *DConfigHelper::initializeDConfig(const QString &appId,
                                          const QString &name,
                                          const QString &subpath)
{
    qCDebug(logCommon) << "Initialize DConfig - appId:" << appId 
                            << "name:" << name << "subpath:" << subpath;
    DConfig *dConfig = DConfig::create(appId, name, subpath, this);
    if (!dConfig) {
        qCWarning(logCommon) << "Create dconfig failed, appId: " << appId << ", name: " << name
                   << ", subpath: " << subpath;
        return nullptr;
    }

    m_dConfigs[packageDConfigPath(appId, name, subpath)] = dConfig;
    m_bindInfos[dConfig] = {};

    // 即时响应数据变化
    connect(dConfig, &DConfig::valueChanged, this, [this, dConfig](const QString &key) {
        const QVariant &value = dConfig->value(key);
        qCDebug(logCommon) << "DConfig value changed - key:" << key << "value:" << value;
        auto it = m_bindInfos.find(dConfig);
        if (it == m_bindInfos.end())
            return;

        auto itBindInfo = it.value().begin();
        for (; itBindInfo != it.value().end(); ++itBindInfo) {
            if (itBindInfo.value().contains(key)) {
                auto callbackIt = m_objCallbackMap.find(itBindInfo.key());
                if (callbackIt != m_objCallbackMap.end())
                    callbackIt.value()(key, value, itBindInfo.key());
            }
        }
    });

    return dConfig;
}

void DConfigHelper::bind(const QString &appId,
                         const QString &name,
                         const QString &subpath,
                         QObject *obj,
                         const QString &key,
                         OnPropertyChangedCallback callback)
{
    qCDebug(logCommon) << "Binding DConfig - appId:" << appId << "name:" << name << "key:" << key;
    if (!obj) {
        qCWarning(logCommon) << "Cannot bind, object is null";
        return;
    }

    DConfig *dConfig = dConfigObject(appId, name, subpath);
    if (!dConfig) {
        qCWarning(logCommon) << "DConfig object is null, cannot bind";
        return;
    }

    auto it = m_bindInfos.find(dConfig);
    if (it == m_bindInfos.end()) {
        qCWarning(logCommon) << "Cannot find bind info for DConfig";
        return;
    }

    QMap<QObject *, QStringList> &bindInfo = it.value();
    auto bindInfoIt = bindInfo.find(obj);
    if (bindInfoIt != bindInfo.end()) {
        if (!bindInfoIt.value().contains(key))
            bindInfoIt.value().append(key);
    } else {
        bindInfo[obj] = QStringList(key);
    }

    m_objCallbackMap.insert(obj, callback);
    connect(obj, &QObject::destroyed, this, [this, obj] {
        unBind(obj);
    });
    qCDebug(logCommon) << "Binding completed successfully";
}

void DConfigHelper::unBind(QObject *obj, const QString &key)
{
    qCDebug(logCommon) << "DConfig unbind key:" << key;
    if (!obj) {
        qCWarning(logCommon) << "Unbinding object is null";
        return;
    }

    bool objStillUseful = false;
    auto it = m_bindInfos.begin();
    for (; it != m_bindInfos.end(); ++it) {
        if (key.isEmpty()) {
            it->remove(obj);
        } else {
            // 移除key，移除完如果obj没有绑定了key了，那么把obj也移除掉
            auto it1 = it.value().find(obj);
            if (it1 != it.value().end()) {
                it1.value().removeAll(key);
                if (it1.value().isEmpty()) {
                    it->remove(obj);
                } else {
                    objStillUseful = true;
                }
            }
        }
    }

    if (key.isEmpty() || !objStillUseful)
        m_objCallbackMap.remove(obj);
}

QVariant DConfigHelper::getConfig(const QString &appId,
                                  const QString &name,
                                  const QString &subpath,
                                  const QString &key,
                                  const QVariant &defaultValue)
{
    qCDebug(logCommon) << "Getting config - appId:" << appId << "name:" << name << "key:" << key;
    DConfig *dConfig = dConfigObject(appId, name, subpath);
    if (!dConfig) {
        qCWarning(logCommon) << "DConfig object is null, returning default";
        return defaultValue;
    }

    if (!dConfig->keyList().contains(key)) {
        qCDebug(logCommon) << "Key not found in config, returning default";
        return defaultValue;
    }

    const QVariant &value = dConfig->value(key);
    return value;
}

void DConfigHelper::setConfig(const QString &appId,
                              const QString &name,
                              const QString &subpath,
                              const QString &key,
                              const QVariant &value)
{
    qCDebug(logCommon) << "Setting config - appId:" << appId << "name:" << name << "key:" << key << "value:" << value;
    DConfig *dConfig = dConfigObject(appId, name, subpath);
    if (!dConfig) {
        qCWarning(logCommon) << "Cannot set config, DConfig object is null";
        return;
    }

    if (!dConfig->keyList().contains(key)) {
        qCWarning(logCommon) << "Cannot set config, DConfig does not contain key:" << key;
        return;
    }

    dConfig->setValue(key, value);
}

DConfig *DConfigHelper::dConfigObject(const QString &appId,
                                      const QString &name,
                                      const QString &subpath)
{
    const QString &configPath = packageDConfigPath(appId, name, subpath);
    qCDebug(logCommon) << "Getting DConfig object for path:" << configPath;
    DConfig *dConfig = nullptr;
    if (m_dConfigs.contains(configPath)) {
        qCDebug(logCommon) << "Found existing DConfig object";
        dConfig = m_dConfigs.value(configPath);
    } else {
        qCDebug(logCommon) << "Creating new DConfig object";
        dConfig = initializeDConfig(appId, name, subpath);
    }

    return dConfig;
}

QString DConfigHelper::packageDConfigPath(const QString &appId,
                                          const QString &name,
                                          const QString &subpath) const
{
    return appId + name + subpath;
}
