// SPDX-FileCopyrightText: 2019 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef CHECKUPGRADEWIDGET_H
#define CHECKUPGRADEWIDGET_H

#include "updatemodel.h"
#include "blurtransparentbutton.h"

#include <QObject>
#include <QFrame>
#include <QLabel>
#include <QPointer>
#include <QSpacerItem>

#include <dpicturesequenceview.h>
#include <DProgressBar>
#include <DWaterProgress>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

class CheckProgressWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CheckProgressWidget(QWidget *parent = nullptr);

public Q_SLOTS:
    void setValue(double value);

protected:
    bool event(QEvent *e) override;

private:
    QLabel *m_logo;
    QLabel *m_tip;
    Dtk::Widget::DPictureSequenceView *m_waitingView;
    Dtk::Widget::DProgressBar *m_progressBar;
    Dtk::Widget::DWaterProgress *m_waterProgress;
    QLabel *m_progressText;
};


class SuccessFrame : public QWidget
{
    Q_OBJECT
public:
    explicit SuccessFrame(QWidget *parent = nullptr);

private:
    bool eventFilter(QObject *o, QEvent *e) override;
    QString getSystemVersionAndEdition();

private:
    BlurTransparentButton *m_enterBtn;
};

class ErrorFrame : public QWidget
{
        Q_OBJECT
public:
    explicit ErrorFrame(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void createButtons(const QList<UpdateModel::UpdateAction> &actions);

private:
    QLabel *m_iconLabel;
    QLabel *m_title;
    QLabel *m_tips;
    QSpacerItem *m_titleSpacer;
    QVBoxLayout *m_mainLayout;
    QTimer *m_countDownTimer;
    int m_countDown;
    QList<QPushButton *> m_actionButtons;
    QSpacerItem *m_buttonSpacer;
    QPointer<QPushButton> m_checkedButton;
};

class CheckSystemWidget : public QWidget
{
    Q_OBJECT
public:
    static CheckSystemWidget* instance();

private:
    CheckSystemWidget(QWidget *parent);
    ~CheckSystemWidget();

    void initUI();
    void initConnections();

private:
    CheckProgressWidget *m_checkProgressWidget;
};

#endif
