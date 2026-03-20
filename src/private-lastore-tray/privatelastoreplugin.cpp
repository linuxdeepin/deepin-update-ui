// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "privatelastoreplugin.h"
#include "privatelastoreitem.h"
#include "qdbusconnection.h"
#include "common/global_util/public_func.h"

#include <DStandardPaths>
#include <DGuiApplicationHelper>

#define PRIVATE_LASTORE_KEY "private-lastore-key"
#define STATE_KEY "enabled"

DCORE_USE_NAMESPACE
DGUI_USE_NAMESPACE
Q_LOGGING_CATEGORY(dockPrivateUpdatePlugin, "org.deepin.dde.dock.update")

PrivateLastorePlugin::PrivateLastorePlugin(QObject *parent)
    : QObject(parent)
    , m_item(new PrivateLastoreItem)
{
    QTranslator *translator = new QTranslator(this);
    translator->load(QLocale(), "private-lastore-tray", "_", "/usr/share/private-lastore-tray/translations");
    QCoreApplication::installTranslator(translator);
}

PrivateLastorePlugin::~PrivateLastorePlugin()
{
    if (m_item) {
        delete m_item;
        m_item = nullptr;
    }
}

void PrivateLastorePlugin::loadPlugin()
{
    m_proxyInter->itemAdded(this, PRIVATE_LASTORE_KEY);
    displayModeChanged(displayMode());
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
    m_proxyInter = proxyInter;

    m_proxyInter->itemAdded(this, pluginName());
    m_proxyInter->saveValue(this, STATE_KEY, true);
}

QWidget *PrivateLastorePlugin::itemWidget(const QString &itemKey)
{
    return m_item;
}
    
QWidget *PrivateLastorePlugin::itemTipsWidget(const QString &itemKey)
{
    return m_item->tipsWidget();
}
