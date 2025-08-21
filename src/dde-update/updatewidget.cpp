// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "updatewidget.h"
#include "updateworker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QScrollArea>
#include <QScrollBar>
#include <QWindow>
#include <QTimer>
#include <QPlainTextEdit>
#include <QTextOption>
#include <QStandardPaths>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QStringConverter>

#include <DFontSizeManager>
#include <QKeyEvent>
#include <DPaletteHelper>
#include <DGuiApplicationHelper>
#include <DIcon>
#include <DSysInfo>
#include <DToolButton>
#include <DAnchors>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logUpdateModal)

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

const int BACKUP_BEGIN_PROGRESS = 0;
const int BACKUP_END_PROGRESS = 50;

#define NOTIFY_TIME_OUT 3000
#define NOTIFY_ICON_SIZE 22

UpdateLogWidget::UpdateLogWidget(QWidget *parent)
    : QFrame(parent)
    , m_logTextEdit(new QPlainTextEdit(this))
    , m_exportButton(new Dtk::Widget::DPushButton(tr("Export"), this))
    , m_notifyWidget(new QWidget(this))
    , m_notifyIconLabel(new QLabel(this))
    , m_notifyTextLabel(new QLabel(this))
    , m_notifyTimer(new QTimer(this))
{
    qCDebug(logUpdateModal) << "Initialize UpdateLogWidget";
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_logTextEdit->setWordWrapMode(QTextOption::WordWrap);
    m_logTextEdit->setFrameStyle(QFrame::NoFrame);
    m_logTextEdit->setContentsMargins(0, 0, 0, 0);
    m_logTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_logTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 禁止文本选择和右键菜单
    m_logTextEdit->setTextInteractionFlags(Qt::NoTextInteraction);
    m_logTextEdit->setContextMenuPolicy(Qt::NoContextMenu);
    
    DFontSizeManager::instance()->bind(m_logTextEdit, DFontSizeManager::T6);
    auto font = m_logTextEdit->font();
    font.setWeight(QFont::ExtraLight);
    m_logTextEdit->setFont(font);

    QPalette palette = DGuiApplicationHelper::instance()->standardPalette(DGuiApplicationHelper::DarkType);
    palette.setColor(QPalette::Base, Qt::transparent);
    m_logTextEdit->setPalette(palette);
    m_logTextEdit->setAttribute(Qt::WA_TranslucentBackground);

    m_notifyIconLabel->setFixedSize(NOTIFY_ICON_SIZE, NOTIFY_ICON_SIZE);
    m_notifyIconLabel->setScaledContents(true);
    QHBoxLayout *notifyLayout = new QHBoxLayout(m_notifyWidget);
    notifyLayout->setContentsMargins(0, 5, 0, 5);
    notifyLayout->addWidget(m_notifyIconLabel, 0, Qt::AlignVCenter);
    notifyLayout->addSpacing(5);
    notifyLayout->addWidget(m_notifyTextLabel, 0, Qt::AlignVCenter);
    m_notifyWidget->setVisible(false);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addStretch();
    mainLayout->addWidget(m_logTextEdit, 1);
    mainLayout->addStretch();
    mainLayout->addSpacing(10);
    m_exportButton->setMinimumWidth(136);
    mainLayout->addWidget(m_exportButton, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_notifyWidget, 0, Qt::AlignHCenter);

    connect(m_exportButton, &DPushButton::clicked, UpdateWorker::instance(), &UpdateWorker::exportUpdateLog);
    connect(UpdateWorker::instance(), &UpdateWorker::exportUpdateLogFinished, this, [this](bool success) {
        if (success) {
            showNotify(QIcon::fromTheme("sp_ok"), tr("The log has been exported to the desktop"));
        } else {
            showNotify(QIcon::fromTheme("sp_warning"), tr("Log export failed, please try again"));
        }
    });

    connect(UpdateModel::instance(), &UpdateModel::updateLogChanged, this, &UpdateLogWidget::setLog);
    connect(UpdateModel::instance(), &UpdateModel::updateLogAppended, this, &UpdateLogWidget::appendLog);
    connect(m_notifyTimer, &QTimer::timeout, this, &UpdateLogWidget::hideNotify);
}

