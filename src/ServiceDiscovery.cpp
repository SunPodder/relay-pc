#include "ServiceDiscovery.h"
#include "Logger.h"

#include <QUdpSocket>
#include <QTimer>
#include <QNetworkInterface>
#include <QDnsLookup>
#include <QDebug>

ServiceDiscovery::ServiceDiscovery(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_discoveryTimer(nullptr)
    , m_queryTimer(nullptr)
    , m_dnsLookup(nullptr)
    , m_isDiscovering(false)
    , m_targetServiceType(MDNS_SERVICE_TYPE)
    , m_targetServiceName(MDNS_APPLICATION_NAME)
{
    // Setup discovery timer
    m_discoveryTimer = new QTimer(this);
    m_discoveryTimer->setSingleShot(true);
    m_discoveryTimer->setInterval(DISCOVERY_TIMEOUT);
    connect(m_discoveryTimer, &QTimer::timeout, this, &ServiceDiscovery::onDiscoveryTimeout);
    
    // Setup query timer for periodic queries
    m_queryTimer = new QTimer(this);
    m_queryTimer->setInterval(QUERY_INTERVAL);
    connect(m_queryTimer, &QTimer::timeout, this, &ServiceDiscovery::sendMdnsQuery);
}

ServiceDiscovery::~ServiceDiscovery()
{
    stopDiscovery();
}

void ServiceDiscovery::startDiscovery()
{
    if (m_isDiscovering) {
        return;
    }
    
    setupMulticastSocket();
    
    if (!m_socket) {
        emit errorOccurred("Failed to setup multicast socket");
        return;
    }
    
    m_isDiscovering = true;
    m_discoveredServices.clear();
    
    // Start timers
    m_discoveryTimer->start();
    m_queryTimer->start();
    
    // Send initial query
    sendMdnsQuery();
    
    emit discoveryStarted();
}

void ServiceDiscovery::stopDiscovery()
{
    if (!m_isDiscovering) {
        return;
    }
    
    m_isDiscovering = false;
    
    // Stop timers
    if (m_discoveryTimer) {
        m_discoveryTimer->stop();
    }
    if (m_queryTimer) {
        m_queryTimer->stop();
    }
    
    // Clean up socket
    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    
    emit discoveryStopped();
}

void ServiceDiscovery::setupMulticastSocket()
{
    if (m_socket) {
        m_socket->deleteLater();
    }
    
    m_socket = new QUdpSocket(this);
    
    // Bind to mDNS port
    if (!m_socket->bind(QHostAddress::AnyIPv4, MDNS_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        Logger::warning(QString("Failed to bind to mDNS port: %1").arg(m_socket->errorString()));
        // Try to bind to any available port for sending queries
        if (!m_socket->bind(QHostAddress::AnyIPv4, 0)) {
            Logger::warning(QString("Failed to bind to any port: %1").arg(m_socket->errorString()));
            emit errorOccurred("Failed to bind UDP socket: " + m_socket->errorString());
            m_socket->deleteLater();
            m_socket = nullptr;
            return;
        }
    }
    
    // Join multicast group
    QHostAddress multicastAddress("224.0.0.251"); // mDNS multicast address
    
    // Find a suitable network interface
    bool joinedGroup = false;
    for (const QNetworkInterface& interface : QNetworkInterface::allInterfaces()) {
        if (interface.flags() & QNetworkInterface::IsUp &&
            interface.flags() & QNetworkInterface::IsRunning &&
            interface.flags() & QNetworkInterface::CanMulticast &&
            !(interface.flags() & QNetworkInterface::IsLoopBack)) {
            
            if (m_socket->joinMulticastGroup(multicastAddress, interface)) {
                joinedGroup = true;
                break;
            }
        }
    }
    
    if (!joinedGroup) {
        Logger::warning("Failed to join mDNS multicast group");
        // Continue anyway, we might still be able to send/receive
    }
    
    // Connect socket signals
    connect(m_socket, &QUdpSocket::readyRead, this, &ServiceDiscovery::processMulticastPacket);
}

void ServiceDiscovery::sendMdnsQuery()
{
    if (!m_socket || !m_isDiscovering) {
        return;
    }
    
    QByteArray query = createMdnsQuery(m_targetServiceType);
    QHostAddress multicastAddress("224.0.0.251");
    
    qint64 sent = m_socket->writeDatagram(query, multicastAddress, MDNS_PORT);
    if (sent == -1) {
        Logger::warning(QString("Failed to send mDNS query: %1").arg(m_socket->errorString()));
    }
}

QByteArray ServiceDiscovery::createMdnsQuery(const QString& serviceName)
{
    QByteArray query;
    
    // mDNS header
    query.append(char(0x00)); query.append(char(0x00)); // Transaction ID
    query.append(char(0x00)); query.append(char(0x00)); // Flags (standard query)
    query.append(char(0x00)); query.append(char(0x01)); // Questions
    query.append(char(0x00)); query.append(char(0x00)); // Answer RRs
    query.append(char(0x00)); query.append(char(0x00)); // Authority RRs
    query.append(char(0x00)); query.append(char(0x00)); // Additional RRs
    
    // Question section
    // Encode service name (e.g., "_relay-notifications._tcp.local")
    QStringList labels = serviceName.split('.');
    for (const QString& label : labels) {
        if (!label.isEmpty()) {
            query.append(char(label.length()));
            query.append(label.toUtf8());
        }
    }
    query.append(char(0x00)); // End of name
    
    query.append(char(0x00)); query.append(char(0x0C)); // Type: PTR
    query.append(char(0x00)); query.append(char(0x01)); // Class: IN
    
    return query;
}

void ServiceDiscovery::processMulticastPacket()
{
    while (m_socket && m_socket->hasPendingDatagrams()) {
        QByteArray data;
        QHostAddress sender;
        quint16 senderPort;
        
        data.resize(m_socket->pendingDatagramSize());
        qint64 received = m_socket->readDatagram(data.data(), data.size(), &sender, &senderPort);
        
        if (received > 0) {
            parseMdnsResponse(data, sender);
        }
    }
}

void ServiceDiscovery::parseMdnsResponse(const QByteArray& data, const QHostAddress& sender)
{
    if (data.size() < 12) {
        return; // Invalid mDNS packet
    }
    
    int offset = 0;
    
    // Skip header
    offset += 12;
    
    // Read header fields
    quint16 questions = (static_cast<quint8>(data[4]) << 8) | static_cast<quint8>(data[5]);
    quint16 answers = (static_cast<quint8>(data[6]) << 8) | static_cast<quint8>(data[7]);
    
    // Skip questions
    for (int i = 0; i < questions && offset < data.size(); ++i) {
        extractServiceName(data, offset); // Skip name
        offset += 4; // Skip type and class
    }
    
    // Process answers
    for (int i = 0; i < answers && offset < data.size(); ++i) {
        QString name = extractServiceName(data, offset);
        
        if (offset + 10 > data.size()) break;
        
        quint16 type = extractUint16(data, offset);
        quint16 classField = extractUint16(data, offset);
        quint32 ttl = extractUint32(data, offset);
        quint16 dataLength = extractUint16(data, offset);
        
        Q_UNUSED(type)
        Q_UNUSED(classField)
        Q_UNUSED(ttl)
        
        if (offset + dataLength > data.size()) break;
        
        // Check if this is our target service
        if (name.contains(m_targetServiceName) || name.contains("relay")) {
            ServiceInfo service;
            service.serviceName = m_targetServiceName;
            service.serviceType = m_targetServiceType;
            service.hostName = sender.toString();
            service.address = sender;
            service.port = 9999; // Default port, should be extracted from SRV record
            
            // Check if we already found this service
            bool alreadyFound = false;
            for (const ServiceInfo& existing : m_discoveredServices) {
                if (existing.address == service.address) {
                    alreadyFound = true;
                    break;
                }
            }
            
            if (!alreadyFound) {
                m_discoveredServices.append(service);
                emit serviceFound(service);
            }
        }
        
        offset += dataLength;
    }
}

QString ServiceDiscovery::extractServiceName(const QByteArray& data, int& offset)
{
    QString name;
    
    while (offset < data.size()) {
        quint8 length = static_cast<quint8>(data[offset]);
        offset++;
        
        if (length == 0) {
            break; // End of name
        }
        
        if (length >= 192) { // Compression pointer
            if (offset < data.size()) {
                // Skip compression pointer
                offset++;
            }
            break;
        }
        
        if (offset + length > data.size()) {
            break;
        }
        
        if (!name.isEmpty()) {
            name += ".";
        }
        
        name += QString::fromUtf8(data.mid(offset, length));
        offset += length;
    }
    
    return name;
}

quint16 ServiceDiscovery::extractUint16(const QByteArray& data, int& offset)
{
    if (offset + 2 > data.size()) {
        return 0;
    }
    
    quint16 value = (static_cast<quint8>(data[offset]) << 8) | static_cast<quint8>(data[offset + 1]);
    offset += 2;
    return value;
}

quint32 ServiceDiscovery::extractUint32(const QByteArray& data, int& offset)
{
    if (offset + 4 > data.size()) {
        return 0;
    }
    
    quint32 value = (static_cast<quint8>(data[offset]) << 24) |
                    (static_cast<quint8>(data[offset + 1]) << 16) |
                    (static_cast<quint8>(data[offset + 2]) << 8) |
                    static_cast<quint8>(data[offset + 3]);
    offset += 4;
    return value;
}

void ServiceDiscovery::onDiscoveryTimeout()
{
    
    if (m_discoveredServices.isEmpty()) {
        emit errorOccurred("No relay notification server found on local network");
    }
    
    stopDiscovery();
}

void ServiceDiscovery::onDnsLookupFinished()
{
    if (!m_dnsLookup) {
        return;
    }
    
    if (m_dnsLookup->error() != QDnsLookup::NoError) {
        Logger::warning(QString("DNS lookup failed: %1").arg(m_dnsLookup->errorString()));
    } else {
        // Process DNS results if needed
        for (const QDnsHostAddressRecord& record : m_dnsLookup->hostAddressRecords()) {
        }
    }
    
    m_dnsLookup->deleteLater();
    m_dnsLookup = nullptr;
}
