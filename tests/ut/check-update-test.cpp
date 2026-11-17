// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "operation/updatework.h"

#include <QApplication>
#include <QDateTime>
#include <QDBusConnectionInterface>
#include <QDBusMetaType>
#include <QDBusVirtualObject>
#include <QJsonObject>
#include <QProcess>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

using namespace dcc::update::common;

static const QString Service = QStringLiteral("org.deepin.dde.Lastore1");
static const QString Root = QStringLiteral("/org/deepin/dde/Lastore1");
static const QString Job = Root + QStringLiteral("/Jobupdate_source");
static const QString JobInterface = Service + QStringLiteral(".Job");

class MockLastore : public QDBusVirtualObject
{
public:
    bool exists = false;
    QString status = "running";
    double progress = 0.1;
    int replyDelay = 0;
    int snapshotDelay = 0;
    int starts = 0;
    int cleans = 0;
    int snapshots = 0;
    qlonglong createTime = 0;
    QString snapshotError;
    bool retainCleanedJob = false;
    bool availableUpdates = false;
    QString description = R"({"ErrType":"fetchFailed","ErrDetail":"test failure"})";

    QString introspect(const QString &) const override
    {
        return QStringLiteral(
            "<interface name='org.deepin.dde.Lastore1.Job'>"
            "<property name='Id' type='s' access='read'/>"
            "<property name='CreateTime' type='x' access='read'/>"
            "<property name='Status' type='s' access='read'/>"
            "<property name='Progress' type='d' access='read'/>"
            "</interface>"
            "<interface name='org.deepin.dde.Lastore1.Manager'/>"
            "<interface name='org.deepin.dde.Lastore1.Updater'/>");
    }

