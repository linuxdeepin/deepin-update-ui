// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import QtQuick.Controls 2.0
import Qt.labs.qmlmodels 1.2
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS
import org.deepin.dcc 1.0

ColumnLayout {
    id: rootLayout

    property alias updateListModels: updatelistModel.model;
    property string updateStateIcon: ""
    property string updateTitle : ""
    property string updateTips: ""
    property string actionBtnText : ""
    property string processTitle: ""
    property double processValue: 0
    property bool processState: false

    signal btnClicked(int updateType)
    signal downloadJobCtrl(int updateCtrlType)
    signal closeDownload()

    RowLayout {
        Layout.preferredWidth: parent.width
        ColumnLayout {
            Layout.leftMargin: 22
            Layout.topMargin: 10
            Layout.bottomMargin: 10
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
                    font.pixelSize: 16
                    font.bold: true
                    text: updateTitle
                }
            }

            D.Label {
                visible: updateTips.length !== 0
                text: updateTips
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }
        }

        D.Button {
            Layout.rightMargin: 12
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            text: actionBtnText
            font.pixelSize: 14
            textColor: DS.Style.highlightedButton.background1
            visible: actionBtnText.length !== 0
            enabled: updatelistModel.model.isUpdateEnable
            onClicked: {
                rootLayout.btnClicked(updateListModels.getAllUpdateType())
            }
        }

        ColumnLayout {
            visible: processValue !== 0 && processValue !== 1
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
                //         processState = !processState
                //         root.downloadJobCtrl(processState)
                //     }
                // }


                // D.ToolButton {
                //     id: stopIcon
                //     icon.name: "qrc:/icons/deepin/builtin/icons/update_close.png"
                //     width: 12
                //     height: 12

                //     onClicked: {
                //         root.closeDownload()
                //     }
                // }
            }

            Label {
                Layout.alignment: Qt.AlignRight
                // Layout.rightMargin: startIcon.width + stopIcon.width + progressCtl.spacing * 2
                width: parent.width
                text: processTitle + Math.floor(process.value * 100) + "%"
                font.pixelSize: 12
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
    }
}
