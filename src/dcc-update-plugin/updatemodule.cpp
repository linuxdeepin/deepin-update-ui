// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "updatemodule.h"
#include "updatemodel.h"
#include "updatework.h"
#include "updatewidget.h"
#include "mirrorswidget.h"
#include "widgets/utils.h"
#include "window/utils.h"
#include "window/gsettingwatcher.h"
#include "window/dconfigwatcher.h"

#include <QVBoxLayout>

#include <DLog>

DCORE_USE_NAMESPACE

using namespace dcc;
using namespace dcc::update;
using namespace DCC_NAMESPACE;
using namespace DCC_NAMESPACE::update;

Q_LOGGING_CATEGORY(dcc::update::DCC_UPDATE, "org.deepin.dde.control-center.update")

UpdateModule::UpdateModule(QObject *parent)
    : QObject(parent)
    , m_model(nullptr)
    , m_work(nullptr)
    , m_updateWidget(nullptr)
    , m_mirrorsWidget(nullptr)
{
    QTranslator *translator = new QTranslator(this);
    translator->load(QString("/usr/share/deepin-update-ui/translations/dcc-update-plugin_%1.qm").arg(QLocale().name()));
    QCoreApplication::installTranslator(translator);
    logger->logToGlobalInstance(DCC_UPDATE().categoryName(), true);
}

UpdateModule::~UpdateModule()
{
    if (m_workThread) {
        m_workThread->quit();
        m_workThread->wait();
    }
}

void UpdateModule::preInitialize(bool sync, FrameProxyInterface::PushType pushtype)
{
    qCInfo(DCC_UPDATE) << "Update module pre initialize";
    if (!DSysInfo::isDeepin() || DSysInfo::uosEditionType() == DSysInfo::UosEuler) {
        qCInfo(DCC_UPDATE) << displayName() << " is disabled";
        setAvailable(false);
        return;
    }

    Q_UNUSED(sync);
    Q_UNUSED(pushtype);

    m_workThread = QSharedPointer<QThread>(new QThread);
    m_model = new UpdateModel(this);
    m_work  = QSharedPointer<UpdateWorker>(new UpdateWorker(m_model));
    m_work->moveToThread(m_workThread.get());
    m_workThread->start(QThread::LowPriority);

    connect(m_work.get(), &UpdateWorker::requestInit, m_work.get(), &UpdateWorker::init);
    connect(m_work.get(), &UpdateWorker::requestActive, m_work.get(), &UpdateWorker::activate);
    connect(m_work.get(), &UpdateWorker::requestRefreshLicenseState, m_work.get(), &UpdateWorker::licenseStateChangeSlot);

#ifndef DISABLE_SYS_UPDATE_MIRRORS
    connect(m_work.get(), &UpdateWorker::requestRefreshMirrors, m_work.get(), &UpdateWorker::refreshMirrors);
#endif
    connect(m_model, &UpdateModel::updateNotifyChanged, this, &UpdateModule::updateNotifyRedPoint);
    connect(m_model, &UpdateModel::isUpdatableChanged, this, &UpdateModule::updateNotifyRedPoint);

    // 初始化更新小红点处理
    updateNotifyRedPoint();

    if (DSysInfo::uosEditionType() == DSysInfo::UosEuler) {
        m_frameProxy->setModuleVisible(this, false);
        setDeviceUnavailabel(true);
    } else {
        bool bShowUpdate = valueByQSettings<bool>(DCC_CONFIG_FILES, "", "showUpdate", true);
        m_frameProxy->setModuleVisible(this, bShowUpdate);
        setDeviceUnavailabel(!bShowUpdate);
    }

#ifndef DISABLE_ACTIVATOR
    connect(m_model, &UpdateModel::systemActivationChanged, this, [ = ](UiActiveState systemActivation) {
        if (systemActivation == UiActiveState::Authorized || systemActivation == UiActiveState::TrialAuthorized || systemActivation == UiActiveState::AuthorizedLapse) {
            if (m_updateWidget)
                m_updateWidget->setSystemVersion(m_model->systemVersionInfo());

            // 授权发生变化时触发检查更新
            if (m_model->isActivationValid())
                Q_EMIT m_model->beginCheckUpdate();
        }
    });
#endif

    Q_EMIT m_work->requestInit();
    Q_EMIT m_work->requestActive();

    addChildPageTrans();
    initSearchData();
}

