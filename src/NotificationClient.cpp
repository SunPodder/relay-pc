#include "NotificationClient.h"
#include <QDebug>
#include <QJsonParseError>
#include <QJsonArray>

NotificationClient::NotificationClient(QObject *parent)
    : QObject(parent)
    , m_serviceDiscovery(new ServiceDiscovery(this))
    , m_socket(nullptr)
    , m_reconnectTimer(new QTimer(this))
    , m_serverPort(DEFAULT_PORT)
    , m_isConnected(false)
    , m_autoReconnect(true)
{
    // Connect service discovery signals
    connect(m_serviceDiscovery, &ServiceDiscovery::serviceFound,
            this, &NotificationClient::onServiceFound);
    connect(m_serviceDiscovery, &ServiceDiscovery::errorOccurred,
            this, &NotificationClient::onDiscoveryError);
    
    // Setup reconnect timer
    m_reconnectTimer->setSingleShot(true);
    m_reconnectTimer->setInterval(RECONNECT_INTERVAL);
    connect(m_reconnectTimer, &QTimer::timeout,
            this, &NotificationClient::onReconnectTimer);
    
    setupSocket();
}

NotificationClient::~NotificationClient()
{
    disconnectFromServer();
}

void NotificationClient::setupSocket()
{
    if (m_socket) {
        m_socket->deleteLater();
    }
    
    m_socket = new QTcpSocket(this);
    
    connect(m_socket, &QTcpSocket::connected,
            this, &NotificationClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &NotificationClient::onSocketDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &NotificationClient::onSocketError);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &NotificationClient::onDataReceived);
}

void NotificationClient::startDiscoveryAndConnect()
{
    // Start mDNS discovery
    m_serviceDiscovery->startDiscovery();
    
    emit errorOccurred("Searching for notification server...");
}

void NotificationClient::connectToServer(const QHostAddress& address, quint16 port)
{
    if (m_isConnected && m_serverAddress == address && m_serverPort == port) {
        return;
    }
    
    disconnectFromServer();
    
    m_serverAddress = address;
    m_serverPort = port;
    m_socket->connectToHost(address, port);
}

void NotificationClient::disconnectFromServer()
{
    stopReconnectTimer();
    
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(3000);
        }
    }
    
    m_isConnected = false;
    m_receiveBuffer.clear();
}

bool NotificationClient::isConnected() const
{
    return m_isConnected && m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

bool NotificationClient::isDiscovering() const
{
    return m_serviceDiscovery && m_serviceDiscovery->isDiscovering();
}

void NotificationClient::onServiceFound(const ServiceDiscovery::ServiceInfo& service)
{
    emit serverDiscovered(service.address, service.port);
    
    // Stop discovery and connect to the first found server
    m_serviceDiscovery->stopDiscovery();
    connectToServer(service.address, service.port);
}

void NotificationClient::onDiscoveryError(const QString& error)
{
    qWarning() << "Service discovery error:" << error;
    emit errorOccurred("Service discovery failed: " + error);
    
    // Retry discovery after a delay
    QTimer::singleShot(RECONNECT_INTERVAL, this, [this]() {
        if (!isConnected()) {
            startDiscoveryAndConnect();
        }
    });
}

void NotificationClient::onSocketConnected()
{
    m_isConnected = true;
    stopReconnectTimer();
    
    emit connected();
}

void NotificationClient::onSocketDisconnected()
{
    m_isConnected = false;
    m_receiveBuffer.clear();
    
    emit disconnected();
    
    // Start reconnect timer if auto-reconnect is enabled
    if (m_autoReconnect) {
        startReconnectTimer();
    }
}

void NotificationClient::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    qWarning() << "Socket error:" << error << "-" << errorString;
    
    m_isConnected = false;
    emit errorOccurred("Connection error: " + errorString);
    
    // Start reconnect timer if auto-reconnect is enabled
    if (m_autoReconnect) {
        startReconnectTimer();
    }
}

void NotificationClient::onDataReceived()
{
    QByteArray newData = m_socket->readAll();
    m_receiveBuffer.append(newData);
    
    processReceivedData();
}

void NotificationClient::processReceivedData()
{
    // Process complete JSON messages (assuming line-delimited JSON)
    while (m_receiveBuffer.contains('\n')) {
        int newlineIndex = m_receiveBuffer.indexOf('\n');
        QByteArray messageData = m_receiveBuffer.left(newlineIndex);
        m_receiveBuffer.remove(0, newlineIndex + 1);
        
        if (messageData.isEmpty()) {
            continue;
        }
        
        // Parse JSON message
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << "Failed to parse notification JSON:" << parseError.errorString();
            continue;
        }
        
        if (!doc.isObject()) {
            qWarning() << "Received notification is not a JSON object";
            continue;
        }
        
        QJsonObject notificationJson = doc.object();
        NotificationData notification = parseNotificationJson(notificationJson);
        
        if (!notification.title.isEmpty()) {
            emit notificationReceived(notification);
        }
    }
}

NotificationData NotificationClient::parseNotificationJson(const QJsonObject& json)
{
    NotificationData notification;
    
    notification.id = json.value("id").toInt();
    notification.title = json.value("title").toString();
    notification.body = json.value("body").toString();
    notification.appName = json.value("app").toString();
    notification.packageName = json.value("package").toString();
    notification.timestamp = QDateTime::fromSecsSinceEpoch(json.value("timestamp").toInteger());
    
    // Parse actions if present
    QJsonArray actionsArray = json.value("actions").toArray();
    for (const QJsonValue& actionValue : actionsArray) {
        if (actionValue.isObject()) {
            QJsonObject actionObj = actionValue.toObject();
            NotificationAction action;
            action.key = actionObj.value("key").toString();
            action.title = actionObj.value("title").toString();
            action.type = actionObj.value("type").toString();
            notification.actions.append(action);
        }
    }
    
    // Set default timestamp if not provided
    if (!notification.timestamp.isValid()) {
        notification.timestamp = QDateTime::currentDateTime();
    }
    
    return notification;
}

void NotificationClient::onReconnectTimer()
{
    if (!isConnected()) {
        if (!m_serverAddress.isNull()) {
            // Try to reconnect to the last known server
            connectToServer(m_serverAddress, m_serverPort);
        } else {
            // Start discovery again
            startDiscoveryAndConnect();
        }
    }
}

void NotificationClient::startReconnectTimer()
{
    if (!m_reconnectTimer->isActive()) {
        m_reconnectTimer->start();
    }
}

void NotificationClient::stopReconnectTimer()
{
    if (m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }
}
