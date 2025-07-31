#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include "NotificationData.h"

class NotificationClient;

class NotificationManager : public QObject
{
    Q_OBJECT

public:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    void addNotification(const NotificationData& notification);
    void removeNotification(int notificationId);
    void clearAllNotifications();
    
    // Network connectivity
    void startNetworkClient();
    void stopNetworkClient();
    bool isConnectedToServer() const;
    
    // Getter for testing
    NotificationClient* getClient() const { return m_client; }
    
    // For testing purposes
    void addDummyNotifications();

signals:
    void notificationReceived(const NotificationData& notification);
    void notificationRemoved(int notificationId);
    void serverConnected();
    void serverDisconnected();
    void connectionError(const QString& error);

private slots:
    void generateTestNotification();
    void onClientNotificationReceived(const NotificationData& notification);
    void onClientConnected();
    void onClientDisconnected();
    void onClientError(const QString& error);

private:
    QList<NotificationData> m_notifications;
    QTimer* m_testTimer;
    NotificationClient* m_client;
    int m_nextId;
    int m_testNotificationCount;
    
    static constexpr int MAX_NOTIFICATIONS = 100;
};

#endif // NOTIFICATIONMANAGER_H
