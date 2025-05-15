// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0

import org.deepin.dcc.update 1.0

DccObject {

    Connections {
        target: DccApp
        function onActiveObjectChanged(obj) {
            if (obj === dccModule) {
                dccData.work().checkNeedDoUpdates()
            }
        }
    }

    Connections {
        target: dccData.model()
        function onSystemActivationChanged() {
            if (dccData.model().systemActivation) {
                dccData.work().checkNeedDoUpdates()
            }
        }
    }

    Connections {
        target: dccData.model()
        function onIsUpdateDisabledChanged() {
            if (!dccData.model().isUpdateDisabled) {
                dccData.work().checkNeedDoUpdates()
            }
        }
    }

    DccObject {
        id: updateDisable
        name: "updateDisable"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: DccObject.AutoBg
        weight: 10
        visible: !dccData.model().systemActivation || dccData.model().isUpdateDisabled
        page: UpdateDisable {}
    }

    DccObject {
        name: "checkUpdatePage"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: DccObject.Normal
        visible: !updateDisable.visible && dccData.model().showCheckUpdate
        weight: 20
        page: CheckUpdate {}
    }

    // 正在安装列表
    DccObject {
        name: "installingList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 30
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().installinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installinglistModel
            updateTitle: qsTr("Installing updates...")
            processTitle: qsTr("Installing")
            processValue: dccData.model().distUpgradeProgress
            processState: true
            updateListEnable: false
        }
    }

    // 正在备份列表
    DccObject {
        name: "backupList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 40
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().backingUpListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().backingUpListModel
            updateTitle: qsTr("Backing up in progress...")
            processTitle: qsTr("Backing up in progress")
            processValue: dccData.model().backupProgress
            processState: true
            updateListEnable: false
        }
    }
    
    // 正在下载列表
    DccObject {
        name: "downloadingList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 50
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().downloadinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().downloadinglistModel
            updateTitle: qsTr("Downloading updates...")
            updateTips: qsTr("Update size: ") + dccData.model().downloadinglistModel.downloadSize
            processTitle: qsTr("Downloading")
            processValue: dccData.model().downloadProgress
            processState: true
            updateListEnable: false

            onDownloadJobCtrl: function(updateCtrlType) {
                dccData.work().onDownloadJobCtrl(updateCtrlType)
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
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().installCompleteListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installCompleteListModel
            updateStateIcon: "qrc:/icons/deepin/builtin/icons/success.svg"
            updateTitle: qsTr("Update installation successful")
            updateTips: qsTr("To ensure proper functioning of your system and applications, please restart your computer after the update")
            btnActions: [ qsTr("Reboot now") ]

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
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().installFailedListModel.anyVisible
        enabled: !dccData.model().backingUpListModel.anyVisible && !dccData.model().installinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installFailedListModel
            updateStateIcon: "qrc:/icons/deepin/builtin/icons/warning.svg"
            updateTitle: qsTr("Installation update failed")
            updateTips: dccData.model().downloadFailedTips
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
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().backupFailedListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().backupFailedListModel
            processTitle: qsTr("Backup failed")
            updateTitle: qsTr("Backup failed")
            updateTips: qsTr("If you continue the updates, you cannot roll back to the old system later.")
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
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().preInstallListModel.anyVisible
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
                dccData.work().doUpgrade(updateType, true)
            }
        }
    }

    // 下载失败列表
    DccObject {
        name: "downloadFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 100
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().downloadFailedListModel.anyVisible
        enabled: !dccData.model().downloadinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().downloadFailedListModel
            updateTitle: qsTr("Update download failed")
            btnActions: [ qsTr("Retry") ]
            updateTips: dccData.model().installFailedTips
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
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate && dccData.model().preUpdatelistModel.anyVisible
        enabled: !dccData.model().downloadinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().preUpdatelistModel
            updateTitle: qsTr("Updates Available")
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
        visible: !updateDisable.visible

        UpdateSetting {}
    }

    // 隐私协议
    DccObject {
        name: "privacyAgreement"
        parentName: "update"
        weight: 1000
        backgroundType: DccObject.Normal
        visible: !updateDisable.visible && !dccData.model().showCheckUpdate
        pageType: DccObject.Item

        page: RowLayout {
            Rectangle {
                color: 'transparent'
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                D.Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                    textFormat: Text.RichText
                    text: dccData.model().privacyAgreementText()
                    onLinkActivated: (link)=> {
                        dccData.work().openUrl(link)
                    }
                }
            }
        }
    }
}
