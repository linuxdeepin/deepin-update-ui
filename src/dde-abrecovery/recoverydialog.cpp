// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "recoverydialog.h"
#include "backgroundwidget.h"
#include "dconfig_helper.h"

#include <DLabel>
#include <DFontSizeManager>
#include <DSpinner>
#include <DStyle>
#include <DSysInfo>
#include <DSuggestButton>
#include <DTipLabel>
#include <DHiDPIHelper>
#include <DVerticalLine>

#include <QApplication>
#include <QWindow>
#include <QHBoxLayout>
#include <QScreen>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QDBusReply>
#include <DIcon>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logUpdateRecovery)

DCORE_USE_NAMESPACE

Manage::Manage(QObject *parent)
    : QObject(parent)
    , m_updateDBusProxy(new UpdateDBusProxy(this))
    , m_recoveryWidget(nullptr)
{
    qCDebug(logUpdateRecovery) << "Initialize recovery manager";
    // 满足配置条件,再判断是否满足恢复的条件
    // TODO : 恢复条件判断
    if (true) {
        qCInfo(logUpdateRecovery) << "Recovery can restore, start recovery process";
        recoveryCanRestore();
    } else {
        qCInfo(logUpdateRecovery) << "No need to recovery, exit app";
        // 不满足配置条件,退出app
        exitApp(false);
    }
}

void Manage::showDialog()
{
    qCDebug(logUpdateRecovery) << "Show recovery dialog requested";
    if (m_recoveryWidget) {
        qCDebug(logUpdateRecovery) << "Recovery widget already exists, returning";
        return;
    }

    qCDebug(logUpdateRecovery) << "Creating new recovery widget";
    m_recoveryWidget = new RecoveryWidget();
    m_recoveryWidget->backupInfomation(DSysInfo::productVersion() , m_backupTime);

    connect(m_recoveryWidget, &RecoveryWidget::notifyButtonClicked, this, [ = ](bool state) {
        // 能够进入到弹框页面,说明是满足一切版本回退的条件
        // true: 确认 , 要恢复旧版本
        qCInfo(logUpdateRecovery) << "Recovery widget button clicked, user choice:" << state;
        doConfirmRollback(state);
    });


    const QPointer<QScreen> primaryScreen = QGuiApplication::primaryScreen();
    if (primaryScreen) {
        qCDebug(logUpdateRecovery) << "Primary screen found, setting up centering";
        auto moveRecoveryWidget2Center = [this, primaryScreen] {
            qCDebug(logUpdateRecovery) << "Moving recovery widget to center of screen";
            if (!primaryScreen) {
                qCWarning(logUpdateRecovery) << "Primary screen is nullptr";
                return;
            }
            qCDebug(logUpdateRecovery) << "Screen geometry:" << primaryScreen->geometry();
            QRect rect = m_recoveryWidget->geometry();
            rect.moveCenter(primaryScreen->geometry().center());
            m_recoveryWidget->move(rect.topLeft());
        };
        connect(primaryScreen, &QScreen::geometryChanged, this, moveRecoveryWidget2Center);
        moveRecoveryWidget2Center();
    } else {
        qCWarning(logUpdateRecovery) << "Primary screen is nullptr";
    }

    qCDebug(logUpdateRecovery) << "Showing recovery widget";
    m_recoveryWidget->setVisible(true);
    m_recoveryWidget->activateWindow();
    m_recoveryWidget->show();
}

