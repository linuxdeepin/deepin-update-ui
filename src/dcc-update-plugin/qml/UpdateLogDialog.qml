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
    width: 480
    icon: "preferences-system"
    modality: Qt.WindowModal
    title: qsTr("Update Log")
    property string logContent: ""

    signal exportBtnClicked()

    ColumnLayout {
        width: root.width - root.leftPadding - root.rightPadding
        spacing: 10

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.preferredHeight: 340 - DS.Style.dialogWindow.titleBarHeight - buttonRow.implicitHeight - DS.Style.dialogWindow.contentVMargin
            clip: true

            background: Rectangle {
                radius: 8
                color: D.DTK.themeType === D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 0.08) : Qt.rgba(1, 1, 1, 0.1)
            }

            // 滚动条策略
            ScrollBar.vertical.policy: ScrollBar.AsNeeded
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            TextArea {
                id: textArea1
                selectByMouse: true
                readOnly: true
                wrapMode: TextArea.Wrap
                text: logContent
                background: null

                onTextChanged: {
                    // 滚动到底部
                    textArea1.cursorPosition = textArea1.length
                }                
            }
        }

        // 按钮行
        RowLayout {
            id: buttonRow
            Layout.bottomMargin: DS.Style.dialogWindow.contentHMargin
            spacing: 10
            
            Button {
                Layout.fillWidth: true
                text: qsTr("Close")
                onClicked: {
                    root.close()
                }
            }

            D.RecommandButton {
                Layout.fillWidth: true
                text: qsTr("Export to desktop")
                onClicked: {
                    root.exportBtnClicked()
                }
            }
        }
    }
}