    void emitProperties(const QString &path, const QString &iface, const QVariantMap &properties,
                        const QDBusConnection &bus)
    {
        auto signal = QDBusMessage::createSignal(path, "org.freedesktop.DBus.Properties", "PropertiesChanged");
        signal << iface << properties << QStringList{};
        bus.send(signal);
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &bus) override
    {
        const auto args = message.arguments();
        auto reply = [&](const QList<QVariant> &values = {}) { bus.send(message.createReply(values)); };
        if (message.interface() == "org.example.Test") {
            if (message.member() == "Reset") {
                exists = false;
                status = "running";
                progress = 0.1;
                starts = 0;
                cleans = 0;
                replyDelay = 0;
                snapshotDelay = 0;
                snapshots = 0;
                snapshotError.clear();
                retainCleanedJob = false;
                availableUpdates = false;
                description = R"({"ErrType":"fetchFailed","ErrDetail":"test failure"})";
            } else if (message.member() == "Create") {
                exists = true;
                ++createTime;
            } else if (message.member() == "SetState") {
                status = args.at(0).toString();
                progress = args.at(1).toDouble();
                if (args.at(2).toBool())
                    emitProperties(Job, JobInterface, {{"Status", status}, {"Progress", progress}}, bus);
            } else if (message.member() == "Remove") {
                exists = false;
                if (args.at(0).toBool())
                    emitProperties(Root, Service + ".Manager",
                                   {{"JobList", QVariant::fromValue(QList<QDBusObjectPath>{})}}, bus);
            } else if (message.member() == "DelayReply") {
                replyDelay = args.at(0).toInt();
            } else if (message.member() == "DelaySnapshot") {
                snapshotDelay = args.at(0).toInt();
            } else if (message.member() == "SnapshotError") {
                snapshotError = args.at(0).toString();
            } else if (message.member() == "Snapshots") {
                reply({snapshots});
                return true;
            } else if (message.member() == "Starts") {
                reply({starts});
                return true;
            } else if (message.member() == "Cleans") {
                reply({cleans});
                return true;
            } else if (message.member() == "CompleteThenReplace") {
                emitProperties(Job, JobInterface, {{"Status", "succeed"}, {"Progress", 1.0}}, bus);
                ++createTime;
                status = "running";
                progress = 0.4;
            } else if (message.member() == "CompleteAndRemove") {
                // Lastore emits succeed, immediately ends the job, then unexports it.
                status = "succeed";
                progress = 1.0;
                emitProperties(Job, JobInterface, {{"Status", status}, {"Progress", progress}}, bus);
                status = "end";
                emitProperties(Job, JobInterface, {{"Status", status}}, bus);
                exists = false;
                emitProperties(Root, Service + ".Manager",
                               {{"JobList", QVariant::fromValue(QList<QDBusObjectPath>{})}}, bus);
            } else if (message.member() == "RetainCleanedJob") {
                retainCleanedJob = args.at(0).toBool();
            } else if (message.member() == "SetDescription") {
                description = args.at(0).toString();
            } else if (message.member() == "AvailableUpdates") {
                availableUpdates = args.at(0).toBool();
            }
            reply();
            return true;
        }
        if (message.path() == Job && !exists) {
            // Match Lastore's godbus error, which Qt classifies as Other.
            bus.send(message.createErrorReply(QStringLiteral("org.freedesktop.DBus.Error.NoSuchObject"),
                                              QStringLiteral("Job was removed")));
            return true;
        }
        const QVariantMap properties = {
            {"Id", "update_source"}, {"Name", ""}, {"Status", status}, {"Progress", progress},
            {"CreateTime", createTime},
            {"Description", description},
            {"ClassifiedUpdatablePackages", QVariant::fromValue(availableUpdates
                ? LastoreUpdatePackagesInfo{{SYSTEM_UPGRADE_TYPE_STRING, {"mock-system-update"}}}
                : LastoreUpdatePackagesInfo{})}
        };
        if (message.interface() == "org.freedesktop.DBus.Properties") {
            if (message.member() == "GetAll") {
                ++snapshots;
                if (!snapshotError.isEmpty()) {
                    bus.send(message.createErrorReply(snapshotError, "Injected polling error"));
                    return true;
                }
                const auto response = message.createReply(QList<QVariant>{properties});
                QTimer::singleShot(snapshotDelay, this, [bus, response] { bus.send(response); });
            } else if (message.member() == "Get") {
                reply({QVariant::fromValue(QDBusVariant(properties.value(args.at(1).toString(), QString())))});
            }
        } else if (message.member() == "UpdateSource") {
            ++starts;
            if (!exists) {
                ++createTime;
                status = "running";
                progress = 0.1;
            }
            exists = true;
            auto response = message.createReply({QVariant::fromValue(QDBusObjectPath(Job))});
            QTimer::singleShot(replyDelay, this, [bus, response] { bus.send(response); });
            if (replyDelay > 0)
                emitProperties(Root, Service + ".Manager",
                               {{"JobList", QVariant::fromValue(QList<QDBusObjectPath>{QDBusObjectPath(Job)})}}, bus);
        } else if (message.member() == "GetCheckIntervalAndTime") {
            reply({168.0, QStringLiteral("2026-10-28 10:00:00 +0800 CST")});
        } else if (message.member() == "GetUpdateLogs") {
            reply({QStringLiteral("{}")});
        } else if (message.member() == "CleanJob") {
            ++cleans;
            if (retainCleanedJob) {
                status = "end";
                emitProperties(Job, JobInterface, {{"Status", status}}, bus);
            } else {
                exists = false;
            }
            reply();
        } else {
            bus.send(message.createErrorReply(QDBusError::UnknownMethod, message.member()));
        }
        return true;
    }
};

class CheckUpdateTest : public QObject
{
    Q_OBJECT
    QProcess mock;

    QDBusMessage control(const QString &method, const QList<QVariant> &args = {})
    {
        auto message = QDBusMessage::createMethodCall(Service, Root, "org.example.Test", method);
        message.setArguments(args);
        return QDBusConnection::systemBus().call(message);
    }

private Q_SLOTS:
    void initTestCase()
    {
        mock.start(QCoreApplication::applicationFilePath(), {"--mock-service"});
        QVERIFY(mock.waitForStarted());
        QTRY_VERIFY(QDBusConnection::systemBus().interface()->isServiceRegistered(Service).value());
    }

    void cleanupTestCase()
    {
        mock.terminate();
        QVERIFY(mock.waitForFinished());
    }

    void init()
    {
        QCOMPARE(control("Reset").type(), QDBusMessage::ReplyMessage);
    }

    void unavailableObserverConnectionIsRetryable_data()
    {
        QTest::addColumn<bool>("adopted");
        QTest::newRow("user-started") << false;
        QTest::newRow("background-job") << true;
    }

