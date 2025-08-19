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
        spacing: 10

        D.Label {
            id: titleLabel
            Layout.alignment: Qt.AlignHCenter
            text: title
            font: D.DTK.fontManager.t6
            color: D.DTK.themeType == D.ApplicationHelper.LightType ? Qt.rgba(0, 0, 0, 1) : Qt.rgba(1, 1, 1, 1)
            wrapMode: Text.WordWrap
        }

        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.preferredHeight: root.height - titleLabel.height - buttonRow.height - 80

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
            id: buttonRow
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
