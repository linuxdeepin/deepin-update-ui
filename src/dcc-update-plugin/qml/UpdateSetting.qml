// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick 2.0
import QtQuick.Controls 2.0
import Qt.labs.qmlmodels 1.2
import QtQuick.Layouts 1.15
import QtQuick

import org.deepin.dtk 1.0 as D
import org.deepin.dcc 1.0

import org.deepin.dcc.update 1.0

DccObject {
    id: root
    property bool syncingUpgradeDeliverySwitch: false

    FontMetrics {
        id: fm
    }

    D.DialogWindow {
        id: upgradeDeliverySetConfigFailedDialog
        width: 360
        icon: "preferences-system"
        modality: Qt.ApplicationModal
        visible: false

        ColumnLayout {
            width: parent.width

            D.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("Failed to change Delivery Optimization setting")
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
                    text: qsTr("Cancel")
                    Layout.fillWidth: true
                    focus: upgradeDeliverySetConfigFailedDialog.visible
                    onClicked: {
                        upgradeDeliverySetConfigFailedDialog.close()
                    }
                    Keys.onEscapePressed: {
                        upgradeDeliverySetConfigFailedDialog.close()
                    }
                }

                component ButtonWithToolTip: D.Button {
                    id: customButton

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

    Connections {
        target: dccData.work()
        function onUpgradeDeliveryConfigSetFailed() {
            upgradeDeliverySetConfigFailedDialog.show()
        }
    }

    D.DialogWindow {
        id: upgradeDeliverySetEnableFailedDialog
        width: 360
        icon: "preferences-system"
        modality: Qt.ApplicationModal
        visible: false

        ColumnLayout {
            width: parent.width

            D.Label {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: qsTr("Update Delivery Optimization service exception")
            }

            Item {
                Layout.fillHeight: true
                Layout.preferredHeight: 22
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.bottomMargin: 10
                spacing: 10

                EnableFailedDialogButton {
                    text: qsTr("Try again")
                    Layout.fillWidth: true
                    focus: upgradeDeliverySetEnableFailedDialog.visible
                    onClicked: {
                        upgradeDeliverySetEnableFailedDialog.close()
                        dccData.work().setUpgradeDeliveryEnabled(!dccData.model().upgradeDeliveryEnable, true)
                    }
                    Keys.onEscapePressed: {
                        upgradeDeliverySetEnableFailedDialog.close()
                    }
                }

                EnableFailedDialogButton {
                    text: qsTr("Cancel")
                    Layout.fillWidth: true
                    onClicked: {
                        upgradeDeliverySetEnableFailedDialog.close()
                    }
                }

                component EnableFailedDialogButton: D.Button {
                    id: customButton

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

    Connections {
        target: dccData.work()
        function onUpgradeDeliveryEnableSetFailed() {
            upgradeDeliverySetEnableFailedDialog.show()
        }
    }

    DccTitleObject {
        name: "updateTypeGrpTitle"
        parentName: "updateSettingsPage"
        displayName: qsTr("Update Type")
        weight: 10
        visible: !dccData.model().isPrivateUpdate
    }

    DccObject {
        id: updateTypeGrp
        name: "updateTypeGrp"
        parentName: "updateSettingsPage"
        weight: 20
        pageType: DccObject.Item
        visible: !dccData.model().isPrivateUpdate
        page: DccGroupView {
            height: implicitHeight + 10
            spacing: 0
        }

        // 下载、备份和安装过程中，按钮置灰不可用
        property bool editorEnabled: !dccData.model().downloadWaiting && 
                                     !dccData.model().upgradeWaiting && 
                                     !dccData.model().installinglistModel.anyVisible && 
                                     !dccData.model().backingUpListModel.anyVisible && 
                                     !dccData.model().downloadinglistModel.anyVisible &&
                                     !dccData.model().updateProhibited

        DccObject {
            name: "functionUpdate"
            parentName: "updateTypeGrp"
            displayName: qsTr("Function Updates")
            description: qsTr("Delivers a cumulative update including new features, quality updates, and  security updates")
            weight: 10
            enabled: updateTypeGrp.editorEnabled
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().functionUpdate
                onCheckedChanged: {
                    if (checked !== dccData.model().functionUpdate) {
                        dccData.work().setFunctionUpdate(checked)
                    }
                }
            }
        }
        DccObject {
            name: "securityUpdate"
            parentName: "updateTypeGrp"
            displayName: qsTr("Security Updates")
            description: qsTr("Delivers security updates")
            weight: 20
            visible: dccData.model().securityUpdateEnabled && !dccData.model().isCommunitySystem()
            enabled: updateTypeGrp.editorEnabled
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().securityUpdate
                onCheckedChanged: {
                    if (checked !== dccData.model().securityUpdate) {
                        dccData.work().setSecurityUpdate(checked)
                    }
                }
            }
        }
        DccObject {
            name: "thirdPartyUpdate"
            parentName: "updateTypeGrp"
            displayName: qsTr("Third-party Updates")
            description: qsTr("Delivers  updates for additional repository sources")
            weight: 30
            visible: dccData.model().thirdPartyUpdateEnabled
            enabled: updateTypeGrp.editorEnabled
            pageType: DccObject.Editor
            page: D.Switch {
                checked: dccData.model().thirdPartyUpdate
                onCheckedChanged: {
                    if (checked !== dccData.model().thirdPartyUpdate) {
                        dccData.work().setThirdPartyUpdate(checked)
                    }
                }
            }
        }
    }

    DccObject {
        id: advancedSetting
        property bool showDetails: dccData.model().isPrivateUpdate
        name: "advancedSettingTitle"
        parentName: "updateSettingsPage"
        displayName: qsTr("Advanced Settings")
        weight: 30
        pageType: DccObject.Item
        page: RowLayout {
            Component.onDestruction:   {
                advancedSetting.showDetails = false || dccData.model().isPrivateUpdate
            }
            DccLabel {
                property D.Palette textColor: D.Palette {
                    normal: Qt.rgba(0, 0, 0, 0.9)
                    normalDark: Qt.rgba(1, 1, 1, 0.9)
                }
                color: D.ColorSelector.textColor
                // Use the same font style as DccTitleObject (weight: 500)
                font: DccUtils.copyFont(D.DTK.fontManager.t5, {
                                            "weight": 500
                                        })
                text: dccObj.displayName
            }
            Item {
                Layout.fillWidth: true
            }
            D.ToolButton {
                textColor: D.Palette {
                    normal {
                        common: D.DTK.makeColor(D.Color.Highlight)
                    }
                    normalDark: normal
                    hovered {
                        common: D.DTK.makeColor(D.Color.Highlight).lightness(+30)
                    }
                    hoveredDark: hovered
                }
                bottomPadding: 0
                Layout.rightMargin: 8
                font: D.DTK.fontManager.t8
                text: advancedSetting.showDetails ? qsTr("Collapse") : qsTr("Expand")
                onClicked: {
                    advancedSetting.showDetails = !advancedSetting.showDetails
                }
                background: Item {}
            }
        }

        onParentItemChanged: {
            if (parentItem) {
                parentItem.leftPadding = 10
            }
        }
    }
    DccObject {
        id: advancedSettingGroup
        name: "advancedSettingGroup"
        parentName: "updateSettingsPage"
        weight: 40
        pageType: DccObject.Item
        page: DccGroupView {
            visible: advancedSetting.showDetails
            spacing: 0
        }
        Connections {
            target: DccApp
            function onTriggeredObjectsChanged(objs) {
                if (objs.includes(advancedSettingGroup)) {
                    advancedSetting.showDetails = true
                }
            }
        }

        DccObject {
            id: upgradeDeliveryGrp
            name: "upgradeDeliveryGrp"
            parentName: "advancedSettingGroup"
            weight: 40
            visible: dccData.work().p2pUpdateSupported()
            pageType: DccObject.Item
            page: DccGroupView {
                height: implicitHeight + 10
                spacing: 0
            }
            Connections {
                target: dccData.work()
                function onP2PUpdateSupportChanged() {
                    upgradeDeliveryGrp.visible = dccData.work().p2pUpdateSupported()
                }
            }

            DccObject {
                name: "upgradeDeliverySwitch"
                parentName: "upgradeDeliveryGrp"
                displayName: qsTr("Delivery Optimization")
                description: qsTr("When enabled, your device may share previously downloaded system updates with other devices on your local network.When you turn it off, cached files from update delivery will be cleared during the next restart.")
                weight: 10
                enabled: !dccData.model().isPrivateUpdate
                pageType: DccObject.Editor
                page: D.Switch {
                    id: upgradeDeliverySwitch
                    checked: dccData.model().upgradeDeliveryEnable
                    onCheckedChanged: {
                        if (root.syncingUpgradeDeliverySwitch) {
                            return
                        }
                        dccData.work().setUpgradeDeliveryEnabled(checked)
                    }
                    Connections {
                        target: dccData.model()
                        function onUpgradeDeliveryEnableChanged() {
                            root.syncingUpgradeDeliverySwitch = true
                            upgradeDeliverySwitch.checked = dccData.model().upgradeDeliveryEnable
                            root.syncingUpgradeDeliverySwitch = false
                        }
                    }
                }
            }

            DccObject {
                id: upgradeDeliveryUploadLimitSetting
                name: "upgradeDeliveryUploadLimitSetting"
                parentName: "upgradeDeliveryGrp"
                displayName: qsTr("Delivery Optimization-Upload throttling")
                visible: dccData.model().upgradeDeliveryEnable
                weight: 20
                enabled: !dccData.model().upgradeUploadSpeedIsOnline
                pageType: DccObject.Item
                Connections {
                    target: dccData.model()
                    function onUpgradeUploadSpeedLimitConfigChanged() {
                        upgradeDeliveryUploadLimitSetting.enabled = !dccData.model().upgradeUploadSpeedIsOnline
                    }
                }
                page: RowLayout {
                    D.CheckBox {
                        id: uploadLimitCheckBox
                        Layout.leftMargin: 14
                        Layout.fillWidth: true
                        text: dccObj.displayName
                        ToolTip {
                            visible: uploadLimitCheckBox.hovered && uploadLimitCheckBox.width < uploadLimitCheckBox.implicitWidth
                            text: uploadLimitCheckBox.text
                        }
                        checked: dccData.model().upgradeUploadSpeedEnable
                        Connections {
                            target: dccData.model()
                            function onUpgradeUploadSpeedLimitConfigChanged() {
                                uploadLimitCheckBox.checked = dccData.model().upgradeUploadSpeedEnable
                            }
                        }
                        onCheckedChanged: {
                            dccData.work().setUpgradeDeliveryUploadLimitSpeed(uploadLineEdit.text, uploadLimitCheckBox.checked)
                        }
                    }

                    RowLayout {
                        spacing: 10
                        Layout.topMargin: 5
                        Layout.bottomMargin: 5
                        Layout.rightMargin: 10
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        D.LineEdit {
                            id: uploadLineEdit
                            maximumLength: 6
                            validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                            alertText: qsTr("Only numbers between 10-999999 are allowed")
                            alertDuration: 3000
                            enabled: uploadLimitCheckBox.checked
                            clearButton.active: uploadLineEdit.activeFocus && (text.length !== 0)
                            text: dccData.model().upgradeUploadSpeedCurrentRate

                            function triggerAlert() {
                                uploadLineEdit.showAlert = true
                                uploadAlertResetTimer.restart()
                            }

                            Timer {
                                id: uploadAlertResetTimer
                                interval: uploadLineEdit.alertDuration
                                onTriggered: uploadLineEdit.showAlert = false
                            }

                            onTextChanged: {
                                if (uploadLineEdit.text.length !== 0 && (uploadLineEdit.text[0] === "0" || Number(uploadLineEdit.text) > 999999)) {
                                    uploadLineEdit.text = ""
                                    uploadLineEdit.triggerAlert()
                                } else if (uploadLineEdit.showAlert && (uploadLineEdit.text.length === 0 || (Number(uploadLineEdit.text) >= 10 && Number(uploadLineEdit.text) <= 999999))) {
                                    uploadLineEdit.showAlert = false
                                }
                            }
                            onEditingFinished: {
                                if (uploadLineEdit.text.length === 0) {
                                    uploadLineEdit.text = dccData.model().upgradeUploadSpeedCurrentRate
                                    return
                                }
                                if (Number(uploadLineEdit.text) < 10 || Number(uploadLineEdit.text) > 999999) {
                                    uploadLineEdit.text = dccData.model().upgradeUploadSpeedCurrentRate
                                    uploadLineEdit.triggerAlert()
                                    return
                                }
                                dccData.work().setUpgradeDeliveryUploadLimitSpeed(uploadLineEdit.text, uploadLimitCheckBox.checked)
                            }
                            Keys.onPressed: {
                                if (event.key === Qt.Key_Return) {
                                    uploadLineEdit.forceActiveFocus(false);
                                }
                            }
                            Connections {
                                target: dccData.model()
                                function onUpgradeUploadSpeedLimitConfigChanged() {
                                    uploadLineEdit.text = dccData.model().upgradeUploadSpeedCurrentRate
                                }
                            }
                        }

                        D.Label {
                            text: "KB/S"
                        }
                    }
                }
            }

            DccObject {
                id: upgradeDeliveryDownloadLimitSetting
                name: "upgradeDeliveryDownloadLimitSetting"
                parentName: "upgradeDeliveryGrp"
                displayName: qsTr("Delivery Optimization-Limit Speed")
                visible: dccData.model().upgradeDeliveryEnable
                weight: 30
                enabled: !dccData.model().upgradeDownloadSpeedIsOnline
                pageType: DccObject.Item
                Connections {
                    target: dccData.model()
                    function onUpgradeDownloadSpeedLimitConfigChanged() {
                        upgradeDeliveryDownloadLimitSetting.enabled = !dccData.model().upgradeDownloadSpeedIsOnline
                    }
                }
                page: RowLayout {
                    D.CheckBox {
                        id: downloadLimitCheckBox
                        Layout.leftMargin: 14
                        Layout.fillWidth: true
                        text: dccObj.displayName
                        ToolTip {
                            visible: downloadLimitCheckBox.hovered && downloadLimitCheckBox.width < downloadLimitCheckBox.implicitWidth
                            text: downloadLimitCheckBox.text
                        }
                        checked: dccData.model().upgradeDownloadSpeedEnable
                        Connections {
                            target: dccData.model()
                            function onUpgradeDownloadSpeedLimitConfigChanged() {
                                downloadLimitCheckBox.checked = dccData.model().upgradeDownloadSpeedEnable
                            }
                        }
                        onCheckedChanged: {
                            dccData.work().setUpgradeDeliveryDownloadLimitSpeed(downloadLineEdit.text, downloadLimitCheckBox.checked)
                        }
                    }

                    RowLayout {
                        spacing: 10
                        Layout.topMargin: 5
                        Layout.bottomMargin: 5
                        Layout.rightMargin: 10
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        D.LineEdit {
                            id: downloadLineEdit
                            maximumLength: 6
                            validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                            alertText: qsTr("Only numbers between 10-999999 are allowed")
                            alertDuration: 3000
                            enabled: downloadLimitCheckBox.checked
                            clearButton.active: downloadLineEdit.activeFocus && (text.length !== 0)
                            text: dccData.model().upgradeDownloadSpeedCurrentRate

                            function triggerAlert() {
                                downloadLineEdit.showAlert = true
                                downloadAlertResetTimer.restart()
                            }

                            Timer {
                                id: downloadAlertResetTimer
                                interval: downloadLineEdit.alertDuration
                                onTriggered: downloadLineEdit.showAlert = false
                            }

                            onTextChanged: {
                                if (downloadLineEdit.text.length !== 0 && (downloadLineEdit.text[0] === "0" || Number(downloadLineEdit.text) > 999999)) {
                                    downloadLineEdit.text = ""
                                    downloadLineEdit.triggerAlert()
                                } else if (downloadLineEdit.showAlert && (downloadLineEdit.text.length === 0 || (Number(downloadLineEdit.text) >= 10 && Number(downloadLineEdit.text) <= 999999))) {
                                    downloadLineEdit.showAlert = false
                                }
                            }
                            onEditingFinished: {
                                if (downloadLineEdit.text.length === 0) {
                                    downloadLineEdit.text = dccData.model().upgradeDownloadSpeedCurrentRate
                                    return
                                }
                                if (Number(downloadLineEdit.text) < 10 || Number(downloadLineEdit.text) > 999999) {
                                    downloadLineEdit.text = dccData.model().upgradeDownloadSpeedCurrentRate
                                    downloadLineEdit.triggerAlert()
                                    return
                                }
                                dccData.work().setUpgradeDeliveryDownloadLimitSpeed(downloadLineEdit.text, downloadLimitCheckBox.checked)
                            }
                            Keys.onPressed: {
                                if (event.key === Qt.Key_Return) {
                                    downloadLineEdit.forceActiveFocus(false);
                                }
                            }
                            Connections {
                                target: dccData.model()
                                function onUpgradeDownloadSpeedLimitConfigChanged() {
                                    downloadLineEdit.text = dccData.model().upgradeDownloadSpeedCurrentRate
                                }
                            }
                        }

                        D.Label {
                            text: "KB/S"
                        }
                    }
                }
            }
        }

        DccObject {
            id: downloadLimitGrp
            name: "downloadLimitGrp"
            parentName: "advancedSettingGroup"
            weight: 50
            enabled: !dccData.model().downloadIsOnlineSpeedLimit
            pageType: DccObject.Item
            page: DccGroupView {
                height: implicitHeight + 10
                spacing: 0
            }
            Connections {
                target: dccData.model()
                function onDownloadSpeedLimitConfigChanged() {
                    downloadLimitGrp.enabled = !dccData.model().downloadIsOnlineSpeedLimit
                }
            }

            DccObject {
                name: "downloadLimit"
                parentName: "downloadLimitGrp"
                displayName: qsTr("Limit Speed")
                weight: 10
                enabled: !dccData.model().updateProhibited
                pageType: DccObject.Editor
                page: D.Switch {
                    checked: dccData.model().downloadSpeedLimitEnabled || dccData.model().downloadIsOnlineSpeedLimit
                    onCheckedChanged: {
                        if (checked !== dccData.model().downloadSpeedLimitEnabled) {
                            dccData.work().setDownloadSpeedLimitEnabled(checked)
                        }
                    }
                }
            }

            DccObject {
                name: "limitSetting"
                parentName: "downloadLimitGrp"
                displayName: qsTr("Limit Setting")
                visible: dccData.model().downloadSpeedLimitEnabled
                weight: 20
                enabled: !dccData.model().updateProhibited
                pageType: DccObject.Editor
                page: RowLayout {
                    D.LineEdit {
                        id: lineEdit
                        maximumLength: 6
                        validator: RegularExpressionValidator { regularExpression: /^\d*$/ }
                        alertText: qsTr("Only numbers between 10-999999 are allowed")
                        alertDuration: 3000
                        clearButton.active: lineEdit.activeFocus && (text.length !== 0)
                        text: dccData.model().downloadSpeedLimitSize

                        function triggerAlert() {
                            lineEdit.showAlert = true
                            alertResetTimer.restart()
                        }

                        Timer {
                            id: alertResetTimer
                            interval: lineEdit.alertDuration
                            onTriggered: lineEdit.showAlert = false
                        }

                        onTextChanged: {
                            if (lineEdit.text.length !== 0 && (lineEdit.text[0] === "0" || Number(lineEdit.text) > 999999)) {
                                lineEdit.text = ""
                                lineEdit.triggerAlert()
                            } else if (lineEdit.showAlert && (lineEdit.text.length === 0 || (Number(lineEdit.text) >= 10 && Number(lineEdit.text) <= 999999))) {
                                lineEdit.showAlert = false
                            }
                        }
                        onEditingFinished: {
                            if (lineEdit.text.length === 0) {
                                lineEdit.text = dccData.model().downloadSpeedLimitSize
                                return
                            }
                            if (Number(lineEdit.text) < 10 || Number(lineEdit.text) > 999999) {
                                lineEdit.text = dccData.model().downloadSpeedLimitSize
                                lineEdit.triggerAlert()
                                return
                            }
                            dccData.work().setDownloadSpeedLimitSize(lineEdit.text)
                        }
                        Keys.onPressed: {
                            if (event.key === Qt.Key_Return) {
                                lineEdit.forceActiveFocus(false);
                            }
                        }
                    }

                    D.Label {
                        text: "KB/S"
                    }
                }
            }
        }

        DccObject {
            name: "autoDownloadGrp"
            parentName: "advancedSettingGroup"
            weight: 50
            visible: !dccData.model().isPrivateUpdate
            pageType: DccObject.Item
            page: DccGroupView {
                height: implicitHeight + 10
                spacing: 0
            }
            DccObject {
                name: "autoDownload"
                parentName: "autoDownloadGrp"
                displayName: qsTr("Auto Download")
                description: qsTr("Enabling \"Auto Download Updates\" will automatically download updates when connected to the internet")
                weight: 10
                enabled: !dccData.model().updateProhibited && !dccData.model().updateModeDisabled
                pageType: DccObject.Editor
                page: D.Switch {
                    checked: dccData.model().autoDownloadUpdates
                    onCheckedChanged: {
                        if (checked !== dccData.model().autoDownloadUpdates) {
                            dccData.work().setAutoDownloadUpdates(checked)
                        }
                    }
                }
            }

            DccObject {
                name: "limitSetting"
                parentName: "autoDownloadGrp"
                displayName: qsTr("Download when Inactive")
                visible: dccData.model().autoDownloadUpdates && !dccData.model().isPrivateUpdate
                weight: 20
                enabled: !dccData.model().updateProhibited && !dccData.model().updateModeDisabled
                pageType: DccObject.Item
                page: RowLayout {
                    D.CheckBox {
                        id: inactiveDownloadCheckBox
                        Layout.leftMargin: 14
                        Layout.fillWidth: true
                        ToolTip {
                            visible: inactiveDownloadCheckBox.width < inactiveDownloadCheckBox.implicitWidth && inactiveDownloadCheckBox.hovered
                            text: inactiveDownloadCheckBox.text
                        }
                        text: dccObj.displayName
                        checked: {
                            if (!dccData.model().autoDownloadUpdates)
                                return false

                            return dccData.model().idleDownloadEnabled
                        }
                        onCheckedChanged: {
                            if (checked !== dccData.model().idleDownloadEnabled) {
                                dccData.work().setIdleDownloadEnabled(checked)
                            }
                        }
                    }

                    RowLayout {
                        spacing: 10
                        Layout.topMargin: 5
                        Layout.bottomMargin: 5
                        Layout.rightMargin: 10
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

                        D.Label {
                           text: qsTr("Start at")
                           enabled: inactiveDownloadCheckBox.checked
                        }

                        DccTimeRange {
                            id: beginTimeRange
                            enabled: inactiveDownloadCheckBox.checked
                            hour: dccData.model().beginTime.split(':')[0]
                            minute: dccData.model().beginTime.split(':')[1]
                            onTimeChanged: {
                                dccData.work().setIdleDownloadBeginTime(beginTimeRange.timeString)
                            }
                        }

                        D.Label {
                            Layout.leftMargin: 10
                            text: qsTr("End at")
                            enabled: inactiveDownloadCheckBox.checked
                        }

                        DccTimeRange {
                            id: endTimeRange
                            enabled: inactiveDownloadCheckBox.checked
                            hour: dccData.model().endTime.split(':')[0]
                            minute: dccData.model().endTime.split(':')[1]
                            onTimeChanged: {
                                dccData.work().setIdleDownloadEndTime(endTimeRange.timeString)
                            }
                        }
                    }
                }
            }
        }

        DccObject {
            name: "advancedSettingGrp"
            parentName: "advancedSettingGroup"
            weight: 60
            pageType: DccObject.Item
            page: DccGroupView {
                height: implicitHeight + 10
                spacing: 0
            }

            DccObject {
                name: "updateReminder"
                parentName: "advancedSettingGrp"
                displayName: qsTr("Updates Notification")
                weight: 10
                pageType: DccObject.Editor
                visible: !dccData.model().isPrivateUpdate
                enabled: !dccData.model().updateProhibited && !dccData.model().updateModeDisabled
                page: D.Switch {
                    checked: dccData.model().updateNotify
                    onCheckedChanged: {
                        if (checked !== dccData.model().updateNotify) {
                            dccData.work().setUpdateNotify(checked)
                        }
                    }
                }
            }

            DccObject {
                name: "cleanCache"
                parentName: "advancedSettingGrp"
                displayName: qsTr("Clear Package Cache")
                weight: 20
                visible: !dccData.model().isPrivateUpdate
                pageType: DccObject.Editor
                page: D.Switch {
                    checked: dccData.model().autoCleanCache
                    onCheckedChanged: {
                        if (checked !== dccData.model().autoCleanCache) {
                            dccData.work().setAutoCleanCache(checked)
                        }
                    }
                }
            }

            DccObject {
                name: "updateHistory"
                parentName: "advancedSettingGrp"
                displayName: qsTr("Update History")
                weight: 40
                visible: !dccData.model().isPrivateUpdate
                backgroundType: DccObject.Normal
                pageType: DccObject.Editor
                page: D.Button {
                    implicitWidth: fm.advanceWidth(text) + fm.averageCharacterWidth * 2
                    text: qsTr("View")
                    onClicked: {
                        dccData.model().historyModel.refreshHistory()
                        uhloader.active = true
                    }

                    Loader {
                        id: uhloader
                        active: false
                        sourceComponent: UpdateHistoryDialog {
                            onClosing: function (close) {
                                uhloader.active = false
                            }
                        }
                        onLoaded: function () {
                            uhloader.item.show()
                        }
                    }
                }
            }
        }

        DccObject {
            name: "mirrorSettingGrp"
            parentName: "advancedSettingGroup"
            visible: dccData.model().isCommunitySystem() && !dccData.model().isPrivateUpdate
            weight: 70
            pageType: DccObject.Item
            page: DccGroupView {
                height: implicitHeight + 10
                spacing: 0
            }

            DccObject {
                name: "autoMirror"
                parentName: "mirrorSettingGrp"
                displayName: qsTr("Smart Mirror Switch")
                weight: 10
                enabled: !dccData.model().updateProhibited
                pageType: DccObject.Editor
                page: D.Switch {
                    checked: dccData.model().smartMirrorSwitch
                    onCheckedChanged: {
                        if (checked !== dccData.model().smartMirrorSwitch) {
                            dccData.work().setSmartMirror(checked)
                        }
                    }
                }
            }

            DccObject {
                visible: false
                name: "defautMirror"
                parentName: "mirrorSettingGrp"
             //   displayName: qsTr("默认镜像源")
                weight: 20
                enabled: !dccData.model().updateProhibited
                pageType: DccObject.Editor
                page: D.ComboBox {
                    model:["[SG]Ox.sg", "[AU]JAARNET", "[SE]Academic Computer Club", "[CN]阿里云"]
                    // checked: dccData.model().audioMono

                    currentIndex: 0
                }
            }
        }

        DccObject {
            name: "testingChannel"
            parentName: "advancedSettingGroup"
            displayName: qsTr("Join Internal Testing Channel")
            description: qsTr("Join the internal testing channel to get deepin latest updates.")
            backgroundType: DccObject.Normal
            visible: dccData.model().isCommunitySystem()
            weight: 80
            pageType: DccObject.Editor
            enabled: {
                if (dccData.model().testingChannelStatus === Common.WaitJoined ||
                    dccData.model().testingChannelStatus === Common.WaitToLeave)
                    return false
                else
                    return true
            }

            page: D.Switch {
                checked: {
                    if (dccData.model().testingChannelStatus === Common.WaitToLeave ||
                        dccData.model().testingChannelStatus === Common.Joined)
                        return true
                    else
                        return false
                }

                onClicked: {
                    dccData.work().setTestingChannelEnable(checked)
                }

                Connections {
                    target: dccData.work()
                    function onRequestCloseTestingChannel() {
                        ctcloader.active = true
                    }
                }

                Loader {
                    id: ctcloader
                    active: false
                    sourceComponent: QuitTestingChannelDialog {
                        onClosing: function (close) {
                            ctcloader.active = false
                        }
                    }
                    onLoaded: function () {
                        ctcloader.item.show()
                    }
                }
            }
        }

        DccObject {
            name: "testingChannelUrl"
            parentName: "advancedSettingGroup"
            backgroundType: DccObject.AutoBg
            weight: 90
            visible: dccData.model().testingChannelStatus === Common.WaitJoined
            pageType: DccObject.Editor
            page: D.ToolButton {
                text: qsTr("Click here to complete the application")
                textColor: D.Palette {
                    normal {
                        common: D.DTK.makeColor(D.Color.Highlight)
                    }
                    normalDark: normal
                    hovered {
                        common: D.DTK.makeColor(D.Color.Highlight).lightness(+30)
                    }
                    hoveredDark: hovered
                }
                background: Item {}
                onClicked: {
                    dccData.work().openTestingChannelUrl()
                }
            }
        }
    }
}
