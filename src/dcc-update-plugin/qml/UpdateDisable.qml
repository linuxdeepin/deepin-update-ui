// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
// import org.deepin.dtk 1.0 as D
import QtQuick 2.15
import QtQuick.Controls 2.0
import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 500

        color: "transparent"
        clip: true

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 0

            Image {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                source: dccData.model().isUpdateDisabled ? "update_prohibit" : "update_no_active"     
                height: 140
            }

            D.Label {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                width: implicitWidth
                height: 30
                text: dccData.model().isUpdateDisabled ? qsTr("The system updates are disabled. Please contact your administrator for help")
                                                       : qsTr("Your system is not activated, and it failed to connect to update services")
                font.pixelSize: 12
            }
        }
    }
}
