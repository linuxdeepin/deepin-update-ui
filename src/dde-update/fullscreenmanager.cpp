// SPDX-FileCopyrightText: 2015 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fullscreenmanager.h"
#include "abstractfullbackgroundinterface.h"
#include "fullscreenbackground.h"

#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(logUpdateModal)

FullScreenManager::FullScreenManager(std::function<QWidget *(QScreen *)> function, QObject *parent)
    : QObject(parent)
    , m_registerFun(function)
    , m_copyModeFlag(false)
{
    qCDebug(logUpdateModal) << "Initialize FullScreenManager";
    Q_ASSERT(m_registerFun);

    connect(qApp, &QGuiApplication::screenAdded, this, &FullScreenManager::screenCountChanged);
    connect(qApp, &QGuiApplication::screenRemoved, this, &FullScreenManager::screenCountChanged);

    screenCountChanged();
}

/**判断屏幕是否为复制模式的依据，第一个屏幕的X和Y值是否和其他的屏幕的X和Y值相等
 * 对于复制模式，这两个值肯定是相等的，如果不是复制模式，这两个值肯定不等，目前支持双屏
 * @brief DisplayManager::isCopyMode
 * @return
 */
bool FullScreenManager::isCopyMode() const
{
    qCDebug(logUpdateModal) << "Checking if display is in copy mode";
    QList<QScreen *> screens = qApp->screens();
    if (screens.size() < 2) {
        qCDebug(logUpdateModal) << "Less than 2 screens, not copy mode";
        return false;
    }

    // 在多个屏幕的情况下，如果所有屏幕的位置的X和Y值都相等，则认为是复制模式
    QRect screenRect = screens[0]->availableGeometry();
    qCDebug(logUpdateModal) << "Primary screen geometry:" << screenRect;
    for (int i = 1; i < screens.size(); i++) {
        QRect rect = screens[i]->availableGeometry();
        if (screenRect.x() != rect.x() || screenRect.y() != rect.y())
            return false;
    }

    return true;
}

void FullScreenManager::checkCopyModeChanged()
{
    qCDebug(logUpdateModal) << "Checking copy mode change, current flag:" << m_copyModeFlag;
    bool currentCopyMode = isCopyMode();
    if (m_copyModeFlag != currentCopyMode) {
        qCDebug(logUpdateModal) << "Copy mode changed from" << m_copyModeFlag << "to" << currentCopyMode;
        m_copyModeFlag = !m_copyModeFlag;
        handleCopyModeChanged(m_copyModeFlag);

        Q_EMIT copyModeChanged(m_copyModeFlag);
    }
}

// 主动检查是否复制模式，响应其变化，避免wayland下显示模式设置在应用启动后
void FullScreenManager::handleCopyModeChanged(bool isCopyMode)
{
    qCDebug(logUpdateModal) << "Handling copy mode change to:" << isCopyMode;
    if(!isCopyMode) {
        qCDebug(logUpdateModal) << "Not copy mode, calling screenCountChanged";
        screenCountChanged();
        return;
    }

    QList<ScreenPtr> screens = m_screenContents.values();
    // 取消关联
    int removedScreenCounter = 0;
    for (auto &s : screens) {
        removedScreenCounter++;
        if (screens.size() == removedScreenCounter) {
            break;
        }

        disconnect(s);
        auto backgroundFrame = m_screenContents.key(s);
        if (backgroundFrame) {
            backgroundFrame->setVisible(false);
        }
        s = nullptr;
    }

}

void FullScreenManager::screenCountChanged()
{
    qCDebug(logUpdateModal) << "Screen count changed, current app screens:" << qApp->screens().size();
    QList<ScreenPtr> screens = m_screenContents.values();

    // 找到过期的screen指针
    QList<ScreenPtr> screens_to_remove;
    for (const auto &s : screens) {
        if (!qApp->screens().contains(s))
            screens_to_remove.append(s);
    }

    // 找出新增的screen指针
    QList<ScreenPtr> screens_to_add;
    m_copyModeFlag = isCopyMode();
    for (const auto &s : qApp->screens()) {
        // 复制模式，留下一个可用的屏
        if ((!m_copyModeFlag || screens_to_add.count() < 1) && !screens.contains(s)) {
            qCDebug(logUpdateModal) << "Found new screen to add";
            screens_to_add.append(s);
        }

        // 屏幕的 geometry 变化时检查显示模式是否有变化
        connect(s, &QScreen::geometryChanged, this, &FullScreenManager::checkCopyModeChanged, Qt::UniqueConnection);
    }

    // 取消关联
    qCDebug(logUpdateModal) << "Removing" << screens_to_remove.size() << "expired screens";
    for (auto &s : screens_to_remove) {
        disconnect(s);
        auto backgroundFrame = qobject_cast<FullScreenBackground*>(m_screenContents.key(s));
        if (backgroundFrame) {
            qCWarning(logUpdateModal) << "Hiding background frame for removed screen" << s;
            backgroundFrame->setVisible(false);
            m_screenContents.remove(backgroundFrame);
            backgroundFrame->unBindContent();
            backgroundFrame->deleteLater();
        }
    }

    // 创建关联
    qCDebug(logUpdateModal) << "Adding" << screens_to_add.size() << "new screens";
    for (const auto &s : screens_to_add) {

        // 显示器信息发生任何变化时，都应该重新刷新一次任务栏的显示位置
        connect(s, &QScreen::geometryChanged, this, &FullScreenManager::handleScreenChanged);
        connect(s, &QScreen::availableGeometryChanged, this, &FullScreenManager::handleScreenChanged);
        connect(s, &QScreen::physicalSizeChanged, this, &FullScreenManager::handleScreenChanged);
        connect(s, &QScreen::physicalDotsPerInchChanged, this, &FullScreenManager::handleScreenChanged);
        connect(s, &QScreen::logicalDotsPerInchChanged, this, &FullScreenManager::handleScreenChanged);
        connect(s, &QScreen::virtualGeometryChanged, this, &FullScreenManager::handleScreenChanged);
        connect(s, &QScreen::primaryOrientationChanged, this, &FullScreenManager::handleScreenChanged);
        connect(s, &QScreen::orientationChanged, this, &FullScreenManager::handleScreenChanged);
        connect(s, &QScreen::refreshRateChanged, this, &FullScreenManager::handleScreenChanged);

        auto findContentNotUsed = [=](QMap<QWidget *, ScreenPtr> map) -> QWidget * {
            QMapIterator<QWidget *, ScreenPtr> it(map);
            while (it.hasNext()) {
                it.next();
                if (!it.value() && it.key()) {
                    return it.key();
                }
            }
            return nullptr;
        };

        // try to find a content frame not used now
        QWidget *content = findContentNotUsed(m_screenContents);

        // or create new content frame
        if (!content) {
            content = m_registerFun(s);
        }

        qCDebug(logUpdateModal) << "Making screen content visible and mapping to screen";
        m_screenContents.insert(content, s);
        content->setVisible(true);
    }
    handleScreenChanged();
    qCDebug(logUpdateModal) << "Screen management update completed";
}

void FullScreenManager::handleScreenChanged()
{
    qCDebug(logUpdateModal) << "Handling screen changed event, Screen contents size:" << m_screenContents.count();
    for (QWidget *w : m_screenContents.keys()) {
        auto inter = dynamic_cast<AbstractFullBackgroundInterface *>(w);
        if (inter && m_screenContents.value(w)) {
            inter->setScreen(m_screenContents.value(w));
        }
    }
}
