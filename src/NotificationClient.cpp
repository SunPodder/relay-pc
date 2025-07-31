#include "NotificationClient.h"
#include <QDebug>
#include <QJsonParseError>
#include <QJsonArray>
#include <QUuid>
#include <QTimer>

NotificationClient::NotificationClient(QObject *parent)
    : QObject(parent)
    , m_serviceDiscovery(new ServiceDiscovery(this))
    , m_socket(nullptr)
    , m_reconnectTimer(new QTimer(this))
    , m_serverPort(DEFAULT_PORT)
    , m_isConnected(false)
    , m_autoReconnect(true)
    , m_handshakeComplete(false)
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

void NotificationClient::connectToServerDirect(const QString& hostAddress, quint16 port)
{
    QHostAddress addr(hostAddress);
    if (addr.isNull()) {
        qWarning() << "Invalid host address:" << hostAddress;
        emit errorOccurred("Invalid host address: " + hostAddress);
        return;
    }
    
    qDebug() << "Connecting directly to server at" << hostAddress << ":" << port;
    connectToServer(addr, port);
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
    qDebug() << "TCP socket connected to server at" << m_serverAddress.toString() << ":" << m_serverPort;
    m_isConnected = true;
    m_handshakeComplete = false;
    stopReconnectTimer();
    
    // Send connection request
    qDebug() << "Sending connection handshake...";
    sendConnectionRequest();
    
    // Set a timeout for handshake response
    QTimer::singleShot(10000, this, [this]() {
        if (!m_handshakeComplete) {
            qDebug() << "Handshake timeout - no ACK received within 10 seconds";
        }
    });
}

void NotificationClient::onSocketDisconnected()
{
    m_isConnected = false;
    m_handshakeComplete = false;
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
    qDebug() << "Received" << newData.length() << "bytes of data";
    
    // Show the data content (safely, replacing null bytes with \0 for display)
    QByteArray displayData = newData;
    displayData.replace('\0', "\\0");
    qDebug() << "Data content:" << displayData;
    
    m_receiveBuffer.append(newData);
    qDebug() << "Total buffer size:" << m_receiveBuffer.length() << "bytes";
    
    // Process messages immediately without counting - avoid potential issues with count()
    processReceivedData();
}

void NotificationClient::processReceivedData()
{
    // Process complete JSON messages (terminated by double null bytes)
    // Add safety limit to prevent infinite loops
    int processedMessages = 0;
    const int maxMessagesPerCall = 10; // Safety limit

    while (m_receiveBuffer.contains("\x00\x00") && processedMessages < maxMessagesPerCall) {
        int termIndex = m_receiveBuffer.indexOf("\x00\x00");
        if (termIndex == -1) {
            break; // No more complete messages
        }
        
        QByteArray messageData = m_receiveBuffer.left(termIndex);
        m_receiveBuffer.remove(0, termIndex + 2);
        
        if (messageData.isEmpty()) {
            processedMessages++;
            continue;
        }
        
        // Parse JSON message
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << parseError.errorString();
            processedMessages++;
            continue;
        }
        
        if (!doc.isObject()) {
            qDebug() << "Received data is not a JSON object";
            processedMessages++;
            continue;
        }
        
        QJsonObject messageJson = doc.object();
        qDebug() << "Parsed message:" << QJsonDocument(messageJson).toJson(QJsonDocument::Compact);
        
        // Handle the message
        handleMessage(messageJson);
        processedMessages++;
    }

    if (processedMessages >= maxMessagesPerCall && m_receiveBuffer.contains("\x00\x00")) {
        qDebug() << "Hit message processing limit, scheduling continuation";
        // Schedule processing of remaining messages to avoid blocking the event loop
        QTimer::singleShot(0, this, &NotificationClient::processReceivedData);
    }
}

