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
        backgroundType: DccObject.Audobg
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

    DccObject {
        name: "installCompleteList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 30
        visible:  dccData.model().installCompleteListModel.anyVisible
        pageType: DccObject.Item

        page: UpdateControl{

            updateListModels: dccData.model().installCompleteListModel
            updateStateTips: "Update installation successful"
            updateTips: qsTr("In order for you to use the system and applications normally, please restart your computer after the update")
            actionBtnText: qsTr("Restart Now")
            checkVisible: false

            onBtnClicked: function(updateType){
                console.log("立即重启 =================== ", updateType)
                dccData.work().reStart()
            }
        }
    }

    DccObject {
        name: "installFailedList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight:35
        pageType: DccObject.Item
        visible: dccData.model().installFailedListModel.anyVisible
        page:  UpdateControl{
            updateListModels: dccData.model().installFailedListModel
            updateStateTips: qsTr("Update failed")
            actionBtnText: qsTr("retry")
            updateTips: dccData.model().downloadFailedTips

            checkVisible: false
            onBtnClicked: function(updateType){
                console.log("更新失败 =================== ", updateType)

                dccData.work().onRequestRetry(2, updateType)
            }
        }
    }


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
            updateStateTips: qsTr("Installing updates")
            updateTitle: qsTr("Installing")

            processValue: dccData.model().distUpgradeProgress

        }
    }

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
            updateTips: qsTr("Update size: ") + dccData.model().downloadinglistModel.downloadSize + "G"

            onBtnClicked: function(updateType) {

                console.log("安装更新 =================== ", updateType)
                dccData.work().doUpgrade(updateType, false)
            }
        }
    }

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
            actionBtnText: qsTr("retry")
            checkVisible: false
            updateTips: dccData.model().installFailedTips

            onBtnClicked: function(updateType){
                console.log("重试 =================== ", updateType)
                dccData.work().onRequestRetry(6, updateType)
            }
        }
    }

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
            updateTips: qsTr("Update size: ") + dccData.model().downloadinglistModel.downloadSize + "G"
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

    DccObject {
        name: "preUpdateList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 70
        pageType: DccObject.Item
        visible: dccData.model().preUpdatelistModel.anyVisible
        page:  UpdateControl{
            updateListModels: dccData.model().preUpdatelistModel
            updateStateTips: qsTr("Detected updates available")
            actionBtnText: qsTr("download")
            checkVisible: false
            updateTips: qsTr("Update size: ") + dccData.model().downloadinglistModel.downloadSize + "G"

            onBtnClicked: function(updateType){

                console.log("下载 =================== ", updateType)
                dccData.work().startDownload(updateType)
            }
        }
    }

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