void UpdateModule::initialize()
{
}

const QString UpdateModule::name() const
{
    return QStringLiteral("update");
}

const QString UpdateModule::displayName() const
{
    return tr("Updates");
}

void UpdateModule::active()
{
    qCDebug(DCC_UPDATE) << "Active update module";
    connect(m_model, &UpdateModel::beginCheckUpdate, m_work.get(), &UpdateWorker::checkForUpdates);
    connect(m_model, &UpdateModel::updateHistoryAppInfos, m_work.get(), &UpdateWorker::refreshHistoryAppsInfo, Qt::DirectConnection);
    connect(m_model, &UpdateModel::updateCheckUpdateTime, m_work.get(), &UpdateWorker::refreshLastTimeAndCheckCircle, Qt::DirectConnection);

    m_updateWidget = new UpdateWidget;
    m_updateWidget->setVisible(false);
    m_updateWidget->initialize();

    Q_EMIT m_work->requestRefreshLicenseState();

    if (m_model->isActivationValid()) {
        m_updateWidget->setSystemVersion(m_model->systemVersionInfo());
    }

    m_updateWidget->setModel(m_model, m_work.get());

    connect(m_updateWidget, &UpdateWidget::pushMirrorsView, this, [ = ]() {
        m_mirrorsWidget = new MirrorsWidget(m_model);
        m_mirrorsWidget->setVisible(false);
        int topWidgetWidth = m_updateWidget->parentWidget()->parentWidget()->width();
        m_work->checkNetselect();
        m_mirrorsWidget->setMinimumWidth(topWidgetWidth / 2);
        m_mirrorsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        connect(m_mirrorsWidget, &MirrorsWidget::requestSetDefaultMirror, m_work.get(), &UpdateWorker::setMirrorSource);
        connect(m_mirrorsWidget, &MirrorsWidget::requestTestMirrorSpeed, m_work.get(), &UpdateWorker::testMirrorSpeed);
        connect(m_mirrorsWidget, &MirrorsWidget::notifyDestroy, this, [this]() {
            //notifyDestroy信号是此对象被销毁，析构时发出的，资源销毁了要将其对象赋值为空
            m_mirrorsWidget = nullptr;
        });
        connect(m_model, &UpdateModel::smartMirrorSwitchChanged, this, &UpdateModule::onNotifyDealMirrorWidget);

        //mainWidget->parentWidget()->parentWidget()即mainwindow
        //1690是能正常完全显示二级页面的宽度(包括注释说明文字)
        if (topWidgetWidth <= 1690) {
            m_frameProxy->pushWidget(this, m_mirrorsWidget, dccV20::FrameProxyInterface::PushType::DirectTop);
        } else {
            m_frameProxy->pushWidget(this, m_mirrorsWidget);
        }
        m_mirrorsWidget->setVisible(true);
    });

    connect(m_updateWidget, &UpdateWidget::requestLastoreHeartBeat, m_work.get(), &UpdateWorker::onRequestLastoreHeartBeat);

#ifndef DISABLE_ACTIVATOR
    if (m_model->isActivationValid()) {
        m_updateWidget->setSystemVersion(m_model->systemVersionInfo());
    }
#else
    mainWidget->setSystemVersion(m_model->systemVersionInfo());
#endif

    m_frameProxy->pushWidget(this, m_updateWidget);
    m_updateWidget->setVisible(true);
    m_updateWidget->refreshWidget(UpdateWidget::UpdateFrameType::UpdateCheck);

    // 控制中心第一次进来 & 超过设定时间 的时候检查一次
    static bool doCheckUpdate = true;
    if (doCheckUpdate || m_model->enterCheckUpdate()) {
        doCheckUpdate = false;
        Q_EMIT m_model->beginCheckUpdate();
    }
}