void UpdateLogWidget::setLog(const QString &log)
{
    qCDebug(logUpdateModal) << "setLog" << log;
    m_logTextEdit->setPlainText(log);
    scrollToBottom();
}

void UpdateLogWidget::appendLog(const QString &log)
{
    qCDebug(logUpdateModal) << "append log: " << log;
    QString cleanLog = log;
    if (cleanLog.endsWith('\n')) {
        cleanLog.chop(1);  // 移除最后一个换行符，appendPlainText会自动换行
    }
    m_logTextEdit->appendPlainText(cleanLog);
    scrollToBottom();
}

void UpdateLogWidget::scrollToBottom()
{
    if (m_logTextEdit && m_logTextEdit->verticalScrollBar()) {
        QScrollBar *verticalScrollBar = m_logTextEdit->verticalScrollBar();
        
        // 检查当前是否已经在底部（允许小的误差范围）
        const int tolerance = 5; // 像素容差，处理可能的四舍五入误差
        bool isAtBottom = verticalScrollBar->value() >= (verticalScrollBar->maximum() - tolerance);
        qCDebug(logUpdateModal) << "Scroll to bottom check, is at bottom:" << isAtBottom;
        
        // 只有当前在底部时才自动滚动到新的底部
        if (isAtBottom) {
            // 使用 QTimer::singleShot 确保在文本更新后再滚动
            QTimer::singleShot(0, this, [this]() {
                QScrollBar *verticalScrollBar = m_logTextEdit->verticalScrollBar();
                verticalScrollBar->setValue(verticalScrollBar->maximum());
                qCDebug(logUpdateModal) << "Scrolled to bottom";
            });
        }
    }
}

void UpdateLogWidget::showNotify(const QIcon &icon, const QString &text)
{
    qCDebug(logUpdateModal) << "Show notification:" << text;
    m_notifyIconLabel->setPixmap(icon.pixmap(QSize(NOTIFY_ICON_SIZE, NOTIFY_ICON_SIZE)));
    m_notifyTextLabel->setText(text);
    m_exportButton->setVisible(false);
    m_notifyWidget->setVisible(true);
    m_notifyTimer->start(NOTIFY_TIME_OUT);
}

void UpdateLogWidget::hideNotify()
{
    qCDebug(logUpdateModal) << "Hide notification";
    if (m_notifyTimer->isActive()) {
        m_notifyTimer->stop();
    }
    m_notifyWidget->setVisible(false);
    m_exportButton->setVisible(true);
}

UpdatePrepareWidget::UpdatePrepareWidget(QWidget *parent) : QFrame(parent)
{
    qCDebug(logUpdateModal) << "Initialize UpdatePrepareWidget";
    m_tip = new QLabel(this);
    m_tip->setText(tr("Preparing for updates…"));
    DFontSizeManager::instance()->bind(m_tip, DFontSizeManager::T6);

    m_spinner = new Dtk::Widget::DSpinner(this);
    m_spinner->setFixedSize(72, 72);

    QVBoxLayout *pLayout = new QVBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addStretch();
    pLayout->addWidget(m_spinner, 0, Qt::AlignCenter);
    pLayout->addSpacing(30);
    pLayout->addWidget(m_tip, 0, Qt::AlignCenter);
    pLayout->addStretch();
}

void UpdatePrepareWidget::showPrepare()
{
    qCDebug(logUpdateModal) << "Show prepare widget";
    m_spinner->setVisible(true);
    m_spinner->start();
    m_tip->setVisible(true);
}

