#ifndef NOTIFICATIONCLIENT_H
#define NOTIFICATIONCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include "NotificationData.h"
#include "ServiceDiscovery.h"

class NotificationClient : public QObject
{
    Q_OBJECT

public:
    explicit NotificationClient(QObject *parent = nullptr);
    ~NotificationClient();

    void startDiscoveryAndConnect();
    void connectToServer(const QHostAddress& address, quint16 port);
    void connectToServerDirect(const QString& hostAddress, quint16 port); // For testing
    void disconnectFromServer();
    
    // Notification interaction methods
    void sendNotificationReply(const QString& notificationId, const QString& actionKey, const QString& replyText);
    void sendNotificationAction(const QString& notificationId, const QString& actionKey);
    void sendNotificationDismiss(const QString& notificationId);
    
    bool isConnected() const;
    bool isDiscovering() const;
    
    QHostAddress serverAddress() const { return m_serverAddress; }
    quint16 serverPort() const { return m_serverPort; }

signals:
    void connected();
    void disconnected();
    void notificationReceived(const NotificationData& notification);
    void errorOccurred(const QString& error);
    void serverDiscovered(const QHostAddress& address, quint16 port);

private slots:
    void onServiceFound(const ServiceDiscovery::ServiceInfo& service);
    void onDiscoveryError(const QString& error);
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onDataReceived();
    void onReconnectTimer();

private:
    void setupSocket();
    void processReceivedData();
    void sendConnectionRequest();
    void sendMessage(const QJsonObject& message);
    void handleMessage(const QJsonObject& message);
    void handlePing(const QJsonObject& message);
    void sendPong(const QString& pingId);
    NotificationData parseNotificationJson(const QJsonObject& json);
    void startReconnectTimer();
    void stopReconnectTimer();
    
    ServiceDiscovery* m_serviceDiscovery;
    QTcpSocket* m_socket;
    QTimer* m_reconnectTimer;
    
    QHostAddress m_serverAddress;
    quint16 m_serverPort;
    
    QByteArray m_receiveBuffer;
    bool m_isConnected;
    bool m_autoReconnect;
    bool m_handshakeComplete;
    
    static constexpr int RECONNECT_INTERVAL = 5000; // 5 seconds
    static constexpr quint16 DEFAULT_PORT = 9999;
};

#endif // NOTIFICATIONCLIENT_H