void Manage::recoveryCanRestore()
{
    qCDebug(logUpdateRecovery) << "Checking if recovery can restore";
    QDBusPendingCall call = m_updateDBusProxy->CanRollback();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
        qCDebug(logUpdateRecovery) << "CanRollback DBus call finished";
        QDBusPendingReply<bool, QString> reply = *watcher;
        watcher->deleteLater();
        if (!reply.isError()) {
            qCDebug(logUpdateRecovery) << "CanRollback call successful";
            if (reply.count() >= 2) {
                bool canRollback = reply.argumentAt<0>();
                QString backupData = reply.argumentAt<1>();
                
                qCInfo(logUpdateRecovery) << "Rollback status:" << canRollback << "backup data length:" << backupData.length();
                
                if (canRollback) {
                    qCDebug(logUpdateRecovery) << "Parsing backup data JSON";
                    QJsonDocument doc = QJsonDocument::fromJson(backupData.toUtf8());
                    QJsonObject obj = doc.object();
                    qlonglong backupTime = obj["time"].toVariant().toLongLong();
                    bool noConfirm = obj["auto"].toBool();
                    
                    m_backupTime = QDateTime::fromSecsSinceEpoch(backupTime).toString("yyyy/MM/dd hh:mm:ss");
                    qCDebug(logUpdateRecovery) << "Backup time:" << m_backupTime << "auto confirm:" << noConfirm << "show dialog";
                    
                    showDialog();

                    // 存在auto=true,就不需要用户确认是否回退，直接执行回退流程
                    if (noConfirm) {
                        qCInfo(logUpdateRecovery) << "Auto confirm enabled, starting rollback immediately" << "do confirm rollback";
                        doConfirmRollback(true);
                    }

                } else {
                    qCInfo(logUpdateRecovery) << "Cannot rollback, exiting application" << "exit app";
                    exitApp();
                }
            } else {
                // 不满足恢复条件,退出app
                qCWarning(logUpdateRecovery) << "Invalid reply count:" << reply.count() << "expected >= 2" << "exit app";
                exitApp();
            }
        } else {
            qCWarning(logUpdateRecovery) << "CanRollback DBus call failed:" << reply.error().message() << "exit app";
            exitApp();
        }
    });
}

void Manage::exitApp(bool isExec)
{
    qCDebug(logUpdateRecovery) << "Exit application requested, isExec:" << isExec;
    if (m_recoveryWidget) {
        qCDebug(logUpdateRecovery) << "Cleaning up recovery widget";
        delete m_recoveryWidget;
        m_recoveryWidget = nullptr;
    }

    if (isExec) {
        qApp->quit();
    } else {
        exit(0);
    }
}

void Manage::requestReboot()
{
    qCDebug(logUpdateRecovery) << "Requesting system reboot";
    QDBusPendingCall call = m_updateDBusProxy->Poweroff(false);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
        QDBusPendingReply<void> reply = watcher->reply();
        if (!reply.isError()) {
            qCInfo(logUpdateRecovery) << "System reboot request successful";
        } else {
            qCWarning(logUpdateRecovery) << "System reboot request failed:" << reply.error().message();
        }
        watcher->deleteLater();
        exitApp();
    });
}

void Manage::doConfirmRollback(bool confirm)
{
    qCDebug(logUpdateRecovery) << "Confirming rollback with user choice:" << confirm;
    if (confirm) {
        qCDebug(logUpdateRecovery) << "User confirmed rollback, updating UI to waiting state";
        m_recoveryWidget->updateRestoringWaitUI();
        QDBusPendingCall call = m_updateDBusProxy->ConfirmRollback(true);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
            QDBusPendingReply<void> reply = *watcher;
            if (reply.isError()) {
                qCWarning(logUpdateRecovery) << "Confirm rollback failed:" << reply.error().message();
            } else {
                qCInfo(logUpdateRecovery) << "Confirm rollback successful";
            }
            watcher->deleteLater();
            exitApp();
        });
    } else {
        qCInfo(logUpdateRecovery) << "User declined rollback, requesting reboot";
        // false: 取消并重启
        m_updateDBusProxy->ConfirmRollback(false);
    }
}

RecoveryWidget::RecoveryWidget(QWidget *parent)
    : QWidget(parent)
    , m_backupVersion("")
    , m_backupTime("")
    , m_restoreWidget(nullptr)
    , m_confirmBtn(nullptr)
    , m_rebootBtn(nullptr)
{
    qCDebug(logUpdateRecovery) << "Initialize RecoveryWidget dialog";
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setFixedSize(490, 220);
}

RecoveryWidget::~RecoveryWidget()
{
    qCDebug(logUpdateRecovery) << "Destroying RecoveryWidget";
}

void RecoveryWidget::backupInfomation(QString version, QString time)
{
    qCDebug(logUpdateRecovery) << "Setting backup information - version:" << version << "time:" << time;
    if (m_backupVersion != version) {
        qCDebug(logUpdateRecovery) << "Backup version changed from" << m_backupVersion << "to" << version;
        m_backupVersion = version;
    }

    if (m_backupTime != time) {
        qCDebug(logUpdateRecovery) << "Backup time changed from" << m_backupTime << "to" << time;
        m_backupTime = time;
    }

    qCDebug(logUpdateRecovery) << "Initializing UI with backup information";
    initUI();
}

void RecoveryWidget::updateRestoringWaitUI()
{
    qCDebug(logUpdateRecovery) << "Updating to restoring wait UI";
    if (isVisible()) {
        qCDebug(logUpdateRecovery) << "Hiding current recovery widget";
        setVisible(false);
    }

    qCDebug(logUpdateRecovery) << "Creating and showing background wait widget";
    m_restoreWidget = new BackgroundWidget(true);
    m_restoreWidget->show();
}

