// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window
import org.deepin.dtk 1.0 as D

Loader {
    id: loader
    active: false
    property var view: null
    property var viewWidth: 380
    property var anchorItem: null
    property int windowWidth: 380
    property int windowHeight: Screen.height * 0.7
    property bool contentScrolling: false
    required property DelegateModel delegateModel

    signal opened()
    signal closed()
    signal testButtonClicked()

    function setViewIndex(viewIndex) {
        if (!view) return

        if (viewIndex < 0) {
            viewIndex = 0
        } else if (viewIndex >= view.count) {
            viewIndex = view.count - 1
        }

        view.currentIndex = viewIndex
    }

    function show() {
        active = true
    }

    function isVisible() {
        return active
    }

    function close() {
        if (item) {
            item.closeWindow()
        }
    }

    function setPositionByItem(item) {
        anchorItem = item
        if (active && this.item) {
            this.item.positionWindow()
        }
    }

    sourceComponent: Window {
        id: mirrorWindow
        width: windowWidth
        height: windowHeight
        D.DWindow.enabled: true
        D.DWindow.enableSystemResize: false
        D.DWindow.enableSystemMove: false
        D.DWindow.enableBlurWindow: true
        flags: Qt.Dialog | Qt.WindowCloseButtonHint
        color: D.DTK.palette.window
        palette: D.DTK.palette

        Component.onCompleted: {
            positionWindow()
            loader.opened()
        }

        function positionWindow() {
            if (!loader.anchorItem) return

            var globalPos = loader.anchorItem.mapToGlobal(0, 0)
            mirrorWindow.x = globalPos.x
            mirrorWindow.y = globalPos.y + loader.anchorItem.height
        }

        ColumnLayout {
            id: contentLayout
            anchors.fill: parent
            anchors.margins: 6
            spacing: 0

            ListView {
                id: listView
                clip: true
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: loader.delegateModel
                highlightMoveDuration: -1
                highlightMoveVelocity: -1
                highlightFollowsCurrentItem: true
                focus: true
                activeFocusOnTab: true
                ScrollBar.vertical: ScrollBar {
                    id: verticalScrollBar
                    active: hovered || pressed || listView.moving || listView.flicking
                }
                interactive: true

                // 传递给delegate使用  
                property bool keyboardScrolling: loader.contentScrolling

                // 键盘滚动保护计时器
                Timer {
                    id: keyboardScrollTimer
                    interval: 50
                    onTriggered: {
                        loader.contentScrolling = false
                    }
                }

                // 键盘事件处理（参考SearchBar模式）
                Keys.onUpPressed: {
                    // 键盘导航时启用滚动保护
                    loader.contentScrolling = true
                    keyboardScrollTimer.restart()
                    loader.setViewIndex(currentIndex - 1)
                }

                Keys.onDownPressed: {
                    // 键盘导航时启用滚动保护
                    loader.contentScrolling = true
                    keyboardScrollTimer.restart()
                    loader.setViewIndex(currentIndex + 1)
                }

                Keys.onReturnPressed: {
                    if (count > 0 && currentIndex >= 0 && currentItem) {
                        currentItem.checked = true
                    }
                }

                Keys.onEscapePressed: {
                    mirrorWindow.closeWindow()
                }

                Component.onCompleted: {
                    loader.view = listView
                    loader.viewWidth = listView.width

                    // 查找已选中项并设置高亮
                    for (var i = 0; i < count; i++) {
                        var item = itemAtIndex(i)
                        if (item && item.checked === true) {
                            loader.setViewIndex(i)
                            positionViewAtIndex(i, ListView.Center)
                            break
                        }
                    }

                    forceActiveFocus()
                }

                onWidthChanged: {
                    loader.viewWidth = listView.width
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 9

                Item {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: parent.height
                }

                // 底部按钮
                D.ToolButton {
                    id: testButton
                    text: qsTr("Connectivity Test")
                    implicitHeight: 36
                    visible: dccData.model().netselectExist

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

                    background: Item {}

                    onClicked: {
                        loader.testButtonClicked()
                    }
                }
            }
        }

        function closeWindow() {
            mirrorWindow.close()
            loader.active = false
            loader.closed()
        }

        onActiveFocusItemChanged: {
            if (!activeFocusItem) {
                mirrorWindow.closeWindow()
            }
        }

        onActiveChanged: {
            if (!active) {
                mirrorWindow.closeWindow()
            }
        }
    }

    onLoaded: {
        item.show()
        Qt.callLater(function() {
            item.requestActivate();
        });
    }
}