UpdateProgressWidget::UpdateProgressWidget(QWidget *parent)
    : QFrame(parent)
    , m_logo(new QLabel(this))
    , m_tip(new QLabel(this))
    , m_waitingView(new DPictureSequenceView(this))
    , m_progressBar(new DProgressBar(this))
    , m_progressText(new QLabel(this))
    , m_showLogButton(new Dtk::Widget::DCommandLinkButton(tr("View update logs"), this))
    , m_logWidget(new UpdateLogWidget(this))
    , m_headSpacer(new QSpacerItem(0, 0))
    , m_contentWidget(new QWidget(this))
{
    m_logo->setFixedSize(286, 57);
    if (DSysInfo::uosEditionType() == DSysInfo::UosCommunity) {
        qCDebug(logUpdateModal) << "Using Deepin community logo";
        m_logo->setPixmap(DIcon::loadNxPixmap(":img/deepin_logo.svg"));
    } else {
        qCDebug(logUpdateModal) << "Using UOS logo";
        m_logo->setPixmap(DIcon::loadNxPixmap(":img/uos_logo.svg"));
    }

    auto palette = m_tip->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    m_tip->setPalette(palette);
    m_tip->setText(tr("Do not force a shutdown or power off when installing updates. Otherwise, your system may be damaged."));
    DFontSizeManager::instance()->bind(m_tip, DFontSizeManager::T6);

    m_waitingView->setAccessibleName("WaitingUpdateSequenceView");
    m_waitingView->setFixedSize(20 * qApp->devicePixelRatio(), 20 * qApp->devicePixelRatio());
    m_waitingView->setSingleShot(false);
    QStringList pics;
    for (int i = 0; i < 40; ++i)
        pics << QString(":img/waiting_update/waiting_update_%1.png").arg(QString::number(i));
    m_waitingView->setPictureSequence(pics, true);

    QHBoxLayout *tipsLayout = new QHBoxLayout;
    tipsLayout->setSpacing(0);
    tipsLayout->addStretch();
    tipsLayout->addWidget(m_tip);
    tipsLayout->addSpacing(5);
    tipsLayout->addWidget(m_waitingView);
    tipsLayout->addStretch();

    m_progressBar->setFixedWidth(500);
    m_progressBar->setFixedHeight(8);
    m_progressBar->setRange(0, 100);
    m_progressBar->setAlignment(Qt::AlignRight);
    m_progressBar->setAccessibleName("ProgressBar");
    m_progressBar->setValue(1);
    DPaletteHelper::instance()->setPalette(m_progressBar, DGuiApplicationHelper::instance()->standardPalette(DGuiApplicationHelper::DarkType));

    m_progressText->setText("1%");
    DFontSizeManager::instance()->bind(m_progressText, DFontSizeManager::T6);

    QHBoxLayout *pProgressLayout = new QHBoxLayout;
    pProgressLayout->addStretch();
    pProgressLayout->addWidget(m_progressBar, 0, Qt::AlignCenter);
    pProgressLayout->addSpacing(10);
    pProgressLayout->addWidget(m_progressText, 0, Qt::AlignCenter);
    pProgressLayout->addStretch();

    QVBoxLayout *pLayout = new QVBoxLayout(m_contentWidget);
    pLayout->setSpacing(0);
    pLayout->addItem(m_headSpacer);
    pLayout->addWidget(m_logo, 0, Qt::AlignCenter);
    pLayout->addSpacing(48);
    pLayout->addLayout(pProgressLayout, 0);
    pLayout->addSpacing(10);
    pLayout->addLayout(tipsLayout, 0);
    pLayout->addSpacing(10);
    pLayout->addWidget(m_showLogButton, 0, Qt::AlignCenter);

    auto mainAnchors = new DAnchors<QWidget>(this);
    auto contentAnchors = new DAnchors<QWidget>(m_contentWidget);
    contentAnchors->setCenterIn(mainAnchors);

    auto logWidgetAnchors = new DAnchors<QWidget>(m_logWidget);
    m_logWidget->setFixedWidth(718);
    m_logWidget->setVisible(false);
    logWidgetAnchors->setTop(contentAnchors->bottom());
    logWidgetAnchors->setHorizontalCenter(mainAnchors->horizontalCenter());
    logWidgetAnchors->setBottom(mainAnchors->bottom());
    logWidgetAnchors->setBottomMargin(30);


    connect(m_showLogButton, &DCommandLinkButton::clicked, this, [this] {
        bool toShow = !m_logWidget->isVisible();
        qCDebug(logUpdateModal) << "Toggle log widget visibility to:" << toShow;
        m_logWidget->setVisible(toShow);
        m_showLogButton->setText(toShow ? tr("Collapse update logs") : tr("View update logs"));
        m_contentWidget->adjustSize();
    });
}

