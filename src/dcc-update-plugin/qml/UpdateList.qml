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
                                    // 折叠时显示"查看详细"，展开后同位置变为"收起"，避免收起按钮沉在长列表末尾
                                    visible: model.detailInfos.length !== 0
                                    bottomPadding: 0
                                    font: D.DTK.fontManager.t8
                                    text: itemCtl.showDetails ? qsTr("Collapse") : qsTr("View More")
                                    onClicked: {
                                        repeater.model.setExpanded(itemCtl.itemIndex, !itemCtl.showDetails)
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

                            // 详情列表：Loader 惰性创建 + ListView 虚拟化
                            // 折叠时 Loader 不创建任何 delegate（零开销）；展开时 ListView 只实例化可视区项
                            Loader {
                                id: detailLoader
                                Layout.fillWidth: true
                                Layout.minimumHeight: 0
                                Layout.preferredHeight: active && item ? item.implicitHeight : 0

                                active: itemCtl.showDetails

                                property var detailModel: itemCtl.detailModel
                                property bool isSecurityUpdate: itemCtl.isSecurityUpdate

                                sourceComponent: Component {
                                    ListView {
                                        id: detailView
                                        width: parent.width
                                        // 最小 1 仅防死锁（contentHeight 初始为 0，视口为 0 则不实例化 delegate）；
                                        // 最大 500 限制视口高度，内容超出时 ListView 内部滚动并只实例化可视区项。
                                        implicitHeight: Math.min(Math.max(contentHeight, 1), 500)
                                        clip: true
                                        spacing: 6
                                        model: detailLoader.detailModel
                                        // 内容不足一屏时自动隐藏（AsNeeded）
                                        ScrollBar.vertical: ScrollBar {
                                            policy: ScrollBar.AsNeeded
                                            active: true
                                        }

                                        delegate: ColumnLayout {
                                            width: detailView.width
                                            spacing: 6

                                            D.Label {
                                                Layout.alignment: Qt.AlignLeft
                                                horizontalAlignment: Text.AlignLeft
                                                Layout.fillWidth: true
                                                font: D.DTK.fontManager.t8
                                                color: D.DTK.themeType == D.ApplicationHelper.LightType ?
                                                                        Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                                visible: modelData.name !== ""
                                                text: (detailLoader.isSecurityUpdate ? qsTr("Vulnerability ID: ") : qsTr("Version:")) + modelData.name
                                            }

                                            D.Label {
                                                Layout.alignment: Qt.AlignLeft
                                                horizontalAlignment: Text.AlignLeft
                                                Layout.fillWidth: true
                                                font: D.DTK.fontManager.t8
                                                color: D.DTK.themeType == D.ApplicationHelper.LightType ?
                                                                        Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                                visible: detailLoader.isSecurityUpdate && modelData.vulLevel !== ""
                                                text: qsTr("Severity: ") + modelData.vulLevel
                                            }

                                            D.Label {
                                                Layout.alignment: Qt.AlignLeft
                                                horizontalAlignment: Text.AlignLeft
                                                Layout.fillWidth: true
                                                font: D.DTK.fontManager.t8
                                                visible: modelData.info !== ""
                                                text: detailLoader.isSecurityUpdate ? qsTr("Description: ") + modelData.info : modelData.info
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

                                            D.Label {
                                                Layout.alignment: Qt.AlignLeft
                                                horizontalAlignment: Text.AlignLeft
                                                Layout.fillWidth: true
                                                font: D.DTK.fontManager.t8
                                                visible: modelData.updateTime !== ""
                                                text: qsTr("Release time:") + modelData.updateTime
                                            }

                                            Rectangle {
                                                height: 1
                                                color: D.DTK.themeType === D.ApplicationHelper.LightType ?
                                                                        Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(1, 1, 1, 0.05)
                                                Layout.fillWidth: true
                                                visible: index !== detailView.count - 1
                                            }
                                        }
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