    void unavailableObserverConnectionIsRetryable()
    {
        QFETCH(bool, adopted);
        UpdateModel model;
        QList<QDBusConnection> blockers;
        QStringList blockerNames;
        const auto releaseConnections = [&] {
            for (const auto &name : blockerNames)
                QDBusConnection::disconnectFromBus(name);
            blockers.clear();
            blockerNames.clear();
        };
        const auto restoreConnections = qScopeGuard(releaseConnections);
        bool quotaReached = false;
        for (int i = 0; i < 32; ++i) {
            const auto name = QStringLiteral("check-update-quota-%1").arg(i);
            blockerNames << name;
            const auto connection = QDBusConnection::connectToBus(QDBusConnection::SystemBus, name);
            if (!connection.isConnected()) {
                quotaReached = true;
                break;
            }
            blockers << connection;
        }
        QVERIFY(quotaReached);
        QVERIFY(!blockers.isEmpty());
        const auto blockedService = blockers.first().baseService();

        // The shared bus still works, but the worker's new observer cannot open.
        QCOMPARE(control("Create").type(), QDBusMessage::ReplyMessage);
        UpdateWorker worker(&model);
        if (adopted)
            worker.onJobListChanged({QDBusObjectPath(Job)});
        else
            worker.doCheckUpdates();
        QTRY_COMPARE_WITH_TIMEOUT(model.checkUpdateStatus(), CheckingFailed, 1000);
        QCOMPARE(model.lastError(CheckingFailed), UnKnown);
        QVERIFY(!model.lastErrorLog(CheckingFailed).isEmpty());
        QCOMPARE(control("Cleans").arguments().first().toInt(), 0);
        QCOMPARE(control("Starts").arguments().first().toInt(), 0);

        // Repeated attempts while the quota is full must also leave checking.
        worker.doCheckUpdates();
        QCOMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(control("Starts").arguments().first().toInt(), 0);

        releaseConnections();
        QTRY_VERIFY(!QDBusConnection::systemBus().interface()->isServiceRegistered(blockedService).value());
        worker.doCheckUpdates();
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        QCOMPARE(model.checkUpdateStatus(), Checking);
        QCOMPARE(control("Starts").arguments().first().toInt(), 1);
        control("CompleteAndRemove");
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
    }

    void sharedConnectionQuotaDoesNotLoseCompletion_data()
    {
        QTest::addColumn<bool>("adopted");
        QTest::newRow("user-started") << false;
        QTest::newRow("background-job") << true;
    }

