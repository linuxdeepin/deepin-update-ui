// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include "commoniconbutton.h"
#include "constants.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include <DGuiApplicationHelper>

DGUI_USE_NAMESPACE

CommonIconButton::CommonIconButton(QWidget *parent)
    : QWidget(parent)
    , m_refreshTimer(nullptr)
    , m_clickable(false)
    , m_hover(false)
    , m_state(Default)
    , m_lightThemeColor(Qt::black)
    , m_darkThemeColor(Qt::white)
    , m_activeState(false)
    , m_hoverEnable(true)
    , m_iconSize(QSize())
    , m_rotation(0)
    , m_animLabel1(new QLabel(this))
    , m_animLabel2(new QLabel(this))
    , m_effect1(new QGraphicsOpacityEffect(this))
    , m_effect2(new QGraphicsOpacityEffect(this))
    , m_fadeOutAnim(new QPropertyAnimation)
    , m_fadeInAnim(new QPropertyAnimation)
    , m_animTimer(new QTimer(this))
    , m_showingFirst(true)
{
    setAccessibleName("IconButton");
    setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    if (parent)
        setForegroundRole(parent->foregroundRole());

    m_defaultPalette = palette();

    m_animLabel1->setGraphicsEffect(m_effect1);
    m_animLabel2->setGraphicsEffect(m_effect2);
    m_animLabel1->setScaledContents(true);
    m_animLabel2->setScaledContents(true);
    m_animLabel1->setGeometry(this->rect());
    m_animLabel2->setGeometry(this->rect());
    m_animLabel1->setPixmap(QPixmap(":resources/private-lastore-sleep_16px.svg"));
    m_animLabel2->setPixmap(QPixmap(":resources/private-lastore-active_16px.svg"));
    m_animLabel1->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    m_animLabel2->setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    m_animLabel1->hide();
    m_animLabel2->hide();

    m_effect1->setOpacity(1.0);
    m_effect2->setOpacity(0.0);

    m_fadeOutAnim->setDuration(500);
    m_fadeOutAnim->setPropertyName("opacity");
    m_fadeInAnim->setDuration(500);
    m_fadeInAnim->setPropertyName("opacity");

    m_fadeOutAnim->setEasingCurve(QEasingCurve::InOutQuad);
    m_fadeInAnim->setEasingCurve(QEasingCurve::InOutQuad);

    connect(m_animTimer, &QTimer::timeout, this, &CommonIconButton::switchIcon);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &CommonIconButton::refreshIcon);
}

void CommonIconButton::setState(State state)
{
    m_state = state;
    if (m_fileMapping.contains(state)) {
        auto pair = m_fileMapping.value(state);
        setIcon(pair.first, pair.second);
    }
    if (!m_icon.isNull()) {
        updatePalette();
    }
}

void CommonIconButton::startRotate()
{
    if (!m_refreshTimer) {
        m_refreshTimer = new QTimer(this);
        m_refreshTimer->setInterval(70);
        // 定时累加角度，驱动旋转绘制。
        connect(m_refreshTimer, &QTimer::timeout, this, &CommonIconButton::startRotate);
    }
    m_refreshTimer->start();
    m_rotation += 54;
    update();
}

void CommonIconButton::stopRotate()
{
    m_refreshTimer->stop();
    m_rotation = 0;
    update();
}

void CommonIconButton::startAnimation()
{
    // 通过两张图标交替淡入淡出显示活动状态。
    m_showingFirst = true;
    m_effect1->setOpacity(1.0);
    m_effect2->setOpacity(0.0);
    m_animLabel1->setVisible(true);
    m_animLabel2->setVisible(true);
    if (!m_animTimer->isActive()) {
        m_animTimer->start(2000);
    }
}

void CommonIconButton::stopAnimation()
{
    m_animLabel1->setVisible(false);
    m_animLabel2->setVisible(false);
    m_animTimer->stop();
}

void CommonIconButton::setIcon(const QIcon &icon, QColor lightThemeColor, QColor darkThemeColor)
{
    m_icon = icon;
    if (lightThemeColor.isValid() && darkThemeColor.isValid()) {
        m_lightThemeColor = lightThemeColor;
        m_darkThemeColor = darkThemeColor;
    }

    updatePalette();
}

