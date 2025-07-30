#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include "NotificationData.h"

class NotificationManager : public QObject
{
    Q_OBJECT

public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    void addNotification(const NotificationData& notification);
    void removeNotification(int notificationId);
    void clearAllNotifications();
    
    // For testing purposes
    void addDummyNotifications();

signals:
    void notificationReceived(const NotificationData& notification);
    void notificationRemoved(int notificationId);

private slots:
    void generateTestNotification();

private:
    QList<NotificationData> m_notifications;
    QTimer* m_testTimer;
    int m_nextId;
    int m_testNotificationCount;
    
    static constexpr int MAX_NOTIFICATIONS = 100;
};

#endif // NOTIFICATIONMANAGER_H
