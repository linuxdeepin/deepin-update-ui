// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
// import org.deepin.dtk 1.0 as D
import QtQuick 2.15
import QtQuick.Controls 2.0
import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0
import QtQuick.Layouts 1.15

DccObject {
    DccObject {
        name: "noActive11"
        parentName: "update"
        pageType: DccObject.Item
        backgroundType: DccObject.Audobg
        weight: 10
        visible: false
        page: NoActive{}
    }

    DccObject {
        name: "checkUpdatePage"
        parentName: "update"
        //displayName: qsTr("check update")
        //description: dccData.model().upgradable ? qsTr("Your system is already the latest version") : qsTr("You have a new system update, please check and update")
        pageType: DccObject.Item
        backgroundType: DccObject.Audobg
        visible: !dccData.model().showUpdateCtl
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

            updateStateTips: "更新完成"

            // updateListModels: ListModel {
            //         ListElement {
            //             name: qsTr("Feature Updates")
            //             checked: true
            //         }
            //     }
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
            updateStateTips: "更新失败"
        }
    }


    DccObject {
        name: "installingList"
        parentName: "update"
        backgroundType: DccObject.Normal
        weight: 40
        //visible: false
        visible: dccData.model().installinglistModel.anyVisible
        pageType: DccObject.Item
        page: UpdateControl{

            updateListModels: dccData.model().installinglistModel
            updateStateTips: "正在下载更新"

            // updateListModels: ListModel {
            //     ListElement {
            //         name: qsTr("Feature Updates")
            //         checked: true
            //     }
            //     ListElement {
            //         name: qsTr("Feature Updates")
            //         checked: true
            //     }
            //     ListElement {
            //         name: qsTr("Feature Updates")
            //         checked: true
            //     }
            // }
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
            updateStateTips: "更新下载完成"
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
            updateStateTips: "更新下载失败"
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
            updateStateTips: "正在下载。。"
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
            updateStateTips: "检查到有更新可用"
        }
    }

    DccObject {
        name: "updateSettingsPage"
        parentName: "update"
        displayName: qsTr("Update Settings")
      //  description: qsTr("You can set system updates, security updates, idle updates, update reminders, etc.")
        icon: "update_set"
        weight: 100

        visible: DccApp.uosEdition() === DccApp.UosCommunity

        UpdateSetting{}
    }

}