bool UpdateProgressWidget::event(QEvent *e)
{
    if (e->type() == QEvent::Show)
        m_waitingView->play();
    else if (e->type() == QEvent::Hide)
        m_waitingView->stop();

    return false;
}

void UpdateProgressWidget::setValue(int value)
{
    qCDebug(logUpdateModal) << "Setting progress value to:" << value;
    // 进度条不能大于100，不能小于0，不能回退
    if (value > 100 || value < 0 || value <= m_progressBar->value())
        return;

    qInfo() << "Update progress value: " << value;
    m_progressBar->setValue(value);
    m_progressText->setText(QString::number(value) + "%");
}

UpdateCompleteWidget::UpdateCompleteWidget(QWidget *parent)
    : QFrame(parent)
    , m_iconLabel(new QLabel(this))
    , m_title(new QLabel(this))
    , m_tips(new QLabel(this))
    , m_mainLayout(nullptr)
    , m_countDownTimer(nullptr)
    , m_countDown(3)
    , m_showLogButton(new Dtk::Widget::DCommandLinkButton(tr("View update logs"), this))
    , m_checkedButton(nullptr)
    , m_expendWidget(new QWidget(this))
    , m_logWidget(new UpdateLogWidget(this))
    , m_contentWidget(new QWidget(this))
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, Qt::white);
    setPalette(palette);

    m_iconLabel->setFixedSize(128, 128);

    m_title->setAlignment(Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(m_title, DFontSizeManager::T4);

    m_tips->setAlignment(Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(m_tips, DFontSizeManager::T6);

    m_mainLayout = new QVBoxLayout(m_contentWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(10);
    m_mainLayout->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_iconLabel, 0, Qt::AlignCenter);
    m_mainLayout->addWidget(m_title, 0, Qt::AlignCenter);
    m_mainLayout->addWidget(m_tips,0 , Qt::AlignCenter);
    m_mainLayout->addWidget(m_showLogButton,0 , Qt::AlignCenter);

    m_expendLayout = new QVBoxLayout(m_expendWidget);
    m_expendLayout->setContentsMargins(0, 0, 0, 0);
    m_expendLayout->setSpacing(10);
    m_expendLayout->addWidget(m_logWidget, 1, Qt::AlignHCenter);
    // 始终在底部保持一个弹簧，让按钮在顶部对齐
    m_expendLayout->addStretch();

    m_logWidget->setFixedWidth(718);

    auto mainAnchors = new DAnchors<QWidget>(this);
    auto contentAnchors = new DAnchors<QWidget>(m_contentWidget);
    contentAnchors->setCenterIn(mainAnchors);

    auto expendAnchors = new DAnchors<QWidget>(m_expendWidget);
    expendAnchors->setTop(contentAnchors->bottom());
    expendAnchors->setHorizontalCenter(mainAnchors->horizontalCenter());
    expendAnchors->setBottom(mainAnchors->bottom());
    expendAnchors->setLeft(mainAnchors->left());
    expendAnchors->setRight(mainAnchors->right());
    expendAnchors->setTopMargin(10);
    expendAnchors->setBottomMargin(30);

    collapseLogWidget();
    connect(m_showLogButton, &DCommandLinkButton::clicked, this, [this](){
        bool toShow = !m_logWidget->isVisible();
        if (toShow) {
            expendLogWidget();
            m_showLogButton->setText(tr("Collapse update logs"));
        } else {
            collapseLogWidget();
            m_showLogButton->setText(tr("View update logs"));
        }
        m_contentWidget->adjustSize();
    });
}

void UpdateCompleteWidget::showResult(bool success, UpdateModel::UpdateError error)
{
    qCDebug(logUpdateModal) << "Show update result - success:" << success << "error:" << error;
    if (success) {
        qCInfo(logUpdateModal) << "Update completed successfully";
        showSuccessFrame();
    } else {
        qCWarning(logUpdateModal) << "Update failed with error:" << error;
        showErrorFrame(error);
    }
}

void UpdateCompleteWidget::showSuccessFrame()
{
    qCDebug(logUpdateModal) << "Showing success frame with countdown";
    qDeleteAll(m_actionButtons);
    m_actionButtons.clear();
    m_mainLayout->invalidate();
    m_showLogButton->setVisible(false);

    m_iconLabel->setPixmap(DIcon::loadNxPixmap(":img/success.svg"));
    m_title->setText(tr("Updates successful"));
    m_countDown = 3;

    auto setTipsText = [this] {
        const QString &tips = UpdateModel::instance()->isReboot() ?
                tr("Your computer will reboot soon %1") : tr("Your computer will be turned off soon %1");
        m_tips->setText(tips.arg(QString::number(m_countDown)));
        m_tips->setVisible(true);
        m_countDown--;
    };

    if (m_countDownTimer == nullptr) {
        m_countDownTimer = new QTimer(this);
        m_countDownTimer->setInterval(1000);
        m_countDownTimer->setSingleShot(false);
        connect(m_countDownTimer, &QTimer::timeout, this, [this, setTipsText] {
            if (m_countDown <= 0) {
                UpdateWorker::instance()->doPowerAction(UpdateModel::instance()->isReboot());
                m_countDownTimer->stop();
                return;
            }
            setTipsText();
        });
    }

    setTipsText();
    m_countDownTimer->start();

    // 没有成功重启/关机的时候（小概率事件）显示强制重启/关机按钮
    QTimer::singleShot((m_countDown + 2) * 1000, this, [this] {
        m_tips->setText(UpdateModel::instance()->isReboot() ?
            tr("The automatic reboot process has failed. Please try to manually reboot your device.") :
            tr("The automatic shutdown process has failed. Please try to manually shut down your device."));
        auto button = new QPushButton(UpdateModel::instance()->isReboot() ?
            UpdateModel::instance()->updateActionText(UpdateModel::Reboot) :
            UpdateModel::instance()->updateActionText(UpdateModel::ShutDown), this);
        m_contentWidget->adjustSize();

        button->setFixedSize(240, 48);
        // 在 stretch 之前插入按钮（stretch 总是在最后一个位置）
        int insertIndex = m_expendLayout->count() - 1; // stretch 的位置
        m_expendLayout->insertWidget(insertIndex, button, 0, Qt::AlignHCenter);
        m_actionButtons.append(button);
        button->setFocusPolicy(Qt::NoFocus);
        button->setCheckable(true);
        m_mainLayout->invalidate();
        QApplication::setOverrideCursor(Qt::ArrowCursor);

        connect(button, &QPushButton::clicked, this, [] {
            UpdateWorker::instance()->forceReboot(UpdateModel::instance()->isReboot());
        });
    });

    collapseLogWidget();
    m_contentWidget->adjustSize();
}

void UpdateCompleteWidget::showErrorFrame(UpdateModel::UpdateError error)
{
    qCDebug(logUpdateModal) << "Showing error frame for error:" << error;

    static const QMap<UpdateModel::UpdateError, QList<UpdateModel::UpdateAction>> ErrorActions = {
        {UpdateModel::CanNotBackup, {UpdateModel::ContinueUpdating, UpdateModel::ExitUpdating}},
        {UpdateModel::BackupInterfaceError, {UpdateModel::ExitUpdating, UpdateModel::ContinueUpdating}},
        {UpdateModel::BackupFailedUnknownReason, {UpdateModel::DoBackupAgain, UpdateModel::ExitUpdating, UpdateModel::ContinueUpdating}},
        {UpdateModel::BackupNoSpace, {UpdateModel::ContinueUpdating, UpdateModel::ExitUpdating}},
        {UpdateModel::UpdateInterfaceError, {UpdateModel::Reboot, UpdateModel::ShutDown}},
        {UpdateModel::InstallNoSpace, {UpdateModel::Reboot, UpdateModel::ShutDown}},
        {UpdateModel::DependenciesBrokenError, {UpdateModel::Reboot, UpdateModel::ShutDown}},
        {UpdateModel::DpkgInterrupted, {UpdateModel::Reboot, UpdateModel::ShutDown}},
        {UpdateModel::UnKnown, {UpdateModel::Reboot, UpdateModel::ShutDown}}
    };

    m_iconLabel->setPixmap(DIcon::loadNxPixmap(":img/failed.svg"));
    const auto actions = ErrorActions.value(error);
    auto pair = UpdateModel::updateErrorMessage(error);
    m_title->setText(pair.first);
    m_tips->setVisible(!pair.second.isEmpty());
    m_tips->setText(pair.second);
    m_showLogButton->setVisible(error >= UpdateModel::UpdateInterfaceError);
    createButtons(actions);
    m_contentWidget->adjustSize();

    m_mainLayout->invalidate();

    // 安装失败后10分钟后自动关机
    if (error >= UpdateModel::UpdateInterfaceError) {
        QTimer::singleShot(1000*60*10, this, [] {
            qCInfo(logUpdateModal) << "User has not operated for a long time， do shut down action now";
            UpdateWorker::instance()->doPowerAction(false);
        });
    }

    collapseLogWidget();
}

void UpdateCompleteWidget::createButtons(const QList<UpdateModel::UpdateAction> &actions)
{
    qCDebug(logUpdateModal) << "Creating action buttons, count:" << actions.size();
    qDeleteAll(m_actionButtons);
    m_actionButtons.clear();

    m_checkedButton = nullptr;
    for (auto action : actions) {
        qCDebug(logUpdateModal) << "Creating button for action:" << action;
        auto button = new QPushButton(UpdateModel::updateActionText(action), this);
        button->setFixedSize(240, 48);
        // 在 stretch 之前插入按钮（stretch 总是在最后一个位置）
        int insertIndex = m_expendLayout->count() - 1; // stretch 的位置
        m_expendLayout->insertWidget(insertIndex, button, 0, Qt::AlignHCenter);
        m_actionButtons.append(button);
        button->setFocusPolicy(Qt::NoFocus);
        button->setCheckable(true);
        // 按钮的选中状态跟随用户最初选择的是关机还是重启
        if ((action == UpdateModel::Reboot && UpdateModel::instance()->isReboot())
            || (action == UpdateModel::ShutDown && !UpdateModel::instance()->isReboot())) {
            button->setChecked(true);
            m_checkedButton = button;
        }

        connect(button, &QPushButton::clicked, this, [action] {
            UpdateWorker::instance()->doAction(action);
        });
    }

    // 非重启/关机按钮的情况，默认选中第一个按钮
    if (!m_checkedButton && !m_actionButtons.isEmpty()) {
        m_checkedButton = m_actionButtons.first();
        m_checkedButton->setChecked(true);
    }
}

void UpdateCompleteWidget::expendLogWidget()
{
    qCDebug(logUpdateModal) << "Expanding log widget, hiding" << m_actionButtons.size() << "action buttons";
    m_logWidget->setVisible(true);
    for(auto button : m_actionButtons) {
        button->setVisible(false);
    }
}

void UpdateCompleteWidget::collapseLogWidget()
{
    qCDebug(logUpdateModal) << "Collapsing log widget, showing" << m_actionButtons.size() << "action buttons";
    m_logWidget->setVisible(false);
    for(auto button : m_actionButtons) {
        button->setVisible(true);
    }
}

void UpdateCompleteWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Tab: {
        qCDebug(logUpdateModal) << "Navigation key pressed, switching button selection";
        if (m_actionButtons.size() > 1) {
            int index = 0;
            if (m_checkedButton) {
                index = m_actionButtons.indexOf(m_checkedButton.data());
                if (index == m_actionButtons.length() - 1)
                    index = 0;
                else
                    index++;

                m_checkedButton->setChecked(false);
            }
            m_checkedButton = m_actionButtons.at(index);
            m_checkedButton->setChecked(true);
        }

        break;
    }
    case Qt::Key_Return:
        qCDebug(logUpdateModal) << "Return key pressed, clicking checked button";
        if(m_checkedButton)
            m_checkedButton->clicked();
        break;
    case Qt::Key_Enter:
        qCDebug(logUpdateModal) << "Enter key pressed, clicking checked button";
        if (m_checkedButton)
            m_checkedButton->clicked();
        break;
    }
    QWidget::keyPressEvent(event);
}

