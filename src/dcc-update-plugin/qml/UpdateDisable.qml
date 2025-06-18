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
            anchors.fill: parent
            spacing: 0

            Item {
                Layout.fillHeight: true
            }

            Image {
                Layout.alignment: Qt.AlignHCenter
                source: dccData.model().updateDisabledIcon
                height: 140
            }

            D.Label {
                Layout.fillWidth: true
                text: dccData.model().updateDisabledTips
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
