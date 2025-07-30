#include "AnimationManager.h"
#include "qwindowdefs.h"
#include "src/NotificationPanel.h"

#include <QWidget>
#include <QPropertyAnimation>
#include <QApplication>
#include <QScreen>
#include <QRect>

AnimationManager::AnimationManager(QWidget* targetWidget, QObject *parent)
    : QObject(parent)
    , m_targetWidget(targetWidget)
    , m_slideAnimation(nullptr)
    , m_fadeAnimation(nullptr)
    , m_isVisible(false)
    , m_isAnimatingOut(false)
{
    setupAnimations();
}

AnimationManager::~AnimationManager()
{
    // QObject destructor handles child objects
}

void AnimationManager::setupAnimations()
{
    if (!m_targetWidget) return;
    
    // Setup slide animation
    m_slideAnimation = new QPropertyAnimation(m_targetWidget, "geometry", this);
    m_slideAnimation->setDuration(ANIMATION_DURATION);
    m_slideAnimation->setEasingCurve(QEasingCurve::OutCubic);
    
    // Setup fade animation
    m_fadeAnimation = new QPropertyAnimation(m_targetWidget, "windowOpacity", this);
    m_fadeAnimation->setDuration(ANIMATION_DURATION);
    m_fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    
    // Connect signals
    connect(m_slideAnimation, &QPropertyAnimation::finished, 
            this, &AnimationManager::onAnimationFinished);
    connect(m_fadeAnimation, &QPropertyAnimation::finished,
            this, &AnimationManager::onAnimationFinished);
}

void AnimationManager::slideIn()
{
    if (!m_targetWidget || m_isVisible) return;
    
    m_isAnimatingOut = false; // We're animating in
    
    // Position widget off-screen first
    QRect hiddenPos = getHiddenPosition();
    QRect visiblePos = getVisiblePosition();
    
    m_targetWidget->setGeometry(hiddenPos);
    m_targetWidget->show();
    m_targetWidget->setWindowOpacity(1.0);
    
    // Animate sliding in
    m_slideAnimation->setStartValue(hiddenPos);
    m_slideAnimation->setEndValue(visiblePos);
    m_slideAnimation->start();
    
    m_isVisible = true;
}


void AnimationManager::slideOut()
{
    if (!m_targetWidget || !m_isVisible) return;
    
    m_isAnimatingOut = true; // We're animating out
    
    // Ensure the widget is visible during the animation
    m_targetWidget->show();
    
    QRect visiblePos = m_targetWidget->geometry(); // Use current position
    QRect hiddenPos = getHiddenPosition();
    
    // Animate sliding out - widget should remain visible during animation
    m_slideAnimation->setStartValue(visiblePos);
    m_slideAnimation->setEndValue(hiddenPos);
    m_slideAnimation->start();
    
    m_isVisible = false;
}

void AnimationManager::fadeIn()
{
    if (!m_targetWidget) return;
    
    m_targetWidget->show();
    m_fadeAnimation->setStartValue(0.0);
    m_fadeAnimation->setEndValue(1.0);
    m_fadeAnimation->start();
}

void AnimationManager::fadeOut()
{
    if (!m_targetWidget) return;
    
    m_fadeAnimation->setStartValue(1.0);
    m_fadeAnimation->setEndValue(0.0);
    m_fadeAnimation->start();
}

QRect AnimationManager::getHiddenPosition()
{
    if (!m_targetWidget) return QRect();
    
    if (QScreen* screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->availableGeometry();
        int width = m_targetWidget->width();
        int height = m_targetWidget->height();
        
        // Get the visible position to maintain same Y coordinate
        QRect visiblePos = getVisiblePosition();
        int y = visiblePos.y();
        
        // Place the panel completely off-screen to the right
        // The left edge of the panel should be at the right edge of the screen
        int x = screenGeometry.right();
        
        return QRect(x, y, width, height);
    }
    
    // Fallback: move far to the right
    QRect visiblePos = getVisiblePosition();
    return QRect(visiblePos.x() + visiblePos.width() + 100,
                 visiblePos.y(),
                 visiblePos.width(),
                 visiblePos.height());
}

QRect AnimationManager::getVisiblePosition()
{
    if (!m_targetWidget) return QRect();
    
    if (QScreen* screen = QApplication::primaryScreen()) {
        QRect screenGeometry = screen->availableGeometry();
        
        int width = m_targetWidget->width();
        int height = m_targetWidget->height();
        
        // Position on the right side of the screen with consistent margins
        int x = screenGeometry.right() - width - NotificationPanel::RIGHT_MARGIN;
        int y = screenGeometry.top() + NotificationPanel::TOP_MARGIN;
        
        return QRect(x, y, width, height);
    }
    
    return QRect(0, 0, m_targetWidget->width(), m_targetWidget->height());
}

void AnimationManager::onAnimationFinished()
{
    // If we were animating out, hide the widget for optimization
    if (m_isAnimatingOut && m_targetWidget) {
        m_targetWidget->hide();
    }
    
    emit animationFinished();
}