UpdateWidget::UpdateWidget(QWidget *parent)
    : QFrame(parent)
{
    qCDebug(logUpdateModal) << "Initialize UpdateWidget";
    initUi();
    initConnections();
}

UpdateWidget* UpdateWidget::instance()
{
    qCDebug(logUpdateModal) << "Getting UpdateWidget instance";
    static UpdateWidget* updateWidget = nullptr;
    if (!updateWidget) {
        qCDebug(logUpdateModal) << "Creating new UpdateWidget instance";
        updateWidget = new UpdateWidget;
    }
    return updateWidget;
}

void UpdateWidget::showUpdate()
{
    qCDebug(logUpdateModal) << "Show update widget, setting status to Ready";
    UpdateModel::instance()->setIsUpdating(true);
    UpdateModel::instance()->setUpdateStatus(UpdateModel::Ready);
}

void UpdateWidget::onUpdateStatusChanged(UpdateModel::UpdateStatus status)
{
    qCDebug(logUpdateModal) << "Update status changed to:" << status;
    switch (status) {
        case UpdateModel::UpdateStatus::Ready:
            showChecking();
            UpdateWorker::instance()->startUpdateProgress();
            break;
        case UpdateModel::UpdateStatus::BackingUp:
            m_stackedWidget->setCurrentWidget(m_progressWidget);
            break;
        case UpdateModel::UpdateStatus::BackupSuccess:
            m_stackedWidget->setCurrentWidget(m_progressWidget);
            break;
        case UpdateModel::UpdateStatus::Installing:
            m_stackedWidget->setCurrentWidget(m_progressWidget);
            break;
        case UpdateModel::UpdateStatus::InstallSuccess:
            // 升级成功
            m_stackedWidget->setCurrentWidget(m_updateCompleteWidget);
            m_updateCompleteWidget->showResult(true);
            break;
        case UpdateModel::UpdateStatus::InstallFailed:
        case UpdateModel::UpdateStatus::BackupFailed:
        case UpdateModel::UpdateStatus::PrepareFailed:
            m_stackedWidget->setCurrentWidget(m_updateCompleteWidget);
            m_updateCompleteWidget->showResult(false, UpdateModel::instance()->updateError());

        default:
            break;
    }
}

