// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.5

import org.deepin.dtk 1.0 as D

D.DialogWindow {
    id: root

    width: 380
    icon: "preferences-system"
    modality: Qt.WindowModal

    property bool bExit: false

    onClosing: {
        dccData.work().exitTestingChannel(bExit)
    }

    ColumnLayout {
        width: parent.width
        spacing: 10
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 50
            Layout.rightMargin: 50
            text: qsTr("If you exit the beta program, you will no longer receive beta updates.")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
        RowLayout {
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            spacing: 10
            D.Button {
                Layout.fillWidth: true
                text: qsTr("Cancel")
                onClicked: {
                    close()
                }
            }
            D.RecommandButton {
                Layout.fillWidth: true
                text: qsTr("Exit")
                onClicked: {
                    bExit = true
                    close()
                }
            }
        }
    }
}
