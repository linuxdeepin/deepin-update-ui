// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import QtQuick.Controls 2.0
import Qt.labs.qmlmodels 1.2
import QtQuick.Layouts 1.15
import QtQuick

import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0

import org.deepin.dcc.update 1.0

DccObject {
    FontMetrics {
        id: fm
    }
    DccTitleObject {
        name: "updateTypeGrpTitle"
        parentName: "updateSettingsPage"
        displayName: qsTr("Update Type")
        weight: 10
    }

    DccObject {
        id: updateTypeGrp
        name: "updateTypeGrp"
        parentName: "updateSettingsPage"
        weight: 20
        pageType: DccObject.Item
        page: DccGroupView {
            height: implicitHeight + 10
            spacing: 0
        }

        // 下载、备份和安装过程中，按钮置灰不可用
        property bool editorEnabled: !dccData.model().downloadWaiting && 
                                     !dccData.model().upgradeWaiting && 
                                     !dccData.model().installinglistModel.anyVisible && 
                                     !dccData.model().backingUpListModel.anyVisible && 
                                     !dccData.model().downloadinglistModel.anyVisible &&
                                     !dccData.model().updateProhibited

        DccObject {
            name: "functionUpdate"
            parentName: "updateTypeGrp"
            displayName: qsTr("Function Updates")
            description: qsTr("Delivers a cumulative update including new features, quality updates, and  security updates")
            weight: 10
            enabled: updateTypeGrp.editorEnabled
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().functionUpdate
                onCheckedChanged: {
                    dccData.work().setFunctionUpdate(checked)
                }
            }
        }
        DccObject {
            name: "securityUpdate"
            parentName: "updateTypeGrp"
            displayName: qsTr("Security Updates")
            description: qsTr("Delivers security updates")
            weight: 20
            visible: dccData.model().securityUpdateEnabled && !dccData.model().isCommunitySystem()
            enabled: updateTypeGrp.editorEnabled
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().securityUpdate
                onCheckedChanged: {
                    dccData.work().setSecurityUpdate(checked)
                }
            }
        }
        DccObject {
            name: "thirdPartyUpdate"
            parentName: "updateTypeGrp"
            displayName: qsTr("Third-party Updates")
            description: qsTr("Delivers  updates for additional repository sources")
            weight: 30
            visible: dccData.model().thirdPartyUpdateEnabled
            enabled: updateTypeGrp.editorEnabled
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().thirdPartyUpdate
                onCheckedChanged: {
                    dccData.work().setThirdPartyUpdate(checked)
                }
            }
        }
    }

    DccObject {
        id: advancedSetting
        property bool showDetails: false
        name: "advancedSettingTitle"
        parentName: "updateSettingsPage"
        displayName: qsTr("Advanced Settings")
        weight: 30
        pageType: DccObject.Item
        page: RowLayout {
            Component.onCompleted: {
                advancedSetting.showDetails = false
            }
            DccLabel {
                property D.Palette textColor: D.Palette {
                    normal: Qt.rgba(0, 0, 0, 0.9)
                    normalDark: Qt.rgba(1, 1, 1, 0.9)
                }
                font: DccUtils.copyFont(D.DTK.fontManager.t5, {
                                            "weight": 700
                                        })
                text: dccObj.displayName
            }
            Item {
                Layout.fillWidth: true
            }
            D.ToolButton {
                visible: !advancedSetting.showDetails
                textColor: D.Palette {
                    normal {
                        common: D.DTK.makeColor(D.Color.Highlight)
                    }
                    normalDark: normal
                    hovered {
                        common: D.DTK.makeColor(D.Color.Highlight).lightness(+30)
                    }
                    hoveredDark: hovered
                }
                text: qsTr("Expand")
                onClicked: {
                    advancedSetting.showDetails = true
                }
                background: Item {}
            }
        }

        onParentItemChanged: {
            if (parentItem) {
                parentItem.leftPadding = 10
            }
        }
    }

    DccObject {
        name: "downloadLimitGrp"
        parentName: "updateSettingsPage"
        weight: 40
        visible: advancedSetting.showDetails
        pageType: DccObject.Item
        page: DccGroupView {
            height: implicitHeight + 10
            spacing: 0
        }

        DccObject {
            name: "downloadLimit"
            parentName: "downloadLimitGrp"
            displayName: qsTr("Limit Speed")
            weight: 10
            enabled: !dccData.model().updateProhibited
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().downloadSpeedLimitEnabled
                onCheckedChanged: {
                    dccData.work().setDownloadSpeedLimitEnabled(checked)
                }
            }
        }

        DccObject {
            name: "limitSetting"
            parentName: "downloadLimitGrp"
            displayName: qsTr("Limit Setting")
            visible: dccData.model().downloadSpeedLimitEnabled
            weight: 20
            enabled: !dccData.model().updateProhibited
            pageType: DccObject.Editor
            page: RowLayout {
                D.LineEdit {
                    width: Math.max(implicitWidth, editInputMinWidth)
                    text: dccData.model().downloadSpeedLimitSize
                    font.pixelSize: 14

                    onEditingFinished: {
                        dccData.work().setDownloadSpeedLimitSize(text)
                    }
                }

                D.Label {
                    text: "KB/S"
                    font.pixelSize: 14
                }
            }
        }
    }

    DccObject {
        name: "autoDownloadGrp"
        parentName: "updateSettingsPage"
        weight: 50
        pageType: DccObject.Item
        visible: advancedSetting.showDetails
        page: DccGroupView {
            height: implicitHeight + 10
            spacing: 0
        }
        DccObject {
            name: "autoDownload"
            parentName: "autoDownloadGrp"
            displayName: qsTr("Auto Download")
            description: qsTr("Enabling \"Auto Download Updates\" will automatically download updates when connected to the internet")
            weight: 10
            enabled: !dccData.model().updateProhibited && !dccData.model().updateModeDisabled
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().autoDownloadUpdates
                onCheckedChanged: {
                    dccData.work().setAutoDownloadUpdates(checked)
                }
            }
        }

        DccObject {
            name: "limitSetting"
            parentName: "autoDownloadGrp"
            displayName: qsTr("Download when Inactive")
            visible: dccData.model().autoDownloadUpdates
            weight: 20
            enabled: !dccData.model().updateProhibited && !dccData.model().updateModeDisabled
            pageType: DccObject.Item
            page: RowLayout {
                D.CheckBox {
                    id: inactiveDownloadCheckBox
                    Layout.leftMargin: 14
                    text: dccObj.displayName
                    font.pixelSize: 12
                    checked: {
                        if (!dccData.model().autoDownloadUpdates)
                            return false
                        
                        return dccData.model().idleDownloadEnabled
                    }
                    onCheckedChanged: {
                        dccData.work().setIdleDownloadEnabled(checked)
                    }
                }

                RowLayout {
                    spacing: 10
                    Layout.topMargin: 5
                    Layout.bottomMargin: 5
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                    D.Label {
                       text: qsTr("Start at")
                       enabled: inactiveDownloadCheckBox.checked
                    }

                    D.SpinBox {
                        value: dccData.model().beginTime
                        from: 0
                        to: 1440
                        implicitWidth: 80
                        font.pixelSize: 14
                        enabled: inactiveDownloadCheckBox.checked
                        textFromValue: function (value, locale) {
                            var time = Math.floor(value / 60)
                            var timeStr = time < 10 ? ("0" + time.toString()) : time.toString()
                            var minute =  value % 60
                            var minuteStr = minute < 10 ? ("0" + minute.toString()) : minute.toString()
                            return timeStr + ":"+ minuteStr
                        }
                        onValueChanged: function() {
                            dccData.work().setIdleDownloadBeginTime(value)
                        }
                    }

                    D.Label {
                        Layout.leftMargin: 10
                        text: qsTr("End at")
                        enabled: inactiveDownloadCheckBox.checked
                    }

                    D.SpinBox {
                        value: dccData.model().endTime
                        implicitWidth: 80
                        from: 0
                        to: 1439
                        font.pixelSize: 14
                        enabled: inactiveDownloadCheckBox.checked
                        textFromValue: function (value, locale) {
                            var time = Math.floor(value / 60)
                            var timeStr = time < 10 ? ("0" + time.toString()) : time.toString()
                            var minute =  value % 60
                            var minuteStr = minute < 10 ? ("0" + minute.toString()) : minute.toString()
                            return timeStr + ":"+ minuteStr
                        }
                        onValueChanged: function() {
                            dccData.work().setIdleDownloadEndTime(value)
                        }
                    }
                }
            }
        }
    }

    DccObject {
        name: "advancedSettingGrp"
        parentName: "updateSettingsPage"
        weight: 60
        visible: advancedSetting.showDetails
        pageType: DccObject.Item
        page: DccGroupView {
            height: implicitHeight + 10
            spacing: 0
        }

        DccObject {
            name: "updateReminder"
            parentName: "advancedSettingGrp"
            displayName: qsTr("Updates Notification")
            weight: 10
            pageType: DccObject.Editor
            enabled: !dccData.model().updateProhibited && !dccData.model().updateModeDisabled
            page: D.Switch {
                checked: dccData.model().updateNotify
                onCheckedChanged: {
                    dccData.work().setUpdateNotify(checked)
                }
            }
        }

        DccObject {
            name: "cleanCache"
            parentName: "advancedSettingGrp"
            displayName: qsTr("Clear Package Cache")
            weight: 20
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().autoCleanCache
                onCheckedChanged: {
                    dccData.work().setAutoCleanCache(checked)
                }
            }
        }

        DccObject {
            name: "updateHistory"
            parentName: "advancedSettingGrp"
            displayName: qsTr("Update History")
            weight: 40
            backgroundType: DccObject.Normal
            pageType: DccObject.Editor
            page: D.Button {
                implicitWidth: fm.advanceWidth(text) + fm.averageCharacterWidth * 2
                text: qsTr("View")
                onClicked: {
                    dccData.model().historyModel.refreshHistory()
                    uhloader.active = true
                }

                Loader {
                    id: uhloader
                    active: false
                    sourceComponent: UpdateHistoryDialog {
                        onClosing: function (close) {
                            uhloader.active = false
                        }
                    }
                    onLoaded: function () {
                        uhloader.item.show()
                    }
                }
            }
        }
    }

    DccObject {
        name: "mirrorSettingGrp"
        parentName: "updateSettingsPage"
        visible: advancedSetting.showDetails && dccData.model().isCommunitySystem()
        weight: 70
        pageType: DccObject.Item
        page: DccGroupView {
            height: implicitHeight + 10
            spacing: 0
        }

        DccObject {
            name: "autoMirror"
            parentName: "mirrorSettingGrp"
            displayName: qsTr("Smart Mirror Switch")
            weight: 10
            enabled: !dccData.model().updateProhibited
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().smartMirrorSwitch
                onCheckedChanged: {
                    dccData.work().setSmartMirror(checked)
                }
            }
        }

        DccObject {
            visible: false
            name: "defautMirror"
            parentName: "mirrorSettingGrp"
         //   displayName: qsTr("默认镜像源")
            weight: 20
            enabled: !dccData.model().updateProhibited
            pageType: DccObject.Editor
            page: D.ComboBox {
                model:["[SG]Ox.sg", "[AU]JAARNET", "[SE]Academic Computer Club", "[CN]阿里云"]
                // checked: dccData.model().audioMono

                currentIndex: 0
            }
        }
    }

    DccObject {
        name: "testingChannel"
        parentName: "updateSettingsPage"
        displayName: qsTr("Join Internal Testing Channel")
        description: qsTr("Forum users at level 2 and above can join the beta test to receive the latest updates.")
        backgroundType: DccObject.Normal
        visible: advancedSetting.showDetails && dccData.model().isCommunitySystem()
        weight: 80
        pageType: DccObject.Editor
        enabled: {
            if (dccData.model().testingChannelStatus === Common.WaitJoined || 
                dccData.model().testingChannelStatus === Common.WaitToLeave)
                return false
            else
                return true
        }

        page: D.Switch {
            checked: {
                if (dccData.model().testingChannelStatus === Common.WaitToLeave || 
                    dccData.model().testingChannelStatus === Common.Joined)
                    return true
                else
                    return false
            }

            onClicked: {
                dccData.work().setTestingChannelEnable(checked)
            }

            Connections {
                target: dccData.work()
                function onRequestCloseTestingChannel() {
                    ctcloader.active = true
                }
            }

            Loader {
                id: ctcloader
                active: false
                sourceComponent: QuitTestingChannelDialog {
                    onClosing: function (close) {
                        ctcloader.active = false
                    }
                }
                onLoaded: function () {
                    ctcloader.item.show()
                }
            }
        }
    }

    DccObject {
        name: "testingChannelUrl"
        parentName: "updateSettingsPage"
        backgroundType: DccObject.AutoBg
        weight: 90
        visible: {
            if (advancedSetting.showDetails && dccData.model().testingChannelStatus === Common.WaitJoined)
                return true
            else
                return false
        }
        pageType: DccObject.Editor
        page: D.ToolButton {
            text: qsTr("Click here to complete the application")
            textColor: D.Palette {
                normal {
                    common: D.DTK.makeColor(D.Color.Highlight)
                }
                normalDark: normal
                hovered {
                    common: D.DTK.makeColor(D.Color.Highlight).lightness(+30)
                }
                hoveredDark: hovered
            }
            background: Item {}
            onClicked: {
                dccData.work().openTestingChannelUrl()
            }
        }
    }
}