    void sharedConnectionQuotaDoesNotLoseCompletion()
    {
        QFETCH(bool, adopted);
        UpdateModel model;
        UpdateWorker worker(&model);
        // Let existing manager subscriptions settle before exhausting their bus.
        QTest::qWait(100);
        const auto bus = QDBusConnection::systemBus();
        QStringList addedRules;
        const auto restoreRules = qScopeGuard([&] {
            for (const auto &rule : addedRules) {
                auto message = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                            "org.freedesktop.DBus", "RemoveMatch");
                message << rule;
                bus.call(message);
            }
        });
        QDBusError quotaError;
        for (int i = 0; i < 512; ++i) {
            const auto rule = QStringLiteral("type='signal',interface='org.example.Quota',path='/org/example/Quota/%1'").arg(i);
            auto message = QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                        "org.freedesktop.DBus", "AddMatch");
            message << rule;
            const auto reply = bus.call(message);
            if (reply.type() == QDBusMessage::ErrorMessage) {
                quotaError = QDBusError(reply);
                break;
            }
            addedRules << rule;
        }
        QCOMPARE(quotaError.type(), QDBusError::LimitsExceeded);

        if (adopted) {
            control("Create");
            worker.onJobListChanged({QDBusObjectPath(Job)});
        } else {
            worker.doCheckUpdates();
        }
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("CompleteAndRemove");
        // The object has already vanished, so polling cannot recover its result.
        QTRY_COMPARE_WITH_TIMEOUT(model.checkUpdateStatus(), Updated, 3000);
        QCOMPARE(model.checkUpdateProgress(), 1.0);
        worker.onJobListChanged({});
        QTest::qWait(100);
        QCOMPARE(model.checkUpdateStatus(), Updated);
    }

    void silentlyReplacedActiveJobKeepsConfirmedSuccess_data()
    {
        QTest::addColumn<bool>("announceReplacement");
        QTest::addColumn<bool>("alreadySucceeded");
        QTest::newRow("job-list") << true << false;
        QTest::newRow("poll-running") << false << false;
        QTest::newRow("poll-succeeded") << false << true;
    }

    void silentlyReplacedActiveJobKeepsConfirmedSuccess()
    {
        QFETCH(bool, announceReplacement);
        QFETCH(bool, alreadySucceeded);
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        // A disappears without a terminal/removal signal; B reuses its path.
        control("Remove", {false});
        control("Create");
        control("SetState", {alreadySucceeded ? "succeed" : "running", alreadySucceeded ? 1.0 : 0.4, false});
        worker.onJobListChanged(announceReplacement ? QList<QDBusObjectPath>{QDBusObjectPath(Job)}
                                                   : QList<QDBusObjectPath>{});
        if (!alreadySucceeded) {
            QTRY_COMPARE(model.checkUpdateProgress(), 0.4);
            control("SetState", {"succeed", 1.0, true});
        }
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
        // A later list still contains B while its end state awaits removal.
        control("SetState", {"end", 1.0, false});
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTest::qWait(100);
        QCOMPARE(model.checkUpdateStatus(), Updated);
    }

    void replacedJobIgnoresOldPendingSnapshot()
    {
        control("Create");
        control("DelaySnapshot", {600});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTest::qWait(100);
        control("Remove", {false});
        control("Create");
        control("SetState", {"running", 0.7, false});
        control("DelaySnapshot", {0});
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTRY_COMPARE(model.checkUpdateProgress(), 0.7);
        QTest::qWait(700);
        QCOMPARE(model.checkUpdateStatus(), Checking);
        QCOMPARE(model.checkUpdateProgress(), 0.7);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
    }

    void repeatedFailureRefreshesErrorText_data()
    {
        QTest::addColumn<bool>("emitFailure");
        QTest::addColumn<bool>("startWithNetwork");
        QTest::newRow("network-to-space-polled") << false << true;
        QTest::newRow("network-to-space-signal") << true << true;
        QTest::newRow("space-to-network-polled") << false << false;
        QTest::newRow("space-to-network-signal") << true << false;
    }

    void repeatedFailureRefreshesErrorText()
    {
        QFETCH(bool, emitFailure);
        QFETCH(bool, startWithNetwork);
        const auto initialError = startWithNetwork ? NoNetwork : NoSpace;
        const auto retryError = startWithNetwork ? NoSpace : NoNetwork;
        const auto description = [](const QString &errorType) {
            return QString::fromUtf8(QJsonDocument(QJsonObject{
                {"ErrType", errorType}, {"ErrDetail", "retry failure"}
            }).toJson(QJsonDocument::Compact));
        };
        control("Create");
        control("SetDescription", {description(startWithNetwork ? "fetchFailed" : "insufficientSpace")});
        control("SetState", {"failed", 0.2, false});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTRY_COMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(model.lastError(CheckingFailed), initialError);
        QCOMPARE(model.checkUpdateErrTips(), UpdateModel::errorToText(initialError));
        QSignalSpy statusChanged(&model, &UpdateModel::checkUpdateStatusChanged);
        QSignalSpy tipsChanged(&model, &UpdateModel::checkUpdateErrTipsChanged);
        // Backend retry transitions occur between polls and emit no observed signal.
        control("SetState", {"running", 0.3, false});
        control("SetDescription", {description(startWithNetwork ? "insufficientSpace" : "fetchFailed")});
        control("SetState", {"failed", 0.4, emitFailure});
        if (!emitFailure)
            worker.onJobListChanged({});
        QTRY_COMPARE(model.lastError(CheckingFailed), retryError);
        QCOMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(model.checkUpdateErrTips(), UpdateModel::errorToText(retryError));
        QCOMPARE(statusChanged.count(), 0);
        QCOMPARE(tipsChanged.count(), 1);
    }

    void failedModelRefreshesOnlyWhenItsErrorChanges()
    {
        UpdateModel model;
        model.setLastError(CheckingFailed, NoNetwork);
        model.setCheckUpdateStatus(CheckingFailed);
        QSignalSpy statusChanged(&model, &UpdateModel::checkUpdateStatusChanged);
        QSignalSpy tipsChanged(&model, &UpdateModel::checkUpdateErrTipsChanged);
        model.setLastError(CheckingFailed, NoSpace);
        QCOMPARE(model.checkUpdateErrTips(), UpdateModel::errorToText(NoSpace));
        QCOMPARE(tipsChanged.count(), 1);
        QCOMPARE(statusChanged.count(), 0);
        model.setLastError(CheckingFailed, NoSpace);
        model.setLastError(DownloadFailed, NoNetwork);
        QCOMPARE(model.checkUpdateErrTips(), UpdateModel::errorToText(NoSpace));
        QCOMPARE(tipsChanged.count(), 1);
        QCOMPARE(statusChanged.count(), 0);
    }

    void adoptedCheckConsumesFirstVisit_data()
    {
        QTest::addColumn<bool>("availableUpdates");
        QTest::newRow("up-to-date") << false;
        QTest::newRow("updates-available") << true;
    }

    void adoptedCheckConsumesFirstVisit()
    {
        QFETCH(bool, availableUpdates);
        control("Create");
        control("AvailableUpdates", {availableUpdates});
        UpdateModel model;
        model.setUpdateMode(SystemUpdate);
        model.setSystemActivation(true);
        model.setImmutableAutoRecovery(false);
        QVERIFY(!model.isUpdateDisabled());
        if (availableUpdates)
            model.setUpdateStatus(R"({"UpdateStatus":{"system_upgrade":"notDownload"}})");
        UpdateWorker worker(&model);
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        worker.checkNeedDoUpdates();
        QVERIFY(model.showCheckUpdate());
        QCOMPARE(control("Starts").arguments().first().toInt(), 0);
        control("SetState", {"succeed", 1.0, true});
        const auto completedStatus = availableUpdates ? UpdatesAvailable : Updated;
        QTRY_COMPARE(model.checkUpdateStatus(), completedStatus);
        model.setLastCheckUpdateTime(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
        auto info = model.updateItemInfo(SystemUpdate);
        QVERIFY(info);
        const auto packages = info->packages();
        QCOMPARE(packages.isEmpty(), !availableUpdates);
        // Reopening Updates must respect the recent adopted check's result.
        worker.checkNeedDoUpdates();
        QTest::qWait(100);
        QCOMPARE(control("Starts").arguments().first().toInt(), 0);
        QCOMPARE(model.checkUpdateStatus(), completedStatus);
        QCOMPARE(model.showCheckUpdate(), !availableUpdates);
        QCOMPARE(info->packages(), packages);
        // Consuming the first visit must not disable time-based refreshes.
        control("Remove", {false});
        model.setLastCheckUpdateTime(QDateTime::currentDateTime().addSecs(-25 * 3600).toString("yyyy-MM-dd hh:mm:ss"));
        worker.checkNeedDoUpdates();
        QTRY_COMPARE(control("Starts").arguments().first().toInt(), 1);
        QTRY_COMPARE(model.checkUpdateStatus(), Checking);
    }

    void delayedCompletionMustNotSuppressReplacement()
    {
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        // Export the replacement before the client processes the old success.
        control("CompleteThenReplace");
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QCOMPARE(model.checkUpdateStatus(), Checking);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.4);
    }

    void confirmedFailureMustSurviveRediscovery_data()
    {
        QTest::addColumn<QString>("errorType");
        QTest::addColumn<int>("expectedError");
        QTest::newRow("network") << QStringLiteral("fetchFailed") << int(NoNetwork);
        QTest::newRow("space") << QStringLiteral("insufficientSpace") << int(NoSpace);
    }

    void confirmedFailureMustSurviveRediscovery()
    {
        QFETCH(QString, errorType);
        QFETCH(int, expectedError);
        control("Create");
        control("RetainCleanedJob", {true});
        const auto description = QString::fromUtf8(QJsonDocument(QJsonObject{
            {"ErrType", errorType}, {"ErrDetail", "confirmed failure"}
        }).toJson(QJsonDocument::Compact));
        control("SetDescription", {description});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("SetState", {"failed", 0.2, true});
        QTRY_COMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(int(model.lastError(CheckingFailed)), expectedError);
        QTRY_COMPARE(control("Cleans").arguments().first().toInt(), 1);
        // CleanJob has moved this instance to end, but it is still exported.
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QTest::qWait(100);
        QCOMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(int(model.lastError(CheckingFailed)), expectedError);
        QCOMPARE(model.lastErrorLog(CheckingFailed), description);
        // The recorded failure must not block a new instance reusing its path.
        control("Remove", {false});
        control("Create");
        control("SetState", {"running", 0.4, false});
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QCOMPARE(model.checkUpdateStatus(), Checking);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.4);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
    }

    void adoptsBackgroundCheckWithCorrectVisibility_data()
    {
        QTest::addColumn<QString>("backendStatus");
        QTest::addColumn<bool>("showCheckPage");
        QTest::newRow("no-download") << QString() << true;
        QTest::newRow("updates-available") << QStringLiteral("notDownload") << true;
        QTest::newRow("downloading") << QStringLiteral("isDownloading") << false;
        QTest::newRow("download-paused") << QStringLiteral("downloadPause") << false;
    }

    void adoptsBackgroundCheckWithCorrectVisibility()
    {
        QFETCH(QString, backendStatus);
        QFETCH(bool, showCheckPage);
        control("Create");
        UpdateModel model;
        if (!backendStatus.isEmpty()) {
            model.setUpdateStatus(QJsonDocument(QJsonObject{
                {"UpdateStatus", QJsonObject{{"system_upgrade", backendStatus}}}
            }).toJson());
            model.setLastStatus(LastoreDaemonUpdateStatus::string2Status(backendStatus), __LINE__);
        }
        const auto downloadStatus = model.updateStatus(CPT_Downloading);
        if (!showCheckPage)
            QVERIFY(downloadStatus == Downloading || downloadStatus == DownloadPaused);
        QVERIFY(!model.showCheckUpdate());
        UpdateWorker worker(&model);
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QCOMPARE(model.checkUpdateStatus(), Checking);
        QCOMPARE(model.showCheckUpdate(), showCheckPage);
        worker.checkNeedDoUpdates();
        QCOMPARE(model.showCheckUpdate(), showCheckPage);
        QCOMPARE(control("Starts").arguments().first().toInt(), 0);
        if (!showCheckPage) {
            QCOMPARE(model.updateStatus(CPT_Downloading), downloadStatus);
            QCOMPARE(model.lastStatus(), int(downloadStatus));
        }
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("SetState", {"running", 0.4, true});
        QTRY_COMPARE(model.checkUpdateProgress(), 0.4);
        QCOMPARE(model.showCheckUpdate(), showCheckPage);
        control("SetState", {"failed", 0.4, true});
        QTRY_COMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(model.showCheckUpdate(), showCheckPage);
        if (showCheckPage) {
            worker.reCheckWithUi();
            QTRY_COMPARE(model.checkUpdateStatus(), Checking);
            QVERIFY(model.showCheckUpdate());
            QCOMPARE(control("Starts").arguments().first().toInt(), 1);
        }
    }

    void recoversProgressAndSuccessWithoutSignals()
    {
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("SetState", {"running", 0.65, false});
        QTRY_COMPARE_WITH_TIMEOUT(model.checkUpdateProgress(), 0.65, 4000);
        control("SetState", {"succeed", 1.0, false});
        QTRY_COMPARE_WITH_TIMEOUT(model.checkUpdateStatus(), Updated, 4000);
        QCOMPARE(model.checkUpdateProgress(), 1.0);
        // Success must allow an immediate retry even without an end signal.
        control("SetState", {"running", 0.1, false});
        worker.doCheckUpdates();
        QTRY_COMPARE(model.checkUpdateStatus(), Checking);
    }

    void removedJobIsRetryable_data()
    {
        QTest::addColumn<bool>("emitJobList");
        QTest::newRow("missing-all-signals") << false;
        QTest::newRow("job-list-only") << true;
    }

    void removedJobIsRetryable()
    {
        QFETCH(bool, emitJobList);
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("Remove", {emitJobList});
        QTRY_COMPARE_WITH_TIMEOUT(model.checkUpdateStatus(), CheckingFailed, 4000);
        QCOMPARE(model.lastError(CheckingFailed), UnKnown);
        worker.doCheckUpdates();
        QTRY_COMPARE(model.checkUpdateStatus(), Checking);
    }

    void removedBeforeObservationIsRetryable()
    {
        control("Create");
        // The job disappears before its returned path can be observed.
        control("Remove", {false});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(model.lastError(CheckingFailed), UnKnown);
        QCOMPARE(control("Cleans").arguments().first().toInt(), 0);

        worker.doCheckUpdates();
        QTRY_COMPARE(control("Starts").arguments().first().toInt(), 1);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        QCOMPARE(model.checkUpdateStatus(), Checking);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
    }

    void endWithoutSuccessIsNotReportedAsSuccess()
    {
        control("Create");
        control("SetState", {"end", 1.0, false});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateStatus(), CheckingFailed);
    }

    void preservesExplicitFailure()
    {
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("SetState", {"failed", 0.2, true});
        QTRY_COMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(model.lastError(CheckingFailed), NoNetwork);
        QTRY_COMPARE(control("Cleans").arguments().first().toInt(), 1);
    }

    void lateReplyDoesNotRestartCompletedCheck()
    {
        control("DelayReply", {800});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.doCheckUpdates();
        worker.doCheckUpdates();
        QTRY_COMPARE(model.checkUpdateStatus(), Checking);
        QCOMPARE(control("Starts").arguments().first().toInt(), 1);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
        QTest::qWait(1000);
        QCOMPARE(model.checkUpdateStatus(), Updated);
    }

    void staleJobListDoesNotFailExistingJob()
    {
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        worker.onJobListChanged({});
        QTest::qWait(100);
        QCOMPARE(model.checkUpdateStatus(), Checking);
    }

    void oldSnapshotCannotOverwriteNewCheck()
    {
        control("Create");
        control("DelaySnapshot", {600});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTest::qWait(100);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);

        control("DelaySnapshot", {0});
        control("SetState", {"running", 0.7, false});
        worker.doCheckUpdates();
        QTRY_COMPARE(model.checkUpdateProgress(), 0.7);
        QTest::qWait(700);
        QCOMPARE(model.checkUpdateStatus(), Checking);
        QCOMPARE(model.checkUpdateProgress(), 0.7);
    }

    void oldSnapshotCannotOverwriteNewSignal()
    {
        control("Create");
        control("DelaySnapshot", {600});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTest::qWait(100);
        control("SetState", {"running", 0.7, true});
        QTRY_COMPARE(model.checkUpdateProgress(), 0.7);
        QTest::qWait(700);
        QCOMPARE(model.checkUpdateProgress(), 0.7);
    }

    void transientPollingErrorKeepsObserving_data()
    {
        QTest::addColumn<QString>("errorName");
        QTest::newRow("no-reply") << QStringLiteral("org.freedesktop.DBus.Error.NoReply");
        QTest::newRow("timeout") << QStringLiteral("org.freedesktop.DBus.Error.Timeout");
        QTest::newRow("failed") << QStringLiteral("org.freedesktop.DBus.Error.Failed");
        QTest::newRow("disconnected") << QStringLiteral("org.freedesktop.DBus.Error.Disconnected");
        QTest::newRow("other") << QStringLiteral("org.example.Test.TemporaryFailure");
    }

    void transientPollingErrorKeepsObserving()
    {
        QFETCH(QString, errorName);
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        const int snapshots = control("Snapshots").arguments().first().toInt();
        control("SnapshotError", {errorName});
        worker.onJobListChanged({});
        QTRY_VERIFY(control("Snapshots").arguments().first().toInt() > snapshots);
        QTest::qWait(100);
        QCOMPARE(model.checkUpdateStatus(), Checking);
        control("SnapshotError", {QString()});
        control("SetState", {"succeed", 1.0, false});
        QTRY_COMPARE_WITH_TIMEOUT(model.checkUpdateStatus(), Updated, 4000);
        QCOMPARE(control("Cleans").arguments().first().toInt(), 0);
    }

    void missingPollingTargetIsRetryable_data()
    {
        QTest::addColumn<QString>("errorName");
        QTest::newRow("object") << QStringLiteral("org.freedesktop.DBus.Error.UnknownObject");
        QTest::newRow("godbus-object") << QStringLiteral("org.freedesktop.DBus.Error.NoSuchObject");
        QTest::newRow("interface") << QStringLiteral("org.freedesktop.DBus.Error.UnknownInterface");
        QTest::newRow("service") << QStringLiteral("org.freedesktop.DBus.Error.ServiceUnknown");
        QTest::newRow("owner") << QStringLiteral("org.freedesktop.DBus.Error.NameHasNoOwner");
    }

    void missingPollingTargetIsRetryable()
    {
        QFETCH(QString, errorName);
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("SnapshotError", {errorName});
        worker.onJobListChanged({});
        QTRY_COMPARE(model.checkUpdateStatus(), CheckingFailed);
        control("SnapshotError", {QString()});
        worker.doCheckUpdates();
        QTRY_COMPARE(model.checkUpdateStatus(), Checking);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
    }

    void pollingTimeoutKeepsObserving()
    {
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("DelaySnapshot", {6500});
        worker.onJobListChanged({});
        QTest::qWait(5600);
        QCOMPARE(model.checkUpdateStatus(), Checking);
        control("DelaySnapshot", {0});
        control("SetState", {"succeed", 1.0, false});
        QTRY_COMPARE_WITH_TIMEOUT(model.checkUpdateStatus(), Updated, 4000);
    }

    void completedJobIsNotRediscovered_data()
    {
        QTest::addColumn<QString>("terminalStatus");
        QTest::newRow("succeed") << QStringLiteral("succeed");
        QTest::newRow("end") << QStringLiteral("end");
    }

    void completedJobIsNotRediscovered()
    {
        QFETCH(QString, terminalStatus);
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
        control("SetState", {terminalStatus, 1.0, false});
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QCOMPARE(model.checkUpdateStatus(), Updated);
        QTest::qWait(100);
        QCOMPARE(model.checkUpdateStatus(), Updated);
        QCOMPARE(model.checkUpdateProgress(), 1.0);
    }

    void reusedJobPathCanBeDiscovered_data()
    {
        QTest::addColumn<bool>("alreadySucceeded");
        QTest::newRow("running") << false;
        QTest::newRow("already-succeeded") << true;
    }

    void reusedJobPathCanBeDiscovered()
    {
        QFETCH(bool, alreadySucceeded);
        control("Create");
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.1);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
        control("Remove", {false});
        control("Create");
        control("SetState", {alreadySucceeded ? "succeed" : "running", alreadySucceeded ? 1.0 : 0.4, false});
        worker.onJobListChanged({QDBusObjectPath(Job)});
        QCOMPARE(model.checkUpdateStatus(), Checking);
        if (!alreadySucceeded) {
            QTRY_COMPARE(model.checkUpdateProgress(), 0.4);
            control("SetState", {"succeed", 1.0, true});
        }
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
    }

    void polledFailureDoesNotCancelBackendRetry_data()
    {
        QTest::addColumn<bool>("manualRetry");
        QTest::newRow("backend-retry") << false;
        QTest::newRow("user-retry") << true;
    }

    void polledFailureDoesNotCancelBackendRetry()
    {
        QFETCH(bool, manualRetry);
        control("Create");
        control("SetState", {"failed", 0.2, false});
        UpdateModel model;
        UpdateWorker worker(&model);
        worker.setCheckUpdatesJob(Job);
        QTRY_COMPARE(model.checkUpdateStatus(), CheckingFailed);
        QCOMPARE(control("Cleans").arguments().first().toInt(), 0);
        control("SetState", {"running", 0.4, !manualRetry});
        if (manualRetry)
            worker.doCheckUpdates();
        QTRY_COMPARE(model.checkUpdateStatus(), Checking);
        QTRY_COMPARE(model.checkUpdateProgress(), 0.4);
        control("SetState", {"succeed", 1.0, true});
        QTRY_COMPARE(model.checkUpdateStatus(), Updated);
    }
};

int main(int argc, char **argv)
{
    // dbus-run-session provides an isolated bus for both system and session calls.
    if (qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS"))
        return 2;
    qputenv("DBUS_SYSTEM_BUS_ADDRESS", qgetenv("DBUS_SESSION_BUS_ADDRESS"));
    if (argc > 1 && QByteArray(argv[1]) == "--mock-service") {
        QCoreApplication app(argc, argv);
        qDBusRegisterMetaType<LastoreUpdatePackagesInfo>();
        qDBusRegisterMetaType<QList<QDBusObjectPath>>();
        MockLastore service;
        auto bus = QDBusConnection::systemBus();
        if (!bus.registerService(Service) || !bus.registerVirtualObject(Root, &service, QDBusConnection::SubPath))
            return 3;
        return app.exec();
    }
    QApplication app(argc, argv);
    CheckUpdateTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "check-update-test.moc"
