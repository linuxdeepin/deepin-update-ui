// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "updateinteraction.h"
#include "dccfactory.h"

#include <QtQml/qqml.h>

UpdateInteraction::UpdateInteraction(QObject *parent)
    : QObject{ parent }
    , m_model(nullptr)
    , m_work(nullptr)
{
    m_model = new UpdateModel(this);
    m_work = new UpdateWorker(m_model, this);

    qmlRegisterType<UpdateWorker>("org.deepin.dcc.update", 1, 0, "UpdateWorker");
    qmlRegisterType<UpdateModel>("org.deepin.dcc.update", 1, 0, "UpdateModel");

    m_work->activate();
}

void UpdateInteraction::setModel(UpdateModel *newModel)
{
    m_model = newModel;
}

void UpdateInteraction::setWork(UpdateWorker *newWork)
{
    m_work = newWork;
}

DCC_FACTORY_CLASS(UpdateInteraction)

#include "updateinteraction.moc"
