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
    , m_opacity1(0.0)
    , m_opacity2(0.0)
    , m_fadeOutAnim(new QPropertyAnimation(this))
    , m_fadeInAnim(new QPropertyAnimation(this))
    , m_animTimer(new QTimer(this))
    , m_showingFirst(true)
    , m_animating(false)
{
    setAccessibleName("IconButton");
    setFixedSize(Dock::DOCK_PLUGIN_ITEM_FIXED_SIZE);
    if (parent)
        setForegroundRole(parent->foregroundRole());

    m_defaultPalette = palette();

    m_animPixmap1 = QPixmap(":resources/private-lastore-sleep_16px.svg");
    m_animPixmap2 = QPixmap(":resources/private-lastore-active_16px.svg");

    m_fadeOutAnim->setDuration(500);
    m_fadeInAnim->setDuration(500);
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
    if (m_animating) {
        return;
    }
    m_animating = true;
    m_showingFirst = true;
    m_opacity1 = 1.0;
    m_opacity2 = 0.0;
    if (!m_animTimer->isActive()) {
        m_animTimer->start(2000);
    }
    update();
}

void CommonIconButton::stopAnimation()
{
    if (!m_animating) {
        return;
    }
    m_animating = false;
    m_animTimer->stop();
    m_fadeOutAnim->stop();
    m_fadeInAnim->stop();
    update();
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
    if (m_showingFirst) {
        m_fadeOutAnim->setTargetObject(this);
        m_fadeOutAnim->setPropertyName("opacity1");
        m_fadeOutAnim->setStartValue(1.0);
        m_fadeOutAnim->setEndValue(0.0);

        m_fadeInAnim->setTargetObject(this);
        m_fadeInAnim->setPropertyName("opacity2");
        m_fadeInAnim->setStartValue(0.0);
        m_fadeInAnim->setEndValue(1.0);
    } else {
        m_fadeOutAnim->setTargetObject(this);
        m_fadeOutAnim->setPropertyName("opacity2");
        m_fadeOutAnim->setStartValue(1.0);
        m_fadeOutAnim->setEndValue(0.0);

        m_fadeInAnim->setTargetObject(this);
        m_fadeInAnim->setPropertyName("opacity1");
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

    if (m_animating) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        if (m_opacity1 > 0.01) {
            painter.setOpacity(m_opacity1);
            painter.drawPixmap(rect(), m_animPixmap1);
        }
        if (m_opacity2 > 0.01) {
            painter.setOpacity(m_opacity2);
            painter.drawPixmap(rect(), m_animPixmap2);
        }
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
