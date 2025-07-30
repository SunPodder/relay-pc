#ifndef SERVICEDISCOVERY_H
#define SERVICEDISCOVERY_H

#include <QObject>
#include <QTimer>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QDnsLookup>

#define MDNS_SERVICE_TYPE "_relay._tcp.local"
#define MDNS_APPLICATION_NAME "RelayServer"

class ServiceDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit ServiceDiscovery(QObject *parent = nullptr);
    ~ServiceDiscovery();

    void startDiscovery();
    void stopDiscovery();
    bool isDiscovering() const { return m_isDiscovering; }
    
    // Service information
    struct ServiceInfo {
        QString serviceName;
        QString serviceType;
        QString hostName;
        QHostAddress address;
        quint16 port;
        QStringList txtRecords;
    };

signals:
    void serviceFound(const ServiceInfo& service);
    void serviceRemoved(const QString& serviceName);
    void discoveryStarted();
    void discoveryStopped();
    void errorOccurred(const QString& error);

private slots:
    void onDiscoveryTimeout();
    void onDnsLookupFinished();
    void processMulticastPacket();

private:
    void setupMulticastSocket();
    void sendMdnsQuery();
    void parseMdnsResponse(const QByteArray& data, const QHostAddress& sender);
    QByteArray createMdnsQuery(const QString& serviceName);
    QString extractServiceName(const QByteArray& data, int& offset);
    quint16 extractUint16(const QByteArray& data, int& offset);
    quint32 extractUint32(const QByteArray& data, int& offset);
    
    QUdpSocket* m_socket;
    QTimer* m_discoveryTimer;
    QTimer* m_queryTimer;
    QDnsLookup* m_dnsLookup;
    
    bool m_isDiscovering;
    QList<ServiceInfo> m_discoveredServices;
    
    // Target service configuration
    QString m_targetServiceType;
    QString m_targetServiceName;
    
    static constexpr quint16 MDNS_PORT = 5353;
    static constexpr int DISCOVERY_TIMEOUT = 10000; // 10 seconds
    static constexpr int QUERY_INTERVAL = 2000; // 2 seconds
};

#endif // SERVICEDISCOVERY_H