void UpdateWidget::initUi()
{
    qCDebug(logUpdateModal) << "Initializing UpdateWidget UI components";
    m_prepareWidget = new UpdatePrepareWidget(this);
    m_progressWidget = new UpdateProgressWidget(this);
    m_updateCompleteWidget = new UpdateCompleteWidget(this);
    m_logWidget = new UpdateLogWidget(this);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->addWidget(m_prepareWidget);
    m_stackedWidget->addWidget(m_progressWidget);
    m_stackedWidget->addWidget(m_updateCompleteWidget);
    m_stackedWidget->addWidget(m_logWidget);

    auto mainAnchors = new DAnchors<UpdateWidget>(this);
    auto stackedWidgetAnchors = new DAnchors<QStackedWidget>(m_stackedWidget);

    stackedWidgetAnchors->setFill(mainAnchors);

    setFocusProxy(m_stackedWidget);
}

void UpdateWidget::initConnections()
{
    qCDebug(logUpdateModal) << "Initializing UpdateWidget signal connections";
    connect(UpdateModel::instance(), &UpdateModel::JobProgressChanged, this, &UpdateWidget::onJobProgressChanged);
    connect(UpdateModel::instance(), &UpdateModel::updateStatusChanged, this, &UpdateWidget::onUpdateStatusChanged);
    connect(m_updateCompleteWidget, &UpdateCompleteWidget::requestShowLogWidget, this, &UpdateWidget::showLogWidget);
    connect(m_logWidget, &UpdateLogWidget::requestHideLogWidget, this, &UpdateWidget::hideLogWidget);
    connect(m_stackedWidget, &QStackedWidget::currentChanged, this, [this] {
        auto w = m_stackedWidget->currentWidget();
        if (w) {
            m_stackedWidget->setFocusProxy(w);
            m_stackedWidget->setFocus();
        }
    });
}

