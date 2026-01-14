// SPDX-FileCopyrightText: 2015 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UPDATEWIDGET_H
#define UPDATEWIDGET_H

#include "updatemodel.h"

#include <QFrame>
#include <QWidget>
#include <QPushButton>
#include <QStackedLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QSpacerItem>
#include <QPointer>
#include <QScrollArea>
#include <QPlainTextEdit>

#include <DSpinner>
#include <DLabel>
#include <DIconButton>
#include <DFloatingButton>
#include <DProgressBar>
#include <DPushButton>
#include <DCommandLinkButton>
#include <dpicturesequenceview.h>

class StyledPlainTextEdit : public QPlainTextEdit
{
public:
    explicit StyledPlainTextEdit(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {
        setFrameStyle(QFrame::NoFrame);
        setAttribute(Qt::WA_TranslucentBackground);
        viewport()->setAttribute(Qt::WA_TranslucentBackground);
        viewport()->setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);
        QRect r = viewport()->rect();
        QColor bg(0, 0, 0, 255 * 0.1);
        painter.setBrush(bg);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(r.adjusted(0, 0, -1, -1), 6, 6);

        QPlainTextEdit::paintEvent(event);
    }
};

class UpdateLogWidget: public QFrame
{
    Q_OBJECT
public:
    explicit UpdateLogWidget(QWidget *parent = nullptr);

Q_SIGNALS:
    void requestHideLogWidget();

public slots:
    void setLog(const QString &log);
    void appendLog(const QString &log);

private:
    void scrollToBottom();
    void showNotify(const QIcon &icon, const QString &text);
    void hideNotify();

private:
    QPlainTextEdit *m_logTextEdit;
    Dtk::Widget::DPushButton *m_exportButton;

    QWidget *m_notifyWidget;
    QLabel *m_notifyIconLabel;
    QLabel *m_notifyTextLabel;
    QTimer *m_notifyTimer;
};

class UpdatePrepareWidget : public QFrame
{
    Q_OBJECT
public:
    explicit UpdatePrepareWidget(QWidget *parent = nullptr);
    void showPrepare();

private:
    QLabel * m_tip;
    Dtk::Widget::DSpinner *m_spinner;
};

class UpdateProgressWidget : public QFrame
{
    Q_OBJECT
public:
    explicit UpdateProgressWidget(QWidget *parent = nullptr);
    void setValue(int value);

protected:
    bool event(QEvent *e) override;

private:
    QLabel *m_logo;
    QLabel *m_tip;
    Dtk::Widget::DPictureSequenceView  *m_waitingView;
    Dtk::Widget::DProgressBar *m_progressBar;
    QLabel *m_progressText;
    Dtk::Widget::DCommandLinkButton *m_showLogButton;
    UpdateLogWidget *m_logWidget;
    QSpacerItem *m_headSpacer;
    QWidget *m_contentWidget;
};

class UpdateCompleteWidget : public QFrame
{
    Q_OBJECT
public:
    explicit UpdateCompleteWidget(QWidget *parent = nullptr);
    void showResult(bool success, UpdateModel::UpdateError error = UpdateModel::UpdateError::NoError);

Q_SIGNALS:
    void requestShowLogWidget();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void showSuccessFrame();
    void showErrorFrame(UpdateModel::UpdateError error);
    void createButtons(const QList<UpdateModel::UpdateAction> &actions);
    void expendLogWidget();
    void collapseLogWidget();

private:
    QLabel *m_iconLabel;
    QLabel *m_title;
    QLabel *m_tips;
    QVBoxLayout *m_mainLayout;
    QTimer *m_countDownTimer;
    int m_countDown;
    QList<QPushButton *> m_actionButtons;
    Dtk::Widget::DCommandLinkButton *m_showLogButton;
    QPointer<QPushButton> m_checkedButton;
    QWidget *m_expendWidget;
    QVBoxLayout *m_expendLayout;
    UpdateLogWidget *m_logWidget;
    QWidget *m_contentWidget;
};

class UpdateWidget : public QFrame
{
    Q_OBJECT

public:
    static UpdateWidget* instance();
    void showUpdate();

signals:
    void updateExited();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void onUpdateStatusChanged(UpdateModel::UpdateStatus status);
    void showLogWidget();
    void hideLogWidget();
    void onJobProgressChanged(double value);

private:
    explicit UpdateWidget(QWidget *parent = nullptr);
    void initUi();
    void initConnections();
    void showChecking();
    void showProgress();
    void setMouseCursorVisible(bool visible);

private:
    UpdatePrepareWidget *m_prepareWidget;
    UpdateProgressWidget *m_progressWidget;
    UpdateCompleteWidget *m_updateCompleteWidget;
    UpdateLogWidget *m_logWidget;
    QStackedWidget *m_stackedWidget;
};

#endif // UPDATEWIDGET_H
