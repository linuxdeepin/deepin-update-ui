// SPDX-FileCopyrightText: 2024 - 2027 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
// import org.deepin.dtk 1.0 as D
import QtQuick 2.15
import QtQuick.Controls 2.0
import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0
import QtQuick.Layouts 1.15

ColumnLayout {

    width: parent.width
    Rectangle {
        width: parent.width
        height: 500
        color: "transparent"
        clip: true
        property int processVal: 0
        Layout.fillHeight: true

        ColumnLayout {
            width: parent.width
            anchors.centerIn: parent
            spacing: 0
            Image {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                visible: true
                source: "update_no_active"
                height: 140
            }

            D.Label {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                width: implicitWidth
                text: qsTr("Your system is not activated, and it failed to connect to update services")
                font.pixelSize: 12
                height: 30
            }
        }
    }
}
