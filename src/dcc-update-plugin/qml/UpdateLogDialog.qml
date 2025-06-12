// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.5

import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style as DS
import org.deepin.dcc 1.0

D.DialogWindow {
    id: root
    width: 480
    height: 380
    icon: "preferences-system"
    modality: Qt.WindowModal
    title: qsTr("Update Log")
    property string logContent: ""

    signal exportBtnClicked()

    ColumnLayout {
        anchors.fill: parent

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.preferredHeight: 290

            // 滚动条策略
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            TextArea {
                id: textArea1
                selectByMouse: true
                readOnly: true
                wrapMode: TextArea.Wrap
                text: logContent

                onTextChanged: {
                    // 滚动到底部
                    textArea1.cursorPosition = textArea1.length
                }                
            }
        }

        // 按钮行
        RowLayout {

            Label {
                text: qsTr("Export logs and save them automatically to the desktop")
            }

            Item {
                Layout.fillWidth: true
            }

            D.Button {
                Layout.alignment: Qt.AlignRight
                text: qsTr("Export")
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
                    root.exportBtnClicked()
                }
            }
        }
    }
}