void RecoveryWidget::destroyRestoringWaitUI()
{
    qCDebug(logUpdateRecovery) << "Destroying restoring wait UI";
    if (m_restoreWidget) {
        qCDebug(logUpdateRecovery) << "Hiding and deleting restore widget";
        m_restoreWidget->hide();
        m_restoreWidget->deleteLater();
    }
}

void RecoveryWidget::updateRestoringFailedUI()
{
    qCDebug(logUpdateRecovery) << "Updating to restoring failed UI";
    removeContent();

    if (!isVisible()) {
        qCDebug(logUpdateRecovery) << "Making recovery widget visible for error display";
        setVisible(true);
    }

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(QMargins(12, 12, 12, 12));

    DLabel *iconLabel = new DLabel();
    iconLabel->setFixedSize(56, 56);
    iconLabel->setPixmap(DIcon::loadNxPixmap(":icon/images/dialog-warning.svg").scaled(QSize(56, 56), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    mainLayout->addWidget(iconLabel, Qt::AlignLeft);

    DLabel *msgLabel = new DLabel();
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);
    msgLabel->setContentsMargins(QMargins(10, 0, 10, 0));
    DFontSizeManager::instance()->bind(msgLabel, DFontSizeManager::T4, QFont::DemiBold);
    msgLabel->setText(tr("Rollback failed."));

    mainLayout->addSpacing(6);
    mainLayout->addWidget(msgLabel, Qt::AlignCenter);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(20, 0, 20, 0);
    QPushButton *rebootBtn = new QPushButton();
    rebootBtn->setText(tr("Reboot"));
    rebootBtn->setMinimumHeight(45);
    DFontSizeManager::instance()->bind(rebootBtn, DFontSizeManager::T4, QFont::Normal);
    connect(rebootBtn, &QPushButton::clicked, this, [ this ] {
        qCDebug(logUpdateRecovery) << "Reboot button clicked in failed UI";
        Q_EMIT notifyButtonClicked(0);
    });
    btnLayout->addWidget(rebootBtn, Qt::AlignCenter);

    mainLayout->addSpacing(10);
    mainLayout->addLayout(btnLayout);

    this->setLayout(mainLayout);
}

void RecoveryWidget::initUI()
{
    qCDebug(logUpdateRecovery) << "Initializing recovery widget UI";
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(QMargins(12, 12, 12, 12));

    DLabel *iconLabel = new DLabel();
    iconLabel->setFixedSize(56, 56);
    iconLabel->setPixmap(DIcon::loadNxPixmap(":icon/images/dialog-warning.svg").scaled(QSize(56, 56), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    mainLayout->addWidget(iconLabel, Qt::AlignLeft);

    DLabel *reasonLabel = new DLabel();
    reasonLabel->setAlignment(Qt::AlignCenter);
    reasonLabel->setWordWrap(true);
    reasonLabel->setContentsMargins(QMargins(10, 0, 10, 0));
    QString reasonMsg;
    DFontSizeManager::instance()->bind(reasonLabel, DFontSizeManager::T4, QFont::DemiBold);
    const QString upgradeStatus = DConfigHelper::instance()->getConfig("org.deepin.dde.lastore", "org.deepin.dde.lastore", "","upgrade-status", 0).toString();
    QJsonParseError json_err;
    QJsonDocument upgradeStatusMessage = QJsonDocument::fromJson(upgradeStatus.toUtf8(), &json_err);
    if (json_err.error != QJsonParseError::NoError) {
        qCWarning(logUpdateRecovery) << "org.deepin.dde.lastore upgrade-status, json parse error: " << json_err.errorString();
        reasonMsg = tr("Updates failed.");
    } else {
        const QJsonObject &object = upgradeStatusMessage.object();
        const QString status = object.value("Status").toString();
        const QString reasonCode = object.value("ReasonCode").toString();

        qCInfo(logUpdateRecovery) << "org.deepin.dde.lastore, upgrade-status: " << status << ", reason code: " << reasonCode;

        // running, 代表在更新过程中被中断
        if (status == "running") {
            // 如果reason是needCheck，更新已经完成，等待重启检查，此时进入ab界面，可能是手动触发
            if (reasonCode == "needCheck") {
                reasonMsg.clear();
            } else  {
                reasonMsg = tr("Updates failed: it was interrupted.");
            }
        } else if (status == "failed") {
            if (reasonCode == "dpkgError") {
                reasonMsg = tr("Updates failed: DPKG error.");
            } else if (reasonCode == "insufficientSpace") {
                reasonMsg = tr("Updates failed: insufficient disk space.");
            } else {
                reasonMsg = tr("Updates failed.");
            }
        } else {
            reasonMsg.clear();
        }
    }
    reasonLabel->setText(reasonMsg);
    mainLayout->addWidget(reasonLabel, Qt::AlignCenter);

    // 社区版需要显示'deepin 20',专业版显示'uos 20'
    QString systemVersion = DSysInfo::isCommunityEdition() ? "deepin " : "uos ";
    DTipLabel *tip = new DTipLabel(tr("Are you sure you want to roll back to %1 backed up on %2?").arg(m_backupTime).arg(systemVersion + m_backupVersion));
    DFontSizeManager::instance()->bind(tip, DFontSizeManager::T4, QFont::Light);
    tip->setWordWrap(true);
    tip->setContentsMargins(QMargins(5, 0, 5, 0));
    mainLayout->addWidget(tip, Qt::AlignCenter);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);

    m_rebootBtn = new QPushButton();
    m_rebootBtn->setText(tr("Cancel and Reboot"));
    m_rebootBtn->setFixedHeight(45);
    DFontSizeManager::instance()->bind(m_rebootBtn, DFontSizeManager::T4, QFont::Normal);
    btnLayout->addWidget(m_rebootBtn, Qt::AlignCenter);
    connect(m_rebootBtn, &QPushButton::clicked, this, [ this ] {
        Q_EMIT notifyButtonClicked(0);
    });

    DVerticalLine *line = new DVerticalLine;
    line->setObjectName("VLine");
    line->setFixedHeight(30);
    btnLayout->addSpacing(6);
    btnLayout->addWidget(line, Qt::AlignCenter);
    btnLayout->addSpacing(6);

    m_confirmBtn = new DSuggestButton();
    m_confirmBtn->setText(tr("Confirm"));
    m_confirmBtn->setFixedHeight(45);
    DFontSizeManager::instance()->bind(m_confirmBtn, DFontSizeManager::T4, QFont::Normal);
    btnLayout->addWidget(m_confirmBtn, Qt::AlignCenter);
    connect(m_confirmBtn, &DSuggestButton::clicked, this, [ this ] {
        qCDebug(logUpdateRecovery) << "Confirm button clicked in recovery dialog";
        Q_EMIT notifyButtonClicked(1);
    });
    
    // 设置按钮的 autoDefault 属性，这样有焦点的按钮会响应 Enter 键
    m_confirmBtn->setAutoDefault(true);
    m_rebootBtn->setAutoDefault(true);
    
    mainLayout->addSpacing(10);
    mainLayout->addLayout(btnLayout);

    this->setLayout(mainLayout);
}

void RecoveryWidget::mouseMoveEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    return;
}

void RecoveryWidget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_Escape:
        qCDebug(logUpdateRecovery) << "Escape key pressed, ignoring";
        break;
    default:
        QWidget::keyPressEvent(e);
    }
}

void RecoveryWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QColor backgroundColor(255, 255, 255);
    painter.setBrush(backgroundColor);

    painter.drawRoundedRect(rect(), 20, 20);
}

void RecoveryWidget::removeContent()
{
    qCDebug(logUpdateRecovery) << "Removing all content from recovery widget";
    // 删除所有控件和单层子布局
    QLayout *layout = this->layout();

    if (!layout) {
        qCDebug(logUpdateRecovery) << "No layout to remove";
        return;
    }

    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widgetItem = item->widget()) {
            delete widgetItem;
            widgetItem = nullptr;
        } else if (QLayout *childLayout = item->layout()) {
            while (QLayoutItem *childItem = childLayout->takeAt(0)) {
                childItem->widget()->deleteLater();
            }
        }
        delete item;
        item = nullptr;
    }

    qCDebug(logUpdateRecovery) << "Content removal completed, layout deleted";
    delete layout;
    layout = nullptr;
}

void RecoveryWidget::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    
    if (m_confirmBtn && event->reason() != Qt::TabFocusReason && event->reason() != Qt::BacktabFocusReason) {
        qCDebug(logUpdateRecovery) << "Setting focus to confirm button";
        m_confirmBtn->setFocus();
    }
}

void RecoveryWidget::closeEvent(QCloseEvent *event)
{
    // 忽略关闭事件
    event->ignore();
}
