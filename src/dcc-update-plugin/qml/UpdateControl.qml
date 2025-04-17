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
    property string updateStateTips : "aaaaaaa";
    property string actionBtnText : ""
    property string updateTips: ""
    property string updateTitle: ""
    property double processValue: 0
    property bool processState: false

    property bool checkVisible: false

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
                DccCheckIcon {
                    visible: checkVisible
                    checked: true
                    size: 18
                }

                D.Label {
                    font.pixelSize: 16
                    font.bold: true
                    text: updateStateTips
                }
            }

            D.Label {
                visible: updateTips.length !== 0
                text: updateTips
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            // RowLayout {
            //     spacing: 5
            //   //  visible: false
            //     D.Label {
            //         text:
            //         font.pixelSize: 12
            //     }
            //
            //
            // }
        }

        D.Button {
            Layout.rightMargin: 12
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            text: actionBtnText
            font.pixelSize: 14
            textColor: DS.Style.highlightedButton.background1
            visible: actionBtnText.length !== 0
            onClicked: {
               // dccData.work().onActionBtnClicked();
                rootLayout.btnClicked(updateListModels.getAllUpdateType())
            }
        }

        // D.BusyIndicator {
        //     id: scanAnimation
        //
        //     Layout.alignment: Qt.AlignRight
        // //    running: dccData.model().distUpgradeState === 1 || dccData.model().distUpgradeState === 0
        // //    visible: dccData.model().distUpgradeState === 1 || dccData.model().distUpgradeState === 0
        //     implicitWidth: 32
        //     implicitHeight: 32
        // }

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

                D.ToolButton {
                    id: stratIcon
                    icon.name: "qrc:/icons/deepin/builtin/icons/update_stop.png"
                    width: 12
                    height: 12

                    onClicked: {
                        processState = !processState
                        root.downloadJobCtrl(processState)
                    }
                }


                D.ToolButton {
                    id: stopIcon
                    icon.name: "qrc:/icons/deepin/builtin/icons/update_close.png"
                    width: 12
                    height: 12
                    onClicked: {
                        root.closeDownload()
                    }
                }
            }

            Label {
                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: stratIcon.width + stopIcon.width + progressCtl.spacing * 2
                width: parent.width
                text: updateTitle + Math.floor(process.value * 100) + "%"
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
