// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.0
import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0
import QtQuick.Layouts 1.15

DccObject {
    DccObject {
        name: "noActive"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: DccObject.AutoBg
        weight: 10
        visible: !dccData.model().systemActivation
        page: NoActive{}
    }

    DccObject {
        name: "checkUpdatePage"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: DccObject.Normal
        visible: dccData.model().systemActivation && !dccData.model().showUpdateCtl
        weight: 20
        page: CheckUpdate{}
    }

    // 安装完成列表
    DccObject {
        name: "installCompleteList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 30
        visible:  dccData.model().installCompleteListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl{

            updateListModels: dccData.model().installCompleteListModel
            updateStateTips: qsTr("Update installation successful")
            updateTips: qsTr("To ensure proper functioning of your system and applications, please restart your computer after the update")
            actionBtnText: qsTr("Reboot now")
            checkVisible: false

            onBtnClicked: function(updateType){
                dccData.work().reStart()
            }
        }
    }

    // 安装失败列表
    DccObject {
        name: "installFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight:35
        pageType: DccObject.Item
        visible: dccData.model().installFailedListModel.anyVisible
        page:  UpdateControl{
            updateListModels: dccData.model().installFailedListModel
            updateStateTips: qsTr("Installation update failed")
            actionBtnText: qsTr("Continue Update")
            updateTips: dccData.model().downloadFailedTips

            checkVisible: false
            onBtnClicked: function(updateType){
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
        page: UpdateControl{

            checkVisible: false
            updateListModels: dccData.model().installinglistModel
            updateStateTips: qsTr("Installing updates...")
            updateTitle: qsTr("Installing")

            processValue: dccData.model().distUpgradeProgress

        }
    }

    // 下载完成列表
    DccObject {
        name: "preInstallList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 50
        pageType: DccObject.Item
        visible: dccData.model().preInstallListModel.anyVisible
        page:  UpdateControl{
            updateListModels: dccData.model().preInstallListModel
            updateStateTips: qsTr("Update download completed")
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
        pageType: DccObject.Item
        visible: dccData.model().downloadFailedListModel.anyVisible
        page:  UpdateControl{
            updateListModels: dccData.model().downloadFailedListModel
            updateStateTips: qsTr("Update download failed")
            actionBtnText: qsTr("Retry")
            checkVisible: false
            updateTips: dccData.model().installFailedTips

            onBtnClicked: function(updateType){
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
        pageType: DccObject.Item
        visible: dccData.model().downloadinglistModel.anyVisible
        page:  UpdateControl{
            updateListModels: dccData.model().downloadinglistModel
            updateStateTips: qsTr("Downloading updates...")
            updateTips: qsTr("Update size: ") + dccData.model().downloadinglistModel.downloadSize
            updateTitle: qsTr("Downloading")

            checkVisible: false
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
        pageType: DccObject.Item
        visible: dccData.model().preUpdatelistModel.anyVisible
        page: UpdateControl {
            updateListModels: dccData.model().preUpdatelistModel
            updateStateTips: qsTr("Updates Available")
            actionBtnText: qsTr("Download")
            checkVisible: false
            updateTips: qsTr("Update size: ") + dccData.model().preUpdatelistModel.downloadSize

            onBtnClicked: function(updateType){
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
        page: UpdateControl{

            checkVisible: false
            updateListModels: dccData.model().backingUpListModel
            updateStateTips: qsTr("Backing up in progress...")
            updateTitle: qsTr("Backing up in progress")

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
        page: UpdateControl{

            checkVisible: false
            updateListModels: dccData.model().backupFailedListModel
            updateTitle: qsTr("Backup failed")
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
        visible: dccData.model().systemActivation

        UpdateSetting{}
    }
}
