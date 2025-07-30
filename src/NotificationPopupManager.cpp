#include "NotificationPopupManager.h"

#include <QApplication>
#include <QScreen>
#include <QCursor>

NotificationPopupManager::NotificationPopupManager(QObject *parent)
    : QObject(parent)
{
}

NotificationPopupManager::~NotificationPopupManager()
{
    // Clean up any remaining popups
    for (NotificationPopup* popup : m_activePopups) {
        popup->deleteLater();
    }
    m_activePopups.clear();
}

void NotificationPopupManager::showNotificationPopup(const NotificationData& notification)
{
    // Create new popup
    NotificationPopup* popup = new NotificationPopup(notification);
    
    // Connect signals
    connect(popup, &NotificationPopup::closeRequested, 
            this, &NotificationPopupManager::onPopupCloseRequested);
    
    // Add to active popups list
    m_activePopups.append(popup);
    
    // Calculate position for the new popup
    calculatePopupPosition(popup);
    
    // Show the popup with animation
    popup->startShowAnimation();
}

void NotificationPopupManager::calculatePopupPosition(NotificationPopup* popup)
{
    QScreen* screen = getCurrentScreen();
    if (!screen) return;
    
    QRect screenGeometry = screen->availableGeometry();
    
    // Start position: top-right corner of screen
    int x = screenGeometry.right() - popup->width() - SCREEN_MARGIN;
    int y = screenGeometry.top() + SCREEN_MARGIN;
    
    // Check for collisions with existing popups and adjust position
    for (NotificationPopup* existingPopup : m_activePopups) {
        if (existingPopup == popup) continue; // Skip self
        
        QRect existingRect = existingPopup->geometry();
        QRect newRect(x, y, popup->width(), popup->height());
        
        // If there's a collision, move down
        if (newRect.intersects(existingRect)) {
            y = existingRect.bottom() + POPUP_SPACING;
        }
    }
    
    // Ensure popup doesn't go off-screen vertically
    if (y + popup->height() > screenGeometry.bottom() - SCREEN_MARGIN) {
        // If we would go off-screen, start a new column to the left
        x -= (popup->width() + POPUP_SPACING);
        y = screenGeometry.top() + SCREEN_MARGIN;
        
        // Ensure we don't go off-screen horizontally either
        if (x < screenGeometry.left() + SCREEN_MARGIN) {
            x = screenGeometry.left() + SCREEN_MARGIN;
        }
    }
    
    popup->setPosition(x, y);
}

void NotificationPopupManager::repositionExistingPopups()
{
    // Reposition all existing popups to fill gaps
    QScreen* screen = getCurrentScreen();
    if (!screen) return;
    
    QRect screenGeometry = screen->availableGeometry();
    
    int x = screenGeometry.right() - (m_activePopups.isEmpty() ? 0 : m_activePopups.first()->width()) - SCREEN_MARGIN;
    int y = screenGeometry.top() + SCREEN_MARGIN;
    
    for (NotificationPopup* popup : m_activePopups) {
        popup->setPosition(x, y);
        y += popup->height() + POPUP_SPACING;
        
        // Check if we need to start a new column
        if (y + popup->height() > screenGeometry.bottom() - SCREEN_MARGIN) {
            x -= (popup->width() + POPUP_SPACING);
            y = screenGeometry.top() + SCREEN_MARGIN;
            
            // Ensure we don't go off-screen horizontally
            if (x < screenGeometry.left() + SCREEN_MARGIN) {
                break; // Stop if we can't fit more columns
            }
        }
    }
}

QScreen* NotificationPopupManager::getCurrentScreen()
{
    // Get the primary screen or the screen containing the cursor
    QScreen* screen = QApplication::primaryScreen();
    
    // Try to get the screen containing the cursor
    QPoint cursorPos = QCursor::pos();
    for (QScreen* availableScreen : QApplication::screens()) {
        if (availableScreen->geometry().contains(cursorPos)) {
            screen = availableScreen;
            break;
        }
    }
    
    return screen;
}

void NotificationPopupManager::onPopupCloseRequested(int notificationId)
{
    // Find and remove the popup from active list
    for (int i = 0; i < m_activePopups.size(); ++i) {
        if (m_activePopups[i]->getNotificationId() == notificationId) {
            m_activePopups.removeAt(i);
            break;
        }
    }
    
    // Reposition remaining popups to fill gaps
    repositionExistingPopups();
}
