#include "NotificationClient.h"
#include "Logger.h"
#include <QDebug>
#include <QJsonParseError>
#include <QJsonArray>
#include <QUuid>

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
        Logger::warning(QString("Invalid host address: %1").arg(hostAddress));
        emit errorOccurred("Invalid host address: " + hostAddress);
        return;
    }
    
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
    Logger::warning(QString("Service discovery error: %1").arg(error));
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
    m_handshakeComplete = false;
    stopReconnectTimer();
    
    Logger::info(QString("Connected to server at %1:%2").arg(m_serverAddress.toString()).arg(m_serverPort));
    Logger::debug("Sending connection handshake");
    
    sendConnectionRequest();
    
    // Set a timeout for handshake response
    QTimer::singleShot(10000, this, [this]() {
        if (!m_handshakeComplete) {
            Logger::warning("Handshake timeout - no ACK received within 10 seconds");
            emit errorOccurred("Handshake timeout");
        }
    });
}

void NotificationClient::onSocketDisconnected()
{
    m_isConnected = false;
    m_handshakeComplete = false;
    m_receiveBuffer.clear();
    
    Logger::info("Disconnected from server");
    emit disconnected();
    
    // Start reconnect timer if auto-reconnect is enabled
    if (m_autoReconnect) {
        startReconnectTimer();
    }
}

void NotificationClient::onSocketError(QAbstractSocket::SocketError error)
{
    QString errorString = m_socket->errorString();
    Logger::warning(QString("Socket error: %1 - %2").arg(error).arg(errorString));
    
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
    // Process complete messages using length prefix
    while (m_receiveBuffer.length() >= 4) {
        // Read the 4-byte big-endian length prefix
        quint32 messageLength = 0;
        messageLength |= (static_cast<quint8>(m_receiveBuffer[0]) << 24);
        messageLength |= (static_cast<quint8>(m_receiveBuffer[1]) << 16);
        messageLength |= (static_cast<quint8>(m_receiveBuffer[2]) << 8);
        messageLength |= static_cast<quint8>(m_receiveBuffer[3]);
        
        // Check if we have the complete message (prefix + data)
        quint32 totalRequired = 4 + messageLength;
        if (static_cast<quint32>(m_receiveBuffer.length()) < totalRequired) {
            // Wait for more data
            break;
        }
        
        // Extract the message data (skip the 4-byte prefix)
        QByteArray messageData = m_receiveBuffer.mid(4, messageLength);
        m_receiveBuffer.remove(0, totalRequired);
        
        // Parse and handle the message
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            Logger::warning(QString("Failed to parse JSON: %1").arg(parseError.errorString()));
            continue;
        }
        
        if (!doc.isObject()) {
            Logger::warning("Received data is not a JSON object");
            continue;
        }
        
        QJsonObject messageJson = doc.object();
        Logger::debug(QString("Received message: %1").arg(QJsonDocument(messageJson).toJson(QJsonDocument::Compact)));
        
        // Handle the message
        handleMessage(messageJson);
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
    
    sendMessage(connMsg);
}

void NotificationClient::sendMessage(const QJsonObject& message)
{
    if (!m_socket || !m_isConnected) {
        return;
    }
    
    QJsonDocument doc(message);
    QByteArray messageData = doc.toJson(QJsonDocument::Compact);
    
    // Create length prefix (4-byte big-endian unsigned integer)
    quint32 messageLength = static_cast<quint32>(messageData.length());
    QByteArray lengthPrefix(4, 0);
    lengthPrefix[0] = static_cast<char>((messageLength >> 24) & 0xFF);
    lengthPrefix[1] = static_cast<char>((messageLength >> 16) & 0xFF); 
    lengthPrefix[2] = static_cast<char>((messageLength >> 8) & 0xFF);
    lengthPrefix[3] = static_cast<char>(messageLength & 0xFF);
    
    // Send length prefix followed by message data
    QByteArray fullMessage = lengthPrefix + messageData;
    
    m_socket->write(fullMessage);
    m_socket->flush();
}

void NotificationClient::handleMessage(const QJsonObject& message)
{
    QString msgType = message.value("type").toString();
    
    if (msgType == "ack") {
        QJsonObject payload = message.value("payload").toObject();
        QString status = payload.value("status").toString();
        
        if (status == "ok") {
            m_handshakeComplete = true;
            Logger::info("Handshake successful - ready to receive notifications");
            emit connected();
        } else {
            QString reason = payload.value("reason").toString();
            Logger::warning(QString("Connection rejected by server: %1").arg(reason));
            emit errorOccurred("Connection rejected by server: " + reason);
            disconnectFromServer();
        }
    }
    else if (msgType == "notification") {
        if (m_handshakeComplete) {
            QJsonObject payload = message.value("payload").toObject();
            NotificationData notification = parseNotificationJson(payload);
            if (!notification.title.isEmpty()) {
                Logger::debug(QString("Received notification: %1").arg(notification.title));
                emit notificationReceived(notification);
            }
        }
    }
    else if (msgType == "ping") {
        Logger::debug(QString("Received ping with ID: %1").arg(message.value("id").toString()));
        handlePing(message);
    }
    else {
        Logger::warning(QString("Unknown message type: %1").arg(msgType));
    }
}

void NotificationClient::handlePing(const QJsonObject& message)
{
    QString pingId = message.value("id").toString();
    Logger::debug(QString("Sending pong response for ping ID: %1").arg(pingId));
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
