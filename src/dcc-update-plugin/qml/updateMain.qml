// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0

DccObject {
    DccObject {
        name: "noActive"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: DccObject.AutoBg
        weight: 10
        visible: !dccData.model().systemActivation
        page: NoActive {}
    }

    DccObject {
        name: "checkUpdatePage"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: DccObject.Normal
        visible: dccData.model().systemActivation && !dccData.model().showUpdateCtl
        weight: 20
        page: CheckUpdate {}
    }

    // 安装完成列表
    DccObject {
        name: "installCompleteList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 30
        visible: dccData.model().installCompleteListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installCompleteListModel
            updateStateIcon: "qrc:/icons/deepin/builtin/icons/success.svg"
            updateTitle: qsTr("Update installation successful")
            updateTips: qsTr("To ensure proper functioning of your system and applications, please restart your computer after the update")
            actionBtnText: qsTr("Reboot now")

            onBtnClicked: function(updateType) {
                dccData.work().reStart()
            }
        }
    }

    // 安装失败列表
    DccObject {
        name: "installFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 35
        visible: dccData.model().installFailedListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installFailedListModel
            updateStateIcon: "qrc:/icons/deepin/builtin/icons/warning.svg"
            updateTitle: qsTr("Installation update failed")
            updateTips: dccData.model().downloadFailedTips
            actionBtnText: qsTr("Continue Update")

            onBtnClicked: function(updateType) {
                dccData.work().onRequestRetry(2, updateType)
            }
        }
    }

    // 正在安装列表
    DccObject {
        name: "installingList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 40
        visible: dccData.model().installinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().installinglistModel
            updateTitle: qsTr("Installing updates...")
            processTitle: qsTr("Installing")
            processValue: dccData.model().distUpgradeProgress
        }
    }

    // 下载完成列表
    DccObject {
        name: "preInstallList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 50
        visible: dccData.model().preInstallListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().preInstallListModel
            updateTitle: qsTr("Update download completed")
            actionBtnText: qsTr("Install updates")
            updateTips: qsTr("Update size: ") + dccData.model().preInstallListModel.downloadSize

            onBtnClicked: function(updateType) {
                dccData.work().doUpgrade(updateType, true)
            }
        }
    }

    // 下载失败列表
    DccObject {
        name: "downloadFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 60
        visible: dccData.model().downloadFailedListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().downloadFailedListModel
            updateTitle: qsTr("Update download failed")
            actionBtnText: qsTr("Retry")
            updateTips: dccData.model().installFailedTips

            onBtnClicked: function(updateType) {
                dccData.work().onRequestRetry(6, updateType)
            }
        }
    }

    // 正在下载列表
    DccObject {
        name: "downloadingList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 65
        visible: dccData.model().downloadinglistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().downloadinglistModel
            updateTitle: qsTr("Downloading updates...")
            updateTips: qsTr("Update size: ") + dccData.model().downloadinglistModel.downloadSize
            processTitle: qsTr("Downloading")
            processValue: dccData.model().downloadProgress

            onDownloadJobCtrl: function(updateCtrlType) {
                dccData.work().onDownloadJobCtrl(updateCtrlType)
            }

            onCloseDownload: {
                dccData.work().stopDownload()
            }
        }
    }

    // 检测到可更新列表
    DccObject {
        name: "preUpdateList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 70
        visible: dccData.model().preUpdatelistModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().preUpdatelistModel
            updateTitle: qsTr("Updates Available")
            actionBtnText: qsTr("Download")
            updateTips: qsTr("Update size: ") + dccData.model().preUpdatelistModel.downloadSize

            onBtnClicked: function(updateType) {
                dccData.work().startDownload(updateType)
            }
        }
    }

    // 正在备份列表
    DccObject {
        name: "backupList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 80
        visible: dccData.model().backingUpListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().backingUpListModel
            updateTitle: qsTr("Backing up in progress...")
            processTitle: qsTr("Backing up in progress")
            processValue: dccData.model().backupProgress
        }
    }

    // 备份失败列表
    DccObject {
        name: "backupFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 90
        visible: dccData.model().backupFailedListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl {
            updateListModels: dccData.model().backupFailedListModel
            processTitle: qsTr("Backup failed")
            updateTitle: qsTr("Backup failed")
            updateTips: dccData.model().backUpFailedTips
            onBtnClicked: function(updateType) {
            actionBtnText: qsTr("Back Up Again")
                dccData.work().onRequestRetry(3, updateType)
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
        weight: 100
        visible: false //dccData.model().systemActivation

        UpdateSetting {}
    }
}
