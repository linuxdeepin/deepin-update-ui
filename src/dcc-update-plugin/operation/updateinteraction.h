// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef UPDATEINTERACTION_H
#define UPDATEINTERACTION_H

#include <QObject>

#include "updatemodel.h"
#include "updatework.h"

class UpdateInteraction : public QObject
{
    Q_OBJECT
public:
    explicit UpdateInteraction(QObject *parent = nullptr);

    Q_INVOKABLE UpdateModel *model() const { return m_model; }
    void setModel(UpdateModel *newModel);

    Q_INVOKABLE UpdateWorker *work() const { return m_work; }
    void setWork(UpdateWorker *newWork);

private:
    UpdateModel *m_model;
    UpdateWorker *m_work;
};

#endif // UPDATEINTERACTION_H
