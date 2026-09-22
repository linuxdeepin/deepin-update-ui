// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
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

    // 关闭对话框时收起全部卡片，避免下次打开残留上次的展开态
    Component.onDestruction: {
        if (listView.model) {
            listView.model.collapseAll()
        }
    }

    ColumnLayout {
        id: noUpdateLayout
        x: -DS.Style.dialogWindow.contentHMargin
        width: root.width// - DS.Style.dialogWindow.contentHMargin * 2
        spacing: 0


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
            ScrollBar.vertical: D.ScrollBar {}

            model: dccData.model().historyModel
            visible: model.rowCount() > 0

            delegate: D.ItemDelegate {
                id: logItem
                width: listView.width
                padding: 0
                height: children.height
                checkable: false
                cascadeSelected: !checked
                property bool showDetails: expanded
                background: Rectangle {
                    x: 70
                    width: listView.width -140
                    property D.Palette backgroundColor: D.Palette {
                        normal: Qt.rgba(1, 1, 1, 1)
                        normalDark: Qt.rgba(1, 1, 1, 0.05)
                    }
                    radius: 6
                    color: D.ColorSelector.backgroundColor
                }

                contentItem: RowLayout {
                    Layout.margins: 0
                    Layout.fillWidth: true
                    D.DciIcon {
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

                        Label {
                            Layout.alignment: Qt.AlignLeft
                            horizontalAlignment: Text.AlignLeft
                            Layout.fillWidth: true
                            font: D.DTK.fontManager.t8
                            color: D.DTK.themeType == D.ApplicationHelper.LightType ? 
                                                    Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                            visible: Version.length !== 0
                            text: qsTr("Version:") + Version
                        }

                        // 摘要
                        Label {
                            Layout.alignment: Qt.AlignLeft
                            horizontalAlignment: Text.AlignLeft
                            text: Summary
                            textFormat: Text.RichText
                            visible: Summary.length !== 0
                            font: D.DTK.fontManager.t8
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true

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
                            Label {
                                Layout.fillWidth: true
                                text: qsTr("Installation date:") + UpgradeTime
                                font: D.DTK.fontManager.t8
                                wrapMode: Text.WordWrap
                            }

                            Item {
                                Layout.fillWidth: true
                            }

                            // 展开/收起：按钮位于详情列表之前，收起按钮不会沉在上千条列表末尾
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
                                visible: Details.length !== 0
                                bottomPadding: 0
                                font: D.DTK.fontManager.t8
                                text: logItem.showDetails ? qsTr("Collapse") : qsTr("View More")
                                onClicked: {
                                    listView.model.setExpanded(index, !logItem.showDetails)
                                }
                                background: Item {}
                            }
                        }

                        Rectangle {
                            height: 1
                            color: D.DTK.themeType === D.ApplicationHelper.LightType ? 
                                                    Qt.rgba(0, 0, 0, 0.05) : Qt.rgba(1, 1, 1, 0.05)
                            Layout.fillWidth: true
                            visible: logItem.showDetails
                        }

                        // 详情列表：Loader 惰性创建 + ListView 虚拟化
                        // 折叠时 Loader 不创建任何 delegate（零开销）；展开时仅实例化可视区项
                        Loader {
                            id: detailLoader
                            Layout.fillWidth: true
                            Layout.minimumHeight: 0
                            Layout.preferredHeight: active && item ? item.implicitHeight : 0

                            active: logItem.showDetails

                            property var detailModel: Details
                            property int itemType: Type

                            sourceComponent: Component {
                                ListView {
                                    id: detailView
                                    width: parent.width
                                    // 视口高度取内容高度，上限 500：内容不足一屏时随内容收缩，超出时内部滚动
                                    // 非空时最小 1 仅防死锁（contentHeight 初始为 0，视口为 0 则不实例化 delegate）
                                    implicitHeight: count === 0 ? 0 : Math.min(Math.max(contentHeight, 1), 500)
                                    clip: true
                                    spacing: 6
                                    model: detailLoader.detailModel
                                    ScrollBar.vertical: D.ScrollBar {}

                                    delegate: ColumnLayout {
                                        width: detailView.width
                                        spacing: 6

                                        Label {
                                            Layout.alignment: Qt.AlignLeft
                                            horizontalAlignment: Text.AlignLeft
                                            text: {
                                                switch(detailLoader.itemType) {
                                                case 1: return qsTr("Version:") + modelData.name
                                                case 4: return qsTr("Vulnerability ID: ") + modelData.name
                                                default: return ""
                                                }
                                            }
                                            visible: modelData.name !== ""
                                            font: D.DTK.fontManager.t8
                                            color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                            wrapMode: Text.WordWrap
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            Layout.alignment: Qt.AlignLeft
                                            horizontalAlignment: Text.AlignLeft
                                            text: qsTr("Severity: ") + modelData.displayVulLevel
                                            visible: modelData.displayVulLevel !== ""
                                            font: D.DTK.fontManager.t8
                                            color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
                                            wrapMode: Text.WordWrap
                                            Layout.fillWidth: true
                                        }
                                        Label {
                                            Layout.alignment: Qt.AlignLeft
                                            horizontalAlignment: Text.AlignLeft
                                            text: {
                                                detailLoader.itemType === 4 ? qsTr("Description: ") + modelData.description : modelData.description
                                            }
                                            visible: modelData.description !== ""
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
            }
        }
    }
}