NotificationData NotificationClient::parseNotificationJson(const QJsonObject& json)
{
    NotificationData notification;
    
    // Store original string ID and convert to int for internal use
    notification.stringId = json.value("id").toString();
    notification.id = notification.stringId.isEmpty() ? 0 : qHash(notification.stringId);
    notification.title = json.value("title").toString();
    notification.body = json.value("body").toString();
    notification.appName = json.value("app").toString();
    notification.packageName = json.value("package").toString();
    notification.canReply = json.value("can_reply").toBool();
    
    // Handle timestamp
    if (json.contains("timestamp")) {
        notification.timestamp = QDateTime::fromSecsSinceEpoch(json.value("timestamp").toInteger());
    } else {
        notification.timestamp = QDateTime::currentDateTime();
    }
    
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

void NotificationClient::sendConnectionRequest()
{
    QJsonObject connMsg;
    connMsg["type"] = "conn";
    connMsg["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    connMsg["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    QJsonObject payload;
    payload["device_name"] = "Relay-PC";
    payload["platform"] = "linux";
    payload["version"] = "1.0.0";
    QJsonArray supports;
    supports.append("notification");
    supports.append("ping");
    supports.append("pong");
    payload["supports"] = supports;
    payload["auth_token"] = "relay-pc-token";
    
    connMsg["payload"] = payload;
    
    qDebug() << "Sending conn message:" << QJsonDocument(connMsg).toJson(QJsonDocument::Compact);
    sendMessage(connMsg);
}

void NotificationClient::sendMessage(const QJsonObject& message)
{
    if (!m_socket || !m_isConnected) {
        qDebug() << "Cannot send message - socket not connected";
        return;
    }
    
    QJsonDocument doc(message);
    QByteArray messageData = doc.toJson(QJsonDocument::Compact);
    
    qDebug() << "Sending message:" << messageData;
    
    // Add null byte termination
    messageData.append(static_cast<char>(0));
    messageData.append(static_cast<char>(0));
    
    qDebug() << "Total bytes being sent:" << messageData.length();
    
    int bytesWritten = m_socket->write(messageData);
    m_socket->flush();
    
    qDebug() << "Bytes written to socket:" << bytesWritten;
}

void NotificationClient::handleMessage(const QJsonObject& message)
{
    QString msgType = message.value("type").toString();
    qDebug() << "Handling message type:" << msgType;
    
    if (msgType == "ack") {
        qDebug() << "Received ACK message";
        QJsonObject payload = message.value("payload").toObject();
        QString status = payload.value("status").toString();
        qDebug() << "ACK status:" << status;
        
        if (status == "ok") {
            qDebug() << "Handshake successful!";
            m_handshakeComplete = true;
            emit connected();
        } else {
            QString reason = payload.value("reason").toString();
            qDebug() << "Connection rejected:" << reason;
            emit errorOccurred("Connection rejected by server: " + reason);
            disconnectFromServer();
        }
    }
    else if (msgType == "notification") {
        qDebug() << "Received notification message";
        if (m_handshakeComplete) {
            QJsonObject payload = message.value("payload").toObject();
            NotificationData notification = parseNotificationJson(payload);
            if (!notification.title.isEmpty()) {
                qDebug() << "Emitting notification:" << notification.title;
                emit notificationReceived(notification);
            }
        } else {
            qDebug() << "Ignoring notification - handshake not complete";
        }
    }
    else if (msgType == "ping") {
        qDebug() << "Received ping message";
        handlePing(message);
    }
    else {
        qDebug() << "Unknown message type:" << msgType;
    }
}

void NotificationClient::handlePing(const QJsonObject& message)
{
    QString pingId = message.value("id").toString();
    qDebug() << "Responding to ping with ID:" << pingId;
    sendPong(pingId);
}

void NotificationClient::sendPong(const QString& pingId)
{
    QJsonObject pongMsg;
    pongMsg["type"] = "pong";
    pongMsg["id"] = pingId;
    pongMsg["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    QJsonObject payload;
    payload["device"] = "Relay-PC";
    pongMsg["payload"] = payload;
    
    sendMessage(pongMsg);
}

void NotificationClient::sendNotificationReply(const QString& notificationId, const QString& actionKey, const QString& replyText)
{
    if (!m_handshakeComplete) return;
    
    QJsonObject replyMsg;
    replyMsg["type"] = "notification_reply";
    replyMsg["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    replyMsg["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    QJsonObject payload;
    payload["id"] = notificationId;
    payload["key"] = actionKey;
    payload["body"] = replyText;
    replyMsg["payload"] = payload;
    
    sendMessage(replyMsg);
}

void NotificationClient::sendNotificationAction(const QString& notificationId, const QString& actionKey)
{
    if (!m_handshakeComplete) return;
    
    QJsonObject actionMsg;
    actionMsg["type"] = "notification_action";
    actionMsg["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    actionMsg["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    QJsonObject payload;
    payload["id"] = notificationId;
    payload["key"] = actionKey;
    actionMsg["payload"] = payload;
    
    sendMessage(actionMsg);
}

void NotificationClient::sendNotificationDismiss(const QString& notificationId)
{
    if (!m_handshakeComplete) return;
    
    QJsonObject dismissMsg;
    dismissMsg["type"] = "notification_dismiss";
    dismissMsg["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    dismissMsg["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    QJsonObject payload;
    payload["id"] = notificationId;
    dismissMsg["payload"] = payload;
    
    sendMessage(dismissMsg);
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
