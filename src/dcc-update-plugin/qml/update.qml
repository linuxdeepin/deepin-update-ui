// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import QtQuick.Controls 2.0
import Qt.labs.qmlmodels 1.2
import QtQuick.Layouts 1.15

import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0

DccObject {
    id: root

    property bool hasHandledChildrenChange: false
    signal activeUpdateModel()

    name: "update"
    parentName: "root"
    displayName: qsTr("System Update")
    description: qsTr("System update and upgrade")
    icon: "update"
    weight: 100

    page: DccRightView{

        // 切换模块时候，控件会重新创建，从而触发检查更新
        Component.onCompleted: {
            activeUpdateModel();
        }
    }

    // 通过dbus接口直接进入更新模块时，onCompleted已经发送了信号，但其子控件可能还没创建好，需要使用该信号触发检查更新，但只需要触发一次即可
    onChildrenChanged : {
        if (!hasHandledChildrenChange) {
            hasHandledChildrenChange = true
            activeUpdateModel();
        }
    }
}
