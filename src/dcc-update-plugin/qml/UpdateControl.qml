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
    property bool isPauseOrNot: false

    signal btnClicked(int index, int updateType)
    signal downloadJobCtrl(int updateCtrlType)
    signal closeDownload()

    RowLayout {
        Layout.rightMargin: 12
        Layout.leftMargin: 12
        Layout.topMargin: 10
        Layout.bottomMargin: 10
        ColumnLayout {
            spacing: 5
            RowLayout {
                spacing: 5
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

                Image {
                    visible: updateStateIcon.length !== 0
                    sourceSize: Qt.size(24, 24)
                    clip: true
                    source: updateStateIcon
                }

                D.Label {
                    font.pixelSize: D.DTK.fontManager.t5
                    font.bold: true
                    text: updateTitle
                }
            }

            D.Label {
                visible: updateTips.length !== 0
                text: updateTips
                font.pixelSize: D.DTK.fontManager.t8
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
            running: true
            visible: busyState
            implicitWidth: 32
            implicitHeight: 32
        }

        ColumnLayout {
            visible: processState
            Layout.rightMargin: 12
            Layout.alignment: Qt.AlignRight

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
                    implicitWidth: 240
                }

                // TODO: 暂时屏蔽暂停和停止按钮
                // D.ToolButton {
                //     id: startIcon
                //     icon.name: "qrc:/icons/deepin/builtin/icons/update_stop.png"
                //     width: 12
                //     height: 12

                //     onClicked: {
                //         isPauseOrNot = !isPauseOrNot
                //         rootLayout.downloadJobCtrl(isPauseOrNot)
                //     }
                // }

                // D.ToolButton {
                //     id: stopIcon
                //     icon.name: "qrc:/icons/deepin/builtin/icons/update_close.png"
                //     width: 12
                //     height: 12

                //     onClicked: {
                //         rootLayout.closeDownload()
                //     }
                // }
            }

            Label {
                Layout.alignment: Qt.AlignRight
                // Layout.rightMargin: startIcon.width + stopIcon.width + progressCtl.spacing * 2
                width: parent.width
                text: processTitle + Math.floor(process.value * 100) + "%"
                font.pixelSize: D.DTK.fontManager.t8
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
    }
}