void UpdateWidget::showLogWidget()
{
    qCDebug(logUpdateModal) << "Showing log widget with error log";
    m_logWidget->setLog(UpdateModel::instance()->lastErrorLog());
    m_stackedWidget->setCurrentWidget(m_logWidget);
}

void UpdateWidget::hideLogWidget()
{
    qCDebug(logUpdateModal) << "Hiding log widget, returning to complete widget";
    m_stackedWidget->setCurrentWidget(m_updateCompleteWidget);
}

void UpdateWidget::showChecking()
{
    qCDebug(logUpdateModal) << "Showing checking/prepare widget";
    m_stackedWidget->setCurrentWidget(m_prepareWidget);
    m_prepareWidget->showPrepare();
}

void UpdateWidget::showProgress()
{
    qCDebug(logUpdateModal) << "Showing progress widget";
    m_stackedWidget->setCurrentWidget(m_progressWidget);
}

void UpdateWidget::setMouseCursorVisible( bool visible)
{
    qCDebug(logUpdateModal) << "Set mouse cursor visible:" << visible;
    static bool mouseVisible=true;
    if(mouseVisible == visible)
        return;

    mouseVisible= visible;
    QApplication::setOverrideCursor(mouseVisible ? Qt::ArrowCursor : Qt::BlankCursor);
}

