// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
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

    Component.onDestruction: {
        if (repeater.model) {
            repeater.model.collapseAll()
        }
    }

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

                property var detailModel: model.detailInfos
                property bool showDetails: model.expanded
                property int itemIndex: index
                property bool isSecurityUpdate: repeater.model.getUpdateType(index) === Common.SecurityUpdate

                content: ColumnLayout {
                    spacing: 10

                    RowLayout {
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
                                    color: D.DTK.themeType == D.ApplicationHelper.LightType ? 
                                                            Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
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
                                color: D.DTK.themeType == D.ApplicationHelper.LightType ? 
                                                        Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                visible: model.version.length !== 0
                                text: qsTr("Version:") + model.version
                            }

                            D.Label {
                                Layout.alignment: Qt.AlignLeft
                                horizontalAlignment: Text.AlignLeft
                                Layout.fillWidth: true
                                font: D.DTK.fontManager.t8
                                text: model.explain
                                textFormat: Text.RichText
                                wrapMode: Text.WordWrap
                                onLinkActivated: (link)=> {
                                    dccData.work().openUrl(link)
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                                    acceptedButtons: Qt.NoButton
                                }
                            }

                            RowLayout {
                                D.Label {
                                    Layout.alignment: Qt.AlignLeft
                                    horizontalAlignment: Text.AlignLeft
                                    verticalAlignment: Text.AlignVCenter
                                    Layout.fillWidth: true
                                    Layout.minimumHeight: 22
                                    font: D.DTK.fontManager.t8
                                    visible: model.releaseTime.length !== 0
                                    text: qsTr("Release time:") + model.releaseTime
                                }

                                Item {
                                    Layout.fillWidth: true
                                }

                                D.ToolButton {
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
                                    visible: model.detailInfos.length !== 0 && !itemCtl.showDetails
                                    bottomPadding: 0
                                    font: D.DTK.fontManager.t8
                                    text: qsTr("View More")
                                    onClicked: {
                                        repeater.model.setExpanded(itemCtl.itemIndex, true)
                                    }
                                    background: Item {}
                                }
                            }

                            Rectangle {
                                height: 1
                                color: D.DTK.themeType === D.ApplicationHelper.LightType ? 
                                                        Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(1, 1, 1, 0.05)
                                Layout.fillWidth: true
                                visible: itemCtl.showDetails
                            }

                            // 详情列表
                            Repeater {
                                id: innerRepeater
                                model: itemCtl.detailModel
                                ColumnLayout {
                                    spacing: 6

                                    visible: itemCtl.showDetails

                                    D.Label {
                                        Layout.alignment: Qt.AlignLeft
                                        horizontalAlignment: Text.AlignLeft
                                        Layout.fillWidth: true
                                        font: D.DTK.fontManager.t8
                                        color: D.DTK.themeType == D.ApplicationHelper.LightType ?
                                                                Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                        visible: itemCtl.showDetails && modelData.name !== ""
                                        text: (itemCtl.isSecurityUpdate ? qsTr("Vulnerability ID:") : qsTr("Version:")) + modelData.name
                                    }

                                    D.Label {
                                        Layout.alignment: Qt.AlignLeft
                                        horizontalAlignment: Text.AlignLeft
                                        Layout.fillWidth: true
                                        font: D.DTK.fontManager.t8
                                        color: D.DTK.themeType == D.ApplicationHelper.LightType ?
                                                                Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                        visible: itemCtl.showDetails && itemCtl.isSecurityUpdate && modelData.vulLevel !== ""
                                        text: qsTr("Severity:") + modelData.vulLevel
                                    }

                                    D.Label {
                                        Layout.alignment: Qt.AlignLeft
                                        horizontalAlignment: Text.AlignLeft
                                        Layout.fillWidth: true
                                        font: D.DTK.fontManager.t8
                                        visible: itemCtl.showDetails && modelData.info !== ""
                                        text: itemCtl.isSecurityUpdate ? qsTr("Description:") + modelData.info : modelData.info
                                        textFormat: Text.RichText
                                        wrapMode: Text.WordWrap
                                        onLinkActivated: (link)=> {
                                            dccData.work().openUrl(link)
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                                            acceptedButtons: Qt.NoButton
                                        }
                                    }

                                    RowLayout {
                                        D.Label {
                                            Layout.alignment: Qt.AlignLeft
                                            horizontalAlignment: Text.AlignLeft
                                            Layout.fillWidth: true
                                            font: D.DTK.fontManager.t8
                                            visible: itemCtl.showDetails && modelData.updateTime !== ""
                                            text: qsTr("Release time:") + modelData.updateTime
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }

                                        D.ToolButton {
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
                                            visible: itemCtl.showDetails && (index === innerRepeater.count - 1 )
                                            bottomPadding: 0
                                            font: D.DTK.fontManager.t8
                                            text: qsTr("Collapse")
                                            onClicked: {
                                                repeater.model.setExpanded(itemCtl.itemIndex, false)
                                            }
                                            background: Item {}
                                        }
                                    }

                                    Rectangle {
                                        height: 1
                                        color: D.DTK.themeType === D.ApplicationHelper.LightType ? 
                                                                Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(1, 1, 1, 0.05)
                                        Layout.fillWidth: true
                                        visible: itemCtl.showDetails && (index !== innerRepeater.count - 1 )
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        height: 1
                        color: D.DTK.themeType === D.ApplicationHelper.LightType ? 
                                                Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(1, 1, 1, 0.05)
                        Layout.fillWidth: true
                        visible: (index !== repeater.count - 1 )
                    }
                }

                background: DccItemBackground {
                    separatorVisible: true
                }
            }
        }
    }
}
