// SPDX-FileCopyrightText: 2016 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SIGNALBRIDGE_H
#define SIGNALBRIDGE_H

#include <DSingleton>

#include <QObject>

/**
 * @brief 旨在避免对象树很深时，信号层层传递，代码过度冗余。
 * 这个类只声明信号，具体的发送和接受由业务逻辑决定。
 */
class SignalBridge : public QObject, public Dtk::Core::DSingleton<SignalBridge> {
    Q_OBJECT
    friend class Dtk::Core::DSingleton<SignalBridge>;

private:
    explicit SignalBridge(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

signals:
    /**
     * @brief 请求修改com.deepin.lastore.Manger.CheckUpdateMode的值
     *
     * @param updateType 更新类型
     * @param isChecked 是否被选中
     */
    void requestCheckUpdateModeChanged(int updateType, bool isChecked);

     /**
     * @brief 请求后台安装
     *
     * @param updateTypes 更新的类型
     * @param doBackUp 是否进行备份
     */
    void requestBackgroundInstall(int updateTypes, bool doBackup);

    /**
     * @brief 请求下载包
     *
     * @param updateTypes 更新的类型
     */
    void requestDownload(int updateTypes);

    /**
     * @brief 请求重试
     *
     * @param controlType 控制更新的类型ControlPanelType
     * @param updateTypes 更新的类型
     */
    void requestRetry(int controlType, int updateTypes);

    /**
     * @brief 停止下载
     *
     */
    void requestStopDownload();
};

#endif // SIGNALBRIDGE_H
