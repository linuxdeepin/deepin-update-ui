// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.5

import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style as DS
import org.deepin.dcc 1.0

D.DialogWindow {
    id: root
    width: 712
    height: 530
    icon: "preferences-system"
    modality: Qt.WindowModal
    title: qsTr("Update History")

    ColumnLayout {
        id: noUpdateLayout
        x: -DS.Style.dialogWindow.contentHMargin
        width: root.width// - DS.Style.dialogWindow.contentHMargin * 2
        spacing: 0

        D.Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 12
            text: title
            font: D.DTK.fontManager.t6
            color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
            wrapMode: Text.WordWrap
        }

        // 无更新历史时的提示
        Control {
            visible: listView.model.rowCount() === 0
            Layout.fillWidth: parent
            height: 400
            Item {
                anchors.centerIn: parent
                implicitWidth: Math.max(noUpdateIcon.implicitWidth, noUpdateLabel.implicitWidth)
                implicitHeight: noUpdateIcon.implicitHeight + noUpdateLabel.implicitHeight + 20
                D.DciIcon {
                    id: noUpdateIcon
                    anchors.horizontalCenter: parent.horizontalCenter
                    palette: D.DTK.makeIconPalette(root.palette)
                    mode: root.D.ColorSelector.controlState
                    theme: root.D.ColorSelector.controlTheme
                    fallbackToQIcon: false
                    sourceSize: Qt.size(136, 136)
                    name: "update_no_update_history"
                }
                Label {
                    id: noUpdateLabel
                    anchors.top: noUpdateIcon.bottom
                    anchors.topMargin: 20
                    anchors.horizontalCenter: noUpdateIcon.horizontalCenter
                    text: qsTr("No update history")
                    font.family: D.DTK.fontManager.t8.family
                    font.pixelSize: D.DTK.fontManager.t8.pixelSize
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
        }

        // 更新历史列表
        ListView {
            id: listView
            width: parent.width
            height: 450
            spacing: 6
            clip: true
            cacheBuffer: 1000
            ScrollBar.vertical: D.ScrollBar {
                width: 10
            }

            model: dccData.model().historyModel
            visible: model.rowCount() > 0

            delegate: D.ItemDelegate {
                id: logItem
                width: listView.width
                padding: 0
                height: children.height
                checkable: false
                cascadeSelected: !checked
                background: DccItemBackground {
                    x: 70
                    width: listView.width -140
                    backgroundType: DccObject.Normal
                }

                contentItem: RowLayout {
                    Layout.margins: 0
                    Layout.fillWidth: true
                    D.DciIcon {
                        palette: D.DTK.makeIconPalette(logItem.palette)
                        mode: logItem.D.ColorSelector.controlState
                        theme: logItem.D.ColorSelector.controlTheme
                        fallbackToQIcon: false
                        width: 22
                        height: 22
                        name: {
                            switch(Type) {
                            case 1: return "update_maintenance"
                            case 4: return "update_safe"
                            default: return "update_set"
                            }
                        }
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        Layout.leftMargin: 11 + 70
                        Layout.topMargin: 16
                        sourceSize: Qt.size(DS.Style.itemDelegate.checkIndicatorIconSize, DS.Style.itemDelegate.checkIndicatorIconSize)
                    }

                    ColumnLayout {
                        Layout.topMargin: 16
                        Layout.bottomMargin: 16
                        Layout.maximumWidth: 498
                        // 类型
                        Label {
                            text: {
                                switch(Type) {
                                case 1: return qsTr("System Updates")
                                case 4: return qsTr("Security Updates")
                                default: return ""
                                }
                            }
                            font: D.DTK.fontManager.t6
                            color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        // 摘要
                        Label {
                            text: {
                                if (Summary === "") {
                                    switch(Type) {
                                    case 1:
                                        return qsTr("Delivers a cumulative update including new features, quality updates, and security updates")
                                    case 4:
                                        return qsTr("Delivers security updates")
                                    default:
                                        return ""
                                    }
                                } else {
                                    return Summary
                                }
                            }
                            font: D.DTK.fontManager.t8
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                            onLinkActivated: (link)=> {
                                if (link.startsWith("http")) {
                                    Qt.openUrlExternally(link)
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                                acceptedButtons: Qt.NoButton
                            }
                        }

                        // 详情列表
                        Repeater {
                            model: Details
                            ColumnLayout {
                                Layout.margins: 0
                                Layout.fillWidth: true

                                Label {
                                    text: modelData.name
                                    font: D.DTK.fontManager.t8
                                    color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: modelData.displayVulLevel
                                    font: D.DTK.fontManager.t8
                                    color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                                Label {
                                    text: modelData.description
                                    font: D.DTK.fontManager.t8
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        // 升级时间
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Installation date:") + UpgradeTime
                            font: D.DTK.fontManager.t8
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }
}