void CommonIconButton::updatePalette()
{
    if (isEnabled()) {
        if (m_lightThemeColor.isValid() && m_darkThemeColor.isValid()) {
            QColor color = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType ? m_lightThemeColor : m_darkThemeColor;
            if (m_activeState)
                color = palette().color(QPalette::Highlight);
            auto pa = palette();
            pa.setColor(QPalette::WindowText, color);
            setPalette(pa);
        }
    } else {
        setPalette(m_defaultPalette);
    } 

    update();
}

void CommonIconButton::switchIcon()
{
    // 切换当前显示的图标层并启动透明度动画。
    if (m_showingFirst) {
        m_fadeOutAnim->setTargetObject(m_effect1);
        m_fadeOutAnim->setStartValue(1.0);
        m_fadeOutAnim->setEndValue(0.0);

        m_fadeInAnim->setTargetObject(m_effect2);
        m_fadeInAnim->setStartValue(0.0);
        m_fadeInAnim->setEndValue(1.0);
    } else {
        m_fadeOutAnim->setTargetObject(m_effect2);
        m_fadeOutAnim->setStartValue(1.0);
        m_fadeOutAnim->setEndValue(0.0);

        m_fadeInAnim->setTargetObject(m_effect1);
        m_fadeInAnim->setStartValue(0.0);
        m_fadeInAnim->setEndValue(1.0);
    }

    m_fadeOutAnim->start();
    m_fadeInAnim->start();

    m_showingFirst = !m_showingFirst;
}

void CommonIconButton::setActiveState(bool state)
{
    m_activeState = state;
    if (m_lightThemeColor.isValid() && m_darkThemeColor.isValid()) {
        updatePalette();
    } else {
        setForegroundRole(state ? QPalette::Highlight : QPalette::NoRole);
    }
}

void CommonIconButton::setHoverEnable(bool enable)
{
    m_hoverEnable = enable;
}

void CommonIconButton::setIcon(const QString &icon, const QString &fallback, const QString &suffix)
{
    if (!m_fileMapping.contains(Default)) {
        m_fileMapping.insert(Default, QPair<QString, QString>(icon, fallback));
    }

    QString tmp = icon;
    QString tmpFallback = fallback;

    // 浅色主题下优先使用带 `-dark` 后缀的图标资源。
    static auto addDarkMark = [suffix] (QString &file) {
        if (file.contains(suffix)) {
            file.replace(suffix, "-dark" + suffix);
        } else {
            file.append("-dark");
        }
    };
    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        addDarkMark(tmp);
        addDarkMark(tmpFallback);
    }
    m_icon = QIcon::fromTheme(tmp, QIcon::fromTheme(tmpFallback));
    update();
}

void CommonIconButton::setHoverIcon(const QIcon &icon)
{
    m_hoverIcon = icon;
}

void CommonIconButton::setClickable(bool clickable)
{
    m_clickable = clickable;
}

bool CommonIconButton::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Leave:
    case QEvent::Enter:
        m_hover = e->type() == QEvent::Enter;
        update();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

void CommonIconButton::paintEvent(QPaintEvent *e)
{
    QWidget::paintEvent(e);

    if (m_animLabel1->isVisible() || m_animLabel2->isVisible()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    if (m_rotation != 0) {
        painter.translate(this->width() / 2, this->height() / 2);
        painter.rotate(m_rotation);
        painter.translate(-(this->width() / 2), -(this->height() / 2));
    }

    if (m_hoverEnable && m_hover && !m_hoverIcon.isNull()) {
        m_hoverIcon.paint(&painter, rect());
    } else if (!m_icon.isNull()) {
        if (!m_iconSize.isEmpty()) {
            const int left = (width() - m_iconSize.width()) / 2;
            const int top = (height() - m_iconSize.height()) / 2;
            m_icon.paint(&painter, rect().marginsRemoved(QMargins(left, top, left, top)));
        } else {
            m_icon.paint(&painter, rect());
        }
    }
}

void CommonIconButton::mousePressEvent(QMouseEvent *event)
{
    m_pressPos = event->pos();
    return QWidget::mousePressEvent(event);
}

void CommonIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    // 旋转期间不触发点击信号。
    if (m_clickable && rect().contains(m_pressPos) && rect().contains(event->pos()) && (!m_refreshTimer || !m_refreshTimer->isActive())) {
        Q_EMIT clicked();
        return;
    }
    return QWidget::mouseReleaseEvent(event);
}

void CommonIconButton::refreshIcon()
{
    setState(m_state);
}

void CommonIconButton::setIconSize(const QSize &size)
{
    m_iconSize = size;
}

void CommonIconButton::setAllEnabled(bool enable)
{
    setEnabled(enable);
    updatePalette();
}
