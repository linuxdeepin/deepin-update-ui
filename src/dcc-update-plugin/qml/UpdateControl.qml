// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.deepin.dtk as D
import org.deepin.dtk.style as DS
import org.deepin.dcc

ColumnLayout {
    id: rootLayout

    property alias updateListModels: updatelistModel.model;
    property string updateStateIcon: ""
    property string updateTitle : ""
    property string updateTips: ""
    property var btnActions: []
    property string processTitle: ""
    property double processValue: 0
    property bool processState: false
    property bool busyState: false
    property bool updateListEnable: true
    property bool updateListcheck: true
    property bool isDownloading: false
    property bool isPauseOrNot: false
    property bool showLogButton: false

    signal btnClicked(int index, int updateType)
    signal startDownload()
    signal pauseDownload()
    signal closeDownload()
    signal viewLogClicked()

    RowLayout {
        Layout.rightMargin: 12
        Layout.leftMargin: 12
        Layout.topMargin: 10
        Layout.bottomMargin: showLogButton ? 6 : 10

        ColumnLayout {
            spacing: 0

            RowLayout {
                spacing: 5
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop

                Image {
                    visible: updateStateIcon.length !== 0
                    sourceSize: Qt.size(24, 24)
                    clip: true
                    source: updateStateIcon
                }

                D.Label {
                    font {
                        pixelSize: D.DTK.fontManager.t5.pixelSize
                        family: D.DTK.fontManager.t5.family
                        weight: Font.Medium
                    }
                    color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                    text: updateTitle
                }
            }

            D.Label {
                Layout.topMargin: 6
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                visible: updateTips.length !== 0
                text: updateTips
                font: D.DTK.fontManager.t8
                Layout.fillWidth: true
                elide: Text.ElideRight
                ToolTip.delay: 1000
                ToolTip.timeout: 5000
                ToolTip.text: text
                ToolTip.visible: desLabelHover.hovered && implicitWidth > width
                HoverHandler {
                    id: desLabelHover
                }
            }

            D.Button {
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                visible: showLogButton
                text: qsTr("View Update Log")
                leftPadding: 0
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
                background: null
                onClicked: {
                    rootLayout.viewLogClicked()
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Repeater {
            model: btnActions
            delegate: D.Button {
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                text: modelData
                font: D.DTK.fontManager.t6
                visible: modelData.length !== 0 && !initAnimation.visible
                enabled: updatelistModel.model.isUpdateEnable
                onClicked: {
                    rootLayout.btnClicked(index, updateListModels.getAllUpdateType())
                }
            }
        }

        BusyIndicator {
            id: initAnimation
            running: initAnimation.visible
            visible: busyState
            implicitWidth: 24
            implicitHeight: 24
        }

        ColumnLayout {
            visible: processState

            RowLayout {
                id: progressCtl
                spacing: 10
                Layout.alignment: Qt.AlignRight
                D.ProgressBar {
                    id: process
                    Layout.alignment: Qt.AlignHCenter
                    from: 0
                    to: 1
                    value: processValue
                    implicitHeight: 8
                    implicitWidth: 210
                }

                D.ToolButton {
                    id: pauseIcon
                    icon.name: isPauseOrNot ? "update_start" : "update_pause"
                    icon.width: 24
                    icon.height: 24
                    implicitWidth: 24
                    implicitHeight: 24
                    visible: isDownloading

                    onClicked: {
                        if (isPauseOrNot) {
                            rootLayout.startDownload()
                        } else {
                            rootLayout.pauseDownload()
                        }
                    }
                }

                D.ToolButton {
                    id: stopIcon
                    icon.name: "update_stop"
                    icon.width: 24
                    icon.height: 24
                    implicitWidth: 24
                    implicitHeight: 24
                    visible: isDownloading

                    onClicked: {
                        rootLayout.closeDownload()
                    }
                }
            }

            Label {
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: isDownloading ? pauseIcon.width + stopIcon.width + progressCtl.spacing * 2 : 0
                width: parent.width
                text: processTitle + " " + Math.floor(process.value * 100) + "%"
                font: D.DTK.fontManager.t8
            }
        }
    }

    Rectangle {
        height: 1
        color: D.DTK.themeType === D.ApplicationHelper.LightType ?
            Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(1, 1, 1, 0.05)
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        Layout.fillWidth: true
    }

    UpdateList {
        id: updatelistModel
        enabled: updateListEnable
        checkIconVisible: updateListcheck
    }
}
