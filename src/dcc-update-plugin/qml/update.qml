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

    // 标识当前是否有更新模块视图显示
    property bool hasView: false
    // 激活更新模型的信号，用于触发更新检查
    signal activeUpdateModel()

    name: "update"
    parentName: "root"
    displayName: qsTr("System Update")
    description: qsTr("System update and upgrade")
    icon: "update"
    weight: 100

    page: DccRightView{

        // 当组件创建完成时，设置hasView为true并触发更新检查
        Component.onCompleted: {
            hasView = true
            activeUpdateModel();
        }
        // 当组件销毁时，设置hasView为false
        Component.onDestruction: {
            hasView = false
        }
    }
}
