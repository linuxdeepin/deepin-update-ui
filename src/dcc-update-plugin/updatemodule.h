// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "interface/moduleinterface.h"
#include "../module/common.h"

#include <QObject>
#include <QSharedPointer>
#include <QThread>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

using namespace DCC_NAMESPACE;

namespace dcc {
namespace update {
class UpdateWorker;
class UpdateModel;
class MirrorsWidget;
}
}

namespace DCC_NAMESPACE {
namespace update {
class UpdateWidget;
class MirrorsWidget;

class UpdateModule : public QObject, public ModuleInterface
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID ModuleInterface_iid FILE "update.json")
    Q_INTERFACES(DCC_NAMESPACE::ModuleInterface)
    Q_PROPERTY(QString description READ description)

public:
    UpdateModule(QObject *parent = nullptr);
    ~UpdateModule() override;
    virtual void preInitialize(bool sync = false, FrameProxyInterface::PushType = FrameProxyInterface::PushType::Normal) override;
    virtual void initialize() override;
    virtual const QString name() const override;
    virtual const QString displayName() const override;
    virtual QString translationPath() const override { return QStringLiteral(":/translations/dcc-update-plugin_%1.ts");}
    virtual void active() override;
    virtual void deactive() override;
    virtual int load(const QString &path) override;
    QStringList availPage() const override;
    virtual QString path() const override { return MAINWINDOW; }
    virtual QString follow() const override;
    virtual QIcon icon() const override { return QIcon::fromTheme(QString("dcc_nav_update")); }
#ifndef DISABLE_UPDATESEARCH
    virtual void addChildPageTrans() const override;
#else
    void addChildPageTrans() const;
#endif
    QString description() const;

private Q_SLOTS:
    void onNotifyDealMirrorWidget(bool state);

private:
#ifndef DISABLE_UPDATESEARCH
    void initSearchData() override;
#else
    void initSearchData();
#endif

private:
    void updateNotifyRedPoint();

private:
    dcc::update::UpdateModel *m_model;
    QSharedPointer<dcc::update::UpdateWorker> m_work;
    QPointer<UpdateWidget> m_updateWidget;
    MirrorsWidget *m_mirrorsWidget;
    QStringList versionTypeList;
    QSharedPointer<QThread> m_workThread;
};

}// namespace datetime
}// namespace DCC_NAMESPACE
