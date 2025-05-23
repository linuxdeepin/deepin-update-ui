// SPDX-FileCopyrightText: 2011 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef V20_DISPLAY_UTILS_H
#define V20_DISPLAY_UTILS_H

#include <QMargins>
#include <QSize>
#include <QVariant>
#include <QSettings>
#include <QMetaMethod>
#include <DSysInfo>
#include <DPlatformHandle>
#include <QDateTime>

#include <DGuiApplicationHelper>

DCORE_USE_NAMESPACE
DGUI_USE_NAMESPACE

namespace DCC_NAMESPACE {

const QMargins ThirdPageContentsMargins(10, 10, 10, 10);
const QMargins ScrollAreaMargins(10, 10, 10, 10);
const QMargins ScrollAreaOffsetMargins(0, 0, 10, 0);
const QMargins ListViweItemMargin(10, 8, 10, 8);
const QVariant VListViewItemMargin = QVariant::fromValue(ListViweItemMargin);
const QMargins ThirdPageCmbMargins(0, 0, 0, 0);

const int ComboxWidgetHeight = 48;
const int SwitchWidgetHeight = 36;
const int MovedWindowWidgetHeight = 36;
const int LeftTitleWitdh = 110;

const QSize ListViweItemIconSize(84,84);
const QSize ListViweItemSize(170,168);
const QSize ListViweItemCompactModeSize(144,142);
const QSize ListViweItemIconSize_ListMode(42,42);
const QSize ListViweItemSize_ListMode(168,48);
const QSize ListViweItemCompactModeSize_ListMode(168,42);
const QSize ListViweIconSize(32,32);
const QSize ListViewItemCommonSize(168,58);
const QSize ListViewItemCommonCompactModeSize(168,46);

const qint32 ActionIconSize=34;//大图标角标大小
const qint32 ActionListSize=24;//list图标角标大小

const int List_Interval = 10;
const QMargins ArrowEnterClickMargin(8, 8, 8, 8);

const QString SystemTypeName = DSysInfo::uosEditionName(QLocale::c());

const DSysInfo::UosType UosType = DSysInfo::uosType();
const DSysInfo::UosEdition UosEdition = DSysInfo::uosEditionType();
const bool IsServerSystem = (DSysInfo::UosServer == UosType);//是否是服务器版
const bool IsCommunitySystem = (DSysInfo::UosCommunity == UosEdition);//是否是社区版
const bool IsProfessionalSystem = (DSysInfo::UosProfessional == UosEdition);//是否是专业版
const bool IsHomeSystem = (DSysInfo::UosHome == UosEdition);//是否是个人版
const bool IsEducationSystem = (DSysInfo::UosEducation == UosEdition); // 是否是教育版
const bool IsDeepinDesktop = (DSysInfo::DeepinDesktop == DSysInfo::deepinType());//是否是Deepin桌面
const bool IsNotDeepinUos = !DSysInfo::isDeepin(); // 是否是 Deepin/Uos 以外的发行版
const bool DisableDeveloperMode = {
#ifdef DISABLE_DEVELOPER_MODE
  true
#else
  false
#endif
};
const QMap<uint32_t, QString> WaylandkeyMap = { {Qt::Key_A, "A"}, {Qt::Key_B, "B"}, {Qt::Key_C, "C"}, {Qt::Key_D, "D"}, {Qt::Key_E, "E"},
    {Qt::Key_F, "F"}, {Qt::Key_G, "G"}, {Qt::Key_H, "H"}, {Qt::Key_I, "I"}, {Qt::Key_J, "J"}, {Qt::Key_K, "K"}, {Qt::Key_L, "L"}, {Qt::Key_M, "M"},
    {Qt::Key_N, "N"}, {Qt::Key_O, "O"}, {Qt::Key_P, "P"}, {Qt::Key_Q, "Q"}, {Qt::Key_R, "R"}, {Qt::Key_S, "S"}, {Qt::Key_T, "T"}, {Qt::Key_U, "U"},
    {Qt::Key_V, "V"}, {Qt::Key_W, "W"}, {Qt::Key_X, "X"}, {Qt::Key_Y, "Y"}, {Qt::Key_Z, "Z"}, {Qt::Key_Escape, "Escape"}, {Qt::Key_Tab, "Tab"},
    {Qt::Key_Backtab, "Tab"}, {Qt::Key_Backspace, "Backspace"}, {Qt::Key_Return, "Return"}, {Qt::Key_Enter, "Enter"}, {Qt::Key_Insert, "Insert"},
    {Qt::Key_Delete, "Delete"}, {Qt::Key_Pause, "Pause"}, {Qt::Key_Print, "Print"}, {Qt::Key_SysReq, "SysReq"}, {Qt::Key_Clear, "Clear"}, {Qt::Key_Home, "Home"},
    {Qt::Key_End, "End"}, {Qt::Key_Left, "←"}, {Qt::Key_Up, "↑"}, {Qt::Key_Right, "→"}, {Qt::Key_Down, "↓"}, {Qt::Key_PageUp, "PageUp"},
    {Qt::Key_PageDown, "PageDown"}, {Qt::Key_Shift, "Shift_L"}, {Qt::Key_Control, "Control_L"}, {Qt::Key_Super_L, "Super_L"}, {Qt::Key_Super_R, "Super_R"}, {Qt::Key_Alt, "Alt_L"},
    {Qt::Key_CapsLock, "CapLock"}, {Qt::Key_NumLock, "NumLock"}, {Qt::Key_ScrollLock, "ScrollLock"}, {Qt::Key_F1, "F1"}, {Qt::Key_F2, "F2"}, {Qt::Key_F3, "F3"},
    {Qt::Key_F4, "F4"}, {Qt::Key_F5, "F5"}, {Qt::Key_F6, "F6"}, {Qt::Key_F7, "F7"}, {Qt::Key_F8, "F8"}, {Qt::Key_F9, "F9"},
    {Qt::Key_F10, "F10"}, {Qt::Key_F11, "F11"}, {Qt::Key_F12, "F12"}, {Qt::Key_Space, "Space"}, {Qt::Key_AsciiTilde, "~"}
};

struct ListSubItem {
    ListSubItem();
    ListSubItem(QString icon, QString text, QMetaMethod signal, QObject *plugin = nullptr, QString gsettingsName = QString())
    {
        this->itemIcon = icon;
        this->itemText = text;
        this->itemSignal = signal;
        this->plugin = plugin;
        this->gsettingsName = gsettingsName;
    }
    QString itemIcon;
    QString itemText;
    QMetaMethod itemSignal;
    QObject *plugin = nullptr;
    QString gsettingsName;
};

const QList<int> FontSizeList {11, 12, 13, 14, 15, 16, 18, 20};
const QList<int> FontSizeList_Compact {10, 11, 12, 13, 14, 15, 16};

template <typename T>
T valueByQSettings(const QStringList& configFiles, const QString& group, const QString& key, const QVariant& failback)
{
    for (const QString& path : configFiles) {
        QSettings settings(path, QSettings::IniFormat);
        if (!group.isEmpty()) {
            settings.beginGroup(group);
        }

        const QVariant& v = settings.value(key);
        if (v.isValid()) {
            T t = v.value<T>();
            return t;
        }
    }

    return failback.value<T>();
}

inline bool compareVersion(const QString &targetVersion, const QString &baseVersion)
{
    QStringList version1 = baseVersion.split(".");
    QStringList version2 = targetVersion.split(".");

    if (version1.size() != version2.size()) {
        return false;
    }

    for (int i = 0; i < version1.size(); ++i) {
        // 相等判断下一个子版本号
        if (version1[i] == version2[i])
            continue;

        // 转成整形比较
        if (version1[i].toInt() > version2[i].toInt()) {
            return false;
        } else {
            return true;
        }
    }

    return true;
}

/**
 * @brief utc时间格式转本地日期
 *
 * @param utcDateTime utc时间字符串，示例：2022-04-28T11:24:59+08:00
 * @return QString 本地日期字符串，示例：2022-04-28
 */
inline QString utcDateTime2LocalDate(const QString& utcDateTime) {
    if (utcDateTime.isEmpty())
        return "";

    QDateTime dateTime = QDateTime::fromString(utcDateTime, "yyyy-MM-ddTHH:mm:ss+08:00");
    if (!dateTime.isValid())
        return "";

    return dateTime.toLocalTime().toString("yyyy-MM-dd");
}

/**
 * @brief 显示密钥数据格式化
 *
 * @param 输入无"-"的字符串
 * @return 返回每个4个字符添加一个"-"的字符串
 */
inline QString getSecurityKeyDisplayData(QString key)
{
    key = key.remove(QRegularExpression("-"));
    int keyLen = key.length();
    QList<QString> list;
    QString value = "";
    int count = keyLen / 4;
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < 4; j++) {
            value += key[4*i+j];
        }
        list.append(value);
        value = "";
    }
    int reset = keyLen % 4;
    for (int i = 0; i < reset; i++) {
        value += key[4*count+i];
    }
    if (value != "") {
        list.append(value);
    }
    value = "";
    int listCount = list.count();
    for (int i = 0; i < listCount; i++) {
        value += list.at(i);
        value += "-";
    }
    int lastLen = value.length() - 1;
    if ( lastLen > 0 && value[lastLen] == "-") {
        value = value.left(value.length() - 1);
    }
    return value;
}

inline QString getThemeName()
{
    DGuiApplicationHelper::ColorType colorType = DGuiApplicationHelper::instance()->themeType();
    if (DGuiApplicationHelper::ColorType::DarkType == colorType)
        return "dark";

    return "light";
}

}

#endif // V20_DISPLAY_UTILS_H
