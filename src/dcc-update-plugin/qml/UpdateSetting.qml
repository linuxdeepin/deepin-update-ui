// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import QtQuick.Controls 2.0
import Qt.labs.qmlmodels 1.2
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0

import org.deepin.dcc.update 1.0

DccObject {

    DccTitleObject {
        name: "updateTypeGrpTitle"
        parentName: "updateSettingsPage"
        displayName: qsTr("Update Type")
        weight: 10
    }

    DccObject {
        name: "updateTypeGrp"
        parentName: "updateSettingsPage"
        weight: 20
        pageType: DccObject.Item
        page: DccGroupView {
            height: implicitHeight + 10
            spacing: 0
        }

        DccObject {
            name: "functionUpdate"
            parentName: "updateTypeGrp"
            displayName: qsTr("Function Updates")
            description: qsTr("Delivers a cumulative update including new features, quality updates, and  security updates")
            weight: 10
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
            visible: dccData.model().securityUpdateEnabled
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
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().thirdPartyUpdate
                onCheckedChanged: {
                    dccData.work().setThirdPartyUpdate(checked)
                }
            }
        }
    }

    DccTitleObject {
        name: "otherSettingGrpTitle"
        parentName: "updateSettingsPage"
        displayName: qsTr("Other Settings")
        weight: 30
    }

    DccObject {
        name: "otherSettingGrp"
        parentName: "updateSettingsPage"
        weight: 40
        pageType: DccObject.Item
        page: DccGroupView {
            height: implicitHeight + 10
            spacing: 0
        }

        DccObject {
            name: "downloadLimit"
            parentName: "otherSettingGrp"
            displayName: qsTr("Limit Speed")
            weight: 10
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
            parentName: "otherSettingGrp"
            displayName: qsTr("Limit Setting")
            visible: dccData.model().downloadSpeedLimitEnabled
            weight: 20
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

    DccTitleObject {
        name: "advancedSettingTitle"
        parentName: "updateSettingsPage"
        displayName: qsTr("Advanced Settings")
        weight: 60
    }

    DccObject {
        name: "advancedSettingGrp"
        parentName: "updateSettingsPage"
        weight: 70
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
            visible: false
            name: "updateHistory"
            parentName: "advancedSettingGrp"
            displayName: qsTr("Update History")
            weight: 40
            pageType: DccObject.Editor
            page: D.Button {
                text: qsTr("View")
                onClicked: {
                    uhloader.active = true
                }

                Loader {
                    id: uhloader
                    active: false
                    sourceComponent: UpdateHistory {
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
        visible: dccData.mode().isCommunitySystem()
        weight: 80
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
        description: qsTr("Join the internal testing channel to get deepin latest updates")
        backgroundType: DccObject.Normal
        visible: false //dccData.mode().isCommunitySystem()
        weight: 90
        pageType: DccObject.Editor
        page: D.Switch {
            checked: dccData.model().testingChannelStatus != UpdateModel.NotJoined
            onCheckedChanged: {
                dccData.work().testingChannelCheck(checked)
            }
        }
    }
}