void UpdateWidget::keyPressEvent(QKeyEvent *e)
{
    Q_UNUSED(e)
    // 屏蔽esc键，设置event的accept无效，暂时不处理
}

void UpdateWidget::onJobProgressChanged(double value)
{
    qCDebug(logUpdateModal) << "Job progress changed:" << value << "hasBackup:" << UpdateModel::instance()->hasBackup() << "status:" << UpdateModel::instance()->updateStatus();
    int progress = 0;
    bool hasBackup = UpdateModel::instance()->hasBackup();
    if (hasBackup) {
        if (UpdateModel::instance()->updateStatus() == UpdateModel::UpdateStatus::BackingUp) {
            progress = BACKUP_BEGIN_PROGRESS + static_cast<int>(value * (BACKUP_END_PROGRESS - BACKUP_BEGIN_PROGRESS));
        } else if (UpdateModel::instance()->updateStatus() == UpdateModel::UpdateStatus::BackupSuccess) {
            progress = BACKUP_END_PROGRESS;
        } else if (UpdateModel::instance()->updateStatus() == UpdateModel::UpdateStatus::Installing) {
            progress = BACKUP_END_PROGRESS + static_cast<int>(value * (100 - BACKUP_END_PROGRESS));
        } else if (UpdateModel::instance()->updateStatus() == UpdateModel::UpdateStatus::InstallSuccess) {
            progress = 100;
        }
    } else {
        if (UpdateModel::instance()->updateStatus() == UpdateModel::UpdateStatus::Installing) {
            progress = value * 100;
        } else if (UpdateModel::instance()->updateStatus() == UpdateModel::UpdateStatus::InstallSuccess) {
            progress = 100;
        }
    }

    m_progressWidget->setValue(progress);
}
