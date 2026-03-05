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
                        spacing: 10

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
                            Layout.bottomMargin: -20
                            font: D.DTK.fontManager.t8
                            text: model.explain
                            wrapMode: Text.WordWrap
                            onLinkActivated: (link)=> {
                                dccData.work().openUrl(link)
                            }
                        }

                        RowLayout {
                            D.Label {
                                id: releaseTitle
                                Layout.alignment: Qt.AlignLeft
                                horizontalAlignment: Text.AlignLeft
                                visible: model.releaseTime.length !== 0
                                font: D.DTK.fontManager.t8
                                color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                text: qsTr("Release time:") + model.releaseTime
                            }

                            Item {
                                Layout.fillWidth: true
                            }
                        }

                        // 详情列表
                        Repeater {
                            model: repeater.model.getDetailInfos(index)
                            ColumnLayout {
                                Layout.fillWidth: true

                                D.Label {
                                    Layout.alignment: Qt.AlignLeft
                                    horizontalAlignment: Text.AlignLeft
                                    Layout.fillWidth: true
                                    Layout.bottomMargin: -12
                                    font: D.DTK.fontManager.t8
                                    visible: modelData.info !== ""
                                    text: modelData.info
                                    wrapMode: Text.WordWrap
                                    onLinkActivated: (link)=> {
                                        dccData.work().openUrl(link)
                                    }
                                }

                                D.Label {
                                    Layout.alignment: Qt.AlignLeft
                                    horizontalAlignment: Text.AlignLeft
                                    visible: modelData.updateTime !== ""
                                    font: D.DTK.fontManager.t8
                                    color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                    text: qsTr("Release time:") + modelData.updateTime
                                    wrapMode: Text.WordWrap
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
