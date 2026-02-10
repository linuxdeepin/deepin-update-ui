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
    width: 360
    icon: "preferences-system"
    modality: Qt.WindowModal

    signal silentBtnClicked()
    signal upgradeRebootBtnClicked()
    signal upgradeShutdownBtnClicked()

    ColumnLayout {
        width: parent.width

        D.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: qsTr("The updates have been already downloaded. What do you want to do?")
        }

        Item {
            Layout.fillHeight: true
            Layout.preferredHeight: 22
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.bottomMargin: 10
            spacing: 10

            ButtonWithToolTip {
                text: qsTr("Silent Installation")
                Layout.fillWidth: true
                onClicked: {
                    root.silentBtnClicked()
                }
            }

            ButtonWithToolTip {
                text: qsTr("Update and Reboot")
                Layout.fillWidth: true
                onClicked: {
                    root.upgradeRebootBtnClicked()
                }
            }

            ButtonWithToolTip {
                text: qsTr("Update and Shut Down")
                textColor: D.Palette {
                    normal {
                        common: D.DTK.makeColor(D.Color.Highlight)
                    }
                    normalDark: normal
                }
                Layout.fillWidth: true
                focus: root.visible
                onClicked: {
                    root.upgradeShutdownBtnClicked()
                }
            }

            component ButtonWithToolTip: D.Button {
                id: customButton
                D.ColorSelector.inactived: false
                contentItem: Text {
                    id: buttonText
                    text: customButton.text
                    font: customButton.font
                    color: customButton.D.ColorSelector.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    width: customButton.width
                }

                hoverEnabled: true

                ToolTip {
                    visible: customButton.hovered && buttonText.truncated
                    delay: 500
                    text: customButton.text
                }
            }
        }
    }
}
