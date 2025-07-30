#include "NotificationManager.h"

#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
    , m_testTimer(nullptr)
    , m_nextId(1)
    , m_testNotificationCount(0)
{
    // Initialize test timer for demo purposes
    m_testTimer = new QTimer(this);
    connect(m_testTimer, &QTimer::timeout, this, &NotificationManager::generateTestNotification);
}

NotificationManager::~NotificationManager()
{
    // QObject destructor handles child objects
}

void NotificationManager::addNotification(const NotificationData& notification)
{
    NotificationData newNotification = notification;
    newNotification.id = m_nextId++;
    newNotification.timestamp = QDateTime::currentDateTime();
    
    // Limit the number of stored notifications
    if (m_notifications.size() >= MAX_NOTIFICATIONS) {
        m_notifications.removeFirst();
    }
    
    m_notifications.append(newNotification);
    emit notificationReceived(newNotification);
}

void NotificationManager::removeNotification(int notificationId)
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].id == notificationId) {
            m_notifications.removeAt(i);
            emit notificationRemoved(notificationId);
            break;
        }
    }
}

void NotificationManager::clearAllNotifications()
{
    m_notifications.clear();
    // Could emit a signal here if needed
}

void NotificationManager::addDummyNotifications()
{
    // Add some initial dummy notifications for testing
    QStringList appNames = {"WhatsApp", "Telegram", "Discord", "Gmail", "Slack"};
    QStringList titles = {"New Message", "Meeting Reminder", "File Shared", "System Update", "Battery Low"};
    QStringList bodies = {
        "Hey, are you available for a quick call?",
        "Stand-up meeting in 15 minutes",
        "document.pdf has been shared with you",
        "System restart required to complete updates",
        "Please connect your charger"
    };
    
    for (int i = 0; i < 3; ++i) {
        NotificationData notification(
            appNames[i % appNames.size()],
            titles[i % titles.size()],
            bodies[i % bodies.size()]
        );
        
        // Add some sample actions for testing
        if (i == 0) { // WhatsApp - messaging app
            notification.packageName = "com.whatsapp";
            notification.canReply = true;
            notification.actions.append(NotificationAction("Reply", "remote_input", "quick_reply"));
            notification.actions.append(NotificationAction("Mark as Read", "action", "mark_read"));
        } else if (i == 1) { // Telegram
            notification.packageName = "org.telegram.messenger";
            notification.actions.append(NotificationAction("Snooze", "action", "snooze"));
        }
        // Gmail notification (i == 2) has no actions
        
        addNotification(notification);
    }
    
    // Start generating periodic test notifications (every 10 seconds)
    m_testTimer->start(100);
}

void NotificationManager::generateTestNotification()
{
    if (m_testNotificationCount >= 5) {
        m_testTimer->stop();
        return;
    }
    
    QStringList testApps = {"System", "Chrome", "VS Code", "Spotify", "Calendar"};
    QStringList testTitles = {"Test Notification", "Background Process", "Build Complete", "Now Playing", "Event Starting"};
    QStringList testBodies = {
        "This is a test notification #" + QString::number(m_testNotificationCount + 1),
        "Background sync completed successfully",
        "Project build finished without errors",
        "♪ Your favorite song is now playing",
        "Meeting with team starts in 5 minutes"
    };
    
    int index = m_testNotificationCount % testApps.size();
    NotificationData notification(
        testApps[index],
        testTitles[index],
        testBodies[index]
    );
    
    addNotification(notification);
    m_testNotificationCount++;
}
