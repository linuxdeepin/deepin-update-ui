// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import QtQuick.Controls 2.0
import Qt.labs.qmlmodels 1.2
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0
import org.deepin.dcc.update 1.0

ColumnLayout {
    id: root

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 338
        color: "transparent"
        clip: true

        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width
            spacing: 10

            D.DciIcon {
                id: checkUpdateIcon
                Layout.alignment: Qt.AlignHCenter
                name: dccData.model().checkUpdateIcon
                theme: D.DTK.themeType
                sourceSize {
                    width: 134
                    height: 134
                }
            }

            D.ProgressBar {
                Layout.alignment: Qt.AlignHCenter
                id: process
                from: 0
                to: 1
                value: dccData.model().checkUpdateProgress
                implicitHeight: 8
                implicitWidth: 160
                visible: dccData.model().checkUpdateStatus === Common.Checking
            }

            D.Label {
                Layout.alignment: Qt.AlignHCenter
                width: implicitWidth
                text: dccData.model().checkUpdateErrTips
                font: D.DTK.fontManager.t8
            }

            D.Button {
                Layout.alignment: Qt.AlignHCenter
                implicitWidth: 200
                visible: text.length !== 0
                text: dccData.model().checkBtnText
                onClicked: {
                    dccData.work().doCheckUpdates();
                }
            }

            D.Label {
                visible: dccData.model().checkUpdateStatus === Common.Updated
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Last check: ") + dccData.model().lastCheckUpdateTime
                font: D.DTK.fontManager.t10
            }
        }
   }
}
