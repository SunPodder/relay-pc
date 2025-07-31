#include <QApplication>
#include <QDebug>
#include "MainWindow.h"
#include "NotificationManager.h"
#include "NotificationClient.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application properties
    app.setApplicationName("Relay PC");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("RelayPC");
    
    MainWindow window;
    
    // Check for direct server connection arguments (fallback if mDNS doesn't work)
    if (argc >= 3 && QString(argv[1]) == "--direct") {
        QString serverHost = argv[2];
        quint16 serverPort = argc >= 4 ? QString(argv[3]).toUShort() : 8080;
        
        qDebug() << "Direct mode: connecting to" << serverHost << ":" << serverPort;
        
        // Connect directly to specified server instead of using mDNS discovery
        window.getNotificationManager()->getClient()->connectToServerDirect(serverHost, serverPort);
    } else {
        // Normal mode: use mDNS discovery to find Android server
        qDebug() << "Starting Relay PC - looking for Android notification server via mDNS";
        qDebug() << "Searching for service: _relay._tcp.local";
        qDebug() << "Tip: Use --direct <host> [port] if mDNS discovery fails";
    }
    
    // Don't show the main window - we only use the notification panel
    // window.show(); // Removed this line
    
    return app.exec();
}
