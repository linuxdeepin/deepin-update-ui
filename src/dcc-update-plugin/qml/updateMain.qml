// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0

import org.deepin.dcc.update 1.0

DccObject {

    // 更新通知小红点
    function updateNotifyRedPoint() {
        if (dccData.model().isUpdateDisabled || !dccData.model().updateNotify || !dccData.model().isUpdatable) {
            dccModule.badge = 0
            return
        }
        dccModule.badge = 1
    }

    Component.onCompleted: {
        updateNotifyRedPoint()
    }

    // 处理更新模块激活和子组件变化的连接
    Connections {
        // 标识是否已经检查过更新
        property bool hasCheckUpdate: false
        
        target: dccModule
        // 当更新模块被激活时，检查是否需要更新
        function onActiveUpdateModel() {
            hasCheckUpdate = true
            dccData.work().checkNeedDoUpdates()
        }
        // 当子组件发生变化时，如果视图存在且未检查过更新，则进行检查，这里主要适用于dbus直接调起更新模块的情况
        function onChildrenChanged() {
            if (dccModule.hasView && !hasCheckUpdate) {
                hasCheckUpdate = true
                dccData.work().checkNeedDoUpdates()
            }
        }
    }

    // 处理系统激活状态和更新禁用状态变化的连接
    Connections {
        target: dccData.model()
        // 当更新禁用状态改变时，如果更新未被禁用则检查更新
        function onIsUpdateDisabledChanged() {
            if (dccModule.hasView && !dccData.model().isUpdateDisabled) {
                dccData.work().checkNeedDoUpdates()
            }
            updateNotifyRedPoint()
        }
        function onUpdateNotifyChanged() {
            updateNotifyRedPoint()
        }
        function onIsUpdatableChanged() {
            updateNotifyRedPoint()
        }
    }

    Loader {
        id: dialogloader
        active: false
        sourceComponent: UpdateLogDialog {
            logContent: dccData.model().updateInstallLog
            onClosing: {
                dialogloader.active = false
            }
            onExportBtnClicked: {
                dccData.work().exportLogToDesktop()
            }
        }
        onLoaded: {
            item.show()
        }
    }

    DccObject {
        id: updateDisable
        name: "updateDisable"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: dccData.model().systemActivation ? DccObject.Normal : DccObject.AutoBg
        weight: 10
        visible: dccData.model().isUpdateDisabled
        page: UpdateDisable {}
    }

    DccObject {
        name: "checkUpdatePage"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: DccObject.Normal
        visible: !dccData.model().isUpdateDisabled && dccData.model().showCheckUpdate
        weight: 20
        page: CheckUpdate {}
    }

    // 正在安装列表
    DccObject {
        name: "installingList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 30
        visible: !dccData.model().isUpdateDisabled  && !dccData.model().showCheckUpdate && dccData.model().installinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installinglistModel
            updateTitle: qsTr("Installing updates...")
            processTitle: qsTr("Installing")
            processValue: dccData.model().distUpgradeProgress
            processState: true
            updateListEnable: false

            showLogButton: true
            onViewLogClicked: {
                dialogloader.active = true
            }
        }
    }

    // 正在备份列表
    DccObject {
        name: "backupList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 40
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate && dccData.model().backingUpListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().backingUpListModel
            updateTitle: qsTr("Backing up in progress...")
            processTitle: qsTr("Backing up in progress")
            processValue: dccData.model().backupProgress
            processState: true
            updateListEnable: false

            showLogButton: true
            onViewLogClicked: {
                dialogloader.active = true
            }
        }
    }
    
    // 正在下载列表
    DccObject {
        name: "downloadingList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 50
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate && dccData.model().downloadinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().downloadinglistModel
            updateTitle: qsTr("Downloading updates...")
            updateTips: qsTr("Update size: ") + dccData.model().downloadinglistModel.downloadSize
            processTitle: qsTr("Downloading")
            processValue: dccData.model().downloadProgress
            processState: true
            updateListEnable: false
            isDownloading: true
            isPauseOrNot: dccData.model().downloadPaused

            onStartDownload: {
                dccData.work().downloadJobCtrl(Common.Start)
            }

            onPauseDownload: {
                dccData.work().downloadJobCtrl(Common.Pause)
            }

            onCloseDownload: {
                dccData.work().stopDownload()
            }
        }
    }

    // 安装完成列表
    DccObject {
        name: "installCompleteList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 60
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate && dccData.model().installCompleteListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installCompleteListModel
            updateStateIcon: "qrc:/icons/deepin/builtin/icons/success.svg"
            updateTitle: qsTr("Update installation successful")
            updateTips: qsTr("To ensure proper functioning of your system and applications, please restart your computer after the update")
            btnActions: [ qsTr("Reboot now") ]
            updateListcheck: false

            onBtnClicked: function(index, updateType) {
                dccData.work().reStart()
            }
        }
    }

    // 安装失败列表
    DccObject {
        name: "installFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 70
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate && dccData.model().installFailedListModel.anyVisible
        enabled: !dccData.model().backingUpListModel.anyVisible && !dccData.model().installinglistModel.anyVisible && dccData.model().batterIsOK
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installFailedListModel
            updateStateIcon: "qrc:/icons/deepin/builtin/icons/warning.svg"
            updateTitle: {
                if (!dccData.model().batterIsOK) {
                    return qsTr("The battery capacity is lower than 60%. To get successful updates, please plug in.")
                }
                return qsTr("Installation update failed")
            }

            showLogButton: true
            onViewLogClicked: {
                dialogloader.active = true
            }

            updateTips: dccData.model().installFailedTips
            btnActions: [ qsTr("Continue Update") ]

            onBtnClicked: function(index, updateType) {
                dccData.work().onRequestRetry(Common.CPT_UpgradeFailed, updateType)
            }
        }
    }

    // 备份失败列表
    DccObject {
        name: "backupFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 80
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate && dccData.model().backupFailedListModel.anyVisible
        enabled: !dccData.model().backingUpListModel.anyVisible && !dccData.model().installinglistModel.anyVisible && dccData.model().batterIsOK
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().backupFailedListModel
            processTitle: qsTr("Backup failed")
            updateTitle: qsTr("Backup failed")
            updateTips: {
                if (!dccData.model().batterIsOK) {
                    return qsTr("The battery capacity is lower than 60%. To get successful updates, please plug in.")
                }
                return qsTr("If you continue the updates, you cannot roll back to the old system later.")
            }

            showLogButton: true
            onViewLogClicked: {
                dialogloader.active = true
            }

            btnActions: [ qsTr("Try Again"), qsTr("Proceed to Update") ]
            onBtnClicked: function(index, updateType) {
                console.log("index: " + index, " updateType: " + updateType)
                if (index === 0) {
                    dccData.work().onRequestRetry(Common.CPT_BackupFailed, updateType)
                } else {
                    dccData.work().doUpgrade(updateType, false)
                }
            }
        }
    }

    // 下载完成列表
    DccObject {
        name: "preInstallList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 90
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate && dccData.model().preInstallListModel.anyVisible
        enabled: !dccData.model().backingUpListModel.anyVisible && !dccData.model().installinglistModel.anyVisible && dccData.model().batterIsOK
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().preInstallListModel
            updateTitle: qsTr("Update download completed")
            btnActions: [ qsTr("Install updates") ]
            updateTips: {
                if (!dccData.model().batterIsOK) {
                    return qsTr("The battery capacity is lower than 60%. To get successful updates, please plug in.")
                }
                return qsTr("Update size: ") + dccData.model().preInstallListModel.downloadSize
            }
            busyState: dccData.model().upgradeWaiting
            updateListEnable: !dccData.model().upgradeWaiting

            onBtnClicked: function(index, updateType) {
                updateSelectDialog.updateType = updateType
                updateSelectDialog.show()
            }

            UpdateSelectDialog {
                id: updateSelectDialog
                property var updateType: 0
                palette: parent.palette
                visible: false
                onSilentBtnClicked: {
                    dccData.work().doUpgrade(updateType, true)
                }
                onUpgradeRebootBtnClicked: {
                    dccData.work().modalUpgrade(true)
                }
                onUpgradeShutdownBtnClicked: {
                    dccData.work().modalUpgrade(false)
                }
            }

            Connections {
                target: dccData.work()
                function onStartDoUpgrade() {
                    updateSelectDialog.close()
                }
            }
        }
    }

    // 下载失败列表
    DccObject {
        name: "downloadFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 100
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate && dccData.model().downloadFailedListModel.anyVisible
        enabled: !dccData.model().downloadinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().downloadFailedListModel
            updateTitle: qsTr("Update download failed")
            btnActions: [ qsTr("Retry") ]
            updateTips: dccData.model().downloadFailedTips
            busyState: dccData.model().upgradeWaiting
            updateListEnable: !dccData.model().upgradeWaiting

            onBtnClicked: function(index, updateType) {
                dccData.work().onRequestRetry(Common.CPT_DownloadFailed, updateType)
            }
        }
    }

    // 检测到可更新列表
    DccObject {
        name: "preUpdateList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 110
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate && dccData.model().preUpdatelistModel.anyVisible
        enabled: !dccData.model().downloadinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().preUpdatelistModel
            updateTitle: !dccData.model().downloadWaiting ? qsTr("Updates Available") : qsTr("Downloading updates...")
            btnActions: [ qsTr("Download") ]
            updateTips: qsTr("Update size: ") + dccData.model().preUpdatelistModel.downloadSize
            busyState: dccData.model().downloadWaiting
            updateListEnable: !dccData.model().downloadWaiting

            onBtnClicked: function(index, updateType) {
                dccData.work().startDownload(updateType)
            }
        }
    }

    // 更新设置
    DccObject {
        name: "updateSettingsPage"
        parentName: "update"
        displayName: qsTr("Update Settings")
        description: qsTr("Configure Update settings、Security Updates、Auto Download Updates and Updates Notification")
        icon: "update_set"
        weight: 120
        visible: dccData.model().systemActivation

        UpdateSetting {}
    }

    // 隐私协议
    DccObject {
        name: "privacyAgreement"
        parentName: "update"
        weight: 1000
        backgroundType: DccObject.Normal
        visible: !dccData.model().isUpdateDisabled && !dccData.model().showCheckUpdate
        pageType: DccObject.Item

        page: RowLayout {
            Rectangle {
                color: 'transparent'
                Layout.fillWidth: true
                Layout.preferredHeight: privacyLabel.implicitHeight + 20
                D.Label {
                    id: privacyLabel
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    font: D.DTK.fontManager.t10
                    opacity: 0.7
                    textFormat: Text.RichText
                    text: dccData.model().privacyAgreementText()
                    wrapMode: Text.Wrap
                    onLinkActivated: (link)=> {
                        dccData.work().openUrl(link)
                    }
                }
            }
        }
    }
}
