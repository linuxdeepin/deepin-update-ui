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
        Layout.preferredHeight: dccData.model().systemActivation ? 338 : 500

        color: "transparent"
        clip: true

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 0

            Image {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                source: dccData.model().updateDisabledIcon
                height: 140
            }

            D.Label {
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                width: implicitWidth
                height: 30
                text: dccData.model().updateDisabledTips
                font.pixelSize: 12
            }
        }
    }
}
