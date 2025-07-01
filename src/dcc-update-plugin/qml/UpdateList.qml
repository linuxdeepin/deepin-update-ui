// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import QtQuick.Controls 2.0
import Qt.labs.qmlmodels 1.2
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS
import org.deepin.dcc 1.0
import org.deepin.dcc.update 1.0

Rectangle {
    id: root

    property alias model: repeater.model
    property bool checkIconVisible: true

    color: "transparent"
    implicitHeight: layoutView.height
    Layout.fillWidth: true

    ColumnLayout {
        id: layoutView
        clip: true
        width: parent.width
        spacing: 10

        Repeater {
            id: repeater

            delegate: D.ItemDelegate {
                id: itemCtl
                Layout.fillWidth: true
                leftPadding: 6
                rightPadding: 12
                topPadding: 10
                cascadeSelected: true
                contentFlow: true
                spacing: 0

                content: RowLayout {
                    spacing: 10

                    D.DciIcon {
                        Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                        name: model.iconName
                        sourceSize {
                            width: 28
                            height: 28
                        }
                    }

                    ColumnLayout {
                        Layout.alignment: Qt.AlignRight
                        spacing: 6

                        RowLayout {
                            Label {
                                Layout.alignment: Qt.AlignLeft
                                text: model.title
                                font: D.DTK.fontManager.t6
                                color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                width: 100
                                Layout.fillWidth: true
                            }

                            DccCheckIcon {
                                Layout.alignment: Qt.AlignRight
                                checked: model.checked
                                size: 18
                                visible: checkIconVisible

                                onClicked: {
                                    repeater.model.setChecked(index, !model.checked)
                                    dccData.work().setCheckUpdateMode(repeater.model.getUpdateType(index), model.checked)
                                }
                            }
                        }

                        D.Label {
                            Layout.alignment: Qt.AlignLeft
                            horizontalAlignment: Text.AlignLeft
                            Layout.fillWidth: true
                            font: D.DTK.fontManager.t8
                            color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                            visible: model.version.length !== 0
                            text: qsTr("Version:") + model.version
                        }

                        D.Label {
                            Layout.alignment: Qt.AlignLeft
                            horizontalAlignment: Text.AlignLeft
                            Layout.fillWidth: true
                            font: D.DTK.fontManager.t8
                            text: model.titleDescription
                            wrapMode: Text.WordWrap
                            onLinkActivated: (link)=> {
                                dccData.work().openUrl(link)
                            }  
                        }

                        D.Label {
                            id: shortLog
                            Layout.alignment: Qt.AlignLeft
                            horizontalAlignment: Text.AlignLeft
                            text: model.updateLog
                            font: D.DTK.fontManager.t8
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            visible: false
                        }

                        D.Label {
                            id: detailLog
                            visible: false
                            Layout.alignment: Qt.AlignLeft
                            horizontalAlignment: Text.AlignLeft
                            text: ""
                            font: D.DTK.fontManager.t8
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }

                        RowLayout {
                            D.Label {
                                id: releaseTitle
                                Layout.alignment: Qt.AlignLeft
                                horizontalAlignment: Text.AlignLeft
                                visible: model.releaseTime.length !== 0
                                font: D.DTK.fontManager.t8
                                text: qsTr("Release time:") + model.releaseTime
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            D.ToolButton {
                                Layout.alignment: Qt.AlignRight
                                visible: detailLog.visible
                                text: shortLog.visible ? qsTr("View Details") : qsTr("Collapse")
                                textColor: DS.Style.highlightedButton.text

                                onClicked: {
                                    shortLog.visible = !shortLog.visible
                                    detailLog.visible = !detailLog.visible
                                }
                            }
                        }
                    }
                }

                background: DccItemBackground {
                    separatorVisible: true
                }
            }
        }
    }
}