void UpdateModule::deactive()
{
    if (m_model) {
        m_model->deleteLater();
        m_model = nullptr;
    }

    if (m_work) {
        m_work->deleteLater();
        m_work = nullptr;
    }
}

int UpdateModule::load(const QString &path)
{
    int hasPage = -1;
    if (m_updateWidget) {
        if (path == "Update Settings") {
            hasPage = 0;
            m_updateWidget->refreshWidget(UpdateWidget::UpdateFrameType::UpdateSetting);
        } else if (path == "Update") {
            hasPage = 0;
            m_updateWidget->refreshWidget(UpdateWidget::UpdateFrameType::UpdateCheck);
        } else if (path == "Update Settings/Mirror List") {
            hasPage = 0;
            m_updateWidget->refreshWidget(UpdateWidget::UpdateFrameType::UpdateSettingMir);
        } else if (path == "Check for Updates") {
            hasPage = 0;
            m_updateWidget->refreshWidget(UpdateWidget::UpdateFrameType::UpdateCheck);
        }
    }

    return hasPage;
}

QStringList UpdateModule::availPage() const
{
    return QStringList() << "Update Settings" << "Update" << "Update Settings/Mirror List" << "Checking";
}

QString UpdateModule::follow() const
{
    return "0";
}

void UpdateModule::addChildPageTrans() const
{
#ifndef DISABLE_UPDATESEARCH
    if (m_frameProxy != nullptr) {
        //update
        m_frameProxy->addChildPageTrans("Check for Updates", tr("Check for Updates"));
        m_frameProxy->addChildPageTrans("Update Settings", tr("Update Settings"));
    }
#endif
}

void UpdateModule::onNotifyDealMirrorWidget(bool state)
{
    // m_mirrorsWidget存在表示有第三级页面
    if (state && m_mirrorsWidget) {
        m_frameProxy->popWidget(this);
        //popWidget之后就没有第三级页面了,即m_mirrorsWidget为空指针,需要对其地址赋值为nullptr
        //假如是从开启镜像源列表页面后，切换其他页面此时这里资源已经被释放了，但是指针没有赋值为空；再次进入，关闭此处就会直接返回主页面(接收析构时信号)
        m_mirrorsWidget = nullptr;
        //避免第三级页面不存在后,还会处理该函数
        disconnect(m_model, &UpdateModel::smartMirrorSwitchChanged, this, &UpdateModule::onNotifyDealMirrorWidget);
    }
}

