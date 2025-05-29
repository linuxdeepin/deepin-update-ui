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

DCORE_USE_NAMESPACE

Manage::Manage(QObject *parent)
    : QObject(parent)
    , m_updateDBusProxy(new UpdateDBusProxy(this))
    , m_recoveryWidget(nullptr)
{
    // 满足配置条件,再判断是否满足恢复的条件
    // TODO : 恢复条件判断
    if (true) {
        qInfo() << "Recovery can restore, start recovery ...";
        recoveryCanRestore();
    } else {
        qInfo() << "No need to recovery, exit app";
        // 不满足配置条件,退出app
        exitApp(false);
    }
}

void Manage::showDialog()
{
    if (m_recoveryWidget)
        return;

    m_recoveryWidget = new RecoveryWidget();
    m_recoveryWidget->backupInfomation(DSysInfo::productVersion() , m_backupTime);

    connect(m_recoveryWidget, &RecoveryWidget::notifyButtonClicked, this, [ = ](bool state) {
        // 能够进入到弹框页面,说明是满足一切版本回退的条件
        // true: 确认 , 要恢复旧版本
        qInfo() << "Notify button clicked, state: " << state;
        if (state) {
            m_updateDBusProxy->ConfirmRollback(true);
            m_recoveryWidget->updateRestoringWaitUI();
        } else {
            // false: 取消并重启
            m_updateDBusProxy->ConfirmRollback(false);
        }
    });


    const QPointer<QScreen> primaryScreen = QGuiApplication::primaryScreen();
    if (primaryScreen) {
        auto moveRecoveryWidget2Center = [this, primaryScreen] {
            qInfo() << "Move recovery widget to center of the primary screen";
            if (!primaryScreen) {
                qWarning() << "Primary screen is nulptr";
                return;
            }
            qInfo() << "Primary screen's geometry is " << primaryScreen->geometry();
            QRect rect = m_recoveryWidget->geometry();
            rect.moveCenter(primaryScreen->geometry().center());
            m_recoveryWidget->move(rect.topLeft());
        };
        connect(primaryScreen, &QScreen::geometryChanged, this, moveRecoveryWidget2Center);
        moveRecoveryWidget2Center();
    } else {
        qWarning() << "Primary screen is nullptr";
    }

    m_recoveryWidget->setVisible(true);
    m_recoveryWidget->activateWindow();
    m_recoveryWidget->show();
}

void Manage::recoveryCanRestore()
{
    QDBusPendingCall call = m_updateDBusProxy->CanRollback();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, watcher] {
        QDBusPendingReply<bool, QString> reply = *watcher;
        watcher->deleteLater();
        if (!reply.isError()) {
            
            if (reply.count() >= 2) {
                bool canRollback = reply.argumentAt<0>();
                QString backupData = reply.argumentAt<1>();
                
                qInfo() << "Rollback status:" << canRollback << "Message:" << backupData;
                
                if (canRollback) {
                    QJsonDocument doc = QJsonDocument::fromJson(backupData.toUtf8());
                    QJsonObject obj = doc.object();
                    qlonglong backupTime = obj["time"].toVariant().toLongLong();
                    
                    m_backupTime = QDateTime::fromSecsSinceEpoch(backupTime).toString("yyyy/MM/dd hh:mm:ss");
                    showDialog();
                } else {
                    exitApp();
                }
            } else {
                // 不满足恢复条件,退出app
                qInfo() << "If ab recovery interface can restore: false, exitApp...";
                exitApp();
            }
        } else {
            qWarning() << "Call Recovery can restore error: " << reply.error().message();
            exitApp();
        }
    });
}

void Manage::exitApp(bool isExec)
{
    if (m_recoveryWidget) {
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
    QDBusPendingCall call = m_updateDBusProxy->Poweroff(false);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, call] {
        if (!call.isError()) {
            qInfo() << "Login1 manager interface reboot success";
        } else {
            qWarning() << "Login1 mnager interface reboot error: " << call.error().message();
        }
        exitApp();
    });
}

RecoveryWidget::RecoveryWidget(QWidget *parent)
    : QWidget(parent)
    , m_backupVersion("")
    , m_backupTime("")
    , m_restoreWidget(nullptr)
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setFixedSize(490, 220);
}

RecoveryWidget::~RecoveryWidget()
{

}

void RecoveryWidget::backupInfomation(QString version, QString time)
{
    if (m_backupVersion != version) {
        m_backupVersion = version;
    }

    if (m_backupTime != time) {
        m_backupTime = time;
    }

    initUI();
}

void RecoveryWidget::updateRestoringWaitUI()
{
    if (isVisible())
        setVisible(false);

    m_restoreWidget = new BackgroundWidget(true);
    m_restoreWidget->show();
}

void RecoveryWidget::destroyRestoringWaitUI()
{
    if (m_restoreWidget) {
        m_restoreWidget->hide();
        m_restoreWidget->deleteLater();
    }
}

void RecoveryWidget::updateRestoringFailedUI()
{
    removeContent();

    if (!isVisible())
        setVisible(true);

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
        Q_EMIT notifyButtonClicked(0);
    });
    btnLayout->addWidget(rebootBtn, Qt::AlignCenter);

    mainLayout->addSpacing(10);
    mainLayout->addLayout(btnLayout);

    this->setLayout(mainLayout);
}

void RecoveryWidget::initUI()
{
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
        qWarning() << "org.deepin.dde.lastore upgrade-status, json parse error: " << json_err.errorString();
        reasonMsg = tr("Updates failed.");
    } else {
        const QJsonObject &object = upgradeStatusMessage.object();
        const QString status = object.value("Status").toString();
        const QString reasonCode = object.value("ReasonCode").toString();

        qInfo() << "org.deepin.dde.lastore, upgrade-status: " << status << ", reason code: " << reasonCode;

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

    QPushButton *rebootBtn = new QPushButton();
    rebootBtn->setText(tr("Cancel and Reboot"));
    rebootBtn->setFixedHeight(45);
    DFontSizeManager::instance()->bind(rebootBtn, DFontSizeManager::T4, QFont::Normal);
    btnLayout->addWidget(rebootBtn, Qt::AlignCenter);
    connect(rebootBtn, &QPushButton::clicked, this, [ this ] {
        Q_EMIT notifyButtonClicked(0);
    });

    DVerticalLine *line = new DVerticalLine;
    line->setObjectName("VLine");
    line->setFixedHeight(30);
    btnLayout->addSpacing(6);
    btnLayout->addWidget(line, Qt::AlignCenter);
    btnLayout->addSpacing(6);

    DSuggestButton *confirmBtn = new DSuggestButton();
    confirmBtn->setText(tr("Confirm"));
    confirmBtn->setFixedHeight(45);
    DFontSizeManager::instance()->bind(confirmBtn, DFontSizeManager::T4, QFont::Normal);
    btnLayout->addWidget(confirmBtn, Qt::AlignCenter);
    connect(confirmBtn, &DSuggestButton::clicked, this, [ this ] {
        Q_EMIT notifyButtonClicked(1);
    });
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
    // 删除所有控件和单层子布局
    QLayout *layout = this->layout();

    if (!layout) {
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

    delete layout;
    layout = nullptr;
}

void RecoveryWidget::closeEvent(QCloseEvent *event)
{
    // 忽略关闭事件
    event->ignore();
}