void UpdateModule::initSearchData()
{
#ifndef DISABLE_UPDATESEARCH
    const QString& module = displayName();
    const QString& updates = tr("Check for Updates");
    const QString& updateSettings = tr("Update Settings");
    static QMap<QString, bool> gsettingsMap;

    auto func_is_visible = [=](const QString &gsettings, bool isDConfig = false) {
        if (gsettings.isEmpty())
            return false;

        bool ret = false;
        if (isDConfig)
            ret = DConfigWatcher::instance()->getValue(DConfigWatcher::update, gsettings).toString() != "Hidden";
        else
            ret = GSettingWatcher::instance()->get(gsettings).toString() != "Hidden";
        gsettingsMap.insert(gsettings, ret);
        return ret;
    };

    auto func_process_all = [ = ]() {
        m_frameProxy->setWidgetVisible(module, updates, true);
        m_frameProxy->setDetailVisible(module, updates, tr("Check for Updates"), true);
        m_frameProxy->setDetailVisible(module, updateSettings, tr("System Updates"), func_is_visible("updateSystemUpdate"));
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Security Updates"), func_is_visible("updateSecureUpdate") && func_is_visible("updateSafety", true));
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Third-party Updates"), !IsProfessionalSystem || func_is_visible("updateThirdPartySource", true));
        m_frameProxy->setWidgetVisible(module, updateSettings, true);
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Auto Download"), func_is_visible("updateAutoDownlaod"));
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Download when Inactive"), func_is_visible("updateAutoDownlaod"));
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Limit Speed"), true);
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Updates Notification"), func_is_visible("updateUpdateNotify"));
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Clear Package Cache"), func_is_visible("updateCleanCache"));
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Update History"), func_is_visible("updateHistoryEnabled", true));
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Transferring Cache"), func_is_visible("p2pUpdateEnabled", true));
        m_frameProxy->updateSearchData(module);
     };
    // 如果内测渠道功能隐藏，同步隐藏搜索数据
    connect(m_model, &UpdateModel::testingChannelStatusChanged, this, [ = ]{
        UpdateModel::TestingChannelStatus status = m_model->getTestingChannelStatus();
        bool visible = (status != UpdateModel::TestingChannelStatus::Hidden) && (status != UpdateModel::TestingChannelStatus::NotJoined);
        m_frameProxy->setDetailVisible(module, updateSettings, tr("Updates from Internal Testing Sources"), visible);
        m_frameProxy->updateSearchData(module);
    });

    connect(GSettingWatcher::instance(), &GSettingWatcher::notifyGSettingsChanged, this, [=](const QString &gsetting, const QString &state) {
        if ("" == gsetting || !gsettingsMap.contains(gsetting)) {
            return;
        }

        if (gsettingsMap.value(gsetting) == (GSettingWatcher::instance()->get(gsetting).toString() != "Hidden")) {
            return;
        }

        bool isVisible = func_is_visible(gsetting);
        if ("updateAutoCheck" == gsetting) {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Auto Check for Updates"), isVisible);
        } else if ("updateAutoDownlaod" == gsetting) {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Auto Download Updates"), isVisible);
        } else if ("updateUpdateNotify" == gsetting) {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Updates Notification"), isVisible);
        } else if ("updateCleanCache" == gsetting) {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Clear Package Cache"), isVisible);
        }else if ("updateSystemUpdate" == gsetting) {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("System"), isVisible);
        } else if ("updateSecureUpdate" == gsetting) {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Security Updates"), isVisible);
        } else {
            qCWarning(DCC_UPDATE) << "Do not contain the gsettings : " << gsetting << state;
            return;
        }

        qCInfo(DCC_UPDATE) << "GSettings changed:" << gsetting << ", state:" << state;
        m_frameProxy->updateSearchData(module);
    });

    connect(DConfigWatcher::instance(), &DConfigWatcher::notifyDConfigChanged, this, [this, module, updateSettings, func_is_visible](const QString &moduleName, const QString &configName) {
        if (moduleName != "update") {
            return;
        }

        if (configName == "updateThirdPartySource") {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Third-party Updates"), !IsProfessionalSystem || func_is_visible("updateThirdPartySource", true));
        } else if (configName == "updateSafety") {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Security Updates"), func_is_visible("updateSecureUpdate") && func_is_visible("updateSafety", true));
        } else if (configName == "updateHistoryEnabled") {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Update History"), func_is_visible("updateHistoryEnabled", true));
        } else if (configName == "p2pUpdateEnabled") {
            m_frameProxy->setDetailVisible(module, updateSettings, tr("Transferring Cache"), func_is_visible("p2pUpdateEnabled", true));
        }

        m_frameProxy->updateSearchData(module);
    });

    func_process_all();
#endif
}

void UpdateModule::updateNotifyRedPoint()
{
    m_frameProxy->setModuleSubscriptVisible(name(), m_model->isUpdatable() && m_model->updateNotify());
}

QString UpdateModule::description() const
{
    return tr("Check for updates, Update settings");
}
